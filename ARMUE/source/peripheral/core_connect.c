#include "core_connect.h"
#include "error_code.h"
#include "_types.h"
#include "io.h"
#include <assert.h>

static long int pipe_recv(char *buffer, long int max_size, pipe_t *pipe, bool_t block)
{
    DWORD size_recved;
    // if nonblock mode, check the pipe first
    if(!block){
        PeekNamedPipe(pipe, buffer, max_size, &size_recved, NULL, NULL);
        if(size_recved == 0){
            return 0;
        }
    }
    BOOL success = ReadFile(pipe, buffer, max_size, &size_recved, NULL);
    if(success){
        return size_recved;
    }else{
        return -1;
    }
}

static long int pipe_send(char *buffer, long int send_size, pipe_t *pipe)
{
    DWORD size_sent;
    BOOL success = WriteFile(pipe, buffer, send_size, &size_sent, NULL);
    if(success){
        return size_sent;
    }else{
        return -1;
    }
}

core_connect_t *create_core_connect(unsigned int buf_len, const char *pipe_name)
{
    char *recv_buf = (char *)malloc(buf_len);
    if(recv_buf == NULL){
        goto recv_buf_null;
    }
    char *send_buf = (char *)malloc(buf_len);
    if(send_buf == NULL){
        goto send_buf_null;
    }

    core_connect_t *connect = (core_connect_t *)calloc(1, sizeof(core_connect_t));
    if(connect == NULL){
        goto connect_null;
    }
    connect->recv_buf = recv_buf;
    connect->recv_buf_len = buf_len;
    connect->send_buf = send_buf;
    connect->send_buf_len = buf_len;
    connect->pipe_name = pipe_name;
    return connect;

connect_null:
    free(send_buf);
send_buf_null:
    free(recv_buf);
recv_buf_null:
    return NULL;
}

void destory_core_connect(core_connect_t **connect)
{
    if(connect == NULL || *connect == NULL){
        return;
    }

    free((*connect)->send_buf);
    free((*connect)->recv_buf);
    free(*connect);
    *connect = NULL;
}

/* core side */
int connect_monitor(core_connect_t *monitor_connect)
{
    // already connected
    if(monitor_connect->in_pipe != NULL && monitor_connect->out_pipe != NULL){
        return 1;
    }

    if(monitor_connect->pipe_name == NULL){
        LOG(LOG_ERROR, "No pipe name specified\n");
        return -1;
    }

    BOOL pipe_ok = WaitNamedPipe(monitor_connect->pipe_name, NMPWAIT_WAIT_FOREVER);
    if(!pipe_ok){
        LOG(LOG_ERROR, "Can't wait for the pipe: %s. Last error is %ld\n", monitor_connect->pipe_name, GetLastError());
        return -3;
    }
    monitor_connect->in_pipe = CreateFile(monitor_connect->pipe_name,
                          GENERIC_READ | GENERIC_WRITE,
                          0, NULL, OPEN_EXISTING, 0, NULL);
    // out pipe is the same as in pipe under Windows
    monitor_connect->out_pipe = monitor_connect->in_pipe;
    if(monitor_connect->in_pipe == INVALID_HANDLE_VALUE){
        LOG(LOG_ERROR, "Can't connect to peripheral monitor\n");
        return -4;
    }

    return 0;
}

/* check whether there has data sent from peripherals and receive it if it's true */
bool_t pmp_check_input(core_connect_t *monitor_connect)
{
    long int recv_len = pipe_recv(monitor_connect->recv_buf, monitor_connect->recv_buf_len, monitor_connect->in_pipe, FALSE);
    bool_t retval;
    if(recv_len > 0){
        monitor_connect->recv_len = recv_len;
        retval = TRUE;
    }else{
        retval = FALSE;
    }
    return retval;
}

/* directly send data to peripheral without using buffer */
int send_to_monitor_direct(core_connect_t *monitor_connect, char *data, unsigned int len)
{
    return pipe_send(data, len, monitor_connect->out_pipe);
}

void restart_send_packet(core_connect_t *connect)
{
    connect->send_len = 0;
    connect->retry = 0;
}

int make_pmp_data_packet(core_connect_t *connect, uint8_t peri_kind, uint16_t peri_index, uint8_t data_kind, uint8_t *data, uint32_t data_len)
{
    if(connect == NULL){
        return -1;
    }

    uint32_t packet_len = data_len +
                          sizeof(struct pmp_pkt_head_t) +
                          sizeof(struct data_pkt_head_t);

    if(connect->send_buf_len - connect->send_len < packet_len){
        return -2;
    }

    /* make pmp head */
    uint8_t *buf_start = (uint8_t *)&connect->send_buf[connect->send_len];
    struct pmp_pkt_head_t *pmp_head = (struct pmp_pkt_head_t *)buf_start;
    pmp_head->packet_kind = PMP_DATA;
    pmp_head->packet_len = packet_len;

    /* make data packet head */
    buf_start += sizeof(struct pmp_pkt_head_t);
    struct data_pkt_head_t *data_head = (struct data_pkt_head_t *)buf_start;
    data_head->data_kind = data_kind;
    data_head->peri_kind = peri_kind;
    data_head->peri_index = peri_index;

    buf_start += sizeof(struct data_pkt_head_t);
    memcpy(buf_start, data, data_len);

    connect->send_len += packet_len;
    return 0;
}

/* parse data packet */
static int pmp_data_packet(uint8_t *buf, uint32_t len, pmp_parsed_pkt_t *pkt)
{
    uint8_t *buffer = buf;

    struct data_pkt_head_t *data_head = (struct data_pkt_head_t *)buffer;
    buffer += sizeof(struct data_pkt_head_t);

    uint32_t data_len = len - sizeof(struct data_pkt_head_t);

    pkt->peri_kind = data_head->peri_kind;
    pkt->peri_index = data_head->peri_index;
    pkt->data_kind = data_head->data_kind;
    pkt->data_len = data_len;
    pkt->data = buffer;

    // Success
    return 0;
}

/* return the length of the data parsed this time */
static int parse_packet_once(uint8_t *buf, unsigned int recv_len, pmp_parsed_pkt_t *pkt)
{
    if(recv_len < sizeof(struct pmp_pkt_head_t)){
        LOG(LOG_DEBUG, "parse_packet_once: received packet uncomplete\n");
        return -PMP_ERR_DATA_LEN;
    }
    // do not change the input parameter
    uint8_t *buffer = buf;
    struct pmp_pkt_head_t *pmp_head = (struct pmp_pkt_head_t *)buffer;
    buffer += sizeof(struct pmp_pkt_head_t);

    if(recv_len < pmp_head->packet_len){
        LOG(LOG_DEBUG, "parse_packet_once: received packet uncomplete\n");
        return -PMP_ERR_DATA_LEN;
    }

    pkt->pkt_kind = pmp_head->packet_kind;

    int result;
    int rest_len = pmp_head->packet_len - sizeof(struct pmp_pkt_head_t);
    switch(pmp_head->packet_kind){
    case PMP_DATA:
        result = pmp_data_packet(buffer, rest_len, pkt);
        break;
    default:
        break;
    }

    if(result == 0){
        return pmp_head->packet_len;
    }else{
        return -PMP_ERR_DATA_SEND;
    }
}

/* Parse the peripheral monitor protocol(PMP).
   Return the rest of unparsed data in the buffer.
   */
int pmp_parse_input(core_connect_t *monitor_connect, pmp_parsed_pkt_t *pkt)
{
    uint8_t *recv_buf = (uint8_t *)monitor_connect->recv_buf;
    int recv_len = monitor_connect->recv_len;
    int parsed = monitor_connect->recv_parsed;
    if(recv_len <= 0){
        return -1;
    }
    int cur_result;

    memset(pkt, 0, sizeof(pmp_parsed_pkt_t));
    cur_result = parse_packet_once(recv_buf + parsed, recv_len - parsed, pkt);
    if(cur_result >= 0){
        monitor_connect->recv_parsed += cur_result;
        return parsed;
    }else{
        return cur_result;
    }
}

bool_t is_pmp_parse_finish(core_connect_t *monitor_connect)
{
    int recv_len = monitor_connect->recv_len;
    return monitor_connect->recv_parsed >= recv_len;
}

void pmp_start_parse_input(core_connect_t *monitor_connect)
{
    monitor_connect->recv_parsed = 0;
}

void pmp_clear_recv_buffer(core_connect_t *connect)
{
    connect->recv_len = 0;
}

/* monitor side */
int connect_armue_core(core_connect_t *connect)
{
    if(connect->pipe_name == NULL){
        return -1;
    }
    // create pipe if pipe is not exsisted
    if(connect->in_pipe == NULL && connect->out_pipe == NULL){
        connect->in_pipe = CreateNamedPipe(connect->pipe_name, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 1024, 1024, 0, NULL);
        // the pipe is duplex accessable under Windows
        connect->out_pipe = connect->in_pipe;
        if(connect->in_pipe == INVALID_HANDLE_VALUE){
            LOG(LOG_ERROR, "Can't create pipe\n");
            return -2;
        }
    }

    if(connect->in_pipe == NULL){
        connect->in_pipe = connect->out_pipe;
    }else if(connect->out_pipe == NULL){
        connect->out_pipe = connect->in_pipe;
    }

    // connect pipe;
    BOOL connected = ConnectNamedPipe(connect->in_pipe, NULL);
    if(connected){
        return SUCCESS;
    }else{
        LOG(LOG_ERROR, "Can't connect to core\n");
        return -2;
    }
}

int pmp_recv(core_connect_t *connect, bool_t block)
{
    long int recv_len = pipe_recv(connect->recv_buf, connect->recv_buf_len, connect->in_pipe, block);
    if(recv_len > 0){
        connect->recv_len = recv_len;
    }
    return recv_len;
}

int pmp_send(core_connect_t *connect)
{
    if(connect == NULL){
        return -1;
    }

    if(connect->send_len == 0){
        return 0;
    }

    long int sent = pipe_send(connect->send_buf, connect->send_len, connect->out_pipe);
    if(sent > 0){
        connect->send_len -= sent;
    }
    return sent;
}
