#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "module_helper.h"

static module_list_t *g_cpu_module_list = NULL;
static module_list_t *g_peripheral_module_list = NULL;

module_list_t* create_module_list()
{
    module_list_t* list = (module_list_t*)calloc(1, sizeof(module_list_t));
    if(list == NULL){
        return NULL;
    }

    return list;
}

module_t* create_module()
{
    module_t* module = (module_t*)calloc(1, sizeof(module_t));
    if(module == NULL){
        return NULL;
    }

    return module;
}

error_code_t add_module_to_tail(module_list_t* list, module_t* module)
{
    module_t* last_module = list->last_module;
    module_t* first_module = list->first_module;

    if(list->last_module == NULL){
        list->first_module = module;
        list->last_module = module;
    }else{
        last_module->next_module = module;
        module->pre_module = last_module;
        list->last_module = module;
    }

    list->count++;
    module->list = list;

    return SUCCESS;
}


error_code_t delete_module_from_head(module_list_t* list, module_t* module)
{
    if(list == NULL || module == NULL){
        return ERROR_NULL_POINTER;
    }

    if(module->list != list){
        return ERROR_DISMATCH_LIST;
    }

    if(list->count == 1){
        list->first_module = NULL;
        list->last_module = NULL;
    }else{
        list->first_module = module->next_module;
        list->first_module->pre_module = NULL;
    }

    list->count--;

    return SUCCESS;
}

error_code_t delete_module_from_tail(module_list_t* list, module_t* module)
{
    if(list == NULL || module == NULL){
        return ERROR_NULL_POINTER;
    }

    if(module->list != list){
        return ERROR_DISMATCH_LIST;
    }


    if(list->count == 1){
        list->first_module = NULL;
        list->last_module = NULL;
    }else{
        list->last_module = module->pre_module;
        list->last_module->next_module = NULL;
    }

    list->count--;

    return SUCCESS;
}

error_code_t delete_module_from_middle(module_list_t* list, module_t* module)
{
    if(list == NULL || module == NULL){
        return ERROR_NULL_POINTER;
    }

    if(module->list != list){
        return ERROR_DISMATCH_LIST;
    }

    module_t* pre_module = module->pre_module;
    module_t* next_module = module->next_module;

    if(list->count == 1){
        list->first_module = NULL;
        list->last_module = NULL;
    }else{
        pre_module->next_module = next_module;
        next_module->pre_module = pre_module;
    }

    list->count--;

    return SUCCESS;

}


error_code_t delete_module(module_list_t* list, module_t* module)
{
    if(list == NULL || module == NULL){
        return ERROR_NULL_POINTER;
    }

    error_code_t error_code = SUCCESS;

    if(module->pre_module == NULL){
        error_code = delete_module_from_head(list, module);
    }else if(module->next_module == NULL){
        error_code = delete_module_from_tail(list, module);
    }else{
        error_code = delete_module_from_middle(list, module);
    }

    return error_code;
}

error_code_t destory_module(module_t** module)
{
    if(module == NULL || *module == NULL)
        return ERROR_NULL_POINTER;

    free(*module);
    *module = NULL;

    return SUCCESS;
}

error_code_t set_module_type(module_t* module, module_type_t type)
{
    module->type = type;
    return SUCCESS;
}

error_code_t set_module_content_list(module_t* module, void* content_list)
{
    module->content_list = content_list;
    return SUCCESS;
}

error_code_t set_module_content_create(module_t* module, create_func_t create_func)
{
    module->create_content = create_func;
    return SUCCESS;
}

error_code_t set_module_content_destory(module_t* module, destory_func_t destory_func)
{
    module->destory_content = destory_func;
    return SUCCESS;
}

error_code_t set_module_name(module_t* module, char* name)
{
    strcpy(module->name,name);
    return SUCCESS;
}

error_code_t set_module_unregister(module_t *module, unregister_t unregister)
{
    module->unregister = unregister;
    return SUCCESS;
}

error_code_t register_prepare()
{

    if(g_cpu_module_list == NULL){
        g_cpu_module_list = create_module_list();
    }
    if(g_peripheral_module_list == NULL){
        g_peripheral_module_list = create_module_list();
    }

    return SUCCESS;
}

/****** Help other modules to register themselves to the system ******/
// TODO: check the parameter of module
error_code_t register_module_helper(module_t* module)
{
    //LOG(LOG_DEBUG, "register_module_helper\n");
    if(module == NULL){
        return ERROR_NULL_POINTER;
    }
    if( module->name == NULL ||
        module->type == MODULE_INVALID ||
        module->create_content == NULL ||
        module->destory_content == NULL ||
        module->unregister == NULL){
        if(module->name != NULL){
            LOG(LOG_ERROR, "register_module_helper: register module %s fail\n", module->name);
        }else{
            LOG(LOG_ERROR, "register_module_helper: register unnamed module\n");
        }
        return ERROR_INVALID_MODULE_PARAM;
    }


    module->content_count = 0;

    switch(module->type){
    case MODULE_CPU:
        module->cpu_list = create_cpu_list();
        //module->module_id = g_cpu_module_list->count+1;
        add_module_to_tail(g_cpu_module_list, module);
        // add module to cpu module list
        break;
    case MODULE_PERIPHERAL:
        //module->peripheral_list = create_peripheral_list();
        //add_module_to_tail(g_peripheral_module_list, module);
        // add module to peripheral module list
        break;
    default:
        return ERROR_INVALID_MODULE;
        break;
    }

    return SUCCESS;
}

/****** Help other modules to unregister themselves from the system ******/
error_code_t unregister_module_helper(module_t* module)
{
    LOG(LOG_DEBUG, "unregister_module_helper\n");
    if(module == NULL){
        return ERROR_NULL_POINTER;
    }


    switch(module->type){
    case MODULE_CPU:
        delete_module(g_cpu_module_list, module);
        destory_cpu_list(&module->cpu_list);

        break;
    case MODULE_PERIPHERAL:
        //destory_peripheral_list(module->peripheral_list);

        break;
    default:
        break;
    }

    return SUCCESS;
}

error_code_t validate_module_file(_TCHAR* module_path)
{
    return SUCCESS;
}

/****** Register all the modules to the system ******/
void register_all_modules(){

    register_prepare();

    // register the module, NEED to be replaced by dll later
    extern error_code_t register_armcm3_module();
    register_armcm3_module();

}

module_t* get_first_module(module_list_t* list)
{
    return list->first_module;
}

module_t* get_next_module(module_t* module)
{
    return module->next_module;
}


error_code_t unregister_modules_by_list(module_list_t* list)
{
    if(list == NULL){
        return ERROR_NULL_POINTER;
    }

    module_t* next_module;
    module_t* module = get_first_module(list);

    if(module != NULL){
        do{
            next_module = get_next_module(module);
            module->unregister();
            module = next_module;
        }while(module != NULL);
    }

    return SUCCESS;
}

error_code_t unregister_all_modules()
{
    // unregister cpu modules
    unregister_modules_by_list(g_cpu_module_list);

    // unregister peripheral modules
    unregister_modules_by_list(g_peripheral_module_list);

    return SUCCESS;
}


module_t* find_module(char* module_name)
{
    // find cpu_module
    module_t* next_module;
    module_t* module = get_first_module(g_cpu_module_list);
    if(module != NULL){
        do{
            next_module = get_next_module(module);
            if(strcmp(module_name, module->name) == 0){
                return module;
            }
            module = next_module;
        }while(module != NULL);
    }

    // find peripheral modules
    module = get_first_module(g_peripheral_module_list);

    if(module != NULL){
        do{
            next_module = get_next_module(module);
            if(strcmp(module_name, module->name) == 0){
                return module;
            }
            module = next_module;
        }while(module != NULL);
    }

    return NULL;
}
