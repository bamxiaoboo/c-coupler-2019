/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef ANNOTATION_MGT_H
#define ANNOTATION_MGT_H


struct Dictionary_node {
    struct Dictionary_node *next;
    char *key;
    char *value;
};


class Dictionary
{
	private:
		int num_hashing_entries;
		int num_elements;
		struct Dictionary_node **hashing_table;
		
		unsigned long hash_function(const char*);	
		void initialize_hashing_table(int);
		void delete_hashing_table();
		void increase_hashing_entries();

	public:
		Dictionary(int);
		~Dictionary();
		const char *search(const char*, bool);
		void insert(const char*, const char*);
		void remove(const char*);
};


class Annotation_mgt
{
	private:
		Dictionary *annoation_lookup_table;

	public:
		Annotation_mgt();
		~Annotation_mgt();
		void add_annotation(int, const char*, const char*);
		const char *get_annotation(int, const char*);
};

#endif

