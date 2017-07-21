//
// Created by zhsyourai on 2/15/17.
//

#ifndef ZRPC_HASHMAP_H
#define ZRPC_HASHMAP_H

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

typedef struct zRPC_hashmap_entry {
    void *key;
    int hash;
    void *value;
    struct zRPC_hashmap_entry *next;
} zRPC_hashmap_entry;

typedef struct zRPC_hashmap {
    zRPC_hashmap_entry **buckets;
    size_t bucketCount;

    int (*hash)(void *key);

    int (*equals)(void *keyA, void *keyB);

    size_t size;
} zRPC_hashmap;

zRPC_hashmap *hashmapCreate(size_t initialCapacity,
                            int (*hash)(void *key), int (*equals)(void *keyA, void *keyB));

size_t hashmapSize(zRPC_hashmap *map);

void hashmapFree(zRPC_hashmap *map);

int hashmapHash(void *key, size_t keySize);

void *hashmapPut(zRPC_hashmap *map, void *key, void *value);

void *hashmapGet(zRPC_hashmap *map, void *key);

int hashmapContainsKey(zRPC_hashmap *map, void *key);

void *hashmapMemoize(zRPC_hashmap *map, void *key,
                     void *(*initialValue)(void *key, void *custom), void *custom);

void *hashmapRemove(zRPC_hashmap *map, void *key);

void hashmapForEach(zRPC_hashmap *map,
                    int (*callback)(void *key, void *value, void *custom), void *custom);

size_t hashmapCurrentCapacity(zRPC_hashmap *map);

size_t hashmapCountCollisions(zRPC_hashmap *map);

int hashmapIntHash(void *key);

int hashmapIntEquals(void *keyA, void *keyB);

#endif //ZRPC_HASHMAP_H
