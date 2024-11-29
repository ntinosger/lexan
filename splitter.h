#ifndef SPLITTER_H
#define SPLITTER_H

#include "hashTable.h"

#define MAX_LINE_LENGTH 1024
#define MAX_WORD_LENGTH 100

void load_exclusion_to_hash_table(FILE *excludeFile, HashTable *excludeHashTable);

void process_chunk(FILE *input, HashTable *hashTable, long start, long end, int num_builders, int buidler_pipes[num_builders][2]);

#endif //SPLITTER_H