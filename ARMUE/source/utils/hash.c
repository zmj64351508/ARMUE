#include <stdlib.h>
#include <stdio.h>
#include "hash.h"
#include "_types.h"

bool_t is_prime(int nr)
{
    int d;
    for(d = 2; (d * d) < (nr + 1); ++d){
        if (!(nr % d)){
            return FALSE;
        }
    }
    return TRUE;
}

unsigned int find_first_greater_prime(unsigned int num)
{
    for(; !is_prime(num); num++);
    return num;
}

hash_t *create_hash(int size)
{
    unsigned int new_size = find_first_greater_prime(size);
    hash_element_t *table = (hash_element_t *)calloc(new_size, sizeof(hash_element_t));
    if(table == NULL){
        goto table_null;
    }

    hash_t *hash = (hash_t *)malloc(sizeof(hash_t));
    if(hash == NULL){
        goto hash_null;
    }
    hash->max_size = new_size;
    hash->cur_size = 0;
    hash->table = table;
    return hash;

hash_null:
    free(table);
table_null:
    return NULL;
}

int destory_hash(hash_t **hash, void(*destory_content)(void *content))
{
    if(hash == NULL || *hash == NULL){
        return -1;
    }
    hash_element_t *table = (*hash)->table;

    // destory content if necessery
    int i;
    if(destory_content != NULL){
        for(i = 0; i < (*hash)->max_size; i++){
            destory_content(table[i].data.pdata);
        }
    }

    free(table);
    free(*hash);
    *hash = NULL;
    return 0;
}

/* is only_used = TRUE, only the element marked as HASH_ELEM_USED treated as used
   else HASH_ELEM_USED and HASH_ELEM_DELETED are bothe treated as USED */
static bool_t is_used(int info, bool_t only_used)
{
    if(only_used){
        return info == HASH_ELEM_USED;
    }else{
        return info != HASH_ELEM_EMPTY;
    }
}

static hash_element_t *_hash_find(hash_t *hash, int key, hash_func_t do_hash, bool_t only_used)
{
    int hash_val = do_hash(hash, key);

    int collision_num = 0;
    while(is_used(hash->table[hash_val].info, only_used) && hash->table[hash_val].key != key){
        // check if find fail
        if(collision_num == hash->max_size){
            return NULL;
        }
        // square law detection
        hash_val += 2 * ++collision_num - 1;
        while(hash_val >= hash->max_size){
            hash_val -= hash->max_size;
        }
    }
    return &hash->table[hash_val];
}

hash_element_t *hash_find(hash_t *hash, int key, hash_func_t do_hash)
{
    hash_element_t *elem = _hash_find(hash, key, do_hash, FALSE);

    /* not found */
    if(elem == NULL || elem->info != HASH_ELEM_USED){
        return NULL;
    }

    /* found */
    return elem;
}

int hash_insert(hash_t *hash, int key, hash_data_t data, hash_func_t do_hash)
{
    hash_element_t *elem = _hash_find(hash, key, do_hash, TRUE);
    /* can't insert */
    if(elem == NULL){
        return -1;
    }

    /* the returned position is empty or deleted */
    if(elem->info != HASH_ELEM_USED){
        elem->key = key;
        elem->data = data;
        elem->info = HASH_ELEM_USED;
        return 0;
    }

    /* already in the table */
    return 1;
}

hash_element_t *hash_delete(hash_t *hash, int key, hash_func_t do_hash)
{
    hash_element_t *elem = _hash_find(hash, key, do_hash, FALSE);
    /* find error, maybe the table is full */
    if(elem == NULL){
        return NULL;
    }

    /* found */
    if(elem->info == HASH_ELEM_USED){
        elem->info = HASH_ELEM_DELETED;
        return elem;
    }

    /* not found */
    return NULL;
}

void hash_dump(hash_t *hash, hash_func_t do_hash)
{
    int i;
    for(i = 0; i < hash->max_size; i++){
        printf("table[%d]: ", i);
        if(hash->table[i].info == HASH_ELEM_USED){
            printf("key = %d, data = %d", hash->table[i].key, hash->table[i].data.idata);
        }else if(hash->table[i].info == HASH_ELEM_EMPTY){
            printf("empty");
        }else{
            printf("deleted");
        }
        printf("\n");
    }
}
