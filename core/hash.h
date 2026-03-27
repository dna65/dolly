#pragma once

#include <stddef.h>

struct tb_hash_node
{
    char* key;
    union {
        void* ptr_value;
        int   int_value;
    };
};

typedef struct tb_hash_node tb_hash_node;

struct tb_hash_bucket
{
    tb_hash_node* nodes;
    size_t size;
    size_t capacity;
};

typedef struct tb_hash_bucket tb_hash_bucket;

struct tb_hash_table
{
    tb_hash_bucket* buckets;
    size_t bucket_count;
};

typedef struct tb_hash_table tb_hash_table;

tb_hash_table tb_hash_table_new(size_t buckets);
void tb_hash_table_add_ptr(tb_hash_table* table, const char* key,
                           void* ptr_val);
void tb_hash_table_add_int(tb_hash_table* table, const char* key, int int_val);
const tb_hash_node* tb_hash_table_get(const tb_hash_table* table,
                                      const char* key);
void tb_hash_table_destroy(tb_hash_table* table);
