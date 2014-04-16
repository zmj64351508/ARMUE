#ifndef _CORE_CONNECT_H_
#define _CORE_CONNECT_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <windows.h>
#include "_types.h"

#define PMP_MAX_RETRY 10

enum PMP_PKT_KIND{
    PMP_DATA,
    PMP_CHECKSUM_OK,
    PMP_CHECKSUM_FAIL,
    PMP_INFO_REQUEST,
    PMP_INFO_ACK,
};

enum PMP_ACK{
    PMP_OK,
    PMP_FAIL,
};

enum PMP_DATA_KIND{
    PMP_DATA_KIND_DENERIC,
    PMP_DATA_KIND_PROTOCOL,
    PMP_DATA_KIND_CONTROL,
};

enum PMP_ERR{
    PMP_ERR_CHECKSUM_FAIL = 1,
    PMP_ERR_CHECKSUM_REPLY_OK,
    PMP_ERR_CHECKSUM_REPLY_FAIL,
    PMP_ERR_DATA_LEN,
    PMP_ERR_DATA_SEND,
};

/* PMP headers */
#pragma pack(1)
struct pmp_pkt_head_t{
    uint8_t packet_kind;
    uint32_t packet_len;
};

struct data_pkt_head_t{
    uint8_t peri_kind;
    uint16_t peri_index;
    uint8_t data_kind;
};
#pragma pack()

typedef HANDLE pipe_t;
typedef struct core_connect_t{
    const char *pipe_name;
    pipe_t in_pipe;
    pipe_t out_pipe;
    int recv_buf_len;
    int recv_len;
    int recv_parsed;
    int send_buf_len;
    int send_len;
    char *recv_buf;
    char *send_buf;
    int retry;
}core_connect_t;

struct pmp_parsed_pkt_t{
    int pkt_kind;
    int data_kind;
    int peri_index;
    int peri_kind;
    uint8_t *data;
    uint32_t data_len;
    bool_t valid;
};
typedef struct pmp_parsed_pkt_t pmp_parsed_pkt_t;

#include "peripheral.h"
core_connect_t *create_core_connect(unsigned int buf_len, const char *pipe_name);
void destory_core_connect(core_connect_t **connect);
void restart_send_packet(core_connect_t *connect);
void pmp_clear_recv_buffer(core_connect_t *connect);
int make_pmp_data_packet(core_connect_t *connect, uint8_t peri_kind, uint16_t peri_index, uint8_t data_kind, uint8_t *data, uint32_t data_len);
int pmp_parse_input(core_connect_t *monitor_connect, pmp_parsed_pkt_t *pkt);
void pmp_start_parse_input(core_connect_t *monitor_connect);
bool_t is_pmp_parse_finish(core_connect_t *monitor_connect);
#define pmp_parse_loop(connect) \
    pmp_start_parse_input(connect); \
    while(!is_pmp_parse_finish(connect))
bool_t pmp_check_input(core_connect_t *peri_connect);


/* functions used on core side */
int connect_monitor(core_connect_t *peri_connect);
int send_to_monitor_direct(core_connect_t *peri_connect, char *data, unsigned int len);

/* functions used on monitor side */
int connect_armue_core(core_connect_t *connect);
int pmp_recv(core_connect_t *connect, bool_t block);
int pmp_send(core_connect_t *connect);

#ifdef __cplusplus
}
#endif

#endif // _CORE_CONNECT_H_
