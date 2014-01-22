#ifndef _LIST_H_
#define _LIST_H_
#ifdef __cplusplus
extern "C"{
#endif

typedef union{
    void *pdata;
    long long idata;
    unsigned long long udata;
    double ddata;
}list_data_t;

typedef struct list_t{
    list_data_t data;
    struct list_t *next;
    struct list_t *prev;
}list_t;

#define for_each_list_node(cur, list) \
for((cur) = (list)->next; (cur)->data.pdata != NULL; (cur) = (cur)->next)

#define list_create_node_int(data)\
list_create_node( (list_data_t)(long long)data )

#define list_create_node_ptr(data)\
list_create_node( (list_data_t)(void*)data )

#define list_create_node_double(data)\
list_create_node( (list_data_t)(double)data )

list_t*     list_create_node(list_data_t data);
void        list_destory_node(list_t **node);
list_t*     list_create_empty();
int         list_insert(list_t *node, list_t *insert);
int         list_append(list_t *head, list_t *append);
int         list_ahead(list_t *head, list_t *ahead);
list_data_t list_delete(list_t **node);

#ifdef __cplusplus
}
#endif
#endif // _LIST_H_
