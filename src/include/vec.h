
#ifndef __APOLLO_VEC_H__
#define __APOLLO_VEC_H__

#include <stdint.h>
#include <stdlib.h>

typedef struct Vec {
    uint32_t   size;
    uint32_t   count;
    uint32_t   total;
    uintptr_t *items;
} Vec;

void vec_new(Vec *vec) {
    vec->items = malloc(vec->size * vec->total);
}

void vec_free(Vec *vec) {
    free(vec->items);
}

void vec_push(Vec *vec, uintptr_t item) {
    if (vec->count < vec->total) {
        vec->items[vec->count] = item;
        vec->count++;
        return;
    }

    vec->total = vec->total + vec->total / 2;
    vec->items = realloc(vec->items, vec->size * vec->total);

    vec->items[vec->count] = item;
    vec->count++;
}

#endif  // __APOLLO_VEC_H__
