#include "hash_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HashTable *hash_table_create(const unsigned int size) {
    if (size <= 0) {
        fprintf(stderr, "Invalid Size: size must be positive\n");
        return NULL;
    }
    
    HashTable *tbl = malloc(sizeof(HashTable));
    if (tbl == NULL) {
        fprintf(stderr, "Cannot Allocate Memory\n");
        return NULL;
    }
    // size
    tbl->size = size;
    // Linked List
    tbl->lst = (Node **)malloc(size * sizeof(Node));
    if (tbl->lst == NULL) {
        fprintf(stderr, "Cannot Allocate Memory\n");
        return NULL;
    }
    memset(tbl->lst, 0, size * sizeof(Node));

    return tbl;
}

unsigned int make_hash(const char *key, const unsigned int size) {
    unsigned int hash = 0;
    int i = 0;
    while (key != NULL && key[i] != '\0') {
        hash = (hash + key[i]) % size;
        ++i;
    }
    return hash;
}

void free_node(Node *node) {
    free(node->key);
    // free(node->val);
    free(node);
}

int hash_table_insert(HashTable *tbl, const char *key, const unsigned long long int val) {
    if (tbl == NULL) {
        fprintf(stderr, "Hash Table does not exist\n");
        return 1;
    }
    Node *node = malloc(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    // target node
    node->key = malloc(sizeof(char) * strlen(key));
    strcpy(node->key, key);
    // node->val = malloc(sizeof(char) * strlen(val));
    // strcpy(node->val, val);
    node->val = val;
    node->next = NULL;

    unsigned int hash = make_hash(key, tbl->size);
    if (tbl->lst[hash] == NULL) {
        // first object
        tbl->lst[hash] = node;
        return 0;
    }
    // traverse linked list
    Node *cur = tbl->lst[hash];
    for (; cur != NULL; cur = cur->next) {
        if (strcmp(cur->key, node->key) == 0) {
            // key matched
            break;
        }
    }
    if (cur == NULL) {
        // key did not matched
        // then, insert object
        node->next = tbl->lst[hash];
        tbl->lst[hash] = node;
    } else {
        // delete and insert
        // free(cur->val);
        // cur->val = malloc(sizeof(char) * strlen(node->val));
        // strcpy(cur->val, node->val);
        cur->val = node->val;
        free_node(node);
    }
    return 0;
}

unsigned long long int hash_table_get(HashTable *tbl, const char *key) {
    if (tbl == NULL) {
        fprintf(stderr, "Fatal: Hash Table Does Not Exist\n");
        return -1;
    }
    char *other_key = malloc(sizeof(char) * strlen(key));
    strcpy(other_key, key);
    
    unsigned int hash = make_hash(key, tbl->size);
    Node *cur = tbl->lst[hash];

    for (; cur != NULL; cur = cur->next) {
        if (strcmp(cur->key, other_key) == 0) {
            // key matched
            break;
        }
    }
    free(other_key);
    if (cur == NULL) {
        // not found
        return -1;
    }
    return cur->val;
}

void hash_table_destroy(HashTable *tbl) {
    if (tbl == NULL) {
        fprintf(stderr, "Hash Table Does Not Exist\n");
        return;
    }
    Node *cur;
    for (int i = 0; i < tbl->size; ++i) {
        if (tbl->lst[i] == NULL) continue;
        while (tbl->lst[i] != NULL) {
            cur = tbl->lst[i]->next;
            free_node(tbl->lst[i]);
            tbl->lst[i] = cur;
        }
        free(tbl->lst[i]);
    }
    free(tbl->lst);
    free(tbl);
}