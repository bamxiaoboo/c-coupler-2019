/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/

#include "global_data.h"
#include "runtime_datatype_transformer.h"
#include "cor_global_data.h"


template <typename T1, typename T2> void transform_datatype_of_arrays(const T1 *src_array, T2 *dst_array, long num_local_cells)
{
	for (long i = 0; i < num_local_cells; i ++)
		dst_array[i] = (T2) src_array[i];
}


void Runtime_datatype_transformer::add_pair_fields(Field_mem_info *src_field, Field_mem_info *dst_field, Coupling_timer *timer)
{
	char *data_type_src, *data_type_dst;
	bool data_types_matched = false;


	EXECUTION_REPORT(REPORT_LOG, true, "Add data type transformation for field %s from data type %s to %s", src_field->get_field_name(), src_field->get_field_data()->get_grid_data_field()->data_type_in_application, dst_field->get_field_data()->get_grid_data_field()->data_type_in_application);
	EXECUTION_REPORT(REPORT_ERROR, src_field != NULL && dst_field != NULL, "C-Coupler software error1 in add_pair_fields of Runtime_datatype_transformer");
	EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(src_field->get_grid_name(), dst_field->get_grid_name()) && words_are_the_same(src_field->get_field_name(), dst_field->get_field_name()) 
		             && words_are_the_same(src_field->get_decomp_name(), dst_field->get_decomp_name()) && src_field->get_buf_type() == dst_field->get_buf_type(), 
		             "C-Coupler software error2 in add_pair_fields of Runtime_datatype_transformer");
	if (src_field == dst_field)
		return;

	data_type_src = src_field->get_field_data()->get_grid_data_field()->data_type_in_application;
	data_type_dst = dst_field->get_field_data()->get_grid_data_field()->data_type_in_application;
	EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(data_type_src, data_type_dst), "C-Coupler software error3 in add_pair_fields of Runtime_datatype_transformer");
	
    if (words_are_the_same(data_type_src, DATA_TYPE_DOUBLE) || words_are_the_same(data_type_src, DATA_TYPE_FLOAT)) 
        data_types_matched = words_are_the_same(data_type_dst, DATA_TYPE_DOUBLE) || words_are_the_same(data_type_dst, DATA_TYPE_FLOAT);
    else if (words_are_the_same(data_type_src, DATA_TYPE_LONG) || words_are_the_same(data_type_src, DATA_TYPE_INT) || words_are_the_same(data_type_src, DATA_TYPE_SHORT) || words_are_the_same(data_type_src, DATA_TYPE_BOOL)) 
        data_types_matched = words_are_the_same(data_type_dst, DATA_TYPE_LONG) || words_are_the_same(data_type_dst, DATA_TYPE_INT) || words_are_the_same(data_type_dst, DATA_TYPE_SHORT) || words_are_the_same(data_type_dst, DATA_TYPE_BOOL)
                             || words_are_the_same(data_type_dst, DATA_TYPE_FLOAT) || words_are_the_same(data_type_dst, DATA_TYPE_DOUBLE);

	EXECUTION_REPORT(REPORT_ERROR, data_types_matched, "data types %s and %s for field %s does not match each other", data_type_src, data_type_dst, src_field->get_field_name());

	for (int i = 0; i < src_fields.size(); i ++)
		if (src_fields[i] == src_field && dst_fields[i] == dst_field)
			return;

	src_fields.push_back(src_field);
	dst_fields.push_back(dst_field);
	timers.push_back(timer);
}


void Runtime_datatype_transformer::transform_fields_datatype()
{
	char *data_type_src, *data_type_dst;
	long num_local_cells;


	for (int i = 0; i < src_fields.size(); i ++) {
		if (timers[i] != NULL && !timers[i]->is_timer_on())
			return;
		src_fields[i]->use_field_values("");
		dst_fields[i]->define_field_values(false);
		data_type_src = src_fields[i]->get_field_data()->get_grid_data_field()->data_type_in_application;
		data_type_dst = dst_fields[i]->get_field_data()->get_grid_data_field()->data_type_in_application;
		num_local_cells = src_fields[i]->get_field_data()->get_grid_data_field()->required_data_size;
		if (words_are_the_same(data_type_src, DATA_TYPE_DOUBLE)) {
			if (words_are_the_same(data_type_dst, DATA_TYPE_FLOAT))
				transform_datatype_of_arrays((double*)src_fields[i]->get_data_buf(), (float*) dst_fields[i]->get_data_buf(), num_local_cells);
			else EXECUTION_REPORT(REPORT_ERROR, "C-Coupler software error1 in transform_fields_datatype of Runtime_datatype_transformer");
		}
		else if (words_are_the_same(data_type_src, DATA_TYPE_FLOAT)) {
			if (words_are_the_same(data_type_dst, DATA_TYPE_DOUBLE))
				transform_datatype_of_arrays((float*)src_fields[i]->get_data_buf(), (double*) dst_fields[i]->get_data_buf(), num_local_cells);
			else EXECUTION_REPORT(REPORT_ERROR, "C-Coupler software error2 in transform_fields_datatype of Runtime_datatype_transformer");
		}
		else if (words_are_the_same(data_type_src, DATA_TYPE_LONG)) {
			if (words_are_the_same(data_type_dst, DATA_TYPE_INT))
				transform_datatype_of_arrays((long*)src_fields[i]->get_data_buf(), (int*) dst_fields[i]->get_data_buf(), num_local_cells);
			else if (words_are_the_same(data_type_dst, DATA_TYPE_SHORT))
				transform_datatype_of_arrays((long*)src_fields[i]->get_data_buf(), (short*) dst_fields[i]->get_data_buf(), num_local_cells);
			else if (words_are_the_same(data_type_dst, DATA_TYPE_BOOL))
				transform_datatype_of_arrays((long*)src_fields[i]->get_data_buf(), (bool*) dst_fields[i]->get_data_buf(), num_local_cells);
			else EXECUTION_REPORT(REPORT_ERROR, "C-Coupler software error3 in transform_fields_datatype of Runtime_datatype_transformer");
		}
		else if (words_are_the_same(data_type_src, DATA_TYPE_INT)) {
			if (words_are_the_same(data_type_dst, DATA_TYPE_LONG))
				transform_datatype_of_arrays((int*)src_fields[i]->get_data_buf(), (long*) dst_fields[i]->get_data_buf(), num_local_cells);
			else if (words_are_the_same(data_type_dst, DATA_TYPE_SHORT))
				transform_datatype_of_arrays((int*)src_fields[i]->get_data_buf(), (short*) dst_fields[i]->get_data_buf(), num_local_cells);
			else if (words_are_the_same(data_type_dst, DATA_TYPE_BOOL))
				transform_datatype_of_arrays((int*)src_fields[i]->get_data_buf(), (bool*) dst_fields[i]->get_data_buf(), num_local_cells);
			else EXECUTION_REPORT(REPORT_ERROR, "C-Coupler software error4 in transform_fields_datatype of Runtime_datatype_transformer");
		}
		else if (words_are_the_same(data_type_src, DATA_TYPE_SHORT)) {
			if (words_are_the_same(data_type_dst, DATA_TYPE_LONG))
				transform_datatype_of_arrays((short*)src_fields[i]->get_data_buf(), (long*) dst_fields[i]->get_data_buf(), num_local_cells);
			else if (words_are_the_same(data_type_dst, DATA_TYPE_INT))
				transform_datatype_of_arrays((short*)src_fields[i]->get_data_buf(), (int*) dst_fields[i]->get_data_buf(), num_local_cells);
			else if (words_are_the_same(data_type_dst, DATA_TYPE_BOOL))
				transform_datatype_of_arrays((short*)src_fields[i]->get_data_buf(), (bool*) dst_fields[i]->get_data_buf(), num_local_cells);
			else EXECUTION_REPORT(REPORT_ERROR, "C-Coupler software error5 in transform_fields_datatype of Runtime_datatype_transformer");
		}
		else if (words_are_the_same(data_type_src, DATA_TYPE_BOOL)) {
			if (words_are_the_same(data_type_dst, DATA_TYPE_LONG))
				transform_datatype_of_arrays((bool*)src_fields[i]->get_data_buf(), (long*) dst_fields[i]->get_data_buf(), num_local_cells);
			else if (words_are_the_same(data_type_dst, DATA_TYPE_INT))
				transform_datatype_of_arrays((bool*)src_fields[i]->get_data_buf(), (int*) dst_fields[i]->get_data_buf(), num_local_cells);
			else if (words_are_the_same(data_type_dst, DATA_TYPE_SHORT))
				transform_datatype_of_arrays((bool*)src_fields[i]->get_data_buf(), (short*) dst_fields[i]->get_data_buf(), num_local_cells);
			else EXECUTION_REPORT(REPORT_ERROR, "C-Coupler software error6 in transform_fields_datatype of Runtime_datatype_transformer");
		}
		else EXECUTION_REPORT(REPORT_ERROR, "C-Coupler software error7 in transform_fields_datatype of Runtime_datatype_transformer");
	}
}

