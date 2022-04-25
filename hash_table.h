#pragma once

typedef struct node {
    char *key;
    unsigned long long int val;
    struct node *next;
} Node;

typedef struct hash_table {
    unsigned int size;
    Node **lst;
} HashTable;

unsigned int make_hash(const char *key, const unsigned int size);

HashTable *hash_table_create(const unsigned int size);

void free_node(Node *node);

int hash_table_insert(HashTable *tbl, const char *key, const unsigned long long int val);

unsigned long long int hash_table_get(HashTable *tbl, const char *key);

void hash_table_destroy(HashTable *tbl);