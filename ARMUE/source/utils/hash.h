#ifndef _HASH_H_
#define _HASH_H_
#ifdef __cplusplus
extern "C"{
#endif

typedef union hash_data_t{
    int idata;
    unsigned int uidata;
    void *pdata;
}hash_data_t;

typedef struct hash_element_t{
    int key;
    int info;
    hash_data_t data;
}hash_element_t;

typedef struct hash_t{
    unsigned int max_size;
    unsigned int cur_size;
    hash_element_t *table;
}hash_t;

typedef int(*hash_func_t)(hash_t *hash, int key);

#define HASH_ELEM_EMPTY   0
#define HASH_ELEM_USED    1
#define HASH_ELEM_DELETED 2

hash_t *        create_hash(int size);
int             destory_hash(hash_t **hash, void(*destory_content)(void *content));
hash_element_t *hash_find(hash_t *hash, int key, hash_func_t do_hash);
int             hash_insert(hash_t *hash, int key, hash_data_t data, hash_func_t do_hash);
hash_element_t *hash_delete(hash_t *hash, int key, hash_func_t do_hash);
void            hash_dump(hash_t *hash, hash_func_t do_hash);

#ifdef __cplusplus
}
#endif
#endif // _HASH_H_]
