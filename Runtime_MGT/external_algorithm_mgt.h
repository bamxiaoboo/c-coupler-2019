/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef EXTERNAL_ALGORITHM_H
#define EXTERNAL_ALGORITHM_H


#include <vector>
#include "common_utils.h"


typedef void (*C_Coupler_algorithm) (void **, void **, int *);
typedef void (*Model_algorithm) ();


struct External_algorithm_info
{
	char algorithm_name[NAME_STR_SIZE];
	C_Coupler_algorithm c_coupler_algorithm;
	Model_algorithm model_algorithm;
};


class External_algorithm_mgt
{
	private: 
		std::vector<External_algorithm_info*> external_algorithms;

	public: 
		External_algorithm_mgt();
		~External_algorithm_mgt();
		void register_external_algorithm(const char*, C_Coupler_algorithm, Model_algorithm);
		C_Coupler_algorithm search_c_coupler_algorithm_pointer(const char*);
		Model_algorithm search_model_algorithm_pointer(const char*);
};


#endif
