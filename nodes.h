#ifndef NODES_H_
# define NODES_H_

#include <stdlib.h>
#include <stdio.h>

typedef struct node {
    float val;
    struct node *next;
} Node; 

typedef struct{
    Node *head, *tail;
} Queue;

int dequeue(Node **head, Node **tail) {
    Node *t;
    if (!*head) return -1;
    if (*head == *tail) *tail = NULL;

    t = (*head)->next;
    free(*head);
    *head = t;
    return 0;
}

int enqueue(Node **head, Node **tail, float val) {
    Node *tmp;
    tmp = (Node *) malloc(sizeof(Node));
    if (!tmp) return -1;

    tmp->val = val;
    tmp->next = NULL;
    if (!*head) {
        *head = tmp;
        *tail = tmp;
    } else {
        (*tail)->next = tmp;
        *tail = (*tail)->next;
    }
    return 0;
}

//Not used
int print(Node *head) {
    Node *trenutni;

    if(!head) return -1;

    for(trenutni=head; trenutni!=NULL; trenutni=trenutni->next) {
        printf("%f ", trenutni->val);
    }
    printf("\n");

    return 0;
}

#endif