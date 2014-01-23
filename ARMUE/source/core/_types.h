#ifndef __TYPES_H_
#define __TYPES_H_

#define _IO
#define _I    const
#define _O

#include <stdint.h>

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0
#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1
typedef int8_t bool_t;


#endif
