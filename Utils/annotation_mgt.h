/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef ANNOTATION_MGT_H
#define ANNOTATION_MGT_H


#include "dictionary.h"


class Annotation_mgt
{
	private:
		Dictionary<const char*> *annoation_lookup_table;

	public:
		Annotation_mgt();
		~Annotation_mgt();
		void add_annotation(int, const char*, const char*);
		const char *get_annotation(int, const char*);
};

#endif

