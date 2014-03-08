/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "Runtime_Algorithm_Basis.h"
#include "global_data.h"
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

