#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashTable.h"

// Starting size of 8 items in the hash table
int HASH_TABLE_SIZE = 8;

// Create the hash table
HashTable* create_hash_table() {
    HashTable* newHT = malloc(sizeof(HashTable));
    if (!newHT) {
        fprintf(stderr, "Failed to allocate memory for the hash table.\n");
        exit(EXIT_FAILURE);
    }

    newHT->bucket = calloc(HASH_TABLE_SIZE, sizeof(HashNode*));
    // printf("HASH TABLE SIZE: %d IN CREATE\n", HASH_TABLE_SIZE);
    if (!newHT->bucket) {
        fprintf(stderr, "Failed to allocate memory for the hash table buckets.\n");
        exit(EXIT_FAILURE);
    }

    newHT->itemsCount = 0;
    newHT->tableSize = HASH_TABLE_SIZE;

    return newHT;
}

int hashing_builders(char* word, int num_builders) {
    int sumAsciiCodes = 0;
    for (int i = 0; word[i] != '\0'; i++) {
        sumAsciiCodes += word[i];
    }
    if (num_builders <= 0) {
        return -1;
    }
    
    return sumAsciiCodes % num_builders;
}

// Hashing function: Generate hash code for the word
int hash_code(char* word) {
    int sumAsciiCodes = 0;
    for (int i = 0; word[i] != '\0'; i++) {
        sumAsciiCodes += word[i];
    }

    return sumAsciiCodes % HASH_TABLE_SIZE;
}

// Insert a word into the hash table
void insert_to_hash_table(HashTable** hashTable, char* word) {
    // Calculate hash code
    int key = hash_code(word);

    // Check if the word already exists
    HashNode* existingNode = search_hash_table(*hashTable, word);
    if (existingNode != NULL) {
        // Word already exists, increase its frequency
        existingNode->freq++;
        return;
    }

    // Insert a new word
    HashNode* newNode = malloc(sizeof(HashNode));
    if (!newNode) {
        fprintf(stderr, "Failed to allocate memory for hash node.\n");
        exit(EXIT_FAILURE);
    }
    newNode->word = strdup(word); // Copy the word
    newNode->freq = 1;
    newNode->next = (*hashTable)->bucket[key];
    (*hashTable)->bucket[key] = newNode;

    (*hashTable)->itemsCount++;

    // Check if resizing is needed
    if ((float)(*hashTable)->itemsCount / HASH_TABLE_SIZE > 0.6) {
        double_hash_table(hashTable);
    }
}

// Search for a word in the hash table
HashNode* search_hash_table(HashTable* hashTable, char* word) {
    int key = hash_code(word);
    HashNode* current = hashTable->bucket[key];

    while (current) {
        if (strcmp(current->word, word) == 0) {
            return current; // Return the node if found
        }
        current = current->next;
    }
    return NULL; // Return NULL if the word is not found
}

// Double the size of the hash table
void double_hash_table(HashTable** hashTable) {
    int oldSize = HASH_TABLE_SIZE;
    HASH_TABLE_SIZE *= 2;
    // printf("DOUBLING HASH TABLE FROM SIZE: %d TO: %d\n", oldSize, HASH_TABLE_SIZE);

    HashTable* newHT = create_hash_table();

    for (int i = 0; i < oldSize; i++) {
        HashNode* current = (*hashTable)->bucket[i];
        while (current) {
            insert_to_hash_table(&newHT, current->word);
            current = current->next;
        }
    }

    free_hash_table(*hashTable);
    *hashTable = newHT;
}

// Function to remove a word from the hash table
void delete_from_hash_table(HashTable* hashTable, char* word) {
    // Calculate the hash key
    int key = hash_code(word);

    // Check if the word exists in the hash table
    if (search_hash_table(hashTable, word) == NULL) {
        printf("The word: %s does not exist in the hash table.\n", word);
        return;
    }

    HashNode* currentNode = hashTable->bucket[key];
    HashNode* previousNode = NULL;

    while (currentNode != NULL) {
        if (strcmp(currentNode->word, word) == 0) {
            // Found the word to delete
            if (previousNode == NULL) {
                // If it's the first node in the bucket
                hashTable->bucket[key] = currentNode->next;
            } else {
                // Update the previous node's next pointer
                previousNode->next = currentNode->next;
            }

            // Free the memory allocated for the word and the node
            free(currentNode->word);
            free(currentNode);

            hashTable->itemsCount--;
            return;
        }

        // Move to the next node
        previousNode = currentNode;
        currentNode = currentNode->next;
    }
}

// Free the hash table
void free_hash_table(HashTable* hashTable) {
    for (int i = 0; i < hashTable->tableSize; i++) {
        HashNode* current = hashTable->bucket[i];
        while (current) {
            HashNode* temp = current;
            current = current->next;
            free(temp->word); // Free the word
            free(temp);       // Free the node
        }
    }
    free(hashTable->bucket); // Free the bucket array
    free(hashTable);         // Free the hash table itself
}

// Print the hash table
void print_hash_table(HashTable* hashTable) {
    fprintf(stderr, "Hash Table:\n");
    for (int i = 0; i < hashTable->tableSize; i++) {
        fprintf(stderr, "Bucket %d: ", i);
        HashNode* current = hashTable->bucket[i];
        while (current) {
            fprintf(stderr, "(%s: %d) -> ", current->word, current->freq);
            current = current->next;
        }
        fprintf(stderr, "NULL\n");
    }
}