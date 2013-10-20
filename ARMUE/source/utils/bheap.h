#ifndef _BHEAP_H_
#define _BHEAP_H_
#ifdef __cplusplus
extern "C"{
#endif

typedef struct bheap_t{
    void *data;
    int data_size;
    int current_length;
    int total_length;    
}bheap_t;

typedef int (*bheap_compare_t)(void *a, void *b);

/* general compare method */
int bheap_compare_int_smaller(void *a, void *b);

bheap_t* bheap_create(int total_length, int data_size);
int bheap_insert(bheap_t *heap, void *data_in, bheap_compare_t compare);
int bheap_peek_top(bheap_t *heap, void *data_out);
int bheap_delete_top(bheap_t *heap, void *data_out, bheap_compare_t compare);

#ifdef __cplusplus
}
#endif
#endif