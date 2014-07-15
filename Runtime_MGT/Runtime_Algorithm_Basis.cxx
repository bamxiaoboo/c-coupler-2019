/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "Runtime_Algorithm_Basis.h"
#include <string.h>


Runtime_algorithm_basis::Runtime_algorithm_basis()
{
    num_src_fields = 0;
    num_dst_fields = 0;
    src_fields_data_buffers = NULL;
    dst_fields_data_buffers = NULL;
    num_elements_in_field_buffers = NULL;
}


Runtime_algorithm_basis::~Runtime_algorithm_basis()
{
    if (src_fields_data_buffers)
        delete [] src_fields_data_buffers;
    if (dst_fields_data_buffers)
        delete [] dst_fields_data_buffers;
    if (num_elements_in_field_buffers)
        delete [] num_elements_in_field_buffers;
}


void Runtime_algorithm_basis::runtime_algorithm_common_initialize(const int num_src_fields, 
                                                                        const int num_dst_fields, 
                                                                        const int num_length)
{
    this->num_src_fields = num_src_fields;
    this->num_dst_fields = num_dst_fields;
	if (num_src_fields > 0)
	    src_fields_data_buffers = new void *[num_src_fields];
	if (num_dst_fields > 0)
	    dst_fields_data_buffers = new void *[num_dst_fields];
    if (num_length > 0)
        num_elements_in_field_buffers = new int[num_length];
}


void Runtime_algorithm_basis::add_runtime_datatype_transformation(Field_mem_info *current_field, bool is_input_field, Coupling_timer *timer)
{
	Field_mem_info *pair_field;


	if (is_input_field) {
		pair_field = memory_manager->search_last_define_field(current_field->get_comp_name(), current_field->get_decomp_name(), current_field->get_grid_name(), current_field->get_field_name(), current_field->get_buf_type());
		if (pair_field != current_field)
			datatype_transformer_before_run.add_pair_fields(pair_field, current_field, timer);
	}
	else {
		pair_field = memory_manager->search_registerred_field(current_field->get_comp_name(), current_field->get_decomp_name(), current_field->get_grid_name(), current_field->get_field_name());
		if (pair_field != NULL && pair_field != current_field)
			datatype_transformer_after_run.add_pair_fields(current_field, pair_field, timer);
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

