#ifndef _RAM_H_
#define _RAM_H_

#include "_types.h"

typedef struct{
	bool_t allocated;
	uint32_t size;
	uint32_t base_address;
	uint8_t* memory_base;
}ram_t;

#endif