#include <stdlib.h>
#include "exception_interrupt.h"
#include "error_code.h"

vector_exception_t* create_vector_exception(int table_size)
{
	vector_exception_t* controller = (vector_exception_t*)malloc(sizeof(vector_exception_t));
	if(controller == NULL){
		goto controller_null;
	}

	controller->vector_table = (uint32_t *)calloc(table_size, sizeof(uint32_t));
	if(controller->vector_table == NULL){
		goto vector_table_null;
	}

	controller->prio_table = (int *)calloc(table_size, sizeof(int));
	if(controller->prio_table == NULL){
		goto prio_table_null;
	}

	controller->vector_table_size = table_size;
	return controller;

prio_table_null:
	free(controller->vector_table);
vector_table_null:
	free(controller);
controller_null:
	return NULL;
}

void destory_vector_exception(vector_exception_t **exceptions)
{
	vector_exception_t* controller = *exceptions;
	free(controller->prio_table);
	free(controller->vector_table);
	free(controller);
	*exceptions = NULL;
}


// set the interrupt table
// NOTE: it doesn't check the array index
int set_vector_table(vector_exception_t* controller, uint32_t exception_value, int exception_num)
{
	if(controller == NULL){
		return -ERROR_NULL_POINTER;
	}

	controller->vector_table[exception_num] = exception_value;

	return SUCCESS;
}

uint32_t get_vector_value(vector_exception_t* controller, int exception_num)
{
	return controller->vector_table[exception_num];
}