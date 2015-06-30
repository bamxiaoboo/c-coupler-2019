/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef _ALGORITHM_RUNTIME_BASIS_H_
#define _ALGORITHM_RUNTIME_BASIS_H_


#include "runtime_datatype_transformer.h"
#include "timer_mgt.h"
#include "memory_mgt.h"
#include <vector>


class Runtime_cumulate_average_algorithm;


class Runtime_algorithm_basis
{
    protected:
		int algorithm_id;
		char algorithm_cfg_name[NAME_STR_SIZE];
		bool fields_allocated;
        int num_src_fields;
        int num_dst_fields;
		char **comp_names;
		char **field_names;
        char **field_local_decomp_names;
        char **field_grid_names;
		int *buf_marks;
		bool *average_mark;

		Runtime_cumulate_average_algorithm *cumulate_average_algorithm_before_run;
		Runtime_datatype_transformer datatype_transformer_before_run;
		Runtime_datatype_transformer datatype_transformer_after_run;

        void runtime_algorithm_common_initialize(const int, const int);
		void add_runtime_datatype_transformation(Field_mem_info*, bool, Coupling_timer*, const char*);

    public:
        Runtime_algorithm_basis();
        virtual ~Runtime_algorithm_basis();
        virtual void run(bool) = 0;
		virtual void allocate_src_dst_fields(bool) = 0;
//		virtual void generate_algorithm_info_from_cfg_file() = 0;
		Field_mem_info *add_one_field_for_cumulate_average(Field_mem_info*, Coupling_timer*);
		void allocate_basic_data_structure(int, int);
		void transfer_fields_data_type_before_run();
		void transfer_fields_data_type_after_run();
		void cumulate_average_before_run(bool);
};

extern int global_algorithm_id;

#endif

