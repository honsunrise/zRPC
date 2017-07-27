//
// Created by zhsyourai on 2/15/17.
//

#include "ds/hashmap.h"

zRPC_hashmap* hashmapCreate(size_t initialCapacity,
                       int (*hash)(void* key), int (*equals)(void* keyA, void* keyB)) {
    assert(hash != NULL);
    assert(equals != NULL);

    zRPC_hashmap* map = malloc(sizeof(zRPC_hashmap));
    if (map == NULL) {
        return NULL;
    }
    
    size_t minimumBucketCount = initialCapacity * 4 / 3;
    map->bucketCount = 1;
    while (map->bucketCount <= minimumBucketCount) {
        map->bucketCount <<= 1;
    }
    map->buckets = calloc(map->bucketCount, sizeof(zRPC_hashmap_entry*));
    if (map->buckets == NULL) {
        free(map);
        return NULL;
    }

    map->size = 0;
    map->hash = hash;
    map->equals = equals;
    
    return map;
}

static inline int hashKey(zRPC_hashmap* map, void* key) {
    int h = map->hash(key);

    h += ~(h << 9);
    h ^= (((unsigned int) h) >> 14);
    h += (h << 4);
    h ^= (((unsigned int) h) >> 10);

    return h;
}

size_t hashmapSize(zRPC_hashmap* map) {
    return map->size;
}

static inline size_t calculateIndex(size_t bucketCount, int hash) {
    return ((size_t) hash) & (bucketCount - 1);
}

static void expandIfNecessary(zRPC_hashmap* map) {
    // If the load factor exceeds 0.75...
    if (map->size > (map->bucketCount * 3 / 4)) {
        // Start off with a 0.33 load factor.
        size_t newBucketCount = map->bucketCount << 1;
        zRPC_hashmap_entry** newBuckets = calloc(newBucketCount, sizeof(zRPC_hashmap_entry*));
        if (newBuckets == NULL) {
            // Abort expansion.
            return;
        }

        // Move over existing entries.
        size_t i;
        for (i = 0; i < map->bucketCount; i++) {
            zRPC_hashmap_entry* entry = map->buckets[i];
            while (entry != NULL) {
                zRPC_hashmap_entry* next = entry->next;
                size_t index = calculateIndex(newBucketCount, entry->hash);
                entry->next = newBuckets[index];
                newBuckets[index] = entry;
                entry = next;
            }
        }
        // Copy over internals.
        free(map->buckets);
        map->buckets = newBuckets;
        map->bucketCount = newBucketCount;
    }
}

void hashmapFree(zRPC_hashmap* map) {
    size_t i;
    for (i = 0; i < map->bucketCount; i++) {
        zRPC_hashmap_entry* entry = map->buckets[i];
        while (entry != NULL) {
            zRPC_hashmap_entry* next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(map->buckets);
    free(map);
}

static zRPC_hashmap_entry* create_hashmap_entry(void* key, int hash, void* value) {
    zRPC_hashmap_entry* entry = malloc(sizeof(zRPC_hashmap_entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->key = key;
    entry->hash = hash;
    entry->value = value;
    entry->next = NULL;
    return entry;
}

static inline int equalKeys(void* keyA, int hashA, void* keyB, int hashB,
                             int (*equals)(void*, void*)) {
    if (keyA == keyB) {
        return 1;
    }
    if (hashA != hashB) {
        return 0;
    }
    return equals(keyA, keyB);
}

void* hashmapPut(zRPC_hashmap* map, void* key, void* value) {
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);
    zRPC_hashmap_entry** p = &(map->buckets[index]);
    while (1) {
        zRPC_hashmap_entry* current = *p;
        // Add a new entry.
        if (current == NULL) {
            *p = create_hashmap_entry(key, hash, value);
            if (*p == NULL) {
                errno = ENOMEM;
                return NULL;
            }
            map->size++;
            expandIfNecessary(map);
            return NULL;
        }
        // Replace existing entry.
        if (equalKeys(current->key, current->hash, key, hash, map->equals)) {
            void* oldValue = current->value;
            current->value = value;
            return oldValue;
        }
        // Move to next entry.
        p = &current->next;
    }
}

void* hashmapGet(zRPC_hashmap* map, void* key) {
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);
    zRPC_hashmap_entry* entry = map->buckets[index];
    while (entry != NULL) {
        if (equalKeys(entry->key, entry->hash, key, hash, map->equals)) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

int hashmapContainsKey(zRPC_hashmap* map, void* key) {
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);
    zRPC_hashmap_entry* entry = map->buckets[index];
    while (entry != NULL) {
        if (equalKeys(entry->key, entry->hash, key, hash, map->equals)) {
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}

void* hashmapMemoize(zRPC_hashmap* map, void* key,
                     void* (*initialValue)(void* key, void* custom), void* custom) {
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);
    zRPC_hashmap_entry** p = &(map->buckets[index]);
    while (1) {
        zRPC_hashmap_entry* current = *p;
        // Add a new entry.
        if (current == NULL) {
            *p = create_hashmap_entry(key, hash, NULL);
            if (*p == NULL) {
                errno = ENOMEM;
                return NULL;
            }
            void* value = initialValue(key, custom);
            (*p)->value = value;
            map->size++;
            expandIfNecessary(map);
            return value;
        }
        // Return existing value.
        if (equalKeys(current->key, current->hash, key, hash, map->equals)) {
            return current->value;
        }
        // Move to next entry.
        p = &current->next;
    }
}
void* hashmapRemove(zRPC_hashmap* map, void* key) {
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);
    // Pointer to the current entry.
    zRPC_hashmap_entry** p = &(map->buckets[index]);
    zRPC_hashmap_entry* current;
    while ((current = *p) != NULL) {
        if (equalKeys(current->key, current->hash, key, hash, map->equals)) {
            void* value = current->value;
            *p = current->next;
            free(current);
            map->size--;
            return value;
        }
        p = &current->next;
    }
    return NULL;
}
void hashmapForEach(zRPC_hashmap* map,
                    int (*callback)(void* key, void* value, void* custom),
                    void* custom) {
    size_t i;
    for (i = 0; i < map->bucketCount; i++) {
        zRPC_hashmap_entry* entry = map->buckets[i];
        while (entry != NULL) {
            zRPC_hashmap_entry *next = entry->next;
            if (!callback(entry->key, entry->value, custom)) {
                return;
            }
            entry = next;
        }
    }
}
size_t hashmapCurrentCapacity(zRPC_hashmap* map) {
    size_t bucketCount = map->bucketCount;
    return bucketCount * 3 / 4;
}
size_t hashmapCountCollisions(zRPC_hashmap* map) {
    size_t collisions = 0;
    size_t i;
    for (i = 0; i < map->bucketCount; i++) {
        zRPC_hashmap_entry* entry = map->buckets[i];
        while (entry != NULL) {
            if (entry->next != NULL) {
                collisions++;
            }
            entry = entry->next;
        }
    }
    return collisions;
}
int hashmapIntHash(void* key) {
    // Return the key value itself.
    return *((int*) key);
}
int hashmapIntEquals(void* keyA, void* keyB) {
    int a = *((int*) keyA);
    int b = *((int*) keyB);
    return a == b;
}
