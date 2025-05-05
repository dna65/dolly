#include "hash.h"

#include "core/core.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

tb_hash_table tb_hash_table_new(size_t buckets)
{
    tb_hash_table table = {
        .buckets = malloc_or_abort(buckets * sizeof(tb_hash_node*)),
        .bucket_count = buckets
    };

    for (size_t i = 0; i < buckets; ++i) {
        table.buckets[i] = NULL;
    }

    return table;
}

static size_t compute_hash(const char* string)
{
    size_t hash = 251;
    for (size_t i = 0; string[i]; ++i) {
        hash += (13 * string[i] * i);
    }

    return hash;
}

static tb_hash_node* tb_hash_table_new_node(tb_hash_table* table,
    const char* key)
{
    size_t index = compute_hash(key) % table->bucket_count;

    tb_hash_node** node = &(table->buckets[index]);
    while (*node != NULL) {
        if (strcmp((*node)->key, key) == 0) return NULL;
        node = &((*node)->next);
    }

    *node = malloc_or_abort(sizeof(tb_hash_node));
    (*node)->key = strdup_or_abort(key);
    (*node)->next = NULL;
    return *node;
}

void tb_hash_table_add_ptr(tb_hash_table* table, const char* key,
                           void* ptr_val)
{
    tb_hash_node* node = tb_hash_table_new_node(table, key);
    if (node) node->ptr_value = ptr_val;
}

void tb_hash_table_add_int(tb_hash_table* table, const char* key, int int_val)
{
    tb_hash_node* node = tb_hash_table_new_node(table, key);
    if (node) node->int_value = int_val;
}

const tb_hash_node* tb_hash_table_get(const tb_hash_table* table,
                                      const char* key)
{
    size_t index = compute_hash(key) % table->bucket_count;

    tb_hash_node* node = table->buckets[index];
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) return node;
        node = node->next;
    }

    return NULL;
}

void tb_hash_table_destroy(tb_hash_table* table)
{
    for (size_t i = 0; i < table->bucket_count; ++i) {
        for (tb_hash_node* node = table->buckets[i]; node;) {
            if (node) {
                tb_hash_node* next_node = node->next;
                free(node);
                node = next_node;
            }
        }
    }
    free(table->buckets);
}
