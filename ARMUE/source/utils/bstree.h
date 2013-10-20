#ifndef _BSTREE_H_
#define _BSTREE_H_
#ifdef __cplusplus
extern "C"{
#endif

/* return -1 when data1 < data2
           0 when data1 = data2
           1 when data1 > data2 */
typedef int (*bstree_cmp_t)(void* data1, void* data2);

typedef struct bstree_node_t{
    void* data;
    struct bstree_node_t* lchild;    
    struct bstree_node_t* rchild;    
}bstree_node_t;

bstree_node_t* bstree_create_node(void* data);
bstree_node_t* bstree_add_node(bstree_node_t* root, bstree_node_t* to_add, bstree_cmp_t compare);
bstree_node_t* bstree_find_node(bstree_node_t* root, void* data, bstree_cmp_t compare);
bstree_node_t* bstree_find_delete_min(bstree_node_t** root, bstree_cmp_t compare);
bstree_node_t* bstree_delete_node(bstree_node_t* root, void* data, bstree_cmp_t compare);

#ifdef __cplusplus
}
#endif
#endif