#ifndef _ERROR_CODE_H_
#define _ERROR_CODE_H_

typedef enum
{
    SUCCESS,
    ERROR_CREATE,
    ERROR_ADD,
    ERROR_CREATE_MODULE,
    ERROR_NULL_POINTER,
    ERROR_INVALID_PATH,
    ERROR_INVALID_MODULE,
    ERROR_INVALID_MODULE_PARAM,
    ERROR_DISMATCH_LIST,
    ERROR_REGISTERED,
    ERROR_INVALID_ROM_FILE,
    ERROR_CREATE_ROM_FILE,
    ERROR_MEMORY_MAP,
    ERROR_ROM_PARAM_DISMATCH,
    ERROR_FETCH,
    ERROR_NO_START_ROM,
    ERROR_SOC_STARTUP,
}error_code_t;

#define LOG_NONE             4
#define LOG_INFO             3
#define LOG_ERROR            3
#define LOG_WARN             2
#define LOG_DEBUG            1
#define LOG_ALL              0
#define LOG_CURRENT_LEVEL    LOG_ALL

#include <stdio.h>

#define LOG(debug_level, message, args...) \
    if(debug_level >= LOG_CURRENT_LEVEL){\
        printf("[%s] "message, #debug_level+4, ##args);\
    }

#ifdef _DEBUG
#define LOG_INSTRUCTION(message, args...)\
    printf("[INSTRUCTION] "message, ##args);

#define LOG_REG(cpu)\
        armv7m_print_state(cpu);
#else
#define LOG_INSTRUCTION(message, ...)
#define LOG_REG(regs)
#endif
#endif
