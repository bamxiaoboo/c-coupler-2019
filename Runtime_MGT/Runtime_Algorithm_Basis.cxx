/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "runtime_cumulate_average_algorithm.h"
#include "Runtime_Algorithm_Basis.h"
#include <string.h>


int global_algorithm_id;


Runtime_algorithm_basis::Runtime_algorithm_basis()
{
    num_src_fields = 0;
    num_dst_fields = 0;
	cumulate_average_algorithm_before_run = NULL;
	algorithm_id = global_algorithm_id;
	global_algorithm_id ++;

    comp_names = NULL;
	field_names = NULL;
	field_local_decomp_names = NULL;
	field_grid_names = NULL;
	buf_marks = NULL;
	average_mark = NULL;
}


Runtime_algorithm_basis::~Runtime_algorithm_basis()
{
//	if (num_src_fields + num_dst_fields > 0)
//		EXECUTION_REPORT(REPORT_ERROR, comp_names == NULL && field_names == NULL && field_local_decomp_names == NULL && field_grid_names == NULL && buf_marks == NULL && average_mark == NULL, "C-Coupler software error when deleting Runtime_algorithm_basis");

	if (comp_names == NULL)
		return;

	for (int i = 0; i < num_src_fields+num_dst_fields; i ++) {
		delete [] comp_names[i];
		delete [] field_names[i];
		delete [] field_local_decomp_names[i];
		delete [] field_grid_names[i];
	}
	delete [] comp_names;
	delete [] field_names;
	delete [] field_local_decomp_names;
	delete [] field_grid_names;
	delete [] buf_marks;
	delete [] average_mark;
}


void Runtime_algorithm_basis::runtime_algorithm_common_initialize(const int num_src_fields, const int num_dst_fields)
{
    this->num_src_fields = num_src_fields;
    this->num_dst_fields = num_dst_fields;
}


void Runtime_algorithm_basis::add_runtime_datatype_transformation(Field_mem_info *current_field, bool is_input_field, Coupling_timer *timer, const char *cfg_name)
{
	Field_mem_info *pair_field;


	if (is_input_field) {
		pair_field = memory_manager->search_last_define_field(current_field->get_comp_name(), current_field->get_decomp_name(), current_field->get_grid_name(), current_field->get_field_name(), current_field->get_buf_type(), true, cfg_name);
		if (pair_field != current_field)
			datatype_transformer_before_run.add_pair_fields(pair_field, current_field, timer);
	}
	else {
		pair_field = memory_manager->search_registerred_field(current_field->get_comp_name(), current_field->get_decomp_name(), current_field->get_grid_name(), current_field->get_field_name(), current_field->get_buf_type());
		if (pair_field != NULL && pair_field != current_field)
			datatype_transformer_after_run.add_pair_fields(current_field, pair_field, timer);
	}
}


Field_mem_info *Runtime_algorithm_basis::add_one_field_for_cumulate_average(Field_mem_info *input_field, Coupling_timer *timer)
{
	if (cumulate_average_algorithm_before_run == NULL)
		cumulate_average_algorithm_before_run = new Runtime_cumulate_average_algorithm();
	return cumulate_average_algorithm_before_run->add_one_field(input_field, timer);
}


void Runtime_algorithm_basis::cumulate_average_before_run(bool is_algorithm_in_kernel_stage)
{
	if (cumulate_average_algorithm_before_run != NULL) {
		EXECUTION_REPORT(REPORT_LOG, true, "before implicit cumulating and averaging");
		cumulate_average_algorithm_before_run->run(is_algorithm_in_kernel_stage);
		EXECUTION_REPORT(REPORT_LOG, true, "after implicit cumulating and averaging");
	}
}


void Runtime_algorithm_basis::transfer_fields_data_type_before_run() 
{ 
	datatype_transformer_before_run.transform_fields_datatype(); 
}


void Runtime_algorithm_basis::transfer_fields_data_type_after_run() 
{
	datatype_transformer_after_run.transform_fields_datatype(); 
}


void Runtime_algorithm_basis::allocate_basic_data_structure(int num_src_fields, int num_dst_fields)
{	
	this->num_src_fields = num_src_fields;
	this->num_dst_fields = num_dst_fields;

	EXECUTION_REPORT(REPORT_ERROR, num_src_fields >= 0 && num_dst_fields >= 0,
					 "C-Coupler software error in allocate_basic_data_structure for Runtime_algorithm_basis");

	if (num_src_fields + num_dst_fields == 0)
		return;

	comp_names = new char* [num_src_fields+num_dst_fields];
	field_names = new char* [num_src_fields+num_dst_fields];
    field_local_decomp_names = new char* [num_src_fields+num_dst_fields];
    field_grid_names = new char* [num_src_fields+num_dst_fields];
	buf_marks = new int [num_src_fields+num_dst_fields];
	average_mark = new bool [num_src_fields+num_dst_fields];	

	for (int i = 0; i < num_src_fields+num_dst_fields; i ++) {
		comp_names[i] = new char [NAME_STR_SIZE];
		field_names[i] = new char [NAME_STR_SIZE];
        field_local_decomp_names[i] = new char [NAME_STR_SIZE];
        field_grid_names[i] = new char [NAME_STR_SIZE];
		buf_marks[i] = -1;
		average_mark[i] = false;
	}
}

