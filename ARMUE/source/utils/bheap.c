#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bheap.h"

int bheap_compare_int_smaller(void *a, void *b)
{
    return *(int*)a - *(int*)b;
}

bheap_t* bheap_create(int total_length, int data_size)
{
    bheap_t* heap = (bheap_t*)malloc(sizeof(bheap_t));
    if(heap == NULL){
        goto heap_null;
    }

    heap->data = (void *)calloc(total_length, data_size);
    if(heap->data == NULL){
        goto data_null;
    }

    heap->data_size = data_size;
    heap->total_length = total_length;
    heap->current_length = 0;
    return heap;

data_null:
    free(heap);
heap_null:
    return NULL;
}

int bheap_insert(bheap_t *heap, void *data_in, bheap_compare_t compare)
{
    if(heap->current_length == heap->total_length){
        return -1;
    }
    int cur_len = heap->current_length;
    uint8_t *data = (uint8_t*)heap->data;
    int data_size = heap->data_size;
    // data[cur_len] = data_in

    int child_index = 0, parent_index;
    int i = cur_len;
    uint8_t *parent;
    while(i > 0){
        child_index = i;
        parent_index = (i - 1) / 2;
        parent = data + data_size * parent_index;
        if(compare(data_in, parent) < 0){
            memcpy(data + data_size * child_index, parent, data_size);
            i = parent_index;
        }else{
            break;
        }
    }
    if(i == 0){
        memcpy(data, data_in, data_size);
    }else{
        memcpy(data + data_size * child_index, data_in, data_size);
    }
    heap->current_length++;
    return 0;
}

int bheap_peek_top(bheap_t *heap, void *data_out)
{
    if(heap->current_length == 0){
        return -1;
    }

    memcpy(data_out, heap->data, heap->data_size);
    return 0;
}

int bheap_delete_top(bheap_t *heap, void *data_out, bheap_compare_t compare)
{
    if(heap->current_length == 0){
        return -1;
    }
    int cur_len = heap->current_length;
    uint8_t *data = (uint8_t*)heap->data;
    int data_size = heap->data_size;

    memcpy(data_out, data, data_size);

    int i = 0;
    uint8_t *lchild, *rchild, *parent, *smaller = NULL;
    uint8_t *last = data + data_size * (cur_len - 1);
    while(i < cur_len / 2){
        parent = data + data_size * i;
        i = i * 2 + 1;
        lchild = data + data_size * i;

        /* Does right child exists? */
        if(i + 1 < cur_len){
            rchild = lchild + data_size;
            /* find smaller child */
            if(compare(lchild, rchild) < 0){
                smaller = lchild;
            }else{
                smaller = rchild;
                i++;
            }
        }else{
            smaller = lchild;
        }

        if(compare(last, smaller) > 0){
            memcpy(parent, smaller, data_size);
        }else{
            smaller = parent;
            break;
        }
    }
    heap->current_length--;
    if(heap->current_length > 0){
        memcpy(smaller, last, data_size);
    }
    return 0;
}

