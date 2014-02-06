#ifndef __TYPES_H_
#define __TYPES_H_

#define IOput
#define Input    const
#define Output

#include <stdint.h>

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

/* MAYBE can be set to the bool_t to indicate it should be judged in the future.
   or the program can use the style like "if(foo)" to treat MAYBE as TRUE.
   As this feature, the bool_t is not strict bool type, but it is useful sometimes */
#ifdef MAYBE
#undef MAYBE
#endif
#define MAYBE (-1)
typedef int8_t bool_t;

typedef unsigned long long cycle_t;

#endif
