#ifndef _MODULE_HELPER_H_
#define _MODULE_HELPER_H_

#include <tchar.h>
#include "cpu.h"
#include "soc.h"
#include "peripheral.h"
#include "error_code.h"

#define MODULE_NAME_LENGTH 260
#define MODULE_PATH_LENGTH 260

typedef enum
{
	MODULE_INVALID = 0,
	MODULE_CPU,
	MODULE_PERIPHERAL,
}module_type_t;


typedef void* (*create_func_t)(void);
typedef int (*init_cpu_func_t)(struct cpu_t *cpu, struct soc_conf_t *config);

typedef void (*destory_func_t)(void** content);
typedef void (*destory_cpu_func_t)(cpu_t** cpu);
typedef error_code_t (*unregister_t)(void);


struct module_t;
typedef struct module_list_t
{
	int count;
	struct module_t* first_module;
	struct module_t* last_module;
}module_list_t;

typedef struct module_t
{
	int module_id;								// module id
	module_type_t type;							// module type: cpu or peripheral
	char name[MODULE_NAME_LENGTH];				// name of the module
	_TCHAR path[MODULE_PATH_LENGTH];			// where the module file stores, not use yet
	union{
		cpu_list_t* cpu_list;					
		peripheral_list_t* peripheral_list;
		void* content_list;
	};
	int content_count;							// not use yet

	union{
		init_cpu_func_t init_cpu;
		create_func_t create_content;
	};
	union{
		destory_cpu_func_t destory_cpu;
		destory_func_t destory_content;
	};
	unregister_t unregister;

	// module list
	struct module_t* next_module;
	struct module_t* pre_module;
	module_list_t* list;
}module_t;

module_t*		create_module();
error_code_t	destory_module(module_t** module);


error_code_t set_module_type(module_t* module, module_type_t type);
error_code_t set_module_content(module_t* module, void* content);
error_code_t set_module_content_create(module_t* module, create_func_t create_func);
error_code_t set_module_content_destory(module_t* module, destory_func_t destory_func);
error_code_t set_module_name(module_t* module, char* name);
error_code_t set_module_unregister(module_t *module, unregister_t unregister);

error_code_t	register_prepare();
void			register_all_modules();
error_code_t	unregister_all_modules();
module_t*		find_module(char* module_name);


error_code_t register_module_helper(module_t* module);
error_code_t unregister_module_helper(module_t* module);

#endif