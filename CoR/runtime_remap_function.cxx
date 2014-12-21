/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "runtime_remap_function.h"
#include "cor_global_data.h"


Runtime_remap_function::Runtime_remap_function(Remap_grid_class *interchanged_grid_src,
                                          Remap_grid_class *interchanged_grid_dst,
                                          Remap_grid_class *remap_operator_runtime_grid_src,
                                          Remap_grid_class *remap_operator_runtime_grid_dst,
                                          Remap_operator_basis *runtime_remap_operator,
                                          Remap_grid_data_class *remap_field_data_src,
                                          Remap_grid_data_class *remap_field_data_dst,
                                          Remap_weight_of_strategy_class *remap_weight_of_strategy)
{
    int num_sized_grids_of_remapping_src, num_sized_grids_of_remapping_dst, num_leaf_grids;
    Remap_grid_class *sized_grids_of_remapping_src[256], *sized_grids_of_remapping_dst[256], *leaf_grids[256];
    int num_sized_grids_of_interchanged_grid_src, num_sized_grids_of_interchanged_grid_dst;
    Remap_grid_class *sized_grids_of_interchanged_grid_src[256], *sized_grids_of_interchanged_grid_dst[256], *super_grid;
    Remap_grid_data_class *partial_redundant_mark_field;
    long i, j;


    /* Set the member variables */
    this->interchanged_grid_src = interchanged_grid_src;
    this->interchanged_grid_dst = interchanged_grid_dst;
    this->remap_operator_runtime_grid_src = remap_operator_runtime_grid_src;
    this->remap_operator_runtime_grid_dst = remap_operator_runtime_grid_dst;
    this->remap_field_data_src = remap_field_data_src;
    this->remap_field_data_dst = remap_field_data_dst;
    this->runtime_remap_operator = runtime_remap_operator;
    this->num_remapping_times = interchanged_grid_src->get_grid_size()/remap_operator_runtime_grid_src->get_grid_size();
    this->remap_weight_of_strategy = remap_weight_of_strategy;
    this->last_remapping_time_iter = -1;
	this->last_remap_weight_of_operator_instance = NULL;
    for (i = 0; i < 256; i ++)
        this->last_runtime_index_array[i] = -1;

    /* Check the remap software and then set num_sized_grids_of_interchanged_grid,
         sized_grids_of_interchanged_grid, index_size_array, etc */
    interchanged_grid_src->get_sized_sub_grids(&num_sized_grids_of_interchanged_grid_src, sized_grids_of_interchanged_grid_src);
    interchanged_grid_dst->get_sized_sub_grids(&num_sized_grids_of_interchanged_grid_dst, sized_grids_of_interchanged_grid_dst);
    remap_operator_runtime_grid_src->get_sized_sub_grids(&num_sized_grids_of_remapping_src, sized_grids_of_remapping_src);
    remap_operator_runtime_grid_dst->get_sized_sub_grids(&num_sized_grids_of_remapping_dst, sized_grids_of_remapping_dst);
    remap_operator_runtime_grid_src->get_leaf_grids(&this->num_leaf_grids_of_remap_operator_grid_src, 
                                                    this->leaf_grids_of_remap_operator_grid_src,
                                                    remap_operator_runtime_grid_src);
    remap_operator_runtime_grid_dst->get_leaf_grids(&this->num_leaf_grids_of_remap_operator_grid_dst, 
                                                    this->leaf_grids_of_remap_operator_grid_dst,
                                                    remap_operator_runtime_grid_dst);
    EXECUTION_REPORT(REPORT_ERROR, interchanged_grid_src->get_grid_size()%remap_operator_runtime_grid_src->get_grid_size() == 0 &&
                 interchanged_grid_dst->get_grid_size()%remap_operator_runtime_grid_dst->get_grid_size() == 0, 
                 "remap software error1 in new Runtime_remap_function\n");
    for (i = 0; i < num_sized_grids_of_remapping_src; i ++)
        EXECUTION_REPORT(REPORT_ERROR, sized_grids_of_remapping_src[i] == sized_grids_of_interchanged_grid_src[i], "remap software error2 in new Runtime_remap_function\n");
    for (i = 0; i < num_sized_grids_of_remapping_dst; i ++)
        EXECUTION_REPORT(REPORT_ERROR, sized_grids_of_remapping_dst[i] == sized_grids_of_interchanged_grid_dst[i], "remap software error2 in new Runtime_remap_function\n");
    EXECUTION_REPORT(REPORT_ERROR, num_sized_grids_of_interchanged_grid_src-num_sized_grids_of_remapping_src == num_sized_grids_of_interchanged_grid_dst-num_sized_grids_of_remapping_dst,
                 "remap software error3 in new Runtime_remap_function\n");
    for (i = num_sized_grids_of_remapping_src, j = num_sized_grids_of_remapping_dst; i < num_sized_grids_of_interchanged_grid_src; i ++)
        EXECUTION_REPORT(REPORT_ERROR, sized_grids_of_interchanged_grid_src[i] == sized_grids_of_interchanged_grid_dst[j++], "remap software error5 in new Runtime_remap_function\n");
    this->num_sized_grids_of_interchanged_grid = num_sized_grids_of_interchanged_grid_src - num_sized_grids_of_remapping_src;
    for (i = 0; i < this->num_sized_grids_of_interchanged_grid; i ++) {
        this->sized_grids_of_interchanged_grid[i] = sized_grids_of_interchanged_grid_src[num_sized_grids_of_remapping_src+i];
        this->index_size_array[i] = this->sized_grids_of_interchanged_grid[i]->get_grid_size();
    }

    if (runtime_remap_operator->does_require_grid_vertex_values()) {
        for (i = 0; i < num_leaf_grids_of_remap_operator_grid_src; i ++) {
            EXECUTION_REPORT(REPORT_ERROR, leaf_grids_of_remap_operator_grid_src[i]->super_grid_of_setting_coord_values != NULL, "remap software error6 in new Runtime_remap_function\n");
            EXECUTION_REPORT(REPORT_ERROR, leaf_grids_of_remap_operator_grid_src[i]->grid_vertex_fields.size() == 1, 
							 "remap operator %s (%s) requires users to specify vertex coordinate values in source grid %s", 
							 runtime_remap_operator->get_object_name(), runtime_remap_operator->get_operator_name(), remap_operator_runtime_grid_src->get_grid_name());
            if (leaf_grids_of_remap_operator_grid_src[i]->super_grid_of_setting_coord_values->num_dimensions > 1)
                EXECUTION_REPORT(REPORT_ERROR, !leaf_grids_of_remap_operator_grid_src[i]->super_grid_of_setting_coord_values->are_vertex_values_set_in_default, 
                             "remap operator \"%s\" requires vertex values. The vertex values of \"%s\" in source grid \"%s\" are not given by users\n",
                             runtime_remap_operator->get_object_name(),
                             leaf_grids_of_remap_operator_grid_src[i]->coord_label,
                             leaf_grids_of_remap_operator_grid_src[i]->super_grid_of_setting_coord_values->grid_name);
            EXECUTION_REPORT(REPORT_ERROR, leaf_grids_of_remap_operator_grid_dst[i]->super_grid_of_setting_coord_values != NULL, "remap software error8 in new Runtime_remap_function\n");
            EXECUTION_REPORT(REPORT_ERROR, leaf_grids_of_remap_operator_grid_dst[i]->grid_vertex_fields.size() == 1, 
							 "remap operator %s (%s) requires users to specify vertex coordinate values in target grid %s", 
							 runtime_remap_operator->get_object_name(), runtime_remap_operator->get_operator_name(), remap_operator_runtime_grid_dst->get_grid_name());
            if (leaf_grids_of_remap_operator_grid_dst[i]->super_grid_of_setting_coord_values->num_dimensions > 1)
                EXECUTION_REPORT(REPORT_ERROR, !leaf_grids_of_remap_operator_grid_dst[i]->super_grid_of_setting_coord_values->are_vertex_values_set_in_default, 
                             "remap operator \"%s\" requires vertex values. The vertex values of \"%s\" in destination grid \"%s\" are not given by users\n",
                             runtime_remap_operator->get_object_name(),
                             leaf_grids_of_remap_operator_grid_dst[i]->coord_label,
                             leaf_grids_of_remap_operator_grid_dst[i]->super_grid_of_setting_coord_values->grid_name);
        }
    }

    runtime_remap_operator_grid_src = new Remap_operator_grid(remap_operator_runtime_grid_src, runtime_remap_operator, true, false);
    runtime_remap_operator_grid_dst = new Remap_operator_grid(remap_operator_runtime_grid_dst, runtime_remap_operator, false, false);

    if (remap_field_data_dst != NULL && !remap_field_data_dst->have_data_content()) 
        remap_field_data_dst->grid_data_field->initialize_to_fill_value();

    current_mask_values_src = NULL;
    current_mask_values_dst = NULL;
    last_mask_values_src = NULL;
    last_mask_values_dst = NULL;
    if (remap_operator_runtime_grid_src->grid_mask_field != NULL) {
        current_mask_values_src = (bool*) remap_operator_runtime_grid_src->grid_mask_field->grid_data_field->data_buf;
        last_mask_values_src = new bool [remap_operator_runtime_grid_src->grid_size];
        for (i = 0; i < remap_operator_runtime_grid_src->grid_size; i ++)
            last_mask_values_src[i] = false;
    }
    if (remap_operator_runtime_grid_dst->grid_mask_field != NULL) {
        current_mask_values_dst = (bool*) remap_operator_runtime_grid_dst->grid_mask_field->grid_data_field->data_buf;
        last_mask_values_dst = new bool [remap_operator_runtime_grid_dst->grid_size];
        for (i = 0; i < remap_operator_runtime_grid_dst->grid_size; i ++)
            last_mask_values_dst[i] = false;        
    }

    /* compute overall redundant mask fields */    
    remap_field_data_redundant_mark_field_src = NULL;
    current_redundant_mark_src = NULL;
    last_redundant_mark_src = NULL;
    if (remap_operator_runtime_grid_src->redundant_cell_mark_field != NULL) {
        remap_field_data_redundant_mark_field_src = remap_operator_runtime_grid_src->redundant_cell_mark_field->duplicate_grid_data_field(interchanged_grid_src, 1, false, false);
        for (i = 0; i < remap_field_data_redundant_mark_field_src->get_coord_value_grid()->get_grid_size(); i ++)
            ((bool*)remap_field_data_redundant_mark_field_src->grid_data_field->data_buf)[i] = false;
        remap_operator_runtime_grid_src->get_leaf_grids(&num_leaf_grids, leaf_grids, remap_operator_runtime_grid_src);
        for (i = 0; i < num_leaf_grids; i ++) {
            super_grid = leaf_grids[i]->super_grid_of_setting_coord_values;
            if (super_grid == NULL)
                continue;
			if (super_grid->is_sigma_grid())
				continue;
            partial_redundant_mark_field = remap_field_data_redundant_mark_field_src->get_coord_value_grid()->expand_to_generate_full_coord_value(super_grid->redundant_cell_mark_field);
            for (j = 0; j < remap_field_data_redundant_mark_field_src->get_coord_value_grid()->get_grid_size(); j ++) 
                ((bool*)remap_field_data_redundant_mark_field_src->grid_data_field->data_buf)[j] |= ((bool*)partial_redundant_mark_field->grid_data_field->data_buf)[j];
            delete partial_redundant_mark_field;
        }
        last_redundant_mark_src = new bool [remap_operator_runtime_grid_src->grid_size];
        current_redundant_mark_src = (bool*) remap_operator_runtime_grid_src->redundant_cell_mark_field->grid_data_field->data_buf;
        for (i = 0; i < remap_operator_runtime_grid_src->grid_size; i ++) {
            last_redundant_mark_src[i] = false;
            current_redundant_mark_src[i] = ((bool*)remap_field_data_redundant_mark_field_src->grid_data_field->data_buf)[i];
        }
    }
}


Runtime_remap_function::~Runtime_remap_function()
{
    if (execution_phase_number == 1)
        for (int i = 0; i < num_sized_grids_of_interchanged_grid; i ++)
            EXECUTION_REPORT(REPORT_ERROR, current_runtime_index_array[i] == index_size_array[i] - 1, "remap software error in ~Runtime_remap_function\n");

    delete runtime_remap_operator_grid_src;
    delete runtime_remap_operator_grid_dst;

    if (last_mask_values_src)
        delete [] last_mask_values_src;
    if (last_mask_values_dst)
        delete [] last_mask_values_dst;
    if (remap_field_data_redundant_mark_field_src != NULL) {
        delete [] last_redundant_mark_src;
        delete remap_field_data_redundant_mark_field_src;
    }
}


void Runtime_remap_function::do_runtime_remap(long current_remapping_time_iter)
{
    long i, index_size_iter, field_array_offset;
    bool mask_values_have_been_changed, coord_values_have_been_changed_src, coord_values_have_been_changed_dst;
    int mask_values_status_src, mask_values_status_dst, redundant_mark_status_src;
    double *current_data_values_src, *current_data_values_dst;


    EXECUTION_REPORT(REPORT_ERROR, current_remapping_time_iter < num_remapping_times, "remap software error1 in do_runtime_remap\n");

	current_runtime_remap_operator_grid_src = runtime_remap_operator_grid_src;
	current_runtime_remap_operator_grid_dst = runtime_remap_operator_grid_dst;
    current_runtime_remap_operator = runtime_remap_operator;

    /* Update runtime index array according to current_remapping_time_iter */
    for (i = num_sized_grids_of_interchanged_grid - 1, index_size_iter = 1; i >= 0; i --) {
        current_runtime_index_array[i] = (current_remapping_time_iter/index_size_iter) % index_size_array[i];
        index_size_iter *= index_size_array[i];
    }

    /* Extract the runtime grid field data for runtime remapping */
    coord_values_have_been_changed_src = extract_and_set_runtime_grid_fields(interchanged_grid_src,
																		     num_leaf_grids_of_remap_operator_grid_src,
                                                                             leaf_grids_of_remap_operator_grid_src,
                                                                             remap_operator_runtime_grid_src);
    coord_values_have_been_changed_dst = extract_and_set_runtime_grid_fields(interchanged_grid_dst,
																			 num_leaf_grids_of_remap_operator_grid_dst,
                                                                             leaf_grids_of_remap_operator_grid_dst,
                                                                             remap_operator_runtime_grid_dst);
    if (remap_field_data_redundant_mark_field_src != NULL)
        extract_runtime_grid_field(remap_field_data_redundant_mark_field_src, remap_operator_runtime_grid_src->redundant_cell_mark_field);
    redundant_mark_status_src = check_mask_values_status(last_redundant_mark_src, current_redundant_mark_src, remap_operator_runtime_grid_src->grid_size);
    mask_values_status_src = check_mask_values_status(last_mask_values_src, current_mask_values_src, remap_operator_runtime_grid_src->grid_size);
    mask_values_status_dst = check_mask_values_status(last_mask_values_dst, current_mask_values_dst, remap_operator_runtime_grid_dst->grid_size);

	if (mask_values_status_src == -1 || mask_values_status_dst == -1) {
		return;
	}

    mask_values_have_been_changed = (mask_values_status_src%2==1) || (mask_values_status_dst%2==1);

    if (coord_values_have_been_changed_src || last_remapping_time_iter == -1 || redundant_mark_status_src%2==1)
        runtime_remap_operator_grid_src->update_operator_grid_data();
    if (coord_values_have_been_changed_dst || last_remapping_time_iter == -1)
        runtime_remap_operator_grid_dst->update_operator_grid_data();
    
    if (coord_values_have_been_changed_src || coord_values_have_been_changed_dst || redundant_mark_status_src%2==1 ||
        mask_values_have_been_changed || last_remapping_time_iter == -1) {
        runtime_remap_operator->calculate_remap_weights();
        last_remapping_time_iter = current_remapping_time_iter;
        for (i = 0; i < num_sized_grids_of_interchanged_grid; i ++)
            last_runtime_index_array[i] = current_runtime_index_array[i];
        for (field_array_offset = 0, index_size_iter = 1, i = 0; i < num_sized_grids_of_interchanged_grid; i ++) {
            field_array_offset += current_runtime_index_array[i]*index_size_iter;
            index_size_iter *= index_size_array[i];
        }
        if (remap_weight_of_strategy != NULL) {
            last_remap_weight_of_operator_instance = remap_weight_of_strategy->add_remap_weight_of_operator_instance(interchanged_grid_src, interchanged_grid_dst, current_remapping_time_iter, runtime_remap_operator);
        }
    }
	else last_remap_weight_of_operator_instance->renew_remapping_time_end_iter(current_remapping_time_iter);

    if (remap_field_data_src != NULL) {
        for (field_array_offset = 0, index_size_iter = 1, i = 0; i < num_sized_grids_of_interchanged_grid; i ++) {
            field_array_offset += current_runtime_index_array[i]*index_size_iter;
            index_size_iter *= index_size_array[i];
        }
        current_data_values_src = (double*) remap_field_data_src->grid_data_field->data_buf + field_array_offset*remap_operator_runtime_grid_src->get_grid_size();    
        current_data_values_dst = (double*) remap_field_data_dst->grid_data_field->data_buf + field_array_offset*remap_operator_runtime_grid_dst->get_grid_size();
        runtime_remap_operator->do_remap_values_caculation(current_data_values_src, current_data_values_dst);
    }
}


bool Runtime_remap_function::extract_and_set_runtime_grid_fields(Remap_grid_class *field_data_grid,
																 int num_leaf_grids_of_remap_operator_grid,
                                                                 Remap_grid_class **leaf_grids_of_remap_operator_grid,
                                                                 Remap_grid_class *remap_operator_runtime_grid)
{
    int i;
    Remap_grid_class *super_grid;
    Remap_grid_data_class *center_value_field, *vertex_value_field = NULL;
    bool are_coord_values_updated = false;


    /* Extract runtime grid center fields and vertex fields. If the grid of defining the grid center/vertex field is a sub grid of 
           remap_operator_runtime_grid, only set vertex values in default when necessary. Otherwise, extract runtime grid center
           fields and extract grid vertex fields when necessary */
    for (i = 0; i < num_leaf_grids_of_remap_operator_grid; i ++) {
        if (runtime_remap_operator->does_require_grid_vertex_values() || leaf_grids_of_remap_operator_grid[i]->grid_vertex_fields.size() > 0)
            EXECUTION_REPORT(REPORT_ERROR, leaf_grids_of_remap_operator_grid[i]->grid_vertex_fields.size() == 1, "remap software error6 in new extract_and_set_runtime_grid_fields\n");
		if (leaf_grids_of_remap_operator_grid[i]->has_grid_coord_label(COORD_LABEL_LEV) && field_data_grid->is_sigma_grid())
			super_grid = field_data_grid;
        else super_grid = leaf_grids_of_remap_operator_grid[i]->get_super_grid_of_setting_coord_values();
        if (super_grid == NULL) {
            EXECUTION_REPORT(REPORT_ERROR, !runtime_remap_operator->get_is_operator_regridding() && leaf_grids_of_remap_operator_grid[i]->grid_center_fields.size() == 0, 
                         "remap software error1 in new extract_and_set_runtime_grid_fields\n");
            continue;
        }
        if (super_grid->is_subset_of_grid(remap_operator_runtime_grid))
            continue;
		if (super_grid == field_data_grid) {
			EXECUTION_REPORT(REPORT_ERROR, leaf_grids_of_remap_operator_grid[i]->has_grid_coord_label(COORD_LABEL_LEV) && field_data_grid->is_sigma_grid(), "C-Coupler error in extract_and_set_runtime_grid_fields");
			center_value_field = super_grid->get_unique_center_field();
			if (super_grid->grid_vertex_fields.size() > 0)
				vertex_value_field = super_grid->grid_vertex_fields[0];
		}
        else {
			center_value_field = leaf_grids_of_remap_operator_grid[i]->get_grid_center_field();
	        vertex_value_field = leaf_grids_of_remap_operator_grid[i]->get_grid_vertex_field();
        }
        EXECUTION_REPORT(REPORT_ERROR, leaf_grids_of_remap_operator_grid[i]->grid_center_fields.size() == 1 && center_value_field != NULL, 
                     "remap software error4 in new extract_and_set_runtime_grid_fields\n");    
        check_dimension_order_of_grid_field(center_value_field, remap_operator_runtime_grid);
        check_dimension_order_of_grid_field(leaf_grids_of_remap_operator_grid[i]->grid_center_fields[0], remap_operator_runtime_grid);
        are_coord_values_updated = extract_runtime_grid_field(center_value_field, leaf_grids_of_remap_operator_grid[i]->grid_center_fields[0]);
        if (vertex_value_field != NULL) {
            check_dimension_order_of_grid_field(vertex_value_field, remap_operator_runtime_grid);
            check_dimension_order_of_grid_field(leaf_grids_of_remap_operator_grid[i]->grid_vertex_fields[0], remap_operator_runtime_grid);
            extract_runtime_grid_field(vertex_value_field, leaf_grids_of_remap_operator_grid[i]->grid_vertex_fields[0]);
        }
    }

    /* Extract runtime grid mask fields and then compute the mask field of runtime grid when necessary. 
           When original_grid_mask_field is not NULL, the runtime grid mask field will be extracted from the 
           corresponding super grid when necessary. */
    if (remap_operator_runtime_grid->original_grid_mask_field != NULL) {
        EXECUTION_REPORT(REPORT_ERROR, remap_operator_runtime_grid->grid_mask_field != NULL, "remap software error8 in new extract_and_set_runtime_grid_fields\n");
        check_dimension_order_of_grid_field(remap_operator_runtime_grid->original_grid_mask_field, remap_operator_runtime_grid);
        extract_runtime_grid_field(remap_operator_runtime_grid->original_grid_mask_field, remap_operator_runtime_grid->grid_mask_field);
    }

    return are_coord_values_updated;
}


bool Runtime_remap_function::extract_runtime_grid_field(Remap_grid_data_class *grid_data_global, Remap_grid_data_class *grid_data_runtime)
{
    int i, j;
    long grid_field_size, grid_data_offset_header, iter;
    bool have_index_iter_changed;
    char *data_buf_global, *data_buf_runtime;


    EXECUTION_REPORT(REPORT_ERROR, grid_data_runtime->get_coord_value_grid()->is_subset_of_grid(grid_data_global->get_coord_value_grid()), "remap software error1 in extract_runtime_grid_field\n");

    have_index_iter_changed = false;
    grid_data_offset_header = 0;
    iter = 1;
    for (j = 0; j < grid_data_global->sized_grids.size(); j ++) {
        for (i = 0; i < num_sized_grids_of_interchanged_grid; i ++)
            if (sized_grids_of_interchanged_grid[i] == grid_data_global->sized_grids[j])
                break;
        if (i < num_sized_grids_of_interchanged_grid) {
            if (last_runtime_index_array[i] != current_runtime_index_array[i])
                have_index_iter_changed = true;
            grid_data_offset_header += current_runtime_index_array[i] * iter;
            iter *= index_size_array[i];
        }
    }

    if (!have_index_iter_changed)
        return false;

    grid_field_size = grid_data_runtime->grid_data_field->required_data_size;
    data_buf_global = (char*) grid_data_global->grid_data_field->data_buf;
    data_buf_runtime = (char*) grid_data_runtime->grid_data_field->data_buf;
    memcpy(data_buf_runtime, 
           data_buf_global+grid_data_offset_header*grid_field_size*get_data_type_size(grid_data_runtime->grid_data_field->data_type_in_application),
           grid_field_size*get_data_type_size(grid_data_runtime->grid_data_field->data_type_in_application));

    return true;
}


int Runtime_remap_function::check_mask_values_status(bool *last_mask_values, bool *current_mask_values, long grid_size)
{
    int mask_values_status = 0;
    long i;


    if (last_mask_values == NULL)
        return 0;

	for (i = 0; i < grid_size; i ++)
		if (current_mask_values[i])
			break;
	if (i == grid_size) {
		for (i = 0; i < grid_size; i ++)
			last_mask_values[i] = current_mask_values[i];
		return -1;
	}
    
    for (i = 0; i < grid_size; i ++)
        if (last_mask_values[i] != current_mask_values[i]) {
            mask_values_status += 1;
            break;
        }

    for (i = 0; i < grid_size; i ++) 
        if (current_mask_values[i]) {
            mask_values_status += 2;
            break;
        }

    for (i = 0; i < grid_size; i ++)
        last_mask_values[i] = current_mask_values[i];

    return mask_values_status;
}


void Runtime_remap_function::check_dimension_order_of_grid_field(Remap_grid_data_class *grid_data, Remap_grid_class *remap_grid)
{
    Remap_grid_class *sized_grids_of_remapping[256];
    int i, j, last_order_indx, num_sized_grids_of_remapping;


    remap_grid->get_sized_sub_grids(&num_sized_grids_of_remapping, sized_grids_of_remapping);
    for (i = 0, j = num_sized_grids_of_remapping; i < num_sized_grids_of_interchanged_grid; i ++)
        sized_grids_of_remapping[j++] = sized_grids_of_interchanged_grid[i];
    num_sized_grids_of_remapping += num_sized_grids_of_interchanged_grid;

    last_order_indx = -1;
    for (i = 0; i < grid_data->sized_grids.size(); i ++) {
        for (j = 0; j < num_sized_grids_of_remapping; j ++) {
            if (grid_data->sized_grids[i] == sized_grids_of_remapping[j]) 
                break;
        }
        EXECUTION_REPORT(REPORT_ERROR, j < num_sized_grids_of_remapping, "remap software error1 in check_dimension_order_of_grid_field\n");
        EXECUTION_REPORT(REPORT_ERROR, j > last_order_indx, "remap software error2 in check_dimension_order_of_grid_field\n");
        last_order_indx = j;
    }
}

