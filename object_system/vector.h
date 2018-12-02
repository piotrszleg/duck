#ifndef VECTOR_H
#define VECTOR_H

// TODO: add type generation through macro

#define VECTOR_INIT_CAPACITY 4

typedef struct vector {
    void **items;
    int capacity;
    int total;
} vector;

void vector_init(vector *);
int vector_total(vector *);
void vector_add(vector *, void *);
void vector_set(vector *, int, void *);
void *vector_get(vector *, int);
void vector_delete(vector *, int);
void vector_reverse(vector *);
void vector_free(vector *);

#endif