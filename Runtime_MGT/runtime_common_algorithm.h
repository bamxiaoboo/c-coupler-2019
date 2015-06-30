/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef RUNTIME_COMMON
#define RUNTIME_COMMON


#include "Runtime_Algorithm_Basis.h"
#include "external_algorithm_mgt.h"
#include "timer_mgt.h"


class Runtime_common_algorithm : public Runtime_algorithm_basis
{
	private: 
		Coupling_timer *timer;
        C_Coupler_algorithm c_coupler_algorithm;
		Model_algorithm model_algorithm;
        void **src_fields_data_buffers;
        void **dst_fields_data_buffers;
        int *num_elements_in_field_buffers;
		char input_field_file_name[NAME_STR_SIZE];
		char output_field_file_name[NAME_STR_SIZE];
		
    public:
        Runtime_common_algorithm(const char * cfg);
        ~Runtime_common_algorithm();
        bool is_model_algorithm() { return model_algorithm != NULL; }
		void allocate_src_dst_fields(bool);
        void run(bool);
};


#endif

