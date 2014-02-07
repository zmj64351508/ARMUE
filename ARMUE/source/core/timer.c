#include "timer.h"
#include "list.h"
#include <stdlib.h>
#include "cpu.h"


typedef list_t timer_list_t;

list_t *create_timer_list()
{
    list_t *timer_list = list_create_empty();
    return timer_list;
}

/* create timer and add to the list */
timer_t *create_timer(int exception_num)
{
    timer_t *timer = (timer_t *)calloc(1, sizeof(timer_t));
    if(timer == NULL){
        goto timer_error;
    }
    timer->exception_num = exception_num;
    return timer;

timer_error:
    return NULL;
}

int destory_timer(timer_t **timer)
{
    free(*timer);
    *timer = NULL;
    return 0;
}

/* find timer in the list using exception number as the id */
list_t *find_timer(int exception_num, list_t *timer_list)
{
    list_t *cur;
    timer_t *cur_timer;
    for_each_list_node(cur, timer_list){
        cur_timer = (timer_t *)cur->data.pdata;
        if(cur_timer->exception_num == exception_num){
            return cur;
        }
    }
    return NULL;
}

int add_timer(timer_t *timer, list_t *timer_list, bool_t ignore_duplicate)
{
    if(timer_list == NULL || timer == NULL){
        return -1;
    }

    // check whether the timer is exist in the list already
    timer_t *timer_found;
    list_t *cur = find_timer(timer->exception_num, timer_list);
    if(cur != NULL){
        LOG(LOG_DEBUG, "add_timer: timer with exp_num %d is in the list\n", timer->exception_num);
        if(ignore_duplicate){
            return 1;
        }else{
            timer_found = (timer_t *)cur->data.pdata;
            timer_found->match = timer->match;
            timer_found->reload = timer->reload;
            return 1;
        }
    }

    list_t *new_node = list_create_node_ptr(timer);
    if(new_node == NULL){
        goto error;
    }
    list_append(timer_list, new_node);
    return 0;

error:
    return -1;
}

int delete_timer(int exception_num, list_t *timer_list)
{
    timer_t *timer_deleted;
    list_t *found;
    found = find_timer(exception_num, timer_list);
    if(found != NULL){
        timer_deleted = list_delete(&found).pdata;
        LOG(LOG_DEBUG, "delete_timer: deleted excep_num = %d\n", timer_deleted->exception_num);
        destory_timer(&timer_deleted);
    }
    return 0;
}

void start_timer(timer_t *timer, cpu_t *cpu, cycle_t reload, int (*do_match)(timer_t *timer, cpu_t *cpu))
{
    timer->reload = reload;
    timer->do_match = do_match;
    timer->match = calc_timer_match(cpu, reload);
    timer->start = cpu->cycle;
}

void restart_timer(timer_t *timer, cpu_t *cpu)
{
    timer->match = calc_timer_match(cpu, timer->reload);
    timer->start = cpu->cycle;
}

cycle_t positive_timer_count(timer_t *timer, cpu_t *cpu)
{
    return cpu->cycle - timer->start;
}

cycle_t negative_timer_count(timer_t *timer, cpu_t *cpu)
{
    return timer->match - cpu->cycle;
}

void check_timer(cpu_t *cpu)
{
    list_t *timer_list = cpu->timer_list;
    cycle_t cur_cycle = cpu->cycle;

    int retval;
    list_t *cur_node, *next_node;
    timer_t *timer;
    for_each_list_node_safe(cur_node, next_node, timer_list){
        timer = (timer_t *)cur_node->data.pdata;
        if(timer->match <= cur_cycle){
            retval = timer->do_match(timer, cpu);
            restart_timer(timer, cpu);
        }
    }
}
