#ifndef _CORE_CONNECT_H_
#define _CORE_CONNECT_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <windows.h>
#include "_types.h"
#include "peripheral.h"

enum monitor_pkt_kind_t{
    MONITOR_DATA,
    MONITOR_CONTROL,
};

typedef HANDLE pipe_t;
typedef struct core_connect_t{
    const char *pipe_name;
    pipe_t in_pipe;
    pipe_t out_pipe;
    unsigned int recv_buf_len;
    unsigned int recv_len;
    unsigned int send_buf_len;
    unsigned int send_len;
    char *recv_buf;
    char *send_buf;
}core_connect_t;

core_connect_t *create_core_connect(unsigned int buf_len, const char *pipe_name);
void destory_core_connect(core_connect_t **connect);
void restart_send_packet(core_connect_t *connect);
void make_monitor_data_packet(core_connect_t *connect, uint8_t peri_kind, uint16_t peri_index, uint8_t *data, uint32_t data_len);

/* functions used on core side */
int connect_monitor(core_connect_t *peri_connect);
bool_t check_monitor_input(core_connect_t *peri_connect);
int64_t parse_monitor_input(core_connect_t *monitor_connect, peripheral_table_t *peri_table);

int send_to_monitor_direct(core_connect_t *peri_connect, char *data, unsigned int len);

/* functions used on monitor side */
int connect_armue_core(core_connect_t *connect);
int recv_from_core(core_connect_t *connect, bool_t block);
int send_to_core(core_connect_t *connect);

#ifdef __cplusplus
}
#endif

#endif // _CORE_CONNECT_H_
