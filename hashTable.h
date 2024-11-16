#ifndef HASHTABLE_H
#define HASHTABLE_H

/** 
 * The hash table is implemented with a starting table size and when it's necessary a
 * resizing is performed to make the hash table fully dynamic and allow it to grow freely
 */

int HASH_TABLE_SIZE;

typedef struct HashNode {
    char* word;
    int freq;
    struct HashNode* next;
} HashNode;

typedef struct HashTable {
    HashNode** bucket;      // Array of buckets
    int itemsCount;         // The num of the items in the hash table, used for resizing
    int tableSize;          // The current table size
} HashTable;

HashTable* create_hash_table();

int hash_code(char* word);

void insert_to_hash_table(HashTable** hashTable, char* word);

HashNode* search_hash_table(HashTable* hashTable, char* word);

void double_hash_table(HashTable** hashTable);

void delete_from_hash_table(HashTable* hashTable, char* word);

void free_hash_table(HashTable* hashTable);

void print_hash_table(HashTable* hashTable);

#endif //HASHTABLE_H