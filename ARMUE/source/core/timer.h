#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "_types.h"
#include "list.h"
#include "cpu.h"

struct timer_t{
    cycle_t reload;
    cycle_t match;
    cycle_t start;
    uint32_t exception_num;
    int (*do_match)(struct timer_t *timer, cpu_t *cpu);
    union{
        void *user_data_ptr;
        int user_data_int;
    };
};
typedef struct timer_t timer_t;

static inline cycle_t calc_timer_match(cpu_t *cpu, cycle_t reload)
{
    return cpu->cycle + reload;
}

void check_timer(cpu_t *cpu);
timer_t *create_timer(int exception_num);
int destory_timer(timer_t **timer);
int add_timer(timer_t *timer, list_t *timer_list, bool_t ignore_duplicate);
int delete_timer(int exception_num, list_t *timer_list);
void restart_timer(timer_t *timer, cpu_t *cpu);
void start_timer(timer_t *timer, cpu_t *cpu, cycle_t reload, int (*do_match)(timer_t *timer, cpu_t *cpu));
cycle_t positive_timer_count(timer_t *timer, cpu_t *cpu);
cycle_t negative_timer_count(timer_t *timer, cpu_t *cpu);

#ifdef __cplusplus
}
#endif

#endif /* _TIMER_H_*/
