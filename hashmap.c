
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// TODO: sudo apt install libbsd-dev
// This provides strlcpy
// See "man strlcpy"
#include <bsd/string.h>
#include <string.h>

#include "hashmap.h"


int
hash(char* key)
{
    // TODO: Produce an appropriate hash value.
    int code = 0;
    for (int i = 0; i < strlen(key); i++) {
       code = 67 * code + key[i];
    } 
    return code;
}

hashmap*
make_hashmap_presize(int nn)
{
    hashmap* hh = calloc(1, sizeof(hashmap));
    // TODO: Allocate and initialize a hashmap with capacity 'nn'.
    // Double check "man calloc" to see what that function does.
    hh->data = calloc(nn, sizeof(hashmap_pair*));
    hh->size = 0;
    hh->capacity = nn;

    for (int i = 0; i < hh->capacity; i++) {
        hashmap_pair* pair = malloc(sizeof(hashmap_pair));
        pair->key = 0;
        pair->val = 0;
        pair->used = false;
        pair->tomb = false;
        hh->data[i] = pair;
    }
    return hh;
}

hashmap*
make_hashmap()
{
    return make_hashmap_presize(4);
}

void
free_hashmap(hashmap* hh)
{
    // TODO: Free all allocated data.
    for (int i = 0; i < hh->capacity; i++) {
        free(hh->data[i]->key);
        free(hh->data[i]->val);
        free(hh->data[i]);
    }
    free(hh->data);
    free(hh);
}

int
hashmap_has(hashmap* hh, char* kk)
{
    return hashmap_get(hh, kk) != NULL;
}

char*
hashmap_get(hashmap* hh, char* kk)
{
    // TODO: Return the value associated with the
    // key kk.
    // Note: return -1 for key not found.
    int idx = hash(kk) % hh->capacity;
    while (hh->data[idx]->used || hh->data[idx]->tomb) {
        if (!hh->data[idx]->tomb && strcmp(hh->data[idx]->key, kk) == 0) {
            return hh->data[idx]->val;
        }

        if (idx >= hh->capacity - 1) {
            idx = 0;
        } else {
            idx++;
        }
    }
    return NULL;
}

void
hashmap_put(hashmap* hh, char* kk, char* vv)
{
    // TODO: Insert the value 'vv' into the hashmap
    // for the key 'kk', replacing any existing value
    // for that key.
    if (hh->size * 2 >= hh->capacity) {
        hashmap* new_map = make_hashmap_presize(2 * hh->capacity);
        for (int i = 0; i < hh->capacity; i++) {
            if (hh->data[i]->used) {
                hashmap_put(new_map, hh->data[i]->key, hh->data[i]->val);
            }
        }
        for (int i = 0; i < hh->capacity; i++) {
            free(hh->data[i]->key);
            free(hh->data[i]->val);
            free(hh->data[i]);
        }
        free(hh->data);
        hh->data = new_map->data;
        hh->capacity = 2 * hh->capacity;
        free(new_map);
    }
    int idx = hash(kk) % hh->capacity;
    while ((hh->data[idx]->tomb && strcmp(hh->data[idx]->key, kk) != 0)
            || (hh->data[idx]->used && strcmp(hh->data[idx]->key, kk) != 0)) {
        if (idx >= hh->capacity - 1) {
            idx = 0;
        } else {
            idx++;
        }
    }
    free(hh->data[idx]->key);
    free(hh->data[idx]->val);
    hh->data[idx]->key = strdup(kk);
    hh->data[idx]->val = strdup(vv);
    hh->data[idx]->used = true;
    hh->data[idx]->tomb = false;
    hh->size++;
}

void
hashmap_del(hashmap* hh, char* kk)
{
    // TODO: Remove any value associated with
    // this key in the map.
    if (hashmap_has(hh, kk)) {
        int idx = hash(kk) % hh->capacity;
        while (strcmp(hh->data[idx]->key, kk) != 0) {
            if (idx >= hh->capacity - 1) {
                idx = 0;
            } else {
                idx++;
            }
        }
        free(hh->data[idx]->key);
        free(hh->data[idx]->val);
        hh->data[idx]->used = false;
        hh->data[idx]->tomb = true;
    }
}

hashmap_pair
hashmap_get_pair(hashmap* hh, int ii)
{
    // TODO: Get the {k,v} pair stored in index 'ii'.
    return *(hh->data[ii]);
}

void
hashmap_dump(hashmap* hh)
{
    printf("== hashmap dump ==\n");
    // TODO: Print out all the keys and values currently
    // in the map, in storage order. Useful for debugging.
    for (int i = 0; i < hh->capacity; i++) {
        if (hh->data[i]->used) {
            printf("%s %s\n", hh->data[i]->key, hh->data[i]->val);
        }
    }
    puts("== hashmap end dump ==");
}
