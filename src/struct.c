#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define HASH_TABLE_SIZE 0x100000 // Large enough to avoid collisions with high probability

typedef struct {
    uint64_t key;
    uint32_t m1[4];
} hash_entry;

// Hash table to store chaining values from m1 blocks
hash_entry *hash_table[HASH_TABLE_SIZE];

// Hash function
static inline uint32_t hash_func(uint64_t h) {
    return (h ^ (h >> 24)) % HASH_TABLE_SIZE;
}

// Store a chaining value for m1
void store_hash(uint64_t h, const uint32_t m1[4]) {
    uint32_t idx = hash_func(h);
    hash_table[idx] = (hash_entry *)malloc(sizeof(hash_entry));
    hash_table[idx]->key = h;
    memcpy(hash_table[idx]->m1, m1, 4 * sizeof(uint32_t));
}

// Retrieve an m1 corresponding to a given h, or NULL if none exists
const uint32_t *retrieve_hash(uint64_t h) {
    uint32_t idx = hash_func(h);
    if (hash_table[idx] && hash_table[idx]->key == h) {
        return hash_table[idx]->m1;
    }
    return NULL;
}

// Function to clear the hash table
void clear_hash_table() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (hash_table[i]) {
            free(hash_table[i]);
            hash_table[i] = NULL;
        }
    }
}


