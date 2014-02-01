#ifndef _ARM_GDB_STUB_H_
#define _ARM_GDB_STUB_H_
#ifdef __cplusplus
extern "C"{
#endif

#include <winsock2.h>
#include <windows.h>
#include "cpu.h"
#include "hash.h"
#include "_types.h"

#define MAX_PACKET_SIZE 2000
#define MAX_BP_TABLE_SIZE 1000

#define BP_TYPE_SWBP 0
#define BP_TYPE_HWBP 1
#define BP_TYPE_WWP  2
#define BP_TYPE_RWP  3
#define BP_TYPE AWP  4

enum rsp_status_t{
    RSP_HALTING = 0,
    RSP_CONTINUE,
    RSP_STEP,
};

typedef struct bp_t{
    uint32_t mem;
    int type;
    int length;
}bp_t;

typedef struct gdb_stub_t{
    int status;
    int port;
    SOCKET server;
    SOCKET client;
    char send_buf[MAX_PACKET_SIZE];
    int send_len;
    char recv_buf[MAX_PACKET_SIZE];
    int recv_len;
    hash_t *sw_breakpoint;
}gdb_stub_t;


gdb_stub_t *create_stub();
int init_stub(gdb_stub_t *stub);
int handle_rsp(gdb_stub_t *stub, cpu_t *cpu);
bool_t is_sw_breakpoint(gdb_stub_t *stub, uint32_t addr);


#ifdef __cplusplus
}
#endif
#endif // _ARM_GDB_STUB_H_
