#include "core_connect.h"
#include "error_code.h"
#include "_types.h"
#include "io.h"

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
bool_t check_monitor_input(core_connect_t *monitor_connect)
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

/* packet format like [peri_kind 8 bit] [peri_index 16 bit] [data_len 32 bit] [data] */
#define MONITOR_HEAD_LEN 5
#define MONITOR_DATA_HEAD_KIND_LEN 1
#define MONITOR_DATA_HEAD_INDEX_LEN 2
#define MONITOR_DATA_HEAD_LEN (MONITOR_DATA_HEAD_KIND_LEN + MONITOR_DATA_HEAD_INDEX_LEN)

/* packet format like [peri_kind 8 bit] [peri_index 16 bit] [data] */
static int64_t parse_monitor_data(uint8_t *buffer, unsigned int len, peripheral_table_t *peri_table)
{
    if(len < MONITOR_DATA_HEAD_LEN + 1){
        LOG(LOG_ERROR, "Wrong data length, maybe coursed by an uncomplete receiving\n");
        return -1;
    }
    uint8_t peri_kind = buffer[0];
    uint16_t peri_index = *(uint16_t *)&buffer[MONITOR_DATA_HEAD_KIND_LEN];

    if(peri_kind >= PERI_MAX_KIND || peri_index >= peri_table[peri_kind].num){
        LOG(LOG_ERROR, "Peripheral with kind: %d, index: %d\n doesn't exsit\n", peri_kind, peri_index);
        return -3;
    }

    // call the data process function which should be implement with the peripheral
    int retval;
    peripheral_t *cur_peri = &peri_table[peri_kind].real_peri[peri_index];
    void *user_data = cur_peri->user_data;
    if(cur_peri->data_process != NULL){
        retval = cur_peri->data_process(&buffer[MONITOR_DATA_HEAD_LEN], len - MONITOR_DATA_HEAD_LEN, user_data);
        if(retval < 0){
            return retval;
        }
        return len;
    }else{
        LOG(LOG_ERROR, "data process for the peripheral %d:%d is NULL\n", peri_kind, peri_index);
        return -4;
    }
}

void restart_send_packet(core_connect_t *connect)
{
    connect->send_len = 0;
}

void make_monitor_data_packet(core_connect_t *connect, uint8_t peri_kind, uint16_t peri_index, uint8_t *data, uint32_t data_len)
{
    uint8_t *buffer = (uint8_t *)connect->send_buf + connect->send_len;
    buffer[0] = MONITOR_DATA;
    *(uint32_t *)&buffer[1] = data_len + MONITOR_DATA_HEAD_LEN + MONITOR_HEAD_LEN;
    buffer[MONITOR_HEAD_LEN] = peri_kind;
    *(uint16_t *)&buffer[MONITOR_HEAD_LEN + MONITOR_DATA_HEAD_KIND_LEN] = peri_index;
    memcpy(&buffer[MONITOR_HEAD_LEN + MONITOR_DATA_HEAD_LEN], data, data_len);
    connect->send_len += MONITOR_HEAD_LEN + MONITOR_DATA_HEAD_LEN + data_len;
}

/* return the length of the data parsed this time */
static int64_t parse_packet_once(uint8_t *buffer, unsigned int recv_len, peripheral_table_t *peri_table)
{
    if(recv_len <= 0){
        return 0;
    }
    int64_t parsed_len;
    uint8_t packet_kind = buffer[0];
    uint32_t packet_len = *(uint32_t *)&buffer[1];
    uint32_t rest_len = packet_len - MONITOR_HEAD_LEN;
    // check the packet length and received length
    if(recv_len < packet_len){
        LOG(LOG_DEBUG, "parse_packet_once: received packet uncomplete\n");
        return -1;
    }
    uint8_t *rest_data = buffer + MONITOR_HEAD_LEN;
    switch(packet_kind){
    case MONITOR_DATA:
        parsed_len = parse_monitor_data(rest_data, rest_len , peri_table);
        break;
    // unknow packet, ignoring it
    default:
        parsed_len = rest_len;
        return 0;
    }
    if(parsed_len < 0){
        return parsed_len;
    }else{
        return parsed_len + MONITOR_HEAD_LEN;
    }
}

/* Parse the monitor connect protocol.
   Return the rest of unparsed data in the buffer.

   The packet has the format like:
        [packet kind] [packet length] [packet specific data]
   in which packet kind is an 8-bit value which can be MONITOR_CONTROL, MONITOR_DATA

   For MONITOR_DATA, the rest of packet should be:
        [MONITOR_DATA] [peripheral kind] [peripheral index] [data]
   in which peripheral kind is an 8-bit value
            peripheral index is an 16-bit value
            length of reset data is an 32-bit value
            data stands for the real data to transmit
   */
int64_t parse_monitor_input(core_connect_t *monitor_connect, peripheral_table_t *peri_table)
{
    uint8_t *recv_buf = (uint8_t *)monitor_connect->recv_buf;
    int64_t recv_len = monitor_connect->recv_len;
    if(recv_len <= 0){
        return -1;
    }
    int64_t parsed_len;

    while(1){
        parsed_len = parse_packet_once(recv_buf, recv_len, peri_table);
        if(parsed_len >= 0){
            recv_len -= parsed_len;
            recv_buf += parsed_len;
        }else{
            return recv_len;
        }
        if(recv_len <= 0){
            break;
        }
    }
    return 0;
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

int recv_from_core(core_connect_t *connect, bool_t block)
{
    long int recv_len = pipe_recv(connect->recv_buf, connect->recv_buf_len, connect->in_pipe, block);
    if(recv_len > 0){
        connect->recv_len = recv_len;
    }
    return recv_len;
}

int send_to_core(core_connect_t *connect)
{
    if(connect->send_len == 0){
        return 0;
    }

    long int sent = pipe_send(connect->send_buf, connect->send_len, connect->out_pipe);
    if(sent > 0){
        connect->send_len -= sent;
    }
    return sent;
}
