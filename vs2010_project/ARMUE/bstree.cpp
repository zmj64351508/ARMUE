#include <stdlib.h>
#include "bstree.h"

bstree_node_t* bstree_create_node(void* data)
{
	bstree_node_t* new_node = (bstree_node_t*)malloc(sizeof(bstree_node_t));
	if(new_node == NULL){
		goto out;
	}

	new_node->data = data;
	new_node->lchild = new_node->rchild = NULL;

out:
	return new_node;
}

int bstree_destory_node(bstree_node_t** node)
{
	if(node == NULL || *node == NULL){
		return -1;
	}
	free(*node);
	*node = NULL;
	return 0;
}

/* return the new root if success and return NULL if failed, ignore the duplicate*/
bstree_node_t* bstree_add_node(bstree_node_t* root, bstree_node_t* to_add, bstree_cmp_t compare)
{
	if(root == NULL){
		return to_add;
	}

	bstree_node_t* new_child;
	int result = compare(to_add->data, root->data);
	if(result < 0){
		new_child = bstree_add_node(root->lchild, to_add, compare);
		if(new_child != NULL){
			root->lchild = new_child;
		}else{
			return NULL;
		}
	}else if(result > 0){
		new_child = bstree_add_node(root->rchild, to_add, compare);
		if(new_child != NULL){
			root->rchild = new_child;
		}else{
			return NULL;
		}
	}
	return root;
}

bstree_node_t* bstree_find_node(bstree_node_t* root, void* data, bstree_cmp_t compare)
{
	if(root == NULL){
		return NULL;
	}

	bstree_node_t* found;
	int cmp_result = compare(data, root->data);
	if(cmp_result < 0){
		found = bstree_find_node(root->lchild, data, compare);
		return found;
	}else if(cmp_result > 0){
		found = bstree_find_node(root->rchild, data, compare);
		return found;
	}else{
		return root;
	}
}

bstree_node_t* bstree_find_delete_min(bstree_node_t** root, bstree_cmp_t compare)
{
	bstree_node_t **cur = root;
	while((*cur)->lchild){
		cur = &(*cur)->lchild;	
	}
	bstree_node_t *found = *cur;
	*cur = NULL;
	return found;
}

/* return the new root */
bstree_node_t* bstree_delete_node(bstree_node_t* root, void* data, bstree_cmp_t compare)
{
	if(root == NULL){
		return root;
	}

	bstree_node_t* retval;
	bstree_node_t* found;
	int cmp_result = compare(data, root->data);
	if(cmp_result < 0){
		root->lchild = bstree_delete_node(root->lchild, data, compare);
		return root;
	}else if(cmp_result > 0){
		root->rchild = bstree_delete_node(root->rchild, data, compare);
		return root;
	}else{
		/* 2 children */
		if(root->lchild && root->rchild){
			found = bstree_find_delete_min(&root->rchild, compare);
			root->data = found->data;
			bstree_destory_node(&found);
			retval = root;
		}else{
			/* only left */
			if(root->lchild){
				retval = root->lchild;
			/* only right */
			}else if(root->rchild){
				retval = root->rchild;
			/* no child */
			}else{
				retval = NULL;
			}
			bstree_destory_node(&root);
		}
		return retval;
	}
}