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
#include <string.h>


Remap_weight_of_operator_class::Remap_weight_of_operator_class(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst, 
                                                               long remap_beg_iter, Remap_operator_basis *remap_operator)
{
    this->field_data_grid_src = field_data_grid_src;
    this->field_data_grid_dst = field_data_grid_dst;
    this->remap_beg_iter = remap_beg_iter;
    this->remap_end_iter = -1;
    this->original_remap_operator = remap_operator;
    this->duplicated_remap_operator = remap_operator->duplicate_remap_operator(true);
    this->operator_grid_src = remap_operator->src_grid;
    this->operator_grid_dst = remap_operator->dst_grid;
}


Remap_weight_of_operator_class::Remap_weight_of_operator_class(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst, 
                                                               long remap_beg_iter, Remap_operator_basis *remap_operator, Remap_operator_basis *duplicated_remap_operator)
{
    this->field_data_grid_src = field_data_grid_src;
    this->field_data_grid_dst = field_data_grid_dst;
    this->remap_beg_iter = remap_beg_iter;
    this->remap_end_iter = -1;
    this->original_remap_operator = remap_operator;
    this->duplicated_remap_operator = duplicated_remap_operator;
    this->operator_grid_src = remap_operator->src_grid;
    this->operator_grid_dst = remap_operator->dst_grid;
}


Remap_weight_of_operator_class *Remap_weight_of_operator_class::generate_parallel_remap_weights(Remap_grid_class **decomp_original_grids, int **global_cells_local_indexes_in_decomps)
{
    Remap_weight_of_operator_class *parallel_remap_weights_of_operator;
    int overlap_with_decomp_counter = 0;


    parallel_remap_weights_of_operator = new Remap_weight_of_operator_class();
    parallel_remap_weights_of_operator->original_remap_operator = this->original_remap_operator;

    for (int i = 0; i < 2; i ++)
        if (this->original_remap_operator->get_src_grid()->have_overlap_with_grid(decomp_original_grids[i])) {
            EXECUTION_REPORT(REPORT_ERROR, decomp_original_grids[i]->is_subset_of_grid(this->original_remap_operator->get_src_grid()),
                         "C-Coupler error1 in generate_parallel_remap_weights of Remap_weight_of_operator_class\n");
            overlap_with_decomp_counter ++;
        }
    for (int i = 0; i < 2; i ++)
        if (this->original_remap_operator->get_dst_grid()->have_overlap_with_grid(decomp_original_grids[i])) {
            EXECUTION_REPORT(REPORT_ERROR, decomp_original_grids[i]->is_subset_of_grid(this->original_remap_operator->get_dst_grid()),
                         "C-Coupler error2 in generate_parallel_remap_weights of Remap_weight_of_operator_class\n");
            overlap_with_decomp_counter ++;
        }
    EXECUTION_REPORT(REPORT_ERROR, overlap_with_decomp_counter == 0 || overlap_with_decomp_counter == 2, "C-Coupler error3 in generate_parallel_remap_weights of Remap_weight_of_operator_class\n");

    if (overlap_with_decomp_counter > 0)
        parallel_remap_weights_of_operator->duplicated_remap_operator = this->duplicated_remap_operator->generate_parallel_remap_operator(decomp_original_grids, global_cells_local_indexes_in_decomps);
    else parallel_remap_weights_of_operator->duplicated_remap_operator = this->duplicated_remap_operator->duplicate_remap_operator(true);

    return parallel_remap_weights_of_operator;
}


Remap_weight_of_operator_class::~Remap_weight_of_operator_class()
{
    delete duplicated_remap_operator;
}


Remap_weight_of_strategy_class::Remap_weight_of_strategy_class(const char *object_name, const char *remap_strategy_name, 
                                                               const char *data_grid_name_src, const char *data_grid_name_dst,
                                                               bool read_weights)
{
    strcpy(this->object_name, object_name);
    remap_strategy = remap_strategy_manager->search_remap_strategy(remap_strategy_name);
    data_grid_src = remap_grid_manager->search_remap_grid_with_grid_name(data_grid_name_src);
    data_grid_dst = remap_grid_manager->search_remap_grid_with_grid_name(data_grid_name_dst);

    EXECUTION_REPORT(REPORT_ERROR, data_grid_src->get_num_dimensions() == data_grid_dst->get_num_dimensions(), 
                 "grid %s and %s must have the same number of dimensions\n", data_grid_name_src, data_grid_name_dst);

    if (!read_weights)
        remap_strategy->execute_remap_strategy(NULL, NULL, this);
}


bool Remap_weight_of_strategy_class::match_object_name(const char*object_name)
{
    return words_are_the_same(this->object_name, object_name);
}


Remap_weight_of_strategy_class::~Remap_weight_of_strategy_class()
{
    for (int i = 0; i < remap_weights_of_operators.size(); i ++)
        delete remap_weights_of_operators[i];
}


void Remap_weight_of_strategy_class::add_remap_weight_of_operator(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst,
                                                                  long remap_beg_iter, Remap_operator_basis *remap_operator)
{
    remap_weights_of_operators.push_back(new Remap_weight_of_operator_class(field_data_grid_src, field_data_grid_dst, remap_beg_iter, remap_operator));
}


void Remap_weight_of_strategy_class::do_remap(Remap_grid_data_class *field_data_src, Remap_grid_data_class *field_data_dst)
{
    Remap_grid_class *interchange_grid_src, *sized_sub_grids[256];
    Remap_grid_class *field_data_grid_src, *field_data_grid_dst;
    Remap_grid_data_class *tmp_field_data_src, *tmp_field_data_dst;
    Remap_operator_basis *current_remap_operator;
    double *data_value_src, *data_value_dst;
    bool is_last_remap_operator;
    int i, j, k, num_sized_sub_grids;
    long remap_beg_iter, remap_end_iter;
    long index_size_iter, field_array_offset, index_size_array[256], current_runtime_index_array[256];

    
    EXECUTION_REPORT(REPORT_ERROR, field_data_src->get_coord_value_grid()->is_similar_grid_with(data_grid_src),
                 "the grid of field data \"%s\" can not match the src grid of remap weight object \"%s\"",
                 field_data_src->get_grid_data_field()->field_name_in_application, object_name);
    EXECUTION_REPORT(REPORT_ERROR, field_data_dst->get_coord_value_grid()->is_similar_grid_with(data_grid_dst),
                 "the grid of field data \"%s\" can not match the dst grid of remap weight object \"%s\"",
                 field_data_dst->get_grid_data_field()->field_name_in_application, object_name);

    field_data_src->transfer_field_attributes_to_another(field_data_dst);
    if (!field_data_dst->have_data_content())
        field_data_dst->get_grid_data_field()->initialize_to_fill_value();

    current_remap_operator = NULL;
    interchange_grid_src = NULL;
    tmp_field_data_src = NULL;
    tmp_field_data_dst = field_data_src;
    for (i = 0; i < remap_weights_of_operators.size(); i ++) {
        if (current_remap_operator != remap_weights_of_operators[i]->original_remap_operator) {
            if (tmp_field_data_src != NULL && tmp_field_data_src != field_data_src)
                delete tmp_field_data_src;
            tmp_field_data_src = tmp_field_data_dst;
            for (j = i+1; j < remap_weights_of_operators.size(); j ++)
                if (remap_weights_of_operators[i]->original_remap_operator != remap_weights_of_operators[j]->original_remap_operator)
                    break;
            is_last_remap_operator = j == remap_weights_of_operators.size();
            if (is_last_remap_operator) { 
                tmp_field_data_dst = field_data_dst;
                EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operators[i]->field_data_grid_dst->is_similar_grid_with(tmp_field_data_dst->get_coord_value_grid()), 
                             "remap software error1 in do_remap of Remap_weight_of_strategy_class\n");
                tmp_field_data_dst->interchange_grid_data(remap_weights_of_operators[i]->field_data_grid_dst);
            }
            else tmp_field_data_dst = field_data_src->duplicate_grid_data_field(remap_weights_of_operators[i]->field_data_grid_dst, 1, false, false);
            remap_weights_of_operators[i]->get_field_data_grid_src()->get_sized_sub_grids(&num_sized_sub_grids, sized_sub_grids);
            for (j = 0; j < num_sized_sub_grids; j ++)
                if (!sized_sub_grids[j]->is_subset_of_grid(remap_weights_of_operators[i]->operator_grid_src))
                    break;
            for (k = 0; j < num_sized_sub_grids; j ++) 
                index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
            num_sized_sub_grids = k;
            current_remap_operator = remap_weights_of_operators[i]->original_remap_operator;
			tmp_field_data_src->interchange_grid_data(remap_weights_of_operators[i]->field_data_grid_src);
        }
        EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operators[i]->field_data_grid_src->is_similar_grid_with(tmp_field_data_src->get_coord_value_grid()),
                     "remap software error2 in do_remap of Remap_weight_of_strategy_class\n");

        remap_beg_iter = remap_weights_of_operators[i]->remap_beg_iter;
        if (remap_weights_of_operators[i]->remap_end_iter != -1)
            remap_end_iter = remap_weights_of_operators[i]->remap_end_iter;
        else {
            if (i+1 < remap_weights_of_operators.size() && current_remap_operator == remap_weights_of_operators[i+1]->original_remap_operator)
                remap_end_iter = remap_weights_of_operators[i+1]->remap_beg_iter;
            else remap_end_iter = tmp_field_data_src->get_coord_value_grid()->get_grid_size()/remap_weights_of_operators[i]->operator_grid_src->get_grid_size();
        }
        for (j = remap_beg_iter; j < remap_end_iter; j ++) {
            for (k = num_sized_sub_grids - 1, index_size_iter = 1; k >= 0; k --) {
                current_runtime_index_array[k] = (j/index_size_iter) % index_size_array[k];
                index_size_iter *= index_size_array[k];
            }
            for (field_array_offset = 0, index_size_iter = 1, k = 0; k < num_sized_sub_grids; k ++) {
                field_array_offset += current_runtime_index_array[k]*index_size_iter;
                index_size_iter *= index_size_array[k];
            }
            data_value_src = ((double*) tmp_field_data_src->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operators[i]->operator_grid_src->get_grid_size();
            data_value_dst = ((double*) tmp_field_data_dst->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operators[i]->operator_grid_dst->get_grid_size();
            remap_weights_of_operators[i]->duplicated_remap_operator->do_remap_values_caculation(data_value_src, data_value_dst);
        }
    }
    
    field_data_dst->interchange_grid_data(field_data_dst->get_coord_value_grid());
	field_data_dst->get_grid_data_field()->read_data_size = field_data_dst->get_grid_data_field()->required_data_size;
}


void Remap_weight_of_strategy_class::calculate_src_decomp_recursively(int iter, Remap_operator_basis *current_remap_operator, 
                                                                    Remap_grid_data_class *tmp_field_data_src, Remap_grid_data_class *tmp_field_data_dst,
                                                                    Remap_grid_data_class *field_data_src, Remap_grid_data_class *field_data_dst)
{
    int j, k, num_sized_sub_grids, index_size_array[4], current_runtime_index_array[4];
    bool change_src_dst = false;
    Remap_grid_class *sized_sub_grids[4];
    long remap_beg_iter, remap_end_iter, index_size_iter, field_array_offset;
    bool *decomp_map_values_src, *decomp_map_values_dst;


    if (current_remap_operator != remap_weights_of_operators[iter]->original_remap_operator) {
        change_src_dst = true;
        tmp_field_data_src = tmp_field_data_dst;
        for (j = iter+1; j < remap_weights_of_operators.size(); j ++)
            if (remap_weights_of_operators[iter]->original_remap_operator != remap_weights_of_operators[j]->original_remap_operator)
                break;
        if (j == remap_weights_of_operators.size()) 
            tmp_field_data_dst = field_data_dst;
        else tmp_field_data_dst = field_data_src->duplicate_grid_data_field(remap_weights_of_operators[iter]->field_data_grid_dst, 1, false, false);
        current_remap_operator = remap_weights_of_operators[iter]->original_remap_operator;
    }

    if (iter+1 < remap_weights_of_operators.size())
        calculate_src_decomp_recursively(iter+1, current_remap_operator, tmp_field_data_src, tmp_field_data_dst, field_data_src, field_data_dst);

	tmp_field_data_src->interchange_grid_data(remap_weights_of_operators[iter]->field_data_grid_src);
	tmp_field_data_dst->interchange_grid_data(remap_weights_of_operators[iter]->field_data_grid_dst);

	remap_weights_of_operators[iter]->get_field_data_grid_src()->get_sized_sub_grids(&num_sized_sub_grids, sized_sub_grids);
	for (j = 0; j < num_sized_sub_grids; j ++)
		if (!sized_sub_grids[j]->is_subset_of_grid(remap_weights_of_operators[iter]->operator_grid_src))
			break;
	for (k = 0; j < num_sized_sub_grids; j ++) 
		index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
	num_sized_sub_grids = k;

    remap_beg_iter = remap_weights_of_operators[iter]->remap_beg_iter;
    if (remap_weights_of_operators[iter]->remap_end_iter != -1)
        remap_end_iter = remap_weights_of_operators[iter]->remap_end_iter;
    else if (iter+1 < remap_weights_of_operators.size() && current_remap_operator == remap_weights_of_operators[iter+1]->original_remap_operator)
        remap_end_iter = remap_weights_of_operators[iter+1]->remap_beg_iter;
    else remap_end_iter = tmp_field_data_src->get_coord_value_grid()->get_grid_size()/remap_weights_of_operators[iter]->operator_grid_src->get_grid_size();
    for (j = remap_beg_iter; j < remap_end_iter; j ++) {
        for (k = num_sized_sub_grids - 1, index_size_iter = 1; k >= 0; k --) {
            current_runtime_index_array[k] = (j/index_size_iter) % index_size_array[k];
            index_size_iter *= index_size_array[k];
        }
        for (field_array_offset = 0, index_size_iter = 1, k = 0; k < num_sized_sub_grids; k ++) {
            field_array_offset += current_runtime_index_array[k]*index_size_iter;
            index_size_iter *= index_size_array[k];
        }
        decomp_map_values_src = ((bool*) tmp_field_data_src->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operators[iter]->operator_grid_src->get_grid_size();
        decomp_map_values_dst = ((bool*) tmp_field_data_dst->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operators[iter]->operator_grid_dst->get_grid_size();
        remap_weights_of_operators[iter]->duplicated_remap_operator->do_src_decomp_caculation(decomp_map_values_src, decomp_map_values_dst);
    }

    if (change_src_dst && tmp_field_data_dst != field_data_dst)
        delete tmp_field_data_dst;
}


void Remap_weight_of_strategy_class::calculate_src_decomp(Remap_grid_class *grid_src, Remap_grid_class *grid_dst, bool *decomp_map_src, const bool *decomp_map_dst)
{
    Remap_grid_data_class *decomp_map_field_src, *decomp_map_field_dst;
    Remap_grid_data_class *expanded_decomp_map_field_src, *expanded_decomp_map_field_dst;
    long i, j;
    bool *tmp_decomp_map_src;


    for (i = 0; i < grid_src->get_grid_size(); i ++)
        decomp_map_src[i] = false;
 	
    EXECUTION_REPORT(REPORT_ERROR, grid_src->get_grid_mask_field() != NULL && grid_dst->get_grid_mask_field() != NULL, "C-Coupler error in calculate_src_decomp\n");
    decomp_map_field_src = grid_src->get_grid_mask_field()->duplicate_grid_data_field(grid_src, 1, false, true);
    decomp_map_field_dst = grid_dst->get_grid_mask_field()->duplicate_grid_data_field(grid_dst, 1, false, true);
    memcpy(decomp_map_field_src->get_grid_data_field()->data_buf, decomp_map_src, grid_src->get_grid_size()*sizeof(bool));
    memcpy(decomp_map_field_dst->get_grid_data_field()->data_buf, decomp_map_dst, grid_dst->get_grid_size()*sizeof(bool));
    expanded_decomp_map_field_src = data_grid_src->expand_to_generate_full_coord_value(decomp_map_field_src);
    expanded_decomp_map_field_dst = data_grid_dst->expand_to_generate_full_coord_value(decomp_map_field_dst);

	EXECUTION_REPORT(REPORT_LOG, true, "before calculate_src_decomp_recursively");
    calculate_src_decomp_recursively(0, NULL, NULL, expanded_decomp_map_field_src, expanded_decomp_map_field_src, expanded_decomp_map_field_dst);
	EXECUTION_REPORT(REPORT_LOG, true, "after calculate_src_decomp_recursively");

    expanded_decomp_map_field_src->interchange_grid_data(grid_src);
    for (i = 0; i < data_grid_src->get_grid_size()/grid_src->get_grid_size(); i ++) {
        tmp_decomp_map_src = ((bool*) expanded_decomp_map_field_src->get_grid_data_field()->data_buf) + i*grid_src->get_grid_size();
        for (j = 0; j < grid_src->get_grid_size(); j ++)
            if (tmp_decomp_map_src[j])
                decomp_map_src[j] = true;
    }

    delete decomp_map_field_src;
    delete decomp_map_field_dst;
    delete expanded_decomp_map_field_src;
    delete expanded_decomp_map_field_dst;
}


Remap_grid_class **Remap_weight_of_strategy_class::get_remap_related_grids(int &num_field_data_grids)
{
    Remap_operator_basis *current_remap_operator;
    Remap_grid_class **all_remap_related_grids, **temp_grids;
    int array_size = 100;

    num_field_data_grids = 0;
    current_remap_operator = NULL;
    all_remap_related_grids = new Remap_grid_class *[array_size];

    all_remap_related_grids[num_field_data_grids++] = data_grid_src;
    all_remap_related_grids[num_field_data_grids++] = data_grid_dst;
    for (int i = 0; i < remap_weights_of_operators.size(); i ++) {
        if (num_field_data_grids + 4 > array_size) {
            temp_grids = all_remap_related_grids;
            all_remap_related_grids = new Remap_grid_class *[array_size*2];
            for (int j = 0; j < array_size; j ++)
                all_remap_related_grids[j] = temp_grids[j];
            delete [] temp_grids;
            array_size *= 2;
        }
        all_remap_related_grids[num_field_data_grids++] = remap_weights_of_operators[i]->field_data_grid_src;
        all_remap_related_grids[num_field_data_grids++] = remap_weights_of_operators[i]->field_data_grid_dst;
        all_remap_related_grids[num_field_data_grids++] = remap_weights_of_operators[i]->operator_grid_src;
        all_remap_related_grids[num_field_data_grids++] = remap_weights_of_operators[i]->operator_grid_dst;
    }

    return all_remap_related_grids;
}


Remap_weight_of_strategy_class *Remap_weight_of_strategy_class::generate_parallel_remap_weights(Remap_grid_class **remap_related_decomp_grids, 
                                                                                                 Remap_grid_class **decomp_original_grids, 
                                                                                                 Remap_grid_class **decomp_grids, 
                                                                                                 int **global_cells_local_indexes_in_decomps)
{
    int i, j, k, field_data_grids_iter, num_sized_sub_grids, index_size_array[256], current_runtime_index_array[256];
    Remap_operator_basis *current_remap_operator;
    Remap_weight_of_strategy_class *parallel_remap_weights_of_strategy = new Remap_weight_of_strategy_class;
    Remap_weight_of_operator_class *parallel_remap_weights_of_operator;
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

    current_remap_operator = NULL;
    for (i = 0; i < this->remap_weights_of_operators.size(); i ++) {
        if (current_remap_operator != remap_weights_of_operators[i]->original_remap_operator) {
            for (j = i+1; j < remap_weights_of_operators.size(); j ++)
                if (remap_weights_of_operators[i]->original_remap_operator != remap_weights_of_operators[j]->original_remap_operator)
                    break;
            remap_weights_of_operators[i]->get_field_data_grid_src()->get_sized_sub_grids(&num_sized_sub_grids, sized_sub_grids);
            for (j = 0; j < num_sized_sub_grids; j ++)
                if (!sized_sub_grids[j]->is_subset_of_grid(remap_weights_of_operators[i]->operator_grid_src))
                    break;
            for (k = 0; j < num_sized_sub_grids; j ++) 
                index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
            num_sized_sub_grids = k;
            current_remap_operator = remap_weights_of_operators[i]->original_remap_operator;
        }
        remap_beg_iter = remap_weights_of_operators[i]->remap_beg_iter;
        if (remap_weights_of_operators[i]->remap_end_iter != -1)
            remap_end_iter = remap_weights_of_operators[i]->remap_end_iter;
        else {
            if (i+1 < remap_weights_of_operators.size() && current_remap_operator == remap_weights_of_operators[i+1]->original_remap_operator)
                remap_end_iter = remap_weights_of_operators[i+1]->remap_beg_iter;
            else remap_end_iter = remap_weights_of_operators[i]->get_field_data_grid_src()->get_grid_size()/remap_weights_of_operators[i]->operator_grid_src->get_grid_size();
        }

        if (remap_weights_of_operators[i]->operator_grid_src->get_is_sphere_grid() || remap_end_iter-remap_beg_iter == remap_weights_of_operators[i]->get_field_data_grid_src()->get_grid_size()/remap_weights_of_operators[i]->operator_grid_src->get_grid_size()){
            parallel_remap_weights_of_operator = remap_weights_of_operators[i]->generate_parallel_remap_weights(decomp_original_grids, global_cells_local_indexes_in_decomps);
            parallel_remap_weights_of_operator->field_data_grid_src = remap_related_decomp_grids[field_data_grids_iter+0];
            parallel_remap_weights_of_operator->field_data_grid_dst = remap_related_decomp_grids[field_data_grids_iter+1];
            parallel_remap_weights_of_operator->operator_grid_src = remap_related_decomp_grids[field_data_grids_iter+2];
            parallel_remap_weights_of_operator->operator_grid_dst = remap_related_decomp_grids[field_data_grids_iter+3];
            parallel_remap_weights_of_operator->remap_beg_iter = remap_weights_of_operators[i]->remap_beg_iter; 
            parallel_remap_weights_of_operator->remap_end_iter = -1;
            parallel_remap_weights_of_strategy->remap_weights_of_operators.push_back(parallel_remap_weights_of_operator);
            EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operators[i]->remap_beg_iter == 0, "C-Coupler error3 in generate_parallel_remap_weights of Remap_weight_of_strategy_class\n");
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
				if (remap_weights_of_operators[i]->field_data_grid_src->have_overlap_with_grid(decomp_original_grids[0]))
					local_field_array_offset = global_cells_local_indexes_in_decomps[0][global_field_array_offset];
				else if (remap_weights_of_operators[i]->field_data_grid_src->have_overlap_with_grid(decomp_original_grids[1]))
					local_field_array_offset = global_cells_local_indexes_in_decomps[1][global_field_array_offset];
                else EXECUTION_REPORT(REPORT_ERROR, false, "C-Coupler error4 in generate_parallel_remap_weights of Remap_weight_of_strategy_class\n");
				if (local_field_array_offset == -1)
					continue;
                parallel_remap_weights_of_operator = remap_weights_of_operators[i]->generate_parallel_remap_weights(decomp_original_grids, global_cells_local_indexes_in_decomps);
                parallel_remap_weights_of_operator->field_data_grid_src = remap_related_decomp_grids[field_data_grids_iter+0];
                parallel_remap_weights_of_operator->field_data_grid_dst = remap_related_decomp_grids[field_data_grids_iter+1];
                parallel_remap_weights_of_operator->operator_grid_src = remap_related_decomp_grids[field_data_grids_iter+2];
                parallel_remap_weights_of_operator->operator_grid_dst = remap_related_decomp_grids[field_data_grids_iter+3];
                parallel_remap_weights_of_operator->remap_beg_iter = local_field_array_offset; 
                parallel_remap_weights_of_operator->remap_end_iter = local_field_array_offset+1;
                parallel_remap_weights_of_strategy->remap_weights_of_operators.push_back(parallel_remap_weights_of_operator);
            }
        }

        field_data_grids_iter += 4;
    }

    return parallel_remap_weights_of_strategy;
}

