/* This file is lecture notes from CS 3650, Fall 2018 */
/* Author: Nat Tuck */
/* Modified from HW4 submission */

#ifndef SVEC_H
#define SVEC_H

typedef struct svec {
    int size;
    char** data;
    int capacity;
    // TODO: complete struct
} svec;

svec* make_svec(int slots);
void  free_svec(svec* sv);

char* svec_get(svec* sv, int ii);
void  svec_put(svec* sv, int ii, char* item);

void svec_push_back(svec* sv, char* item);
svec* reverse(svec* sv);
void print_svec(svec* sv);

#endif
