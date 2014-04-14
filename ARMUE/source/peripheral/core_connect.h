#ifndef _CORE_CONNECT_H_
#define _CORE_CONNECT_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <windows.h>
#include "_types.h"
#include "peripheral.h"

enum PMP_PKT_KIND{
    PMP_DATA,
    PMP_CHECKSUM_OK,
    PMP_CHECKSUM_FAIL,
};

enum PMP_DATA_KIND{
    PMP_DATA_KIND_DENERIC,
    PMP_DATA_KIND_PROTOCOL,
    PMP_DATA_KIND_CONTROL,
};

#define PMP_SIZE_PKT_LEN    4 // 32 bit
#define PMP_SIZE_PKT_KIND   1 // 8 bit
#define PMP_SIZE_PERI_KIND  1 // 8 bit
#define PMP_SIZE_PERI_INDEX 2 // 16 bit
#define PMP_SIZE_DATA_KIND  1 // 8 bit
#define PMP_SIZE_CHECKSUM   1 // 8bit

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
}core_connect_t;

typedef struct pmp_parsed_pkt_t{
    int pkt_kind;
    int data_kind;
    int peri_index;
    int peri_kind;
    uint8_t *data;
    uint32_t data_len;
    bool_t valid;
}pmp_parsed_pkt_t;

core_connect_t *create_core_connect(unsigned int buf_len, const char *pipe_name);
void destory_core_connect(core_connect_t **connect);
void restart_send_packet(core_connect_t *connect);
void make_monitor_data_packet(core_connect_t *connect, uint8_t peri_kind, uint16_t peri_index, uint8_t *data, uint32_t data_len);
int pmp_parse_input(core_connect_t *monitor_connect, pmp_parsed_pkt_t *pkt);
void pmp_start_parse_input(core_connect_t *monitor_connect);
bool_t is_pmp_parse_finish(core_connect_t *monitor_connect);
#define pmp_parse_loop(connect) \
    pmp_start_parse_input(connect); \
    while(!is_pmp_parse_finish(connect))
bool_t pmp_check_input(core_connect_t *peri_connect);



/* functions used on core side */
int connect_monitor(core_connect_t *peri_connect);
int dispatch_peri_event(pmp_parsed_pkt_t *pkt, peripheral_table_t *peri_table);
int send_to_monitor_direct(core_connect_t *peri_connect, char *data, unsigned int len);

/* functions used on monitor side */
int connect_armue_core(core_connect_t *connect);
int recv_from_core(core_connect_t *connect, bool_t block);
int send_to_core(core_connect_t *connect);

#ifdef __cplusplus
}
#endif

#endif // _CORE_CONNECT_H_
