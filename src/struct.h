// file.h
#ifndef STRUCT_H
#define STRUCT_H

#include <stdint.h>
#include <stdbool.h>

// hash table size
#define HASH_TABLE_SIZE 0x1000000

// hash table enty structure
typedef struct {
    uint64_t key;       // key 
    uint32_t m1[4];     // value
} hash_entry;

// store this (h,m) at the table hash
void store_hash(uint64_t h, const uint32_t m[4]);

// get the value that has h as a key
const uint32_t *retrieve_hash(uint64_t h);

// clear hash table
void clear_hash_table(void);

// Fonction de hachage pour les valeurs de chaînage
static inline uint32_t hash_func(uint64_t h);

#endif 
