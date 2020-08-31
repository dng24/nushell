/* This file is lecture notes from CS 3650, Fall 2018 */
/* Author: Nat Tuck */
/* Modified from HW4 submission */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "svec.h"

svec*
make_svec(int slots)
{
    svec* sv = malloc(sizeof(svec));
    sv->data = calloc(slots, sizeof(char*));
    sv->size = 0;
    sv->capacity = slots;
    return sv;
}

void
free_svec(svec* sv)
{
    for (int i = 0; i < sv->size; i++) {
        free(sv->data[i]);
    }
    free(sv->data);
    free(sv);
}

char*
svec_get(svec* sv, int ii)
{
    assert(ii >= 0 && ii < sv->size);
    return sv->data[ii];
}

void
svec_put(svec* sv, int ii, char* item)
{
    assert(ii >= 0 && ii < sv->size);
    sv->data[ii] = strdup(item);
}

void
svec_push_back(svec* sv, char* item)
{
    int ii = sv->size;
    if (ii >= sv->capacity) {
        char** new_data = calloc(2 * sv->capacity, sizeof(char*));
        for (int i = 0; i < sv->capacity; i++) {
            new_data[i] = strdup(sv->data[i]);
            free(sv->data[i]);
        }
        free(sv->data);
        sv->data = new_data;
        sv->capacity *= 2;
    }

    sv->size = ii + 1;
    svec_put(sv, ii, item);
}

svec* reverse(svec* sv) {
    svec* rev = make_svec(sv->size);
    for (int i = sv->size - 1; i >= 0; i--) {
        svec_push_back(rev, sv->data[i]);
    }
    return rev;
}

void print_svec(svec* sv) {
    for (int i = 0; i < sv->size; i++) {
        printf("%s\n", sv->data[i]);
    }
}

