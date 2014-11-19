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
    this->remap_end_iter = -1;
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
    this->remap_end_iter = -1;
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


Remap_weight_of_operator_class::Remap_weight_of_operator_class(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst, Remap_operator_basis *remap_operator)
{
    this->field_data_grid_src = field_data_grid_src;
    this->field_data_grid_dst = field_data_grid_dst;
    this->original_remap_operator = remap_operator;
    this->operator_grid_src = remap_operator->get_src_grid();
    this->operator_grid_dst = remap_operator->get_dst_grid();
}


Remap_weight_of_operator_class *Remap_weight_of_operator_class::generate_parallel_remap_weights(Remap_grid_class **decomp_original_grids, int **global_cells_local_indexes_in_decomps)
{
	for (int i = remap_weights_of_operator_instances.size()-1; i >= 0; i --)
		remap_weights_of_operator_instances[i]->generate_parallel_remap_weights(decomp_original_grids, global_cells_local_indexes_in_decomps);
}


Remap_weight_of_operator_class::~Remap_weight_of_operator_class()
{
	for (int i = 0; i < remap_weights_of_operator_instances.size(); i ++)
		delete remap_weights_of_operator_instances[i];
}


void Remap_weight_of_operator_class::do_remap(Remap_grid_data_class *field_data_src, Remap_grid_data_class *field_data_dst)
{
    Remap_grid_class *sized_sub_grids[256];
    double *data_value_src, *data_value_dst;
    int i, j, k, num_sized_sub_grids;
    long remap_beg_iter, remap_end_iter;
    long index_size_iter, field_array_offset, index_size_array[256], current_runtime_index_array[256];

    
    EXECUTION_REPORT(REPORT_ERROR, field_data_src->get_coord_value_grid()->is_similar_grid_with(field_data_grid_src), "C-Coupler error1 in do_remap of Remap_weight_of_operator_class");
    EXECUTION_REPORT(REPORT_ERROR, field_data_dst->get_coord_value_grid()->is_similar_grid_with(field_data_grid_dst), "C-Coupler error2 in do_remap of Remap_weight_of_operator_class");
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
        EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->field_data_grid_src == this->field_data_grid_src && remap_weights_of_operator_instances[i]->field_data_grid_dst == this->field_data_grid_dst,
                     	 "remap software error3 in do_remap of Remap_weight_of_strategy_class\n");

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
            data_value_src = ((double*) field_data_src->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[i]->operator_grid_src->get_grid_size();
            data_value_dst = ((double*) field_data_dst->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[i]->operator_grid_dst->get_grid_size();
            remap_weights_of_operator_instances[i]->duplicated_remap_operator->do_remap_values_caculation(data_value_src, data_value_dst);
        }
    }
}


void Remap_weight_of_operator_class::add_remap_weight_of_operator_instance(Remap_weight_of_operator_instance_class *operator_instance)
{
	remap_weights_of_operator_instances.push_back(operator_instance);
}


Remap_weight_of_strategy_class::Remap_weight_of_strategy_class(const char *object_name, const char *remap_strategy_name, 
                                                               const char *data_grid_name_src, const char *data_grid_name_dst,
                                                               const char *input_IO_file_name, const char *weight_IO_format,
                                                               bool read_from_io)
{
    strcpy(this->object_name, object_name);
    remap_strategy = remap_strategy_manager->search_remap_strategy(remap_strategy_name);
    data_grid_src = remap_grid_manager->search_remap_grid_with_grid_name(data_grid_name_src);
    data_grid_dst = remap_grid_manager->search_remap_grid_with_grid_name(data_grid_name_dst);
	
	if (!read_from_io)
		remap_strategy->execute_remap_strategy(NULL, NULL, this);
	else {
		if (words_are_the_same(weight_IO_format, "SCRIP")) 
			((IO_netcdf*) (io_manager->search_IO_object(input_IO_file_name)))->read_remap_weights(this, remap_strategy, is_master_process_in_computing_node);
		else ((IO_binary*) (io_manager->search_IO_object(input_IO_file_name)))->read_remap_weights(this, remap_strategy, is_master_process_in_computing_node);
	}

    EXECUTION_REPORT(REPORT_ERROR, data_grid_src->get_num_dimensions() == data_grid_dst->get_num_dimensions(), 
    	             "grid %s and %s must have the same number of dimensions\n", data_grid_name_src, data_grid_name_dst);
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
    for (int i = 0; i < remap_weights_of_operator_instances.size(); i ++)
        delete remap_weights_of_operator_instances[i];
}


void Remap_weight_of_strategy_class::add_remap_weight_of_operator_instance(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst,
                                                                  long remap_beg_iter, Remap_operator_basis *remap_operator)
{
	Remap_weight_of_operator_instance_class *remap_weight_of_operator_instance = new Remap_weight_of_operator_instance_class(field_data_grid_src, field_data_grid_dst, remap_beg_iter, remap_operator);
    add_remap_weight_of_operator_instance(remap_weight_of_operator_instance);
}


void Remap_weight_of_strategy_class::add_remap_weight_of_operator_instance(Remap_weight_of_operator_instance_class *weight_of_operator_instance) 
{ 
	if (remap_weights_of_operators.size() == 0 || remap_weights_of_operators[remap_weights_of_operators.size()-1]->field_data_grid_src != weight_of_operator_instance->field_data_grid_src) {
		Remap_weight_of_operator_class *remap_weight_of_operator = new Remap_weight_of_operator_class(weight_of_operator_instance->field_data_grid_src, weight_of_operator_instance->field_data_grid_dst, weight_of_operator_instance->original_remap_operator);
		remap_weights_of_operators.push_back(remap_weight_of_operator);
		printf("okokok %d\n", remap_weights_of_operators.size());
	}
	remap_weights_of_operators[remap_weights_of_operators.size()-1]->add_remap_weight_of_operator_instance(weight_of_operator_instance);
	remap_weights_of_operator_instances.push_back(weight_of_operator_instance); 
}


void Remap_weight_of_strategy_class::update_vertical_remap_weights_of_dynamic_sigma_grid(Remap_grid_data_class *field_data_src, Remap_grid_data_class *field_data_dst)
{
	Remap_grid_class *operator_field_data_grids[256], *sized_grids_src[256], *sized_grids_dst[256];
	int i, j, num_operator_field_data_grids, num_sized_grids_src, num_sized_grids_dst;
	Remap_operator_basis *last_remap_operator = NULL;

	
	if (field_data_src->get_coord_value_grid()->get_num_dimensions() != 3 || !field_data_src->get_coord_value_grid()->has_grid_coord_label(COORD_LABEL_LAT) || 
		!field_data_src->get_coord_value_grid()->has_grid_coord_label(COORD_LABEL_LON) || !field_data_src->get_coord_value_grid()->has_grid_coord_label(COORD_LABEL_LEV))
		return;

	if ((!field_data_src->get_coord_value_grid()->is_sigma_grid()) && (!data_grid_dst->is_sigma_grid())) 
		return;

	if (field_data_src->get_coord_value_grid()->has_specified_sigma_grid_surface_value_field())
		EXECUTION_REPORT(REPORT_ERROR, field_data_src->get_coord_value_grid()->is_sigma_grid(), "C-Coupler error1 in update_vertical_remap_weights_of_dynamic_sigma_grid");
	
	if (field_data_dst->get_coord_value_grid()->has_specified_sigma_grid_surface_value_field())
		EXECUTION_REPORT(REPORT_ERROR, field_data_dst->get_coord_value_grid()->is_sigma_grid(), "C-Coupler error2 in update_vertical_remap_weights_of_dynamic_sigma_grid");

	EXECUTION_REPORT(REPORT_ERROR, (field_data_src->get_coord_value_grid()->has_specified_sigma_grid_surface_value_field()) != (field_data_dst->get_coord_value_grid()->has_specified_sigma_grid_surface_value_field()), 
					 "The surface value fields (for 3D sigma grid) in source and target grids of remapping weights %s are or not both specified. Only one surface value field can be specified by users.", object_name);

	if ((field_data_src->get_coord_value_grid()->has_specified_sigma_grid_surface_value_field() && !field_data_src->get_coord_value_grid()->is_sigma_grid_surface_value_field_updated()) ||
		(field_data_dst->get_coord_value_grid()->has_specified_sigma_grid_surface_value_field() && !field_data_dst->get_coord_value_grid()->is_sigma_grid_surface_value_field_updated()))
		return;

	EXECUTION_REPORT(REPORT_LOG, true, "Need to update vertical remap weights %s due to dynamic sigma grid", object_name);

	if (field_data_src->get_coord_value_grid()->has_specified_sigma_grid_surface_value_field()) {
		operator_field_data_grids[0] = field_data_src->get_coord_value_grid();
		num_operator_field_data_grids = 1;
		for (i = 0; i < remap_weights_of_operator_instances.size(); i ++)
			if (last_remap_operator != remap_weights_of_operator_instances[i]->original_remap_operator) {
				last_remap_operator = remap_weights_of_operator_instances[i]->original_remap_operator;
				operator_field_data_grids[num_operator_field_data_grids++] = remap_weights_of_operator_instances[i]->get_field_data_grid_src();
				operator_field_data_grids[num_operator_field_data_grids++] = remap_weights_of_operator_instances[i]->get_field_data_grid_dst();
			}
	}
	else {
		operator_field_data_grids[0] = field_data_dst->get_coord_value_grid();
		num_operator_field_data_grids = 1;
		for (i = remap_weights_of_operator_instances.size()-1; i >= 0 ; i --)
			if (last_remap_operator != remap_weights_of_operator_instances[i]->original_remap_operator) {
				last_remap_operator = remap_weights_of_operator_instances[i]->original_remap_operator;
				operator_field_data_grids[num_operator_field_data_grids++] = remap_weights_of_operator_instances[i]->get_field_data_grid_dst();
				operator_field_data_grids[num_operator_field_data_grids++] = remap_weights_of_operator_instances[i]->get_field_data_grid_src();
			}

	}

	EXECUTION_REPORT(REPORT_LOG, true, "Find %d grids needing to be checked for updating vertical remap weights of dynamic sigma grid", num_operator_field_data_grids);

	for (i = 1; i < num_operator_field_data_grids; i ++) {
		operator_field_data_grids[i-1]->get_sized_sub_grids(&num_sized_grids_src, sized_grids_src);
		operator_field_data_grids[i]->get_sized_sub_grids(&num_sized_grids_dst, sized_grids_dst);
		EXECUTION_REPORT(REPORT_ERROR, num_sized_grids_src == num_sized_grids_dst, "C-Coupler error4 in update_vertical_remap_weights_of_dynamic_sigma_grid");
		for (j = 0; j < num_sized_grids_src; j ++) {
			if (sized_grids_dst[j]->has_grid_coord_label(COORD_LABEL_LEV)) {
				EXECUTION_REPORT(REPORT_ERROR, sized_grids_dst[j]->get_num_dimensions()== 1, "C-Coupler error5 in update_vertical_remap_weights_of_dynamic_sigma_grid");
				if (sized_grids_dst[j]->get_super_grid_of_setting_coord_values() != NULL && sized_grids_dst[j]->get_super_grid_of_setting_coord_values()->is_sigma_grid())
					operator_field_data_grids[i]->allocate_sigma_grid_specific_fields(NULL, NULL, 0, 0);
			}
		}
		if (operator_field_data_grids[i]->is_sigma_grid()) {
			if (operator_field_data_grids[i] == operator_field_data_grids[i-1])
				continue;
			EXECUTION_REPORT(REPORT_ERROR, !operator_field_data_grids[i]->has_specified_sigma_grid_surface_value_field(), "C-Coupler error6 in update_vertical_remap_weights_of_dynamic_sigma_grid");
			EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i-1]->is_sigma_grid(), "C-Coupler error7 in update_vertical_remap_weights_of_dynamic_sigma_grid");
			EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field() != NULL, "C-Coupler error8 in update_vertical_remap_weights_of_dynamic_sigma_grid");
			if (operator_field_data_grids[i]->is_similar_grid_with(operator_field_data_grids[i-1]))
				operator_field_data_grids[i]->copy_sigma_grid_surface_value_field(operator_field_data_grids[i-1]);
			else {
				for (j = 0; j < num_sized_grids_src; j ++)
					if (!sized_grids_dst[j]->has_grid_coord_label(COORD_LABEL_LEV)) {
						EXECUTION_REPORT(REPORT_ERROR, sized_grids_dst[j]->get_num_dimensions() == 2 && sized_grids_dst[j]->has_grid_coord_label(COORD_LABEL_LON) && sized_grids_dst[j]->has_grid_coord_label(COORD_LABEL_LAT), 
										 "C-Coupler error9 in update_vertical_remap_weights_of_dynamic_sigma_grid");
						if (sized_grids_src[j]->is_similar_grid_with(sized_grids_dst[j])) {
							EXECUTION_REPORT(REPORT_ERROR, sized_grids_src[j] == sized_grids_dst[j], "C-Coupler error10 in update_vertical_remap_weights_of_dynamic_sigma_grid");
							operator_field_data_grids[i]->copy_sigma_grid_surface_value_field(operator_field_data_grids[i-1]);
						}
						else {
							printf("remap to generate surface field values\n");	
						}
					}
			}
			operator_field_data_grids[i]->calculate_lev_sigma_values();
		}
	}
	operator_field_data_grids[0]->calculate_lev_sigma_values();
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
	Remap_grid_class *runtime_lev_grid_src = NULL, *runtime_lev_grid_dst = NULL, *original_lev_grid_src = NULL, *original_lev_grid_dst = NULL;

    
    EXECUTION_REPORT(REPORT_ERROR, field_data_src->get_coord_value_grid()->is_similar_grid_with(data_grid_src),
                 "the grid of field data \"%s\" can not match the src grid of remap weight object \"%s\"",
                 field_data_src->get_grid_data_field()->field_name_in_application, object_name);
    EXECUTION_REPORT(REPORT_ERROR, field_data_dst->get_coord_value_grid()->is_similar_grid_with(data_grid_dst),
                 "the grid of field data \"%s\" can not match the dst grid of remap weight object \"%s\"",
                 field_data_dst->get_grid_data_field()->field_name_in_application, object_name);

	update_vertical_remap_weights_of_dynamic_sigma_grid(field_data_src, field_data_dst);

    field_data_src->transfer_field_attributes_to_another(field_data_dst);
    if (!field_data_dst->have_data_content())
        field_data_dst->get_grid_data_field()->initialize_to_fill_value();

    current_remap_operator = NULL;
    tmp_field_data_src = NULL;
    tmp_field_data_dst = field_data_src;
    for (i = 0; i < remap_weights_of_operator_instances.size(); i ++) {
        if (current_remap_operator != remap_weights_of_operator_instances[i]->original_remap_operator) {
            if (tmp_field_data_src != NULL && tmp_field_data_src != field_data_src)
                delete tmp_field_data_src;
            tmp_field_data_src = tmp_field_data_dst;
            for (j = i+1; j < remap_weights_of_operator_instances.size(); j ++)
                if (remap_weights_of_operator_instances[i]->original_remap_operator != remap_weights_of_operator_instances[j]->original_remap_operator)
                    break;
            is_last_remap_operator = j == remap_weights_of_operator_instances.size();
            if (is_last_remap_operator) { 
                tmp_field_data_dst = field_data_dst;
                EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->field_data_grid_dst->is_similar_grid_with(tmp_field_data_dst->get_coord_value_grid()), 
                             "remap software error1 in do_remap of Remap_weight_of_strategy_class\n");
                tmp_field_data_dst->interchange_grid_data(remap_weights_of_operator_instances[i]->field_data_grid_dst);
            }
            else tmp_field_data_dst = field_data_src->duplicate_grid_data_field(remap_weights_of_operator_instances[i]->field_data_grid_dst, 1, false, false);
            remap_weights_of_operator_instances[i]->get_field_data_grid_src()->get_sized_sub_grids(&num_sized_sub_grids, sized_sub_grids);
            for (j = 0; j < num_sized_sub_grids; j ++)
                if (!sized_sub_grids[j]->is_subset_of_grid(remap_weights_of_operator_instances[i]->operator_grid_src))
                    break;
            for (k = 0; j < num_sized_sub_grids; j ++) 
                index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
            num_sized_sub_grids = k;
            current_remap_operator = remap_weights_of_operator_instances[i]->original_remap_operator;
			tmp_field_data_src->interchange_grid_data(remap_weights_of_operator_instances[i]->field_data_grid_src);
        }
        EXECUTION_REPORT(REPORT_ERROR, remap_weights_of_operator_instances[i]->field_data_grid_src->is_similar_grid_with(tmp_field_data_src->get_coord_value_grid()),
                     "remap software error2 in do_remap of Remap_weight_of_strategy_class\n");

        remap_beg_iter = remap_weights_of_operator_instances[i]->remap_beg_iter;
        if (remap_weights_of_operator_instances[i]->remap_end_iter != -1)
            remap_end_iter = remap_weights_of_operator_instances[i]->remap_end_iter;
        else {
            if (i+1 < remap_weights_of_operator_instances.size() && current_remap_operator == remap_weights_of_operator_instances[i+1]->original_remap_operator)
                remap_end_iter = remap_weights_of_operator_instances[i+1]->remap_beg_iter;
            else remap_end_iter = tmp_field_data_src->get_coord_value_grid()->get_grid_size()/remap_weights_of_operator_instances[i]->operator_grid_src->get_grid_size();
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
            data_value_src = ((double*) tmp_field_data_src->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[i]->operator_grid_src->get_grid_size();
            data_value_dst = ((double*) tmp_field_data_dst->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[i]->operator_grid_dst->get_grid_size();
            remap_weights_of_operator_instances[i]->duplicated_remap_operator->do_remap_values_caculation(data_value_src, data_value_dst);
        }
    }
    
    field_data_dst->interchange_grid_data(field_data_dst->get_coord_value_grid());
	field_data_dst->get_grid_data_field()->read_data_size = field_data_dst->get_grid_data_field()->required_data_size;

	if (runtime_lev_grid_src != NULL) {
		delete runtime_lev_grid_src;
		delete runtime_lev_grid_dst;
	}
}


void Remap_weight_of_strategy_class::calculate_src_decomp_recursively(int iter, Remap_operator_basis *current_remap_operator, 
                                                                    Remap_grid_data_class *tmp_field_data_src, Remap_grid_data_class *tmp_field_data_dst,
                                                                    Remap_grid_data_class *field_data_src, Remap_grid_data_class *field_data_dst)
{
    int j, k, num_sized_sub_grids, index_size_array[4], current_runtime_index_array[4];
    bool change_src_dst = false;
    Remap_grid_class *sized_sub_grids[4];
    long remap_beg_iter, remap_end_iter, index_size_iter, field_array_offset;
    long *decomp_map_values_src, *decomp_map_values_dst;


    if (current_remap_operator != remap_weights_of_operator_instances[iter]->original_remap_operator) {
        change_src_dst = true;
        tmp_field_data_src = tmp_field_data_dst;
        for (j = iter+1; j < remap_weights_of_operator_instances.size(); j ++)
            if (remap_weights_of_operator_instances[iter]->original_remap_operator != remap_weights_of_operator_instances[j]->original_remap_operator)
                break;
        if (j == remap_weights_of_operator_instances.size()) 
            tmp_field_data_dst = field_data_dst;
        else tmp_field_data_dst = field_data_src->duplicate_grid_data_field(remap_weights_of_operator_instances[iter]->field_data_grid_dst, 1, false, false);
        current_remap_operator = remap_weights_of_operator_instances[iter]->original_remap_operator;
    }

    if (iter+1 < remap_weights_of_operator_instances.size())
        calculate_src_decomp_recursively(iter+1, current_remap_operator, tmp_field_data_src, tmp_field_data_dst, field_data_src, field_data_dst);

	tmp_field_data_src->interchange_grid_data(remap_weights_of_operator_instances[iter]->field_data_grid_src);
	tmp_field_data_dst->interchange_grid_data(remap_weights_of_operator_instances[iter]->field_data_grid_dst);

	remap_weights_of_operator_instances[iter]->get_field_data_grid_src()->get_sized_sub_grids(&num_sized_sub_grids, sized_sub_grids);
	for (j = 0; j < num_sized_sub_grids; j ++)
		if (!sized_sub_grids[j]->is_subset_of_grid(remap_weights_of_operator_instances[iter]->operator_grid_src))
			break;
	for (k = 0; j < num_sized_sub_grids; j ++) 
		index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
	num_sized_sub_grids = k;

    remap_beg_iter = remap_weights_of_operator_instances[iter]->remap_beg_iter;
    if (remap_weights_of_operator_instances[iter]->remap_end_iter != -1)
        remap_end_iter = remap_weights_of_operator_instances[iter]->remap_end_iter;
    else if (iter+1 < remap_weights_of_operator_instances.size() && current_remap_operator == remap_weights_of_operator_instances[iter+1]->original_remap_operator)
        remap_end_iter = remap_weights_of_operator_instances[iter+1]->remap_beg_iter;
    else remap_end_iter = tmp_field_data_src->get_coord_value_grid()->get_grid_size()/remap_weights_of_operator_instances[iter]->operator_grid_src->get_grid_size();
    for (j = remap_beg_iter; j < remap_end_iter; j ++) {
        for (k = num_sized_sub_grids - 1, index_size_iter = 1; k >= 0; k --) {
            current_runtime_index_array[k] = (j/index_size_iter) % index_size_array[k];
            index_size_iter *= index_size_array[k];
        }
        for (field_array_offset = 0, index_size_iter = 1, k = 0; k < num_sized_sub_grids; k ++) {
            field_array_offset += current_runtime_index_array[k]*index_size_iter;
            index_size_iter *= index_size_array[k];
        }
        decomp_map_values_src = ((long*) tmp_field_data_src->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[iter]->operator_grid_src->get_grid_size();
        decomp_map_values_dst = ((long*) tmp_field_data_dst->get_grid_data_field()->data_buf) + field_array_offset*remap_weights_of_operator_instances[iter]->operator_grid_dst->get_grid_size();
        remap_weights_of_operator_instances[iter]->duplicated_remap_operator->do_src_decomp_caculation(decomp_map_values_src, decomp_map_values_dst);
    }

    if (change_src_dst && tmp_field_data_dst != field_data_dst)
        delete tmp_field_data_dst;
}


void Remap_weight_of_strategy_class::calculate_src_decomp(Remap_grid_class *grid_src, Remap_grid_class *grid_dst, long *decomp_map_src, const long *decomp_map_dst)
{
    Remap_grid_data_class *decomp_map_field_src, *decomp_map_field_dst;
    Remap_grid_data_class *expanded_decomp_map_field_src, *expanded_decomp_map_field_dst;
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

	EXECUTION_REPORT(REPORT_LOG, true, "before calculate_src_decomp_recursively");
    calculate_src_decomp_recursively(0, NULL, NULL, expanded_decomp_map_field_src, expanded_decomp_map_field_src, expanded_decomp_map_field_dst);
	EXECUTION_REPORT(REPORT_LOG, true, "after calculate_src_decomp_recursively");

    expanded_decomp_map_field_src->interchange_grid_data(grid_src);
    for (i = 0; i < data_grid_src->get_grid_size()/grid_src->get_grid_size(); i ++) {
        tmp_decomp_map_src = ((long*) expanded_decomp_map_field_src->get_grid_data_field()->data_buf) + i*grid_src->get_grid_size();
        for (j = 0; j < grid_src->get_grid_size(); j ++)
			decomp_map_src[j] = (decomp_map_src[j] | tmp_decomp_map_src[j]);
    }

    delete decomp_map_field_src;
    delete decomp_map_field_dst;
    delete expanded_decomp_map_field_src;
    delete expanded_decomp_map_field_dst;
}


Remap_grid_class **Remap_weight_of_strategy_class::get_remap_related_grids(int &num_operator_field_data_grids)
{
    Remap_operator_basis *current_remap_operator;
    Remap_grid_class **all_remap_related_grids, **temp_grids;
    int array_size = 100;

    num_operator_field_data_grids = 0;
    current_remap_operator = NULL;
    all_remap_related_grids = new Remap_grid_class *[array_size];

    all_remap_related_grids[num_operator_field_data_grids++] = data_grid_src;
    all_remap_related_grids[num_operator_field_data_grids++] = data_grid_dst;
    for (int i = 0; i < remap_weights_of_operator_instances.size(); i ++) {
        if (num_operator_field_data_grids + 4 > array_size) {
            temp_grids = all_remap_related_grids;
            all_remap_related_grids = new Remap_grid_class *[array_size*2];
            for (int j = 0; j < array_size; j ++)
                all_remap_related_grids[j] = temp_grids[j];
            delete [] temp_grids;
            array_size *= 2;
        }
        all_remap_related_grids[num_operator_field_data_grids++] = remap_weights_of_operator_instances[i]->field_data_grid_src;
        all_remap_related_grids[num_operator_field_data_grids++] = remap_weights_of_operator_instances[i]->field_data_grid_dst;
        all_remap_related_grids[num_operator_field_data_grids++] = remap_weights_of_operator_instances[i]->operator_grid_src;
        all_remap_related_grids[num_operator_field_data_grids++] = remap_weights_of_operator_instances[i]->operator_grid_dst;
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

    current_remap_operator = NULL;
    for (i = 0; i < this->remap_weights_of_operator_instances.size(); i ++) {
        if (current_remap_operator != remap_weights_of_operator_instances[i]->original_remap_operator) {
            for (j = i+1; j < remap_weights_of_operator_instances.size(); j ++)
                if (remap_weights_of_operator_instances[i]->original_remap_operator != remap_weights_of_operator_instances[j]->original_remap_operator)
                    break;
            remap_weights_of_operator_instances[i]->get_field_data_grid_src()->get_sized_sub_grids(&num_sized_sub_grids, sized_sub_grids);
            for (j = 0; j < num_sized_sub_grids; j ++)
                if (!sized_sub_grids[j]->is_subset_of_grid(remap_weights_of_operator_instances[i]->operator_grid_src))
                    break;
            for (k = 0; j < num_sized_sub_grids; j ++) 
                index_size_array[k++] = sized_sub_grids[j]->get_grid_size();
            num_sized_sub_grids = k;
            current_remap_operator = remap_weights_of_operator_instances[i]->original_remap_operator;
        }
        remap_beg_iter = remap_weights_of_operator_instances[i]->remap_beg_iter;
        if (remap_weights_of_operator_instances[i]->remap_end_iter != -1)
            remap_end_iter = remap_weights_of_operator_instances[i]->remap_end_iter;
        else {
            if (i+1 < remap_weights_of_operator_instances.size() && current_remap_operator == remap_weights_of_operator_instances[i+1]->original_remap_operator)
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
            parallel_remap_weights_of_operator_instance->remap_end_iter = -1;
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
    int num_leaf_grids, i, j, id, grid_num_dimensions, tmp_int_value;
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
    num_remap_operator_instances = get_num_remap_operator_of_weights();
	write_data_into_array(&num_remap_operator_instances, sizeof(int), &output_array, array_size, max_array_size);
    for (i = 0; i < num_remap_operator_instances; i ++) {
        remap_weight_of_operator_instance = remap_weights_of_operator_instances[i];
        tmp_long_value = remap_weight_of_operator_instance->get_remap_begin_iter();
		write_data_into_array(&tmp_long_value, sizeof(long), &output_array, array_size, max_array_size);
        remap_operator_of_one_instance = get_remap_operator_of_weights(i);
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
			tmp_long_value = remap_weight_of_operator_instance->get_remap_end_iter();
			write_data_into_array(&tmp_long_value, sizeof(long), &output_array, array_size, max_array_size);
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


void Remap_weight_of_strategy_class::read_data_from_array(void *data, int data_size, const char *input_array, long &current_array_pos, long array_size, bool read_weight_values)
{
	EXECUTION_REPORT(REPORT_ERROR, current_array_pos+data_size <= array_size, "the access of array is out-of-bound when reading for remapping weights %s", object_name);

	if (read_weight_values)
		for (long i = 0; i < data_size; i ++)
			((char*)data)[i] = input_array[current_array_pos+i];
	current_array_pos += data_size;
}


void Remap_weight_of_strategy_class::read_grid_info_from_array(Remap_grid_class *grid, bool consider_area_or_volumn, const char *input_array, long &current_array_pos, long array_size)
{
    long grid_size;
    int grid_num_dimensions, i, gid, rid, num_leaf_grids, tmp_int_value;
    Remap_grid_class *leaf_grids[256];
    double *area_or_volumn;
    

	read_data_from_array(&grid_size, sizeof(long), input_array, current_array_pos, array_size, true);
    EXECUTION_REPORT(REPORT_ERROR, grid_size == grid->get_grid_size(), "the grid size of %s does not match the binary file\n", grid->get_grid_name());
	read_data_from_array(&grid_num_dimensions, sizeof(int), input_array, current_array_pos, array_size, true);
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
		read_data_from_array(&rid, sizeof(int), input_array, current_array_pos, array_size, true);
        EXECUTION_REPORT(REPORT_ERROR, gid == rid, "the arrange of coordinate systems of grid %s does not match the binary file\n", grid->get_grid_name());
    }

    if (consider_area_or_volumn) {
		read_data_from_array(&tmp_int_value, sizeof(int), input_array, current_array_pos, array_size, true);
        if (tmp_int_value == 1) {
            EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn() != NULL, "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
            area_or_volumn = new double [grid->get_grid_size()];
			read_data_from_array(area_or_volumn, sizeof(double)*grid->get_grid_size(), input_array, current_array_pos, array_size, true);
            for (long i = 0; i < grid->get_grid_size(); i ++)
                EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn()[i] == area_or_volumn[i], "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
            delete [] area_or_volumn;            
        }
        else EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn() == NULL, "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
    }
}


void Remap_weight_of_strategy_class::read_remap_operator_instance_from_array(Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst,
															  Remap_grid_class *operator_grid_src, Remap_grid_class *operator_grid_dst,
                                                              Remap_operator_basis *remap_operator, long remap_iter,
                                                              const char *input_array, long &current_array_pos, long array_size,
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
	read_data_from_array(&num_remap_weights_groups, sizeof(int), input_array, current_array_pos, array_size, true);
    for (i = 0; i < num_remap_weights_groups; i ++) {
		read_data_from_array(&num_weights, sizeof(long), input_array, current_array_pos, array_size, true);
		if (read_weight_values) {
	        indexes_src_grid = new long [num_weights];
	        indexes_dst_grid = new long [num_weights];
    	    weight_values = new double [num_weights];
		}
		read_data_from_array(indexes_src_grid, sizeof(long)*num_weights, input_array, current_array_pos, array_size, read_weight_values);
		read_data_from_array(indexes_dst_grid, sizeof(long)*num_weights, input_array, current_array_pos, array_size, read_weight_values);
		read_data_from_array(weight_values, sizeof(double)*num_weights, input_array, current_array_pos, array_size, read_weight_values);
		read_data_from_array(&num_remaped_dst_cells_indexes, sizeof(long), input_array, current_array_pos, array_size, true);
		if (read_weight_values)
	        remaped_dst_cells_indexes = new long [num_remaped_dst_cells_indexes];
		read_data_from_array(remaped_dst_cells_indexes, sizeof(long)*num_remaped_dst_cells_indexes, input_array, current_array_pos, array_size, read_weight_values);
		if (read_weight_values) {
	        weight_sparse_matrix = new Remap_weight_sparse_matrix(remap_operator, num_weights, indexes_src_grid, indexes_dst_grid, weight_values, num_remaped_dst_cells_indexes, remaped_dst_cells_indexes);
    	    duplicated_remap_operator->add_weight_sparse_matrix(weight_sparse_matrix);
		}
    }
    
    remap_operator_instance = new Remap_weight_of_operator_instance_class(field_data_grid_src, field_data_grid_dst, remap_iter, remap_operator, duplicated_remap_operator);
	remap_operator_instance->operator_grid_src = operator_grid_src;
	remap_operator_instance->operator_grid_dst = operator_grid_dst;
    add_remap_weight_of_operator_instance(remap_operator_instance);
}


void Remap_weight_of_strategy_class::read_remap_weights_from_array(const char *input_array, long array_size, bool read_grid, Remap_grid_class **remap_related_decomp_grids, bool read_weight_values)
{
    Remap_grid_class *field_grid_src, *field_grid_dst, *current_field_grid_src, *current_field_grid_dst;
    Remap_grid_class *leaf_grids_all[256], *leaf_grids_all_sorted[256];
    Remap_grid_class *leaf_grids_remap_operator_src[256], *leaf_grids_remap_operator_dst[256];
    Remap_grid_class *sized_sub_grids[256];
	Remap_grid_class *operator_grid_src, *operator_grid_dst;
    int i, j, k, num_remap_operator_instances, num_remap_operator, num_leaf_grids_all, num_leaf_grids_remap_operator;
    Remap_operator_basis *remap_operator;
    int coord_system_ids[256], tmp_grid_num_dimensions, num_sized_sub_grids;
    long tmp_grid_size, current_remap_iter, last_remap_iter, remap_end_iter;
    char operator_name[256], tmp_grid_name[256];
	long current_array_pos = 0;
	int field_data_grids_iter = 0;
    

	if (read_grid) {
	    field_grid_src = get_data_grid_src();
	    field_grid_dst = get_data_grid_dst();
		EXECUTION_REPORT(REPORT_ERROR, remap_related_decomp_grids == NULL, "software error in read_remap_weights_from_array");
    	read_grid_info_from_array(field_grid_src, true, input_array, current_array_pos, array_size);
    	read_grid_info_from_array(field_grid_dst, true, input_array, current_array_pos, array_size);
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR, remap_related_decomp_grids != NULL, "software error in read_remap_weights_from_array");
	    field_grid_src = remap_related_decomp_grids[field_data_grids_iter++];
    	field_grid_dst = remap_related_decomp_grids[field_data_grids_iter++];
	}

	read_data_from_array(&num_remap_operator_instances, sizeof(int), input_array, current_array_pos, array_size, true);
    num_remap_operator = 0;
    current_field_grid_src = field_grid_src;
    last_remap_iter = -1;
    for (i = 0; i < num_remap_operator_instances; i ++) {
		read_data_from_array(&current_remap_iter, sizeof(long), input_array, current_array_pos, array_size, true);
		read_data_from_array(operator_name, sizeof(char)*256, input_array, current_array_pos, array_size, true);
		if (read_grid) {
	        if (current_remap_iter <= last_remap_iter) {
	            num_remap_operator ++;
    	        current_field_grid_src = current_field_grid_dst;
        	}
	        last_remap_iter = current_remap_iter;
    	    remap_operator = remap_strategy->get_remap_operator(num_remap_operator);
			EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(operator_name, remap_operator->get_operator_name()),
							 "the remap operator %s does not match the binary file, which should be %s\n", 
							 operator_name, remap_operator->get_operator_name());
			read_data_from_array(&tmp_grid_size, sizeof(long), input_array, current_array_pos, array_size, true);
			read_data_from_array(&tmp_grid_num_dimensions, sizeof(int), input_array, current_array_pos, array_size, true);
	        EXECUTION_REPORT(REPORT_ERROR, tmp_grid_num_dimensions == field_grid_src->get_num_dimensions(), "remap software error2 in read_remap_weights_from_array binary\n");
	        for (j = 0; j < tmp_grid_num_dimensions; j ++)
				read_data_from_array(&coord_system_ids[j], sizeof(int), input_array, current_array_pos, array_size, true);
	        current_field_grid_src->get_leaf_grids(&num_leaf_grids_all, leaf_grids_all, current_field_grid_src);
	        for (j = 0; j < tmp_grid_num_dimensions; j ++) {
	            for (k = 0; k < tmp_grid_num_dimensions; k ++) {
	                if (words_are_the_same(leaf_grids_all[k]->get_coord_label(), COORD_LABEL_LON) && coord_system_ids[j] == 1)
	                    break;
	                else if (words_are_the_same(leaf_grids_all[k]->get_coord_label(), COORD_LABEL_LAT) && coord_system_ids[j] == 2) 
	                    break;
	                else if (words_are_the_same(leaf_grids_all[k]->get_coord_label(), COORD_LABEL_LEV) && coord_system_ids[j] == 3)
	                    break;
	                else if (words_are_the_same(leaf_grids_all[k]->get_coord_label(), COORD_LABEL_TIME) && coord_system_ids[j] == 4)
	                    break;
	            }
	            EXECUTION_REPORT(REPORT_ERROR, k < tmp_grid_num_dimensions, "remap software error3 in read_remap_weights_from_array binary\n");
	            leaf_grids_all_sorted[j] = leaf_grids_all[k];
	        }
	        num_sized_sub_grids = 0;
	        for (j = 0; j < tmp_grid_num_dimensions; j ++) {
	            if (num_sized_sub_grids > 0 && leaf_grids_all_sorted[j]->is_subset_of_grid(sized_sub_grids[num_sized_sub_grids-1]))
	                continue;
	            sized_sub_grids[num_sized_sub_grids++] = leaf_grids_all_sorted[j]->get_first_super_grid_of_enable_setting_coord_value();
	        }
			current_field_grid_src = remap_grid_manager->search_remap_grid_with_sized_sub_grids(num_sized_sub_grids, sized_sub_grids);
			if (current_field_grid_src == NULL) {
				sprintf(tmp_grid_name, "TMP_GRID_SRC");
				for (j = 0; j < num_sized_sub_grids; j ++)
					sprintf(tmp_grid_name, "%s_%s", tmp_grid_name, sized_sub_grids[j]->get_grid_name());
	            current_field_grid_src = new Remap_grid_class(tmp_grid_name, num_sized_sub_grids, sized_sub_grids, 0);
				remap_grid_manager->add_remap_grid(current_field_grid_src);
			}
	        EXECUTION_REPORT(REPORT_ERROR, tmp_grid_size == current_field_grid_src->get_grid_size(), 
	                         "the src field grid size of remap operator %s does not match the binary file\n",
	                         remap_operator->get_object_name());
	        remap_operator->get_src_grid()->get_leaf_grids(&num_leaf_grids_remap_operator, leaf_grids_remap_operator_src, remap_operator->get_src_grid());
	        remap_operator->get_dst_grid()->get_leaf_grids(&num_leaf_grids_remap_operator, leaf_grids_remap_operator_dst, remap_operator->get_dst_grid());
	        for (j = 0; j < num_leaf_grids_remap_operator; j ++)
	            for (k = 0; k < num_leaf_grids_all; k ++)
	                if (leaf_grids_all_sorted[k] == leaf_grids_remap_operator_src[j])
	                    leaf_grids_all_sorted[k] = leaf_grids_remap_operator_dst[j];
	        num_sized_sub_grids = 0;
	        for (j = 0; j < tmp_grid_num_dimensions; j ++) {
	            if (num_sized_sub_grids > 0 && leaf_grids_all_sorted[j]->is_subset_of_grid(sized_sub_grids[num_sized_sub_grids-1]))
	                continue;
	            sized_sub_grids[num_sized_sub_grids++] = leaf_grids_all_sorted[j]->get_first_super_grid_of_enable_setting_coord_value();
	        }
			current_field_grid_dst = remap_grid_manager->search_remap_grid_with_sized_sub_grids(num_sized_sub_grids, sized_sub_grids);
			if (current_field_grid_dst == NULL) {
				sprintf(tmp_grid_name, "TMP_GRID_DST");
				for (j = 0; j < num_sized_sub_grids; j ++)
					sprintf(tmp_grid_name, "%s_%s", tmp_grid_name, sized_sub_grids[j]->get_grid_name());
	           	current_field_grid_dst = new Remap_grid_class(tmp_grid_name, num_sized_sub_grids, sized_sub_grids, 0);
				remap_grid_manager->add_remap_grid(current_field_grid_dst);
			}
	        read_grid_info_from_array(current_field_grid_dst, false, input_array, current_array_pos, array_size);
    	    read_grid_info_from_array(remap_operator->get_src_grid(), false, input_array, current_array_pos, array_size);
        	read_grid_info_from_array(remap_operator->get_dst_grid(), false, input_array, current_array_pos, array_size);
			operator_grid_src = remap_operator->get_src_grid();
			operator_grid_dst = remap_operator->get_dst_grid();
			read_remap_operator_instance_from_array(current_field_grid_src, current_field_grid_dst, operator_grid_src, operator_grid_dst, remap_operator, current_remap_iter, input_array, current_array_pos, array_size, read_weight_values);
    	}
		else {
			remap_operator = remap_operator_manager->search_remap_operator(operator_name);
			EXECUTION_REPORT(REPORT_ERROR, remap_operator != NULL, "software error when searching remap operator %s in read_remap_weights_from_array", operator_name);
			read_data_from_array(&remap_end_iter, sizeof(long), input_array, current_array_pos, array_size, true);
			current_field_grid_src = remap_related_decomp_grids[field_data_grids_iter+0];
			current_field_grid_dst = remap_related_decomp_grids[field_data_grids_iter+1];
			operator_grid_src = remap_related_decomp_grids[field_data_grids_iter+2];
			operator_grid_dst = remap_related_decomp_grids[field_data_grids_iter+3];
			field_data_grids_iter += 4;
			read_remap_operator_instance_from_array(current_field_grid_src, current_field_grid_dst, operator_grid_src, operator_grid_dst, remap_operator, current_remap_iter, input_array, current_array_pos, array_size, read_weight_values);
			remap_weights_of_operator_instances[remap_weights_of_operator_instances.size()-1]->remap_end_iter = remap_end_iter;
		}
    }
    EXECUTION_REPORT(REPORT_ERROR, current_field_grid_dst->is_similar_grid_with(field_grid_dst), "remap software error4 in read_remap_weights_from_array\n");
	EXECUTION_REPORT(REPORT_ERROR, current_array_pos == array_size, "the input array does not match the remapping weights %s when reading", object_name);
}


