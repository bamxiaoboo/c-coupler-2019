/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "remap_operator_linear.h"
#include "cor_global_data.h"
#include <string.h>
#include <math.h>


void Remap_operator_linear::set_parameter(const char *parameter_name, const char *parameter_value)
{
    EXECUTION_REPORT(REPORT_ERROR, enable_to_set_parameters, 
                 "the parameter of remap operator object \"%s\" must be set before using it to build remap strategy\n",
                 object_name);
    
    if (words_are_the_same(parameter_name, "period"))
        sscanf(parameter_value, "%d", &num_period);
    else if (words_are_the_same(parameter_name, "enable_extrapolation")) {
		EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(parameter_value,"true") || words_are_the_same(parameter_value,"false"),
		                 "the value of parameter \"enable_extrapolation\" of linear remapping algorithm must be \"true\" or \"false\"");
		if (words_are_the_same(parameter_value,"true"))
			enable_extrapolation = true;
    }
    else EXECUTION_REPORT(REPORT_ERROR, false, 
                      "\"%s\" is a illegal parameter of remap operator \"%s\"\n",
                      parameter_name,
                      operator_name);
}


void Remap_operator_linear::recursively_search_src_cells(double dst_cell_center_coord_value, 
                                                        long current_src_cell_index, 
                                                        long &src_cell_index_of_smaller_center_value, 
                                                        long &src_cell_index_of_bigger_center_value)
{
    long current_src_cell_neighbors_indexes[2];
    double current_src_cell_center_coord_value, difference_of_src_dst_coord_values;
    bool mask_of_current_src_cell;
    bool need_recursion = true;
    int i, num_neighbors;


    if (!visit_cell_in_src_grid(current_src_cell_index))
        return;
    
    get_cell_center_coord_values_of_src_grid(current_src_cell_index, &current_src_cell_center_coord_value);
    difference_of_src_dst_coord_values = compute_difference_of_two_coord_values(current_src_cell_center_coord_value, dst_cell_center_coord_value, 0);

    if (difference_of_src_dst_coord_values >= 0 && src_cell_index_of_bigger_center_value != -1)
        need_recursion = false;
    if (difference_of_src_dst_coord_values < 0 && src_cell_index_of_smaller_center_value != -1)
        need_recursion = false;

    if (!need_recursion)
        return;

    get_cell_mask_of_src_grid(current_src_cell_index, &mask_of_current_src_cell);
    if (mask_of_current_src_cell) {
        if (current_src_cell_center_coord_value == dst_cell_center_coord_value) {
            src_cell_index_of_smaller_center_value = current_src_cell_index;
            return;
        }
        else if (current_src_cell_center_coord_value > dst_cell_center_coord_value) 
            src_cell_index_of_bigger_center_value = current_src_cell_index;
        else src_cell_index_of_smaller_center_value = current_src_cell_index;
    }
    
    get_cell_neighbors_in_src_grid(current_src_cell_index, &num_neighbors, current_src_cell_neighbors_indexes);
    EXECUTION_REPORT(REPORT_ERROR, num_neighbors <= 2, 
                 "remap software error in linear recursively_search_src_cells\n");
    
    for (i = 0; i < num_neighbors; i ++)
        recursively_search_src_cells(dst_cell_center_coord_value, current_src_cell_neighbors_indexes[i], src_cell_index_of_smaller_center_value, src_cell_index_of_bigger_center_value);
}


void Remap_operator_linear::compute_remap_weights_of_one_dst_cell(long dst_cell_index)
{
    double dst_cell_center_coord_value, src_coord_values[3];
    long src_cell_indexes[3];
    bool dst_cell_mask;
    double remap_weight_values[2], coord_differences[2];
    long weight_src_indexes[2];
    int selected_src_index1, selected_src_index2;
    int i;


    get_cell_mask_of_dst_grid(dst_cell_index, &dst_cell_mask);
    if (!dst_cell_mask)
        return;

    initialize_computing_remap_weights_of_one_cell();
    
    get_cell_center_coord_values_of_dst_grid(dst_cell_index, &dst_cell_center_coord_value);
    search_cell_in_src_grid(&dst_cell_center_coord_value, &src_cell_indexes[1], true);
    EXECUTION_REPORT(REPORT_ERROR, src_cell_indexes[1] != -1, "remap software error1 in linear compute_remap_weights_of_one_dst_cell of Remap_operator_linear\n");
    src_cell_indexes[0] = src_cell_indexes[1] - 1;
    src_cell_indexes[2] = src_cell_indexes[1] + 1;
    if (src_grid->get_grid_cyclic()) {
        src_cell_indexes[0] = (src_cell_indexes[0]+src_grid->get_grid_size())%src_grid->get_grid_size();
        src_cell_indexes[2] = src_cell_indexes[2]%src_grid->get_grid_size();
    }
    else if (src_cell_indexes[2] == src_grid->get_grid_size())
        src_cell_indexes[2] = -1;

    for (i = 0; i < 3; i ++)
        if (src_cell_indexes[i] != -1) 
            get_cell_center_coord_values_of_src_grid(src_cell_indexes[i], &src_coord_values[i]);

    if (src_cell_indexes[0] == -1) {
        selected_src_index1 = 1;
        selected_src_index2 = 2;
    }
    else if (src_cell_indexes[2] == -1) {
        selected_src_index1 = 0;
        selected_src_index2 = 1;
    }
    else {
        if (compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[0],0)<= 0 &&
            compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[1],0)>= 0 ||
            compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[0],0)>= 0 &&
            compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[1],0)<= 0) {
            selected_src_index1 = 0;
            selected_src_index2 = 1;
        }
        else if (compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[1],0)<= 0 &&
            compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[2],0)>= 0 ||
            compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[1],0)>= 0 &&
            compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[2],0)<= 0) {
            selected_src_index1 = 1;
            selected_src_index2 = 2;
        }
        else EXECUTION_REPORT(REPORT_ERROR, false, "remap software error2 in linear compute_remap_weights_of_one_dst_cell of Remap_operator_linear\n");
    }

    weight_src_indexes[0] = src_cell_indexes[selected_src_index1];
    weight_src_indexes[1] = src_cell_indexes[selected_src_index2];
    coord_differences[0] = compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[selected_src_index1],0);
    coord_differences[1] = compute_difference_of_two_coord_values(dst_cell_center_coord_value,src_coord_values[selected_src_index2],0);

    if (coord_differences[0]*coord_differences[1] <= 0) {
        remap_weight_values[0] = fabs(coord_differences[1])/(fabs(coord_differences[0])+fabs(coord_differences[1]));
        remap_weight_values[1] = 1 - remap_weight_values[0];
		add_remap_weights_to_sparse_matrix(weight_src_indexes, dst_cell_index, remap_weight_values, 2, 0);		
    }
    else if (enable_extrapolation) {
        if (fabs(coord_differences[0]) > fabs(coord_differences[1])) {
            remap_weight_values[0] = -fabs(coord_differences[1])/fabs(coord_differences[0]-coord_differences[1]);
            remap_weight_values[1] = fabs(coord_differences[0])/fabs(coord_differences[0]-coord_differences[1]);
        }
        else {
            remap_weight_values[0] = fabs(coord_differences[1])/fabs(coord_differences[0]-coord_differences[1]);
            remap_weight_values[1] = -fabs(coord_differences[0])/fabs(coord_differences[0]-coord_differences[1]);            
        }
		add_remap_weights_to_sparse_matrix(weight_src_indexes, dst_cell_index, remap_weight_values, 2, 0);
    }
    finalize_computing_remap_weights_of_one_cell();

	if (!enable_extrapolation)
		EXECUTION_REPORT(REPORT_ERROR, remap_weight_values[0]>=0 && remap_weight_values[0]<=1.0 && remap_weight_values[1]>=0 && remap_weight_values[1]<=1.0,
						 "remap software error in Remap_operator_linear::compute_remap_weights_of_one_dst_cell");
}


void Remap_operator_linear::calculate_remap_weights()
{
    clear_remap_weight_info_in_sparse_matrix();

    for (long dst_cell_index = 0; dst_cell_index < dst_grid->get_grid_size(); dst_cell_index ++)
        compute_remap_weights_of_one_dst_cell(dst_cell_index);
}


Remap_operator_linear::Remap_operator_linear(const char *object_name, int num_remap_grids, Remap_grid_class **remap_grids)
                                       : Remap_operator_basis(object_name, 
                                                              REMAP_OPERATOR_NAME_LINEAR, 
                                                              1, 
                                                              true, 
                                                              false, 
                                                              true,
                                                              num_remap_grids, 
                                                              remap_grids)
{
    remap_weights_groups.push_back(new Remap_weight_sparse_matrix(this));
}


void Remap_operator_linear::do_remap_values_caculation(double *data_values_src, double *data_values_dst)
{
    remap_weights_groups[0]->remap_values(data_values_src, data_values_dst);
}


void Remap_operator_linear::do_src_decomp_caculation(bool *decomp_map_src, const bool *decomp_map_dst)
{
    remap_weights_groups[0]->calc_src_decomp(decomp_map_src, decomp_map_dst);
}


Remap_operator_basis *Remap_operator_linear::duplicate_remap_operator(bool fully_copy)
{
    Remap_operator_basis *duplicated_remap_operator = new Remap_operator_linear();
    copy_remap_operator_basic_data(duplicated_remap_operator, fully_copy);
    return duplicated_remap_operator;
}


Remap_operator_basis *Remap_operator_linear::generate_parallel_remap_operator(Remap_grid_class **decomp_original_grids, int **global_cells_local_indexes_in_decomps)
{
    Remap_operator_basis *parallel_remap_operator = this->duplicate_remap_operator(false);
    this->generate_parallel_remap_weights(parallel_remap_operator, decomp_original_grids, global_cells_local_indexes_in_decomps);
    return parallel_remap_operator;
}
