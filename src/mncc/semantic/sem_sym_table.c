#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE 1024

static int global_symbol_seq = 0;


HashTable *create_table(int size)
{
	HashTable *table = malloc(sizeof(HashTable));
	table->size = size;
	table->buckets = calloc(size, sizeof(HashNode *));
	table->symbol_count = 0;
	table->parent = NULL;
	return table;
}


HashTable *create_child_table(int size, HashTable *parent)
{
	HashTable *table = malloc(sizeof(HashTable));
        table->size = size;
        table->buckets = calloc(size, sizeof(HashNode *));
        table->symbol_count = 0;
        table->parent = parent;
        return table;
}







VarSym search(HashTable *table, const char *key)
{
	unsigned long index = hash_djb2((unsigned char *)key) % table->size;

	HashNode *current = table->buckets[index];

	while (current != NULL) {
		if (strcmp(current->key, key) == 0) {
			return current->value;
		}

		current = current->next;
	}

	if (table->parent != NULL) {
		return search(table->parent, key);
	}

	fprintf(stderr, "Семантическая ошибка переменная  '%s'  ещё не объявлена!\n", key);
	exit(1);
}


void insert(HashTable *table, const char *key, VarSym value)
{
	unsigned long index = hash_djb2((unsigned char *)key) % table->size;

	HashNode *current = table->buckets[index];

	while (current != NULL) {
		if (strcmp(current->key, key) == 0) {
			fprintf(stderr,
				"Семантическая ошибка: Переменная '%s' уже "
				"объявлена!\n",
				key);
			exit(1);
		}
		current = current->next;
	}

	HashNode *new_node = calloc(1, sizeof(HashNode));
	new_node->key = strdup(key);

	value.symbol_id = global_symbol_seq++;

	new_node->value = value;

	new_node->next = table->buckets[index];
	table->buckets[index] = new_node;

	table->symbol_count++;
}

void free_table(HashTable *table)
{
	for (int i = 0; i < table->size; i++) {
		HashNode *current = table->buckets[i];
		while (current != NULL) {
			HashNode *temp = current;
			current = current->next;
			free(temp->key);
			free(temp);
		}
	}
	free(table->buckets);
	free(table);
}
