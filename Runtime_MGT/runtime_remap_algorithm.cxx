/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "global_data.h"
#include "runtime_remap_algorithm.h"
#include "runtime_config_dir.h"
#include "cor_cpl_interface.h"
#include "cor_global_data.h"
#include <stdio.h>


Runtime_remap_algorithm::Runtime_remap_algorithm(Runtime_remapping_weights *runtime_remapping_weights, Field_mem_info *src_field_instance, Field_mem_info *dst_field_instance, int connection_id)
{	
	comp_id = dst_field_instance->get_comp_id();
	specified_src_field_instance = src_field_instance;
	specified_dst_field_instance = dst_field_instance;
	parallel_remap_weights = runtime_remapping_weights->get_parallel_remapping_weights();
	EXECUTION_REPORT(REPORT_ERROR, -1, parallel_remap_weights != NULL, "Software error in Runtime_remap_algorithm::Runtime_remap_algorithm: parallel_remap_weights");
	
	if (words_are_the_same(src_field_instance->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT)) {
		true_src_field_instance = memory_manager->alloc_mem(specified_src_field_instance, BUF_MARK_REMAP_DATATYPE_TRANS_SRC, connection_id, DATA_TYPE_DOUBLE, false);
		true_dst_field_instance = memory_manager->alloc_mem(specified_dst_field_instance, BUF_MARK_REMAP_DATATYPE_TRANS_DST, connection_id, DATA_TYPE_DOUBLE, false);
		transform_data_type = true;
	}
	else if (words_are_the_same(src_field_instance->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE)) {
		true_src_field_instance = specified_src_field_instance;
		true_dst_field_instance = specified_dst_field_instance;
		transform_data_type = false;
	}
	else EXECUTION_REPORT(REPORT_ERROR, -1, false, "Software error in Runtime_remap_algorithm::Runtime_remap_algorithm: data type is wrong");
}


Runtime_remap_algorithm::~Runtime_remap_algorithm()
{
	for (int i = 0; i < operations_for_dynamic_sigma_grid.size(); i ++) {
		if (operations_for_dynamic_sigma_grid[i]->runtime_remap_algorithm != NULL)
			delete operations_for_dynamic_sigma_grid[i]->runtime_remap_algorithm;
		delete operations_for_dynamic_sigma_grid[i];
	}
}


void Runtime_remap_algorithm::do_remap(bool is_algorithm_in_kernel_stage)
{
    long i, j, field_size_src_before_rearrange, field_size_src_after_rearrange;
	long field_size_dst, decomp_size_src_before_rearrange, decomp_size_src_after_rearrange, num_levels;
    double *src_frac_values, *dst_frac_values, *src_field_values, *dst_field_values, frac_1x;
    double *temp_src_values;
	Remap_grid_class *decomp_grid, *original_grid;
	Decomp_info *decomp;
	bool grid_dynamic_surface_field_updated;


	specified_src_field_instance->use_field_values("");
	if (transform_data_type)
		for (int i = 0; i < specified_src_field_instance->get_size_of_field(); i ++)
			((double*)true_src_field_instance->get_data_buf())[i] = ((float*)specified_src_field_instance->get_data_buf())[i];
	parallel_remap_weights->do_remap(true_src_field_instance->get_field_data(), true_dst_field_instance->get_field_data());
	if (transform_data_type)
		for (int i = 0; i < specified_dst_field_instance->get_size_of_field(); i ++)
			((float*)specified_dst_field_instance->get_data_buf())[i] = ((double*)true_dst_field_instance->get_data_buf())[i];
	specified_dst_field_instance->define_field_values(true);
	printf("%s remapping field instance (%s) with value %lf at %d-%05d\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), true_dst_field_instance->get_field_name(), ((double*)true_dst_field_instance->get_data_buf())[0], components_time_mgrs->get_time_mgr(comp_id)->get_current_num_elapsed_day(), components_time_mgrs->get_time_mgr(comp_id)->get_current_second());
}


bool Runtime_remap_algorithm::run(bool is_algorithm_in_kernel_stage)
{
    do_remap(is_algorithm_in_kernel_stage);

	return true;
}

