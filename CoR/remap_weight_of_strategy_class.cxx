/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "remap_weight_of_strategy_class.h"
#include "remap_strategy_class.h"
#include "remap_operator_basis.h"
#include "cor_global_data.h"
#include "io_binary.h"
#include "io_netcdf.h"
#include <string.h>


Remap_weight_of_operator_instance_class::Remap_weight_of_operator_instance_class(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst, 
                                                               long remap_beg_iter, Remap_operator_basis *remap_operator)
{
    this->field_data_grid_src = field_data_grid_src;
    this->field_data_grid_dst = field_data_grid_dst;
    this->remap_beg_iter = remap_beg_iter;
    this->remap_end_iter = remap_beg_iter + 1;
    this->original_remap_operator = remap_operator;
    this->duplicated_remap_operator = remap_operator->duplicate_remap_operator(true);
    this->operator_grid_src = remap_operator->src_grid;
    this->operator_grid_dst = remap_operator->dst_grid;
}


Remap_weight_of_operator_instance_class::Remap_weight_of_operator_instance_class(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst, 
                                                               long remap_beg_iter, Remap_operator_basis *remap_operator, Remap_operator_basis *duplicated_remap_operator)
{
    this->field_data_grid_src = field_data_grid_src;
    this->field_data_grid_dst = field_data_grid_dst;
    this->remap_beg_iter = remap_beg_iter;
    this->remap_end_iter = remap_beg_iter + 1;
    this->original_remap_operator = remap_operator;
    this->duplicated_remap_operator = duplicated_remap_operator;
    this->operator_grid_src = remap_operator->src_grid;
    this->operator_grid_dst = remap_operator->dst_grid;
}


Remap_weight_of_operator_instance_class *Remap_weight_of_operator_instance_class::generate_parallel_remap_weights(Remap_grid_class **decomp_original_grids, int **global_cells_local_indexes_in_decomps)
{
    Remap_weight_of_operator_instance_class *parallel_remap_weights_of_operator_instance;
    int overlap_with_decomp_counter = 0;


    parallel_remap_weights_of_operator_instance = new Remap_weight_of_operator_instance_class();
    parallel_remap_weights_of_operator_instance->original_remap_operator = this->original_remap_operator;

    for (int i = 0; i < 2; i ++)
        if (this->original_remap_operator->get_src_grid()->have_overlap_with_grid(decomp_original_grids[i])) {
            EXECUTION_REPORT(REPORT_ERROR, decomp_original_grids[i]->is_subset_of_grid(this->original_remap_operator->get_src_grid()),
                         "C-Coupler error1 in generate_parallel_remap_weights of Remap_weight_of_operator_instance_class\n");
            overlap_with_decomp_counter ++;
        }
    for (int i = 0; i < 2; i ++)
        if (this->original_remap_operator->get_dst_grid()->have_overlap_with_grid(decomp_original_grids[i])) {
            EXECUTION_REPORT(REPORT_ERROR, decomp_original_grids[i]->is_subset_of_grid(this->original_remap_operator->get_dst_grid()),
                         "C-Coupler error2 in generate_parallel_remap_weights of Remap_weight_of_operator_instance_class\n");
            overlap_with_decomp_counter ++;
        }
    EXECUTION_REPORT(REPORT_ERROR, overlap_with_decomp_counter == 0 || overlap_with_decomp_counter == 2, "C-Coupler error3 in generate_parallel_remap_weights of Remap_weight_of_operator_instance_class\n");
	EXECUTION_REPORT(REPORT_ERROR, this->duplicated_remap_operator != NULL, "C-Coupler error4 in generate_parallel_remap_weights of Remap_weight_of_operator_instance_class\n");

    if (overlap_with_decomp_counter > 0)
        parallel_remap_weights_of_operator_instance->duplicated_remap_operator = this->duplicated_remap_operator->generate_parallel_remap_operator(decomp_original_grids, global_cells_local_indexes_in_decomps);
    else parallel_remap_weights_of_operator_instance->duplicated_remap_operator = this->duplicated_remap_operator->duplicate_remap_operator(true);

    return parallel_remap_weights_of_operator_instance;
}


Remap_weight_of_operator_instance_class::~Remap_weight_of_operator_instance_class()
{
	if (duplicated_remap_operator != NULL)
	    delete duplicated_remap_operator;
}


void Remap_weight_of_operator_instance_class::renew_remapping_time_end_iter(long time_end_iter)
{
	EXECUTION_REPORT(REPORT_ERROR, remap_end_iter == time_end_iter, "C-Coupler error in Remap_weight_of_operator_instance_class::renew_remapping_time_end_iter");
	remap_end_iter = time_end_iter + 1;
}


Remap_weight_of_operator_class::Remap_weight_of_operator_class(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst, Remap_operator_basis *remap_operator,
																	  Remap_grid_class *operator_grid_src, Remap_grid_class *operator_grid_dst)
{
    this->field_data_grid_src = field_data_grid_src;
    this->field_data_grid_dst = field_data_grid_dst;
    this->original_remap_operator = remap_operator;
    this->operator_grid_src = operator_grid_src;
    this->operator_grid_dst = operator_grid_dst;
}


void Remap_weight_of_operator_class::calculate_src_decomp(Remap_grid_data_class *field_data_src, Remap_grid_data_class *field_data_dst)
{
    int i, j, k, num_sized_sub_grids, index_size_array[4], current_runtime_index_array[4];
    Remap_grid_class *sized_sub_grids[4];
    long remap_beg_iter, remap_end_iter, index_size_iter, field_array_offset;
    long *decomp_map_values_src, *decomp_map_values_dst;
	
    EXECUTION_REPORT(REPORT_ERROR, field_data_src->get_coord_value_grid()->is_similar_grid_with(field_data_grid_src), "C-Coupler error1 in calculate_src_decomp of Remap_weight_of_operator_class");
    EXECUTION_REPORT(REPORT_ERROR, field_data_dst->get_coord_value_grid()->is_similar_grid_with(field_data_grid_dst), "C-Coupler error2 in calculate_src_decomp of Remap_weight_of_operator_class");
	field_data_src->interchange_grid_data(field_data_grid_src);
	field_data_dst->interchange_grid_data(field_data_grid_dst);

	field_data_grid_src->get_sized_sub_grids(&num_sized_sub_grids, sized_sub_grids);
	for (j = 0; j < num_sized_sub_grids; j ++)
		if (!sized_sub_grids[j]->is_subset_of_grid(operator_grid_src))
			break;
	for (k = 0; j < num_sized_sub_grids; j ++) 
		index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
	num_sized_sub_grids = k;

	for (i = 0; i < remap_weights_of_operator_instances.size(); i ++) {
        EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->field_data_grid_src == this->field_data_grid_src && remap_weights_of_operator_instances[i]->field_data_grid_dst == this->field_data_grid_dst &&
						 remap_weights_of_operator_instances[i]->operator_grid_src == this->operator_grid_src && remap_weights_of_operator_instances[i]->operator_grid_dst == this->operator_grid_dst,
                     	 "remap software error1 in generate_parallel_remap_weights of Remap_weight_of_operator_class\n");
	    remap_beg_iter = remap_weights_of_operator_instances[i]->remap_beg_iter;
	    if (remap_weights_of_operator_instances[i]->remap_end_iter != -1)
	        remap_end_iter = remap_weights_of_operator_instances[i]->remap_end_iter;
	    else if (i+1 < remap_weights_of_operator_instances.size())
	        remap_end_iter = remap_weights_of_operator_instances[i+1]->remap_beg_iter;
	    else remap_end_iter = field_data_grid_src->get_grid_size()/operator_grid_src->get_grid_size();
	    for (j = remap_beg_iter; j < remap_end_iter; j ++) {
	        for (k = num_sized_sub_grids - 1, index_size_iter = 1; k >= 0; k --) {
	            current_runtime_index_array[k] = (j/index_size_iter) % index_size_array[k];
	            index_size_iter *= index_size_array[k];
	        }
	        for (field_array_offset = 0, index_size_iter = 1, k = 0; k < num_sized_sub_grids; k ++) {
	            field_array_offset += current_runtime_index_array[k]*index_size_iter;
	            index_size_iter *= index_size_array[k];
	        }
	        decomp_map_values_src = ((long*) field_data_src->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[i]->operator_grid_src->get_grid_size();
	        decomp_map_values_dst = ((long*) field_data_dst->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[i]->operator_grid_dst->get_grid_size();			
			EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->duplicated_remap_operator != NULL, "C-Coupler error3 in do_remap of Remap_weight_of_operator_class");
	        remap_weights_of_operator_instances[i]->duplicated_remap_operator->do_src_decomp_caculation(decomp_map_values_src, decomp_map_values_dst);
	    }
	}
}


Remap_weight_of_operator_class *Remap_weight_of_operator_class::generate_parallel_remap_weights(Remap_grid_class **remap_related_decomp_grids, 
                                                                                                 Remap_grid_class **decomp_original_grids, 
                                                                                                 int **global_cells_local_indexes_in_decomps,
                                                                                                 int & field_data_grids_iter,
                                                                                                 Remap_weight_of_strategy_class *parallel_remap_weights_of_strategy)
{
    int i, j, k, num_sized_sub_grids, index_size_array[256], current_runtime_index_array[256];
    Remap_weight_of_operator_instance_class *parallel_remap_weights_of_operator_instance;
    Remap_grid_class *sized_sub_grids[256];
    long remap_beg_iter, remap_end_iter, index_size_iter, global_field_array_offset, local_field_array_offset;


	field_data_grid_src->get_sized_sub_grids(&num_sized_sub_grids, sized_sub_grids);
	for (j = 0; j < num_sized_sub_grids; j ++)
		if (!sized_sub_grids[j]->is_subset_of_grid(operator_grid_src))
			break;
	for (k = 0; j < num_sized_sub_grids; j ++) 
		index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
	num_sized_sub_grids = k;

	EXECUTION_REPORT(REPORT_LOG, true, "Remap_weight_of_operator has %ld instances", remap_weights_of_operator_instances.size());

    for (i = 0; i < remap_weights_of_operator_instances.size(); i ++) {
        EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->field_data_grid_src == this->field_data_grid_src && remap_weights_of_operator_instances[i]->field_data_grid_dst == this->field_data_grid_dst &&
						 remap_weights_of_operator_instances[i]->operator_grid_src == this->operator_grid_src && remap_weights_of_operator_instances[i]->operator_grid_dst == this->operator_grid_dst,
                     	 "remap software error1 in generate_parallel_remap_weights of Remap_weight_of_operator_class\n");
        remap_beg_iter = remap_weights_of_operator_instances[i]->remap_beg_iter;
        if (remap_weights_of_operator_instances[i]->remap_end_iter != -1)
            remap_end_iter = remap_weights_of_operator_instances[i]->remap_end_iter;
        else {
            if (i+1 < remap_weights_of_operator_instances.size())
                remap_end_iter = remap_weights_of_operator_instances[i+1]->remap_beg_iter;
            else remap_end_iter = remap_weights_of_operator_instances[i]->get_field_data_grid_src()->get_grid_size()/remap_weights_of_operator_instances[i]->operator_grid_src->get_grid_size();
        }

        if (remap_weights_of_operator_instances[i]->operator_grid_src->get_is_sphere_grid() || remap_end_iter-remap_beg_iter == remap_weights_of_operator_instances[i]->get_field_data_grid_src()->get_grid_size()/remap_weights_of_operator_instances[i]->operator_grid_src->get_grid_size()){
            parallel_remap_weights_of_operator_instance = remap_weights_of_operator_instances[i]->generate_parallel_remap_weights(decomp_original_grids, global_cells_local_indexes_in_decomps);
            parallel_remap_weights_of_operator_instance->field_data_grid_src = remap_related_decomp_grids[field_data_grids_iter+0];
            parallel_remap_weights_of_operator_instance->field_data_grid_dst = remap_related_decomp_grids[field_data_grids_iter+1];
            parallel_remap_weights_of_operator_instance->operator_grid_src = remap_related_decomp_grids[field_data_grids_iter+2];
            parallel_remap_weights_of_operator_instance->operator_grid_dst = remap_related_decomp_grids[field_data_grids_iter+3];
            parallel_remap_weights_of_operator_instance->remap_beg_iter = remap_weights_of_operator_instances[i]->remap_beg_iter; 
            parallel_remap_weights_of_operator_instance->remap_end_iter = remap_weights_of_operator_instances[i]->remap_end_iter;
			parallel_remap_weights_of_strategy->add_remap_weight_of_operator_instance(parallel_remap_weights_of_operator_instance);
            EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->remap_beg_iter == 0, "C-Coupler error3 in generate_parallel_remap_weights of Remap_weight_of_strategy_class\n");
        }
        else {
            for (j = remap_beg_iter; j < remap_end_iter; j ++) {
                for (k = num_sized_sub_grids - 1, index_size_iter = 1; k >= 0; k --) {
                    current_runtime_index_array[k] = (j/index_size_iter) % index_size_array[k];
                    index_size_iter *= index_size_array[k];
                }
                for (global_field_array_offset = 0, index_size_iter = 1, k = 0; k < num_sized_sub_grids; k ++) {
                    global_field_array_offset += current_runtime_index_array[k]*index_size_iter;
                    index_size_iter *= index_size_array[k];
                }
				if (remap_weights_of_operator_instances[i]->field_data_grid_src->have_overlap_with_grid(decomp_original_grids[0]))
					local_field_array_offset = global_cells_local_indexes_in_decomps[0][global_field_array_offset];
				else if (remap_weights_of_operator_instances[i]->field_data_grid_src->have_overlap_with_grid(decomp_original_grids[1]))
					local_field_array_offset = global_cells_local_indexes_in_decomps[1][global_field_array_offset];
                else EXECUTION_REPORT(REPORT_ERROR, false, "C-Coupler error4 in generate_parallel_remap_weights of Remap_weight_of_strategy_class\n");
				if (local_field_array_offset == -1)
					continue;
                parallel_remap_weights_of_operator_instance = remap_weights_of_operator_instances[i]->generate_parallel_remap_weights(decomp_original_grids, global_cells_local_indexes_in_decomps);
                parallel_remap_weights_of_operator_instance->field_data_grid_src = remap_related_decomp_grids[field_data_grids_iter+0];
                parallel_remap_weights_of_operator_instance->field_data_grid_dst = remap_related_decomp_grids[field_data_grids_iter+1];
                parallel_remap_weights_of_operator_instance->operator_grid_src = remap_related_decomp_grids[field_data_grids_iter+2];
                parallel_remap_weights_of_operator_instance->operator_grid_dst = remap_related_decomp_grids[field_data_grids_iter+3];
                parallel_remap_weights_of_operator_instance->remap_beg_iter = local_field_array_offset; 
                parallel_remap_weights_of_operator_instance->remap_end_iter = local_field_array_offset+1;
				parallel_remap_weights_of_strategy->add_remap_weight_of_operator_instance(parallel_remap_weights_of_operator_instance);
            }
        }

        field_data_grids_iter += 4;
    }
}


Remap_weight_of_operator_class::~Remap_weight_of_operator_class()
{
	for (int i = 0; i < remap_weights_of_operator_instances.size(); i ++)
		delete remap_weights_of_operator_instances[i];
}


void Remap_weight_of_operator_class::prepare_index_size_array()
{
    Remap_grid_class *sized_sub_grids[256];
	int j, k;


	field_data_grid_src->get_sized_sub_grids(&size_index_size_array, sized_sub_grids);
	for (j = 0; j < size_index_size_array; j ++)
		if (!sized_sub_grids[j]->is_subset_of_grid(operator_grid_src))
			break;
	for (k = 0; j < size_index_size_array; j ++) 
		index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
	size_index_size_array = k;
}


long Remap_weight_of_operator_class::calculate_offset_for_operator_instance(int iter)
{
	long k, index_size_iter, current_runtime_index_array[256], field_array_offset;

	
	for (k = size_index_size_array - 1, index_size_iter = 1; k >= 0; k --) {
		current_runtime_index_array[k] = (iter/index_size_iter) % index_size_array[k];
		index_size_iter *= index_size_array[k];
	}
	for (field_array_offset = 0, index_size_iter = 1, k = 0; k < size_index_size_array; k ++) {
		field_array_offset += current_runtime_index_array[k]*index_size_iter;
		index_size_iter *= index_size_array[k];
    }

	return field_array_offset;
}


void Remap_weight_of_operator_class::do_remap(Remap_grid_data_class *field_data_src, Remap_grid_data_class *field_data_dst)
{

    double *data_value_src, *data_value_dst;
    int i, j, k;
    long remap_beg_iter, remap_end_iter;
    long index_size_iter, field_array_offset;
	long field_data_size_src, field_data_size_dst;

    
    EXECUTION_REPORT(REPORT_ERROR, field_data_src->get_coord_value_grid()->is_similar_grid_with(field_data_grid_src), "C-Coupler error1 in do_remap of Remap_weight_of_operator_class");
    EXECUTION_REPORT(REPORT_ERROR, field_data_dst->get_coord_value_grid()->is_similar_grid_with(field_data_grid_dst), "C-Coupler error2 in do_remap of Remap_weight_of_operator_class");
	field_data_src->interchange_grid_data(field_data_grid_src);
	field_data_dst->interchange_grid_data(field_data_grid_dst);

	field_data_size_src = field_data_src->get_grid_data_field()->read_data_size;
	field_data_size_dst = field_data_dst->get_grid_data_field()->read_data_size;

    prepare_index_size_array();
	
    for (i = 0; i < remap_weights_of_operator_instances.size(); i ++) {
        EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->field_data_grid_src == this->field_data_grid_src && remap_weights_of_operator_instances[i]->field_data_grid_dst == this->field_data_grid_dst &&
						 remap_weights_of_operator_instances[i]->operator_grid_src == this->operator_grid_src && remap_weights_of_operator_instances[i]->operator_grid_dst == this->operator_grid_dst,
                     	 "remap software error3 in do_remap of Remap_weight_of_strategy_class\n");
        remap_beg_iter = remap_weights_of_operator_instances[i]->remap_beg_iter;
        if (remap_weights_of_operator_instances[i]->remap_end_iter != -1)
            remap_end_iter = remap_weights_of_operator_instances[i]->remap_end_iter;
        else if (i+1 < remap_weights_of_operator_instances.size())
                remap_end_iter = remap_weights_of_operator_instances[i+1]->remap_beg_iter;
        else remap_end_iter = field_data_grid_src->get_grid_size()/operator_grid_src->get_grid_size();
        for (j = remap_beg_iter; j < remap_end_iter; j ++) {
			field_array_offset = calculate_offset_for_operator_instance(j);
#ifdef DEBUG_CCPL
	        EXECUTION_REPORT(REPORT_ERROR, field_array_offset >= 0 && (field_array_offset+1)*remap_weights_of_operator_instances[i]->operator_grid_src->get_grid_size() <= field_data_size_src,
	        				 "remap software error4 in do_remap of Remap_weight_of_strategy_class");
			EXECUTION_REPORT(REPORT_ERROR, field_array_offset >= 0 && (field_array_offset+1)*remap_weights_of_operator_instances[i]->operator_grid_dst->get_grid_size() <= field_data_size_dst,
				 			 "remap software error5 in do_remap of Remap_weight_of_strategy_class");
#endif	        
            data_value_src = ((double*) field_data_src->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[i]->operator_grid_src->get_grid_size();
            data_value_dst = ((double*) field_data_dst->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[i]->operator_grid_dst->get_grid_size();
			EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->duplicated_remap_operator != NULL, "C-Coupler error3 in do_remap of Remap_weight_of_operator_class");
            remap_weights_of_operator_instances[i]->duplicated_remap_operator->do_remap_values_caculation(data_value_src, data_value_dst);
        }
    }
}


void Remap_weight_of_operator_class::add_remap_weight_of_operator_instance(Remap_weight_of_operator_instance_class *operator_instance)
{
	remap_weights_of_operator_instances.push_back(operator_instance);
}


void Remap_weight_of_operator_class::renew_vertical_remap_weights(Remap_grid_class *runtime_remap_grid_src, Remap_grid_class *runtime_remap_grid_dst)
{
	long i;
	Remap_grid_data_class *lev_center_field_in_3D_src_grid = NULL, *lev_center_field_in_3D_dst_grid = NULL;
	Remap_grid_data_class *lev_vertex_field_in_3D_src_grid = NULL, *lev_vertex_field_in_3D_dst_grid = NULL;
	Remap_operator_grid *runtime_remap_operator_grid_src = NULL, *runtime_remap_operator_grid_dst = NULL;
	Remap_operator_basis *new_remap_operator;
	double *lev_center_values_in_3D_src_grid = NULL, *lev_center_values_in_3D_dst_grid = NULL;
	double *lev_vertex_values_in_3D_src_grid = NULL, *lev_vertex_values_in_3D_dst_grid = NULL;
	long lev_grid_size_src, lev_grid_size_dst, offset;

	
	EXECUTION_REPORT(REPORT_ERROR, runtime_remap_grid_src->get_num_dimensions() == 1 && runtime_remap_grid_src->has_grid_coord_label(COORD_LABEL_LEV) && runtime_remap_grid_dst->get_num_dimensions() == 1 && runtime_remap_grid_dst->has_grid_coord_label(COORD_LABEL_LEV),
					 "C-Coupler error1 in renew_vertical_remap_weights of Remap_weight_of_operator_class");
	EXECUTION_REPORT(REPORT_ERROR, operator_grid_src->get_num_dimensions() == 1 && operator_grid_src->has_grid_coord_label(COORD_LABEL_LEV) && operator_grid_dst->get_num_dimensions() == 1 && operator_grid_dst->has_grid_coord_label(COORD_LABEL_LEV),
					 "C-Coupler error2 in renew_vertical_remap_weights of Remap_weight_of_operator_class");
	EXECUTION_REPORT(REPORT_ERROR, original_remap_operator->get_src_grid()->get_num_dimensions() == 1 && original_remap_operator->get_src_grid()->has_grid_coord_label(COORD_LABEL_LEV) && original_remap_operator->get_dst_grid()->get_num_dimensions() == 1 && original_remap_operator->get_dst_grid()->has_grid_coord_label(COORD_LABEL_LEV),
					 "C-Coupler error3 in renew_vertical_remap_weights of Remap_weight_of_operator_class");
	EXECUTION_REPORT(REPORT_ERROR, original_remap_operator->get_src_grid()->get_num_dimensions() == 1 && original_remap_operator->get_src_grid()->has_grid_coord_label(COORD_LABEL_LEV) && original_remap_operator->get_dst_grid()->get_num_dimensions() == 1 && original_remap_operator->get_dst_grid()->has_grid_coord_label(COORD_LABEL_LEV),
					 "C-Coupler error4 in renew_vertical_remap_weights of Remap_weight_of_operator_class");
	EXECUTION_REPORT(REPORT_ERROR, field_data_grid_src->is_sigma_grid() || field_data_grid_dst->is_sigma_grid(), "C-Coupler error4 in renew_vertical_remap_weights of Remap_weight_of_operator_class");
	
	if (field_data_grid_src->is_sigma_grid()) {
		EXECUTION_REPORT(REPORT_ERROR, operator_grid_src->is_subset_of_grid(field_data_grid_src), "C-Coupler error5 in renew_vertical_remap_weights of Remap_weight_of_operator_class"); 
		lev_center_field_in_3D_src_grid = field_data_grid_src->get_unique_center_field();
		lev_center_field_in_3D_src_grid->interchange_grid_data(field_data_grid_src);
		lev_center_values_in_3D_src_grid = (double*) lev_center_field_in_3D_src_grid->get_grid_data_field()->data_buf;
		lev_vertex_field_in_3D_src_grid = field_data_grid_src->get_unique_vertex_field();
		lev_vertex_field_in_3D_src_grid->interchange_grid_data(field_data_grid_src);
		lev_vertex_values_in_3D_src_grid = (double*) lev_vertex_field_in_3D_src_grid->get_grid_data_field()->data_buf;
	}
	if (field_data_grid_dst->is_sigma_grid()) {
		EXECUTION_REPORT(REPORT_ERROR, operator_grid_dst->is_subset_of_grid(field_data_grid_dst), "C-Coupler error6 in renew_vertical_remap_weights of Remap_weight_of_operator_class"); 
		lev_center_field_in_3D_dst_grid = field_data_grid_dst->get_unique_center_field();
		lev_center_field_in_3D_dst_grid->interchange_grid_data(field_data_grid_dst);
		lev_center_values_in_3D_dst_grid = (double*) lev_center_field_in_3D_dst_grid->get_grid_data_field()->data_buf;
		lev_vertex_field_in_3D_dst_grid = field_data_grid_dst->get_unique_vertex_field();
		lev_vertex_field_in_3D_dst_grid->interchange_grid_data(field_data_grid_dst);
		lev_vertex_values_in_3D_dst_grid = (double*) lev_vertex_field_in_3D_dst_grid->get_grid_data_field()->data_buf;
	}
	lev_grid_size_src = operator_grid_src->get_grid_size();
	lev_grid_size_dst = operator_grid_dst->get_grid_size();

	prepare_index_size_array();

	for (i = 0; i < remap_weights_of_operator_instances.size(); i ++) {
		EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->duplicated_remap_operator != NULL, "C-Coupler error7 in renew_vertical_remap_weights of Remap_weight_of_operator_class"); 
		new_remap_operator = remap_weights_of_operator_instances[i]->duplicated_remap_operator->duplicate_remap_operator(true);
		new_remap_operator->set_src_grid(runtime_remap_grid_src);
		new_remap_operator->set_dst_grid(runtime_remap_grid_dst);
		offset = calculate_offset_for_operator_instance(remap_weights_of_operator_instances[i]->remap_beg_iter);
		if (lev_center_field_in_3D_src_grid != NULL)
			EXECUTION_REPORT(REPORT_ERROR, offset >= 0 && offset*lev_grid_size_src+lev_grid_size_src<= lev_center_field_in_3D_src_grid->get_grid_data_field()->required_data_size, "C-Coupler error7 in renew_vertical_remap_weights of Remap_weight_of_operator_class"); 
		if (lev_center_values_in_3D_src_grid != NULL)
			runtime_remap_grid_src->renew_lev_grid_coord_values(lev_center_values_in_3D_src_grid+offset*lev_grid_size_src, lev_vertex_values_in_3D_src_grid+offset*lev_grid_size_src*2);
		if (lev_center_values_in_3D_dst_grid != NULL) {
			runtime_remap_grid_dst->renew_lev_grid_coord_values(lev_center_values_in_3D_dst_grid+offset*lev_grid_size_dst, lev_vertex_values_in_3D_dst_grid+offset*lev_grid_size_dst*2);
		}
		if (runtime_remap_operator_grid_src == NULL) {
			runtime_remap_operator_grid_src = new Remap_operator_grid(runtime_remap_grid_src, new_remap_operator, true, false);
			runtime_remap_operator_grid_dst = new Remap_operator_grid(runtime_remap_grid_dst, new_remap_operator, false, false);
			current_runtime_remap_operator_grid_src = runtime_remap_operator_grid_src;
			current_runtime_remap_operator_grid_dst = runtime_remap_operator_grid_dst;
		}
		if (lev_center_values_in_3D_src_grid != NULL)
			runtime_remap_operator_grid_src->update_operator_grid_data();
		if (lev_center_values_in_3D_dst_grid != NULL)
			runtime_remap_operator_grid_dst->update_operator_grid_data();
		current_runtime_remap_operator = new_remap_operator;
		new_remap_operator->calculate_remap_weights();
//		new_remap_operator->get_remap_weights_group(0)->compare_to_another_sparse_matrix(remap_weights_of_operator_instances[i]->duplicated_remap_operator->get_remap_weights_group(0));
//		new_remap_operator->get_remap_weights_group(1)->compare_to_another_sparse_matrix(remap_weights_of_operator_instances[i]->duplicated_remap_operator->get_remap_weights_group(1));
//		new_remap_operator->get_remap_weights_group(2)->compare_to_another_sparse_matrix(remap_weights_of_operator_instances[i]->duplicated_remap_operator->get_remap_weights_group(2));
//		new_remap_operator->get_remap_weights_group(3)->compare_to_another_sparse_matrix(remap_weights_of_operator_instances[i]->duplicated_remap_operator->get_remap_weights_group(3));
		delete remap_weights_of_operator_instances[i]->duplicated_remap_operator;
		remap_weights_of_operator_instances[i]->duplicated_remap_operator = new_remap_operator;
	}

	if (runtime_remap_operator_grid_src != NULL) {
		delete runtime_remap_operator_grid_src;
		delete runtime_remap_operator_grid_dst;
	}
}


void Remap_weight_of_strategy_class::initialize_object()
{
	dynamic_vertical_remapping_weights_src = false;
	dynamic_vertical_remapping_weights_dst = false;
	public_remap_weights_of_operators = false;
	num_field_data_grids_in_remapping_process = 0;
}


Remap_weight_of_strategy_class::Remap_weight_of_strategy_class(const char *object_name, const char *remap_strategy_name, 
                                                               const char *data_grid_name_src, const char *data_grid_name_dst,
                                                               const char *input_IO_file_name, const char *weight_IO_format,
                                                               bool read_from_io)
{
	initialize_object();
    strcpy(this->object_name, object_name);
    remap_strategy = remap_strategy_manager->search_remap_strategy(remap_strategy_name);
    data_grid_src = remap_grid_manager->search_remap_grid_with_grid_name(data_grid_name_src);
    data_grid_dst = remap_grid_manager->search_remap_grid_with_grid_name(data_grid_name_dst);

	EXECUTION_REPORT(REPORT_ERROR, remap_strategy != NULL && data_grid_src != NULL && data_grid_dst != NULL, "C-Coupler error in Remap_weight_of_strategy_class::Remap_weight_of_strategy_class");

	generate_remapping_related_grids();
	build_operations_for_calculating_sigma_values_of_grids();
	calculate_sigma_values_of_grids();

	if (!read_from_io)
		remap_strategy->calculate_remapping_weights(this);
	else {
		if (words_are_the_same(weight_IO_format, "SCRIP")) 
			((IO_netcdf*) (io_manager->search_IO_object(input_IO_file_name)))->read_remap_weights(this, remap_strategy, is_master_process_in_computing_node);
		else ((IO_binary*) (io_manager->search_IO_object(input_IO_file_name)))->read_remap_weights(this, remap_strategy, is_master_process_in_computing_node);
	}

    EXECUTION_REPORT(REPORT_ERROR, data_grid_src->get_num_dimensions() == data_grid_dst->get_num_dimensions(), 
    	             "grid %s and %s must have the same number of dimensions\n", data_grid_name_src, data_grid_name_dst);
}


int Remap_weight_of_strategy_class::generate_remapping_related_grids()
{
	int i, j;
    Remap_grid_class *current_remap_src_data_grid, *current_remap_src_data_grid_interchanged, *current_remap_dst_data_grid, *existing_grid;
    Remap_grid_class *runtime_mask_sub_grids_src[256], *runtime_mask_sub_grids_dst[256];
    int num_runtime_mask_sub_grids_src, num_runtime_mask_sub_grids_dst;
    Remap_grid_data_class *runtime_mask_src, *runtime_mask_dst;
	Remap_grid_class *leaf_grids[256];
	int num_leaf_grids;

	
	num_field_data_grids_in_remapping_process = 0;
	field_data_grids_in_remapping_process[num_field_data_grids_in_remapping_process] = data_grid_src;
	runtime_mask_fields_in_remapping_process[num_field_data_grids_in_remapping_process++] = NULL;
    current_remap_src_data_grid = data_grid_src;
    for (i = 0; i < remap_strategy->get_num_remap_operator(); i ++) {
        if (i == 0)
            current_remap_src_data_grid->compute_remap_field_data_runtime_mask(data_grid_src,
                                                                               runtime_mask_sub_grids_src,
                                                                               &num_runtime_mask_sub_grids_src,
                                                                               &runtime_mask_src);
        else current_remap_src_data_grid->compute_remap_field_data_runtime_mask(NULL,
                                                                                runtime_mask_sub_grids_src,
                                                                                &num_runtime_mask_sub_grids_src,
                                                                                &runtime_mask_src);
        current_remap_src_data_grid->generate_interchange_grids(remap_strategy->get_remap_operator(i)->get_src_grid(), 
                                                                &current_remap_src_data_grid_interchanged, 
                                                                runtime_mask_sub_grids_src,
                                                                num_runtime_mask_sub_grids_src);
        current_remap_dst_data_grid = new Remap_grid_class(current_remap_src_data_grid_interchanged, 
                                                           remap_strategy->get_remap_operator(i)->get_src_grid(), 
                                                           remap_strategy->get_remap_operator(i)->get_dst_grid(),
                                                           remap_strategy->get_remap_operator(i)->get_is_operator_regridding()); 
		existing_grid = remap_grid_manager->search_same_remap_grid(current_remap_dst_data_grid);
		if (existing_grid != NULL) {
			delete current_remap_dst_data_grid;
			current_remap_dst_data_grid = existing_grid;
		}

        if (i == remap_strategy->get_num_remap_operator()-1 && !current_remap_dst_data_grid->is_similar_grid_with(data_grid_dst)) {
            data_grid_dst->get_leaf_grids(&num_leaf_grids, leaf_grids, data_grid_dst);
            for (j = 0; j < num_leaf_grids; j ++) 
                EXECUTION_REPORT(REPORT_ERROR, leaf_grids[j]->is_subset_of_grid(current_remap_dst_data_grid), 
   	    	                     "this remap caculation can not get 1D grid %s, which is a sub grid of the grid of destination field data. Please check the %dth remapping operator of the remapping strategy", 
       	    	                 leaf_grids[j]->get_grid_name(), i+1);
        }
        if (i == remap_strategy->get_num_remap_operator()-1)
            current_remap_dst_data_grid->compute_remap_field_data_runtime_mask(data_grid_dst,
                                                                               runtime_mask_sub_grids_dst,
                                                                               &num_runtime_mask_sub_grids_dst,
                                                                               &runtime_mask_dst);
        else current_remap_dst_data_grid->compute_remap_field_data_runtime_mask(NULL,runtime_mask_sub_grids_dst,
                                                                                &num_runtime_mask_sub_grids_dst,
                                                                                &runtime_mask_dst);
		field_data_grids_in_remapping_process[num_field_data_grids_in_remapping_process] = current_remap_src_data_grid_interchanged;
		runtime_mask_fields_in_remapping_process[num_field_data_grids_in_remapping_process++] = runtime_mask_src;
		field_data_grids_in_remapping_process[num_field_data_grids_in_remapping_process] = current_remap_dst_data_grid;
		runtime_mask_fields_in_remapping_process[num_field_data_grids_in_remapping_process++] = runtime_mask_dst;		
        current_remap_src_data_grid = current_remap_dst_data_grid;
    }

	field_data_grids_in_remapping_process[num_field_data_grids_in_remapping_process] = data_grid_dst;
	runtime_mask_fields_in_remapping_process[num_field_data_grids_in_remapping_process++] = NULL;

	return num_field_data_grids_in_remapping_process;
}


void Remap_weight_of_strategy_class::set_basic_fields(const char *object_name, Remap_strategy_class *remap_strategy, Remap_grid_class *data_grid_src, Remap_grid_class *data_grid_dst)
{
        strcpy(this->object_name, object_name);
        this->remap_strategy = remap_strategy;
        this->data_grid_src = data_grid_src;
        this->data_grid_dst = data_grid_dst;
}


bool Remap_weight_of_strategy_class::match_object_name(const char*object_name)
{
    return words_are_the_same(this->object_name, object_name);
}


Remap_weight_of_strategy_class::~Remap_weight_of_strategy_class()
{
	if (!public_remap_weights_of_operators)
	    for (int i = 0; i < remap_weights_of_operators.size(); i ++)
    	    delete remap_weights_of_operators[i];
}


Remap_weight_of_operator_instance_class *Remap_weight_of_strategy_class::add_remap_weight_of_operator_instance(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst,
                                                                  long remap_beg_iter, Remap_operator_basis *remap_operator)
{
	Remap_weight_of_operator_instance_class *remap_weight_of_operator_instance = new Remap_weight_of_operator_instance_class(field_data_grid_src, field_data_grid_dst, remap_beg_iter, remap_operator);
    add_remap_weight_of_operator_instance(remap_weight_of_operator_instance);
	return remap_weight_of_operator_instance;
}


void Remap_weight_of_strategy_class::add_remap_weight_of_operator_instance(Remap_weight_of_operator_instance_class *weight_of_operator_instance) 
{ 
	if (remap_weights_of_operators.size() == 0 || remap_weights_of_operators[remap_weights_of_operators.size()-1]->field_data_grid_src != weight_of_operator_instance->field_data_grid_src) {
		Remap_weight_of_operator_class *remap_weight_of_operator = new Remap_weight_of_operator_class(weight_of_operator_instance->field_data_grid_src, weight_of_operator_instance->field_data_grid_dst, weight_of_operator_instance->original_remap_operator,
																									  weight_of_operator_instance->operator_grid_src, weight_of_operator_instance->operator_grid_dst);
		remap_weights_of_operators.push_back(remap_weight_of_operator);
	}
	remap_weights_of_operators[remap_weights_of_operators.size()-1]->add_remap_weight_of_operator_instance(weight_of_operator_instance);
}


void Remap_weight_of_strategy_class::do_remap(Remap_grid_data_class *field_data_src, Remap_grid_data_class *field_data_dst)
{
    Remap_grid_class *sized_sub_grids[256];
    Remap_grid_class *field_data_grid_src, *field_data_grid_dst;
    Remap_grid_data_class *tmp_field_data_src, *tmp_field_data_dst;
    Remap_operator_basis *current_remap_operator;
    double *data_value_src, *data_value_dst;
    bool is_last_remap_operator;
    int i, j, k, num_sized_sub_grids;
    long remap_beg_iter, remap_end_iter;
    long index_size_iter, field_array_offset, index_size_array[256], current_runtime_index_array[256];
	Remap_grid_class *original_lev_grid_src = NULL, *original_lev_grid_dst = NULL;

    
    EXECUTION_REPORT(REPORT_ERROR, field_data_src->get_coord_value_grid()->is_similar_grid_with(data_grid_src),
                 "the grid of field data \"%s\" can not match the src grid of remap weight object \"%s\"",
                 field_data_src->get_grid_data_field()->field_name_in_application, object_name);
    EXECUTION_REPORT(REPORT_ERROR, field_data_dst->get_coord_value_grid()->is_similar_grid_with(data_grid_dst),
                 "the grid of field data \"%s\" can not match the dst grid of remap weight object \"%s\"",
                 field_data_dst->get_grid_data_field()->field_name_in_application, object_name);

    field_data_src->transfer_field_attributes_to_another(field_data_dst);
    if (!field_data_dst->have_data_content())
        field_data_dst->get_grid_data_field()->initialize_to_fill_value();

	tmp_field_data_dst = field_data_src;
	tmp_field_data_src = NULL;
	for (i = 0; i < remap_weights_of_operators.size(); i ++) {
		if (tmp_field_data_src != NULL && tmp_field_data_src != field_data_src)
			delete tmp_field_data_src;
		tmp_field_data_src = tmp_field_data_dst;
		if (i == remap_weights_of_operators.size()-1) {
			tmp_field_data_dst = field_data_dst;
			EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operators[i]->field_data_grid_dst->is_similar_grid_with(tmp_field_data_dst->get_coord_value_grid()), 
						 "remap software error1 in do_remap of Remap_weight_of_strategy_class\n");
		}	
		else tmp_field_data_dst = field_data_src->duplicate_grid_data_field(remap_weights_of_operators[i]->field_data_grid_dst, 1, false, false);
		remap_weights_of_operators[i]->do_remap(tmp_field_data_src, tmp_field_data_dst);
	}

	if (tmp_field_data_src != NULL && tmp_field_data_src != field_data_src)
		delete tmp_field_data_src;
    field_data_dst->interchange_grid_data(field_data_dst->get_coord_value_grid());
	field_data_dst->get_grid_data_field()->read_data_size = field_data_dst->get_grid_data_field()->required_data_size;
}


void Remap_weight_of_strategy_class::calculate_src_decomp(Remap_grid_class *grid_src, Remap_grid_class *grid_dst, long *decomp_map_src, const long *decomp_map_dst)
{
    Remap_grid_data_class *decomp_map_field_src, *decomp_map_field_dst;
    Remap_grid_data_class *expanded_decomp_map_field_src, *expanded_decomp_map_field_dst;
	Remap_grid_data_class *decomp_map_fields[256];
    long i, j;
    long *tmp_decomp_map_src;


    for (i = 0; i < grid_src->get_grid_size(); i ++)
        decomp_map_src[i] = 0;
 	
    EXECUTION_REPORT(REPORT_ERROR, grid_src->get_grid_mask_field() != NULL && grid_dst->get_grid_mask_field() != NULL, "C-Coupler error in calculate_src_decomp\n");
    decomp_map_field_src = grid_src->get_grid_mask_field()->duplicate_grid_data_field(grid_src, 1, false, true);
	decomp_map_field_src->change_datatype_in_application(DATA_TYPE_LONG);
    decomp_map_field_dst = grid_dst->get_grid_mask_field()->duplicate_grid_data_field(grid_dst, 1, false, true);
	decomp_map_field_dst->change_datatype_in_application(DATA_TYPE_LONG);
	for (i = 0; i < grid_src->get_grid_size(); i ++)
		((long*) decomp_map_field_src->get_grid_data_field()->data_buf)[i] = 0;
	for (i = 0; i < grid_dst->get_grid_size(); i ++)
		((long*) decomp_map_field_dst->get_grid_data_field()->data_buf)[i] = decomp_map_dst[i];
    expanded_decomp_map_field_src = data_grid_src->expand_to_generate_full_coord_value(decomp_map_field_src);
    expanded_decomp_map_field_dst = data_grid_dst->expand_to_generate_full_coord_value(decomp_map_field_dst);

	decomp_map_fields[0] = expanded_decomp_map_field_src;
	for (i = 0; i < remap_weights_of_operators.size() - 1; i ++)
		decomp_map_fields[i+1] = expanded_decomp_map_field_src->duplicate_grid_data_field(remap_weights_of_operators[i]->field_data_grid_dst, 1, false, false);
	decomp_map_fields[remap_weights_of_operators.size()] = expanded_decomp_map_field_dst;

	EXECUTION_REPORT(REPORT_LOG, true, "before calculate_src_decomp");
	for (i = remap_weights_of_operators.size() - 1; i >= 0; i --)
		remap_weights_of_operators[i]->calculate_src_decomp(decomp_map_fields[i], decomp_map_fields[i+1]);
	EXECUTION_REPORT(REPORT_LOG, true, "after calculate_src_decomp");

    expanded_decomp_map_field_src->interchange_grid_data(grid_src);
    for (i = 0; i < data_grid_src->get_grid_size()/grid_src->get_grid_size(); i ++) {
        tmp_decomp_map_src = ((long*) expanded_decomp_map_field_src->get_grid_data_field()->data_buf) + i*grid_src->get_grid_size();
        for (j = 0; j < grid_src->get_grid_size(); j ++)
			decomp_map_src[j] = (decomp_map_src[j] | tmp_decomp_map_src[j]);
    }

    delete decomp_map_field_src;
    delete decomp_map_field_dst;
	for (i = 0; i < remap_weights_of_operators.size()+1; i ++)
		delete decomp_map_fields[i];
}


Remap_grid_class **Remap_weight_of_strategy_class::get_remap_related_grids(int &num_operator_field_data_grids)
{
    Remap_operator_basis *current_remap_operator;
    Remap_grid_class **all_remap_related_grids, **temp_grids;
    int array_size = 100;
	int i, j, k;
	

    num_operator_field_data_grids = 0;
    current_remap_operator = NULL;
    all_remap_related_grids = new Remap_grid_class *[array_size];

    all_remap_related_grids[num_operator_field_data_grids++] = data_grid_src;
    all_remap_related_grids[num_operator_field_data_grids++] = data_grid_dst;
    for (i = 0; i < remap_weights_of_operators.size(); i ++) 
		for (j = 0; j < remap_weights_of_operators[i]->remap_weights_of_operator_instances.size(); j ++) {
	        if (num_operator_field_data_grids + 4 > array_size) {
	            temp_grids = all_remap_related_grids;
	            all_remap_related_grids = new Remap_grid_class *[array_size*2];
	            for (k = 0; k < array_size; k ++)
	                all_remap_related_grids[k] = temp_grids[k];
	            delete [] temp_grids;
	            array_size *= 2;
	        }
	        all_remap_related_grids[num_operator_field_data_grids++] = remap_weights_of_operators[i]->remap_weights_of_operator_instances[j]->field_data_grid_src;
	        all_remap_related_grids[num_operator_field_data_grids++] = remap_weights_of_operators[i]->remap_weights_of_operator_instances[j]->field_data_grid_dst;
	        all_remap_related_grids[num_operator_field_data_grids++] = remap_weights_of_operators[i]->remap_weights_of_operator_instances[j]->operator_grid_src;
	        all_remap_related_grids[num_operator_field_data_grids++] = remap_weights_of_operators[i]->remap_weights_of_operator_instances[j]->operator_grid_dst;
	    }

    return all_remap_related_grids;
}


Remap_weight_of_strategy_class *Remap_weight_of_strategy_class::generate_parallel_remap_weights(Remap_grid_class **remap_related_decomp_grids, 
                                                                                                 Remap_grid_class **decomp_original_grids, 
                                                                                                 int **global_cells_local_indexes_in_decomps)
{
    int i, j, k, field_data_grids_iter, num_sized_sub_grids, index_size_array[256], current_runtime_index_array[256];
    Remap_operator_basis *current_remap_operator;
    Remap_weight_of_strategy_class *parallel_remap_weights_of_strategy = new Remap_weight_of_strategy_class;
    Remap_weight_of_operator_instance_class *parallel_remap_weights_of_operator_instance;
    Remap_grid_class *sized_sub_grids[256];
    long remap_beg_iter, remap_end_iter, index_size_iter, global_field_array_offset, local_field_array_offset;


    EXECUTION_REPORT(REPORT_ERROR, decomp_original_grids[0]->is_subset_of_grid(this->data_grid_src) && decomp_original_grids[1]->is_subset_of_grid(this->data_grid_dst),
                 "C-Coupler error1 in generate_parallel_remap_weights of Remap_weight_of_strategy_class\n");
    EXECUTION_REPORT(REPORT_ERROR, this->data_grid_src->get_num_dimensions() >= 2 && this->data_grid_src->get_num_dimensions() <= 3,
                 "C-Coupler error2 in generate_parallel_remap_weights of Remap_weight_of_strategy_class\n");

    field_data_grids_iter = 0;
    strcpy(parallel_remap_weights_of_strategy->object_name, this->object_name);
    parallel_remap_weights_of_strategy->remap_strategy = this->remap_strategy;
    parallel_remap_weights_of_strategy->data_grid_src = remap_related_decomp_grids[field_data_grids_iter++];
    parallel_remap_weights_of_strategy->data_grid_dst = remap_related_decomp_grids[field_data_grids_iter++];

	for (i = 0; i < remap_weights_of_operators.size(); i ++)
		remap_weights_of_operators[i]->generate_parallel_remap_weights(remap_related_decomp_grids, decomp_original_grids, global_cells_local_indexes_in_decomps, field_data_grids_iter, parallel_remap_weights_of_strategy);

    return parallel_remap_weights_of_strategy;
}


void Remap_weight_of_strategy_class::write_data_into_array(void *data, int data_size, char **array, long &current_array_size, long &max_array_size)
{
	char *new_array;

	
	if (data_size + current_array_size > max_array_size) {
		max_array_size = (data_size+current_array_size)*2;
		new_array = new char [max_array_size];
		for (long i = 0; i < current_array_size; i ++)
			new_array[i] = (*array)[i];
		delete [] (*array);
		(*array) = new_array;
	}

	for (long i = 0; i < data_size; i ++)
		(*array)[current_array_size+i] = ((char*)data)[i];
	current_array_size += data_size;
}


void Remap_weight_of_strategy_class::write_grid_info_into_array(Remap_grid_class *grid, bool consider_area_or_volumn, char **array, long &current_array_size, long &max_array_size)
{
    long grid_size;
    int grid_num_dimensions, i, id, num_leaf_grids, tmp_int_value;
    Remap_grid_class *leaf_grids[256];
    
    
    grid_size = grid->get_grid_size();
	write_data_into_array(&grid_size, sizeof(long), array, current_array_size, max_array_size);
    grid_num_dimensions = grid->get_num_dimensions();
	write_data_into_array(&grid_num_dimensions, sizeof(int), array, current_array_size, max_array_size);
    grid->get_leaf_grids(&num_leaf_grids, leaf_grids, grid);
    for (i = 0; i < num_leaf_grids; i ++) {
        if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LON))
            id = 1;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LAT))
            id = 2;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LEV))
            id = 3;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_TIME))
            id = 4;
		write_data_into_array(&id, sizeof(int), array, current_array_size, max_array_size);
    }

    if (consider_area_or_volumn) {
        if (grid->get_area_or_volumn() != NULL) {
            tmp_int_value = 1;
			write_data_into_array(&tmp_int_value, sizeof(int), array, current_array_size, max_array_size);
			write_data_into_array(grid->get_area_or_volumn(), sizeof(double)*grid->get_grid_size(), array, current_array_size, max_array_size);
        }
        else {
            tmp_int_value = 0;
			write_data_into_array(&tmp_int_value, sizeof(int), array, current_array_size, max_array_size);
        }
    }
}


void Remap_weight_of_strategy_class::write_remap_weights_into_array(char **array, long &array_size, bool write_grid)
{
    Remap_grid_class *remap_grid_src, *remap_grid_dst;
    Remap_grid_class *leaf_grids[256];
    long grid_size, tmp_long_value;
    int num_leaf_grids, i, j, k, id, grid_num_dimensions, tmp_int_value;
    int num_remap_operator_instances;
    Remap_operator_basis *remap_operator_of_one_instance;
    Remap_weight_of_operator_instance_class *remap_weight_of_operator_instance;
    Remap_weight_sparse_matrix *remap_weights_group;
    char operator_name[256];
	char *output_array;
	long max_array_size;

	
	array_size = 0;
	max_array_size = 1024*1024;
	output_array = new char [max_array_size];

    remap_grid_src = get_data_grid_src();
    remap_grid_dst = get_data_grid_dst();

	if (write_grid) {
	    write_grid_info_into_array(remap_grid_src, true, &output_array, array_size, max_array_size);
    	write_grid_info_into_array(remap_grid_dst, true, &output_array, array_size, max_array_size);
	}
	for (i = 0, num_remap_operator_instances = 0; i < remap_weights_of_operators.size(); i ++)
		num_remap_operator_instances += remap_weights_of_operators[i]->remap_weights_of_operator_instances.size();
	write_data_into_array(&num_remap_operator_instances, sizeof(int), &output_array, array_size, max_array_size);
	for (k = 0; k < remap_weights_of_operators.size(); k ++)
	    for (i = 0; i < remap_weights_of_operators[k]->remap_weights_of_operator_instances.size(); i ++) {
	        remap_weight_of_operator_instance = remap_weights_of_operators[k]->remap_weights_of_operator_instances[i];
	        tmp_long_value = remap_weight_of_operator_instance->get_remap_begin_iter();
			write_data_into_array(&tmp_long_value, sizeof(long), &output_array, array_size, max_array_size);
			tmp_long_value = remap_weight_of_operator_instance->get_remap_end_iter();
			write_data_into_array(&tmp_long_value, sizeof(long), &output_array, array_size, max_array_size);
	        remap_operator_of_one_instance = remap_weights_of_operators[k]->remap_weights_of_operator_instances[i]->duplicated_remap_operator;
		    EXECUTION_REPORT(REPORT_ERROR, remap_operator_of_one_instance != NULL, "C-Coupler software error1 in write_remap_weights_into_array");
	        memset(operator_name, 0, 256);
			if (write_grid) {
				strcpy(operator_name, remap_operator_of_one_instance->get_operator_name());
				write_data_into_array(operator_name, sizeof(char)*256, &output_array, array_size, max_array_size);
		        write_grid_info_into_array(remap_weight_of_operator_instance->get_field_data_grid_src(), false, &output_array, array_size, max_array_size);
		        write_grid_info_into_array(remap_weight_of_operator_instance->get_field_data_grid_dst(), false, &output_array, array_size, max_array_size);
		        write_grid_info_into_array(remap_operator_of_one_instance->get_src_grid(), false, &output_array, array_size, max_array_size);
		        write_grid_info_into_array(remap_operator_of_one_instance->get_dst_grid(), false, &output_array, array_size, max_array_size);
			}
			else {
				strcpy(operator_name, remap_operator_of_one_instance->get_object_name());
				write_data_into_array(operator_name, sizeof(char)*256, &output_array, array_size, max_array_size);
			}
	        tmp_int_value = remap_operator_of_one_instance->get_num_remap_weights_groups();
			write_data_into_array(&tmp_int_value, sizeof(int), &output_array, array_size, max_array_size);
	        for (j = 0; j < remap_operator_of_one_instance->get_num_remap_weights_groups(); j ++) {
	            remap_weights_group = remap_operator_of_one_instance->get_remap_weights_group(j);
	            tmp_long_value = remap_weights_group->get_num_weights();
				write_data_into_array(&tmp_long_value, sizeof(long), &output_array, array_size, max_array_size);
				write_data_into_array(remap_weights_group->get_indexes_src_grid(), sizeof(long)*tmp_long_value, &output_array, array_size, max_array_size);
				write_data_into_array(remap_weights_group->get_indexes_dst_grid(), sizeof(long)*tmp_long_value, &output_array, array_size, max_array_size);
				write_data_into_array(remap_weights_group->get_weight_values(), sizeof(double)*tmp_long_value, &output_array, array_size, max_array_size);
	            tmp_long_value = remap_weights_group->get_num_remaped_dst_cells_indexes();
				write_data_into_array(&tmp_long_value, sizeof(long), &output_array, array_size, max_array_size);
				write_data_into_array(remap_weights_group->get_remaped_dst_cells_indexes(), sizeof(long)*tmp_long_value, &output_array, array_size, max_array_size);
	        }
	    }

	*array = output_array;
}


void Remap_weight_of_strategy_class::read_data_from_array(void *data, int data_size, const char *input_array, FILE *fp_binary, long &current_array_pos, long array_size, bool read_weight_values)
{
	EXECUTION_REPORT(REPORT_ERROR, current_array_pos+data_size <= array_size, "the access of array is out-of-bound when reading for remapping weights %s", object_name);

	if (read_weight_values) {
		if (input_array != NULL) {
			for (long i = 0; i < data_size; i ++)
				((char*)data)[i] = input_array[current_array_pos+i];
		}
		else fread((char*)data, 1, data_size, fp_binary);
	}
	else if (fp_binary != NULL)
		fseek(fp_binary, data_size, SEEK_CUR);
	current_array_pos += data_size;
}


void Remap_weight_of_strategy_class::read_grid_info_from_array(Remap_grid_class *grid, bool consider_area_or_volumn, const char *input_array, FILE *fp_binary, long &current_array_pos, long array_size)
{
    long grid_size;
    int grid_num_dimensions, i, gid, rid, num_leaf_grids, tmp_int_value;
    Remap_grid_class *leaf_grids[256];
    double *area_or_volumn;
    

	read_data_from_array(&grid_size, sizeof(long), input_array, fp_binary, current_array_pos, array_size, true);
    EXECUTION_REPORT(REPORT_ERROR, grid_size == grid->get_grid_size(), "the grid size of %s does not match the binary file\n", grid->get_grid_name());
	read_data_from_array(&grid_num_dimensions, sizeof(int), input_array, fp_binary, current_array_pos, array_size, true);
    EXECUTION_REPORT(REPORT_ERROR, grid_num_dimensions == grid->get_num_dimensions(), "the number of dimensions of grid %s does not match the binary file\n", grid->get_grid_name());
    grid->get_leaf_grids(&num_leaf_grids, leaf_grids, grid);
    for (i = 0; i < num_leaf_grids; i ++) {
        if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LON))
            gid = 1;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LAT))
            gid = 2;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LEV))
            gid = 3;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_TIME))
            gid = 4;
		read_data_from_array(&rid, sizeof(int), input_array, fp_binary, current_array_pos, array_size, true);
        EXECUTION_REPORT(REPORT_ERROR, gid == rid, "the arrange of coordinate systems of grid %s does not match the binary file\n", grid->get_grid_name());
    }

    if (consider_area_or_volumn) {
		read_data_from_array(&tmp_int_value, sizeof(int), input_array, fp_binary, current_array_pos, array_size, true);
        if (tmp_int_value == 1) {
            EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn() != NULL, "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
            area_or_volumn = new double [grid->get_grid_size()];
			read_data_from_array(area_or_volumn, sizeof(double)*grid->get_grid_size(), input_array, fp_binary, current_array_pos, array_size, true);
            for (long i = 0; i < grid->get_grid_size(); i ++)
                EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn()[i] == area_or_volumn[i], "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
            delete [] area_or_volumn;            
        }
        else EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn() == NULL, "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
    }
}


void Remap_weight_of_strategy_class::read_remap_operator_instance_from_array(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst,
															  Remap_grid_class *operator_grid_src, Remap_grid_class *operator_grid_dst,
                                                              Remap_operator_basis *remap_operator, long remap_begin_iter, long remap_end_iter,
                                                              const char *input_array, FILE *fp_binary, long &current_array_pos, long array_size,
                                                              bool read_weight_values)    
{
    Remap_operator_basis *duplicated_remap_operator;
    Remap_weight_of_operator_instance_class *remap_operator_instance;
    int i, num_remap_weights_groups;
    long num_weights, num_remaped_dst_cells_indexes, *indexes_src_grid, *indexes_dst_grid, *remaped_dst_cells_indexes;
    Remap_weight_sparse_matrix *weight_sparse_matrix;
    double *weight_values;


	if (read_weight_values)
	    duplicated_remap_operator = remap_operator->duplicate_remap_operator(false);
	else duplicated_remap_operator = NULL;
	read_data_from_array(&num_remap_weights_groups, sizeof(int), input_array, fp_binary, current_array_pos, array_size, true);
    for (i = 0; i < num_remap_weights_groups; i ++) {
		read_data_from_array(&num_weights, sizeof(long), input_array, fp_binary, current_array_pos, array_size, true);
		if (read_weight_values) {
	        indexes_src_grid = new long [num_weights];
	        indexes_dst_grid = new long [num_weights];
    	    weight_values = new double [num_weights];
		}
		read_data_from_array(indexes_src_grid, sizeof(long)*num_weights, input_array, fp_binary, current_array_pos, array_size, read_weight_values);
		read_data_from_array(indexes_dst_grid, sizeof(long)*num_weights, input_array, fp_binary, current_array_pos, array_size, read_weight_values);
		read_data_from_array(weight_values, sizeof(double)*num_weights, input_array, fp_binary, current_array_pos, array_size, read_weight_values);
		read_data_from_array(&num_remaped_dst_cells_indexes, sizeof(long), input_array, fp_binary, current_array_pos, array_size, true);
		if (read_weight_values)
	        remaped_dst_cells_indexes = new long [num_remaped_dst_cells_indexes];
		read_data_from_array(remaped_dst_cells_indexes, sizeof(long)*num_remaped_dst_cells_indexes, input_array, fp_binary, current_array_pos, array_size, read_weight_values);
		if (read_weight_values) {
	        weight_sparse_matrix = new Remap_weight_sparse_matrix(remap_operator, num_weights, indexes_src_grid, indexes_dst_grid, weight_values, num_remaped_dst_cells_indexes, remaped_dst_cells_indexes);
    	    duplicated_remap_operator->add_weight_sparse_matrix(weight_sparse_matrix);
		}
    }
    
    remap_operator_instance = new Remap_weight_of_operator_instance_class(field_data_grid_src, field_data_grid_dst, remap_begin_iter, remap_operator, duplicated_remap_operator);
	remap_operator_instance->operator_grid_src = operator_grid_src;
	remap_operator_instance->operator_grid_dst = operator_grid_dst;
	remap_operator_instance->remap_end_iter = remap_end_iter;
    add_remap_weight_of_operator_instance(remap_operator_instance);
}


void Remap_weight_of_strategy_class::read_remap_weights_from_array(const char *input_array, FILE *fp_binary, long array_size, bool read_grid, Remap_grid_class **remap_related_decomp_grids, bool read_weight_values)
{
    Remap_grid_class *field_grid_src, *field_grid_dst, *current_field_grid_src, *current_field_grid_dst;
	Remap_grid_class *operator_grid_src, *operator_grid_dst;
    int i, j, k, m, num_remap_operator_instances, num_remap_operator, num_leaf_grids_all, num_leaf_grids_remap_operator;
    Remap_operator_basis *remap_operator;
    int coord_system_ids[256], tmp_grid_num_dimensions, num_sized_sub_grids;
    long tmp_grid_size, current_remap_iter, remap_end_iter;
    char operator_name[256], last_operator_name[256], tmp_grid_name[256];
	long current_array_pos = 0;
	int field_data_grids_iter = 0;
    

	if (read_grid) {
	    field_grid_src = get_data_grid_src();
	    field_grid_dst = get_data_grid_dst();
		m = 1;
		EXECUTION_REPORT(REPORT_ERROR, remap_related_decomp_grids == NULL, "software error in read_remap_weights_from_array");
    	read_grid_info_from_array(field_grid_src, true, input_array, fp_binary, current_array_pos, array_size);
    	read_grid_info_from_array(field_grid_dst, true, input_array, fp_binary, current_array_pos, array_size);
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR, remap_related_decomp_grids != NULL, "software error in read_remap_weights_from_array");
	    field_grid_src = remap_related_decomp_grids[field_data_grids_iter++];
    	field_grid_dst = remap_related_decomp_grids[field_data_grids_iter++];
	}

	read_data_from_array(&num_remap_operator_instances, sizeof(int), input_array, fp_binary, current_array_pos, array_size, true);
    num_remap_operator = 0;
    current_field_grid_src = field_grid_src;
    for (i = 0; i < num_remap_operator_instances; i ++) {
		read_data_from_array(&current_remap_iter, sizeof(long), input_array, fp_binary, current_array_pos, array_size, true);
		read_data_from_array(&remap_end_iter, sizeof(long), input_array, fp_binary, current_array_pos, array_size, true);
		read_data_from_array(operator_name, sizeof(char)*256, input_array, fp_binary, current_array_pos, array_size, true);
		if (read_grid) {
			if (i == 0)
				strcpy(last_operator_name, operator_name);
			if (!words_are_the_same(last_operator_name, operator_name)) {
				num_remap_operator ++;
				strcpy(last_operator_name, operator_name);
			}
    	    remap_operator = remap_strategy->get_remap_operator(num_remap_operator);
			EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(operator_name, remap_operator->get_operator_name()),
							 "the remap operator %s does not match the binary file, which should be %s\n", 
							 operator_name, remap_operator->get_operator_name());
			read_data_from_array(&tmp_grid_size, sizeof(long), input_array, fp_binary, current_array_pos, array_size, true);
			read_data_from_array(&tmp_grid_num_dimensions, sizeof(int), input_array, fp_binary, current_array_pos, array_size, true);
	        EXECUTION_REPORT(REPORT_ERROR, tmp_grid_num_dimensions == field_grid_src->get_num_dimensions(), "remap software error2 in read_remap_weights_from_array binary\n");
	        for (j = 0; j < tmp_grid_num_dimensions; j ++)
				read_data_from_array(&coord_system_ids[j], sizeof(int), input_array, fp_binary, current_array_pos, array_size, true);
	        num_sized_sub_grids = 0;
			current_field_grid_src = field_data_grids_in_remapping_process[1+num_remap_operator*2];
			current_field_grid_dst = field_data_grids_in_remapping_process[2+num_remap_operator*2];
	        read_grid_info_from_array(current_field_grid_dst, false, input_array, fp_binary, current_array_pos, array_size);
    	    read_grid_info_from_array(remap_operator->get_src_grid(), false, input_array, fp_binary, current_array_pos, array_size);
        	read_grid_info_from_array(remap_operator->get_dst_grid(), false, input_array, fp_binary, current_array_pos, array_size);
			operator_grid_src = remap_operator->get_src_grid();
			operator_grid_dst = remap_operator->get_dst_grid();

    	}
		else {
			remap_operator = remap_operator_manager->search_remap_operator(operator_name);
			EXECUTION_REPORT(REPORT_ERROR, remap_operator != NULL, "software error when searching remap operator %s in read_remap_weights_from_array", operator_name);
			current_field_grid_src = remap_related_decomp_grids[field_data_grids_iter+0];
			current_field_grid_dst = remap_related_decomp_grids[field_data_grids_iter+1];
			operator_grid_src = remap_related_decomp_grids[field_data_grids_iter+2];
			operator_grid_dst = remap_related_decomp_grids[field_data_grids_iter+3];
			field_data_grids_iter += 4;
		}
		read_remap_operator_instance_from_array(current_field_grid_src, current_field_grid_dst, operator_grid_src, operator_grid_dst, remap_operator, current_remap_iter, remap_end_iter, input_array, fp_binary, current_array_pos, array_size, read_weight_values);
    }
    EXECUTION_REPORT(REPORT_ERROR, current_field_grid_dst->is_similar_grid_with(field_grid_dst), "remap software error4 in read_remap_weights_from_array\n");
	EXECUTION_REPORT(REPORT_ERROR, current_array_pos == array_size, "the input array does not match the remapping weights %s when reading", object_name);
}


void Remap_weight_of_strategy_class::check_remap_weights_format()
{
    Remap_grid_class *grid_src;
    int i, j, k;
    bool have_sphere_grid_remapping = false;
    

	for (k = 0; k < remap_weights_of_operators.size(); k ++)
	    for (i = 0; i < remap_weights_of_operators[k]->remap_weights_of_operator_instances.size(); i ++) {
			if (remap_weights_of_operators[k]->remap_weights_of_operator_instances[i]->duplicated_remap_operator == NULL)
				continue;
	        grid_src = remap_weights_of_operators[k]->remap_weights_of_operator_instances[i]->duplicated_remap_operator->get_src_grid();
	        j = 0;
	        if (grid_src->has_grid_coord_label(COORD_LABEL_LON))
	            j ++;
	        if (grid_src->has_grid_coord_label(COORD_LABEL_LAT))
	            j ++;
	        EXECUTION_REPORT(REPORT_ERROR, j == 0 || j == 2, "the remap operator %s for coupling must remap on both longitude and latitude\n", 
							 remap_weights_of_operators[k]->remap_weights_of_operator_instances[i]->duplicated_remap_operator->get_operator_name());
	        if (grid_src->has_grid_coord_label(COORD_LABEL_LON)) {
	            EXECUTION_REPORT(REPORT_ERROR, !have_sphere_grid_remapping, "the remap weights %s must have only one remap operator remapping on only one grid\n", 
								 remap_weights_of_operators[k]->remap_weights_of_operator_instances[i]->duplicated_remap_operator->get_object_name());
	            have_sphere_grid_remapping = true;
	        }
	    }
}


Remap_operator_basis *Remap_weight_of_strategy_class::get_unique_remap_operator_of_weights() 
{ 
	EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operators.size() > 0 && remap_weights_of_operators[0]->remap_weights_of_operator_instances.size() > 0 &&
					 remap_weights_of_operators[0]->remap_weights_of_operator_instances[0]->duplicated_remap_operator != NULL, 
					 "C-Coupler error in get_unique_remap_operator_of_weights");

	if (remap_weights_of_operators.size() == 1 && remap_weights_of_operators[0]->remap_weights_of_operator_instances.size() == 1)
		return remap_weights_of_operators[0]->remap_weights_of_operator_instances[0]->duplicated_remap_operator;
	else return NULL;
}


void Remap_weight_of_strategy_class::add_remap_weight_of_operators_to_manager(bool are_parallel_remap_weights)
{
	public_remap_weights_of_operators = true;
	for (int i = 0; i < remap_weights_of_operators.size(); i ++)
		if (are_parallel_remap_weights)
			parallel_remap_weight_of_operator_manager->add_remap_weights_of_operator(remap_weights_of_operators[i]);
		else sequential_remap_weight_of_operator_manager->add_remap_weights_of_operator(remap_weights_of_operators[i]);
}


Remap_grid_class *Remap_weight_of_strategy_class::get_field_data_grid_in_remapping_process(int i) 
{ 
	EXECUTION_REPORT(REPORT_ERROR, i < num_field_data_grids_in_remapping_process, "C-Coupler error in get_field_data_grid_in_remapping_process of Remap_weight_of_strategy_class");
	return field_data_grids_in_remapping_process[i]; 
}


Remap_grid_data_class *Remap_weight_of_strategy_class::get_runtime_mask_field_in_remapping_process(int i) 
{ 
	EXECUTION_REPORT(REPORT_ERROR, i < num_field_data_grids_in_remapping_process, "C-Coupler error in get_runtime_mask_field_in_remapping_process of Remap_weight_of_strategy_class");
	return runtime_mask_fields_in_remapping_process[i]; 
}


void Remap_weight_of_strategy_class::build_operations_for_calculating_sigma_values_of_grids()
{
	Remap_weight_of_strategy_class *sequential_weights_of_strategy_for_interpolating_surface_fields;
	Remap_operator_basis *original_remap_operators[256], *operator_for_interpolating_surface_fields;
	Remap_grid_class *operator_field_data_grids[256], *dynamic_surface_field_origin_grid = NULL, *leaf_grids[256];
	Operation_for_caculating_sigma_values *operation_for_caculating_sigma_values;
	Remap_strategy_class *new_remap_strategy;
	int i, j, num_operator_field_data_grids;
	char temp1_object_name[256];
	int original_execution_phase_number, num_leaf_grids;

	
	if (operations_for_caculating_sigma_values_of_grid.size() > 0)
		return;

	if (data_grid_src->is_sigma_grid() && data_grid_src->has_specified_sigma_grid_surface_value_field()) {
		EXECUTION_REPORT(REPORT_ERROR, data_grid_src == data_grid_src->get_a_leaf_grid(COORD_LABEL_LEV)->get_super_grid_of_setting_coord_values(), 
						 "%s should be a 3D sigma grid, but the vertical coordinate values are set in another grid %s. %s cannot be used as a source grid of remapping weights",
						 data_grid_src->get_grid_name(), data_grid_src->get_a_leaf_grid(COORD_LABEL_LEV)->get_super_grid_of_setting_coord_values()->get_grid_name(), data_grid_src->get_grid_name());
        dynamic_surface_field_origin_grid = data_grid_src;
	}
	if (data_grid_dst->is_sigma_grid() && data_grid_dst->has_specified_sigma_grid_surface_value_field()) {
		EXECUTION_REPORT(REPORT_ERROR, data_grid_dst == data_grid_dst->get_a_leaf_grid(COORD_LABEL_LEV)->get_super_grid_of_setting_coord_values(), 
						 "%s should be a 3D sigma grid, but the vertical coordinate values are set in another grid %s. %s cannot be used as a source grid of remapping weights",
						 data_grid_dst->get_grid_name(), data_grid_dst->get_a_leaf_grid(COORD_LABEL_LEV)->get_super_grid_of_setting_coord_values()->get_grid_name(), data_grid_dst->get_grid_name());
		EXECUTION_REPORT(REPORT_ERROR, dynamic_surface_field_origin_grid == NULL, 
						 "The surface value fields (for 3D sigma grid) in source and target grids of remapping weights %s are both specified by users. Only one surface value field can be specified by users.", get_object_name());
        dynamic_surface_field_origin_grid = data_grid_dst;
	}
	
	if (dynamic_surface_field_origin_grid == NULL) {
		if (data_grid_src->is_sigma_grid() && data_grid_dst->is_sigma_grid()) 
			EXECUTION_REPORT(REPORT_ERROR, false, "The bottom field of sigma grid \"%s\" or \"%s\" should be set before generating remapping weights \"%s\"", data_grid_src->get_grid_name(), data_grid_dst->get_grid_name(), object_name);
		else if (data_grid_src->is_sigma_grid())
			EXECUTION_REPORT(REPORT_ERROR, false, "The bottom field of sigma grid \"%s\" should be set before generating remapping weights \"%s\"", data_grid_src->get_grid_name(), object_name);
		else if (data_grid_dst->is_sigma_grid())
			EXECUTION_REPORT(REPORT_ERROR, false, "The bottom field of sigma grid \"%s\" should be set before generating remapping weights \"%s\"", data_grid_dst->get_grid_name(), object_name);
		return;
	}

	if (num_field_data_grids_in_remapping_process > 0) {
		if (data_grid_src->has_specified_sigma_grid_surface_value_field()) {
			for (i = 0; i < num_field_data_grids_in_remapping_process; i ++)
				operator_field_data_grids[i] = field_data_grids_in_remapping_process[i];
			for (i = 0; i < remap_strategy->get_num_remap_operator(); i ++)
				original_remap_operators[i] = remap_strategy->get_remap_operator(i);
		}
		else {
			for (i = num_field_data_grids_in_remapping_process - 1, j = 0; i >= 0; i --)
				operator_field_data_grids[j++] = field_data_grids_in_remapping_process[i];
			for (i = remap_strategy->get_num_remap_operator() - 1, j = 0; i >= 0; i --)
				original_remap_operators[j++] = remap_strategy->get_remap_operator(i);
		}
		num_operator_field_data_grids = num_field_data_grids_in_remapping_process;
	}
	else {
		if (data_grid_src->has_specified_sigma_grid_surface_value_field()) {
			operator_field_data_grids[0] = data_grid_src;
			num_operator_field_data_grids = 1;
			for (i = 0, j = 0; i < remap_strategy->get_num_remap_operator(); i ++) {
				operator_field_data_grids[num_operator_field_data_grids++] = get_remap_weights_of_operator(i)->get_field_data_grid_src();
				operator_field_data_grids[num_operator_field_data_grids++] = get_remap_weights_of_operator(i)->get_field_data_grid_dst();
				original_remap_operators[j++] = get_remap_weights_of_operator(i)->get_original_remap_operator();
			}
			operator_field_data_grids[num_operator_field_data_grids++] = data_grid_dst;
		}
		else {
			operator_field_data_grids[0] = data_grid_dst;
			num_operator_field_data_grids = 1;
			for (i = remap_strategy->get_num_remap_operator()-1, j = 0; i >= 0 ; i --) {
				operator_field_data_grids[num_operator_field_data_grids++] = get_remap_weights_of_operator(i)->get_field_data_grid_dst();
				operator_field_data_grids[num_operator_field_data_grids++] = get_remap_weights_of_operator(i)->get_field_data_grid_src();
				original_remap_operators[j++] = get_remap_weights_of_operator(i)->get_original_remap_operator();
			}
			operator_field_data_grids[num_operator_field_data_grids++] = data_grid_src;
		}
	}

	EXECUTION_REPORT(REPORT_LOG, true, "Find %d grids needing to be checked for updating vertical remap weights of dynamic sigma grid", num_operator_field_data_grids);

	for (i = 1; i < num_operator_field_data_grids; i ++) {
		if (operator_field_data_grids[i-1] == operator_field_data_grids[i])
			continue;
		if (!operator_field_data_grids[i]->is_sigma_grid())
			continue;
		EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i-1]->is_sigma_grid(), "C-Coupler error4 in build_operations_for_calculating_sigma_values_of_grids");			
		EXECUTION_REPORT(REPORT_ERROR,  operator_field_data_grids[i-1]->is_sigma_grid(), "C-Coupler error5 in build_operations_for_calculating_sigma_values_of_grids");
		operator_field_data_grids[i]->allocate_sigma_grid_specific_fields(NULL, NULL, NULL, 0, 0);
		EXECUTION_REPORT(REPORT_ERROR, !operator_field_data_grids[i]->has_specified_sigma_grid_surface_value_field(), "C-Coupler error6 in build_operations_for_calculating_sigma_values_of_grids");
		EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field() != NULL, "C-Coupler error8 in build_operations_for_calculating_sigma_values_of_grids");
		operation_for_caculating_sigma_values = new Operation_for_caculating_sigma_values;
		operation_for_caculating_sigma_values->current_3D_sigma_grid_dst = operator_field_data_grids[i];
		operation_for_caculating_sigma_values->current_3D_sigma_grid_src = operator_field_data_grids[i-1];
		operation_for_caculating_sigma_values->remap_weights = NULL;
		operations_for_caculating_sigma_values_of_grid.push_back(operation_for_caculating_sigma_values);			
		if (operator_field_data_grids[i]->is_similar_grid_with(operator_field_data_grids[i-1]))
			continue;
		if (original_remap_operators[(i-1)/2]->get_src_grid()->has_grid_coord_label(COORD_LABEL_LEV)) {
			EXECUTION_REPORT(REPORT_ERROR, original_remap_operators[(i-1)/2]->get_src_grid()->get_num_dimensions() == 1,
							 "C-Coupler error9: C-Coupler only supports 2D+1D way of 3D interpolation: only 1D interpolation for vertical direction");
			EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->is_subset_of_grid(operator_field_data_grids[i-1]) &&
							 operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->is_subset_of_grid(operator_field_data_grids[i]), "C-Coupler error10 in build_operations_for_calculating_sigma_values_of_grids");
			continue;
		}
		operator_for_interpolating_surface_fields = remap_operator_manager->search_remap_operator(operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid(), operator_field_data_grids[i]->get_sigma_grid_surface_value_field()->get_coord_value_grid(), original_remap_operators[(i-1)/2]->get_operator_name());
		if (operator_for_interpolating_surface_fields == NULL) {
			sprintf(temp1_object_name, "%s_for_interpolating_surface_field_between_%s_and_%s\0", original_remap_operators[(i-1)/2]->get_operator_name(), operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name(), operator_field_data_grids[i]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name());
			operator_for_interpolating_surface_fields = original_remap_operators[(i-1)/2]->duplicate_remap_operator(false);
			operator_for_interpolating_surface_fields->change_remap_operator_info(temp1_object_name, operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid(), operator_field_data_grids[i]->get_sigma_grid_surface_value_field()->get_coord_value_grid());
			remap_operator_manager->add_remap_operator(operator_for_interpolating_surface_fields);
		}
		sprintf(temp1_object_name, "remap_strategy_of_%s_for_interpolating_surface_field_between_%s_and_%s\0", original_remap_operators[(i-1)/2]->get_operator_name(), operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name(), operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name());
		new_remap_strategy = new Remap_strategy_class(temp1_object_name, 1, &operator_for_interpolating_surface_fields);
		remap_strategy_manager->add_remap_strategy(new_remap_strategy);
		original_execution_phase_number = execution_phase_number;
		execution_phase_number = 1;
		EXECUTION_REPORT(REPORT_LOG, true, "Generate remapping weights for surface fields of sigma grid from sphere grid %s to %s", operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name(), operator_field_data_grids[i]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name());
		sequential_weights_of_strategy_for_interpolating_surface_fields = remap_weights_of_strategy_manager->search_or_add_remap_weight_of_strategy(operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid(), operator_field_data_grids[i]->get_sigma_grid_surface_value_field()->get_coord_value_grid(), new_remap_strategy, NULL, NULL, NULL, false);
		execution_phase_number = original_execution_phase_number;
		operation_for_caculating_sigma_values->remap_weights = sequential_weights_of_strategy_for_interpolating_surface_fields;
	}
}


void Remap_weight_of_strategy_class::calculate_sigma_values_of_grids()
{
	Operation_for_caculating_sigma_values *current_operation;
	int i;


	for (i = 0; i < operations_for_caculating_sigma_values_of_grid.size(); i ++) {
		current_operation = operations_for_caculating_sigma_values_of_grid[i];

		EXECUTION_REPORT(REPORT_ERROR, current_operation->current_3D_sigma_grid_src->get_sigma_grid_surface_value_field() != NULL && current_operation->current_3D_sigma_grid_dst->get_sigma_grid_surface_value_field() != NULL,
						 "C-Coupler error in caculate_sigma_values_of_grids");
		if (current_operation->remap_weights == NULL)
			current_operation->current_3D_sigma_grid_dst->copy_sigma_grid_surface_value_field(current_operation->current_3D_sigma_grid_src->get_sigma_grid_surface_value_field());
		else {
			EXECUTION_REPORT(REPORT_LOG, true, "Interpolate sigma grid surface value field from grid %s to %s", current_operation->current_3D_sigma_grid_src->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name(), current_operation->current_3D_sigma_grid_dst->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name());
			current_operation->remap_weights->do_remap(current_operation->current_3D_sigma_grid_src->get_sigma_grid_surface_value_field(), current_operation->current_3D_sigma_grid_dst->get_sigma_grid_surface_value_field());
		}
		current_operation->current_3D_sigma_grid_dst->calculate_lev_sigma_values();
		current_operation->current_3D_sigma_grid_dst->set_coord_vertex_values_in_default();
	}
}


void Remap_weight_of_strategy_class::renew_object_name(const char*new_object_name)
{
	if (words_are_the_same(object_name, new_object_name))
		return;
	
	EXECUTION_REPORT(REPORT_ERROR, strncmp(object_name, "TEMP_WEIGHT", strlen("TEMP_WEIGHT")) == 0, "Remap weights %s is the same as %s. Please do not calculate the same remap weights more than once", object_name, new_object_name);
}

