/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "annotation_mgt.h"
#include "execution_report.h"
#include "remap_common_utils.h"
#include "global_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
  

#define INITIAL_SIZE (1024)
#define GROWTH_FACTOR (2)
#define MAX_LOAD_FACTOR (2)
#define MULTIPLIER (97)

  
Dictionary::Dictionary(int size)
{
	int true_size = size / 4;
  
  
	if (true_size < INITIAL_SIZE)
		true_size = INITIAL_SIZE;
	if (true_size > size)
		true_size = size;
  
	num_elements = 0;
	initialize_hashing_table(true_size);
}
  
  
void Dictionary::initialize_hashing_table(int size)
{
	num_hashing_entries = size;
	hashing_table = (Dictionary_node**) new Dictionary_node *[size];
	for(int i = 0; i < size; i++) 
		hashing_table[i] = NULL;
}
  
  
void Dictionary::delete_hashing_table()
{
	int i;
	Dictionary_node *e, *next;


    for(i = 0; i < num_hashing_entries; i++) {
		for(e = hashing_table[i]; e != NULL; e = next) {
			next = e->next;
			delete [] e->key;
			delete [] e->value;
			delete e;
		}
	}
  
	delete [] hashing_table;
}
  
  
Dictionary::~Dictionary()
{
	delete_hashing_table();
}
  
  
unsigned long Dictionary::hash_function(const char *keyword)
{
	unsigned const char *us;
	unsigned long h;
  
	h = 0;
  
	for(us = (unsigned const char *) keyword; *us; us++) {
		h = h * MULTIPLIER + *us;
	}

	return h % num_hashing_entries;
}
  
  
void Dictionary::increase_hashing_entries()
{   
	int old_num_hashing_entries = num_hashing_entries, new_num_hashing_entries;
	Dictionary_node **old_hashing_table = hashing_table, **new_hashing_table;
	Dictionary_node *e, *next;
	  
  
	initialize_hashing_table(num_hashing_entries * GROWTH_FACTOR);
	new_hashing_table = hashing_table;
	new_num_hashing_entries = num_hashing_entries;
	num_elements = 0;
  
	for(int i = 0; i < old_num_hashing_entries; i++) {
		for(e = old_hashing_table[i]; e != NULL; e = e->next)
			insert(e->key, e->value);
	}
  
	hashing_table = old_hashing_table;
	num_hashing_entries = old_num_hashing_entries;
	delete_hashing_table();
	hashing_table = new_hashing_table;
	num_hashing_entries = new_num_hashing_entries;
}
  
  
void Dictionary::insert(const char *key, const char *value)
{
	Dictionary_node *e;
	unsigned long h;
	  
  
	EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(key,""), "The key to be inserted into the dictionary cannot be empty!");
	EXECUTION_REPORT(REPORT_ERROR, search(key, false) == NULL, "The key \"%s\" has been inserted into the dictionary before. It cannot be inserted again. Please check.");
	e = new Dictionary_node;
	e->key = strdup(key);
	e->value = strdup(value);
	h = hash_function(key);
	e->next = hashing_table[h];
	hashing_table[h] = e;
	num_elements ++;
  
	/* increase_hashing_entries hashing_table if there is not enough room */
	if(num_elements >= num_hashing_entries * MAX_LOAD_FACTOR)
	increase_hashing_entries();
}

  
/* return the most recently inserted value associated with a key */
/* or 0 if no matching key is present */
const char *Dictionary::search(const char *key, bool check)
{
	Dictionary_node *e;
  
  
	for(e = hashing_table[hash_function(key)]; e != NULL; e = e->next) {
		if(strcmp(e->key, key) == 0) {
			/* got it */
			return e->value;
		}
	}
  
	if (check)
		EXECUTION_REPORT(REPORT_ERROR, false, "Cannot find the entry for the keyword \"%s\" in the dictionary", key);
  
	return NULL;
}
  
  
void Dictionary::remove(const char *key)
{
	Dictionary_node **prev, *e;  
  
  
	for(prev = &(hashing_table[hash_function(key)]); *prev != 0; prev = &((*prev)->next)) {
		if(strcmp((*prev)->key, key) == 0) {
			/* got it */
			e = *prev;
			*prev = e->next;
			delete [] e->key;
			delete e;
			num_elements --;
			break;
		}
	}
}
  

Annotation_mgt::Annotation_mgt()
{
	annoation_lookup_table = new Dictionary(1024);
}


Annotation_mgt::~Annotation_mgt()
{
	delete annoation_lookup_table;
}


void Annotation_mgt::add_annotation(int object_id, const char *tag, const char *annotation)
{
	char key[NAME_STR_SIZE];


	sprintf(key, "%x @ %s", object_id, tag);
	annoation_lookup_table->insert(key, annotation);
	printf("add annotation \"%s\" vs \"%s\"\n", annotation);
}


const char *Annotation_mgt::get_annotation(int object_id, const char *tag)
{
	char key[NAME_STR_SIZE];
	const char *annotation;


	sprintf(key, "%x @ %s", object_id, tag);
	annotation = annoation_lookup_table->search(key, true);
	printf("find annotation \"%s\"\n", annotation);
	return annotation;
}
