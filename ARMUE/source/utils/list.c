#include "list.h"
#include <stdio.h>

list_t* list_create_node(list_data_t data)
{
    list_t *node = (list_t*)malloc(sizeof(list_t));
    if(node == NULL){
        goto node_null;
    }
    node->next = NULL;
    node->prev = NULL;
    node->data = data;
    return node;

node_null:
    return NULL;
}

void list_destory_node(list_t **node)
{
    if(node == NULL || *node == NULL){
        return;
    }

    free(*node);
    *node = NULL;
}

list_t* list_create_empty()
{
    list_t *head = list_create_node((list_data_t)NULL);
    if(head == NULL){
        goto head_null;
    }

    head->prev = head;
    head->next = head;

    return head;

head_null:
    return NULL;
}

/* insert before node */
int list_insert(list_t *node, list_t *insert)
{
    if(node == NULL || insert == NULL || insert->data.pdata == NULL){
        return -1;
    }

    node->next->prev = insert;
    insert->next = node->next;
    node->next = insert;
    insert->prev = node;
}

/* append after the last element */
int list_append(list_t *head, list_t *append)
{
    if(append == NULL || head == NULL || append->data.pdata == NULL){
        return -1;
    }

    while(head->data.pdata != NULL){
        head = head->next;
    }
    head = head->prev;

    list_insert(head, append);
}

/* insert before the first element */
int list_ahead(list_t *head, list_t *ahead)
{
    if(head == NULL || ahead == NULL || ahead->data.pdata == NULL){
        return -1;
    }
    while(head->data.pdata != NULL){
        head = head->next;
    }

    list_insert(head, ahead);
}

/* Delete the node and pointer the parameter to next list node.
   Return the data which the deleted node contains */
list_data_t list_delete(list_t **node_to_delete)
{
    if(node_to_delete == NULL || *node_to_delete == NULL){
        return (list_data_t)NULL;
    }

    list_t *node = *node_to_delete;
    list_t *next = node->next;

    node->prev->next = node->next;
    node->next->prev = node->prev;
    list_data_t data = node->data;
    list_destory_node(node_to_delete);

    *node_to_delete = next;
    return data;
}
