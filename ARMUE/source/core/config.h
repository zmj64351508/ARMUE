#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "_types.h"

typedef struct config_t{
    bool_t gdb_debug;
}config_t;


extern config_t config;

#ifdef __cplusplus
}
#endif

#endif // _CONFIG_H_
