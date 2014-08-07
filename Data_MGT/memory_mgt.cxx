/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "memory_mgt.h"
#include "global_data.h"
#include "cor_global_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* Interface for allocate memory buffer for the fields in coupling flow */

Field_mem_info *alloc_mem(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, const char *data_type, const int buf_type, bool is_input_field)
{
    return memory_manager->alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, false, is_input_field);
}


Field_mem_info *alloc_full_grid_mem(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, const char *data_type, const int buf_type, bool is_input_field)
{
    return memory_manager->alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, true, is_input_field);
}


Field_mem_info::Field_mem_info(const char *comp_name, 
                                  const char *decomp_name, 
                                  const char *grid_name,
                                  const char *field_name, 
                                  const char *data_type,
                                  const int buf_type,
                                  bool use_full_grid)
{
    Remap_data_field *remap_data_field;
    Decomp_info *decomp_info;
    Remap_grid_class *decomp_grid;
    long mem_size;
    

    if (strcmp(decomp_name, "NULL") == 0) 
        mem_size = get_data_type_size(data_type);
    else {
        EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "%s is not a grid when allocating memory for field\n", grid_name);
        decomp_info = decomps_info_mgr->search_decomp_info(decomp_name);
        mem_size = decomp_info->get_num_local_cells() * get_data_type_size(data_type) *
                   remap_grid_manager->search_remap_grid_with_grid_name(grid_name)->get_grid_size()/remap_grid_manager->search_remap_grid_with_grid_name(decomp_info->get_grid_name())->get_grid_size();
    }

    remap_data_field = new Remap_data_field;
    strcpy(remap_data_field->field_name_in_application, field_name);
    strcpy(remap_data_field->field_name_in_IO_file, field_name);
    strcpy(remap_data_field->data_type_in_application, data_type);
    remap_data_field->required_data_size = mem_size / get_data_type_size(data_type);
    remap_data_field->read_data_size = remap_data_field->required_data_size;
    remap_data_field->data_buf = new char [mem_size];
    remap_data_field->set_field_long_name(fields_info->get_field_long_name(field_name));
    remap_data_field->set_field_unit(fields_info->get_field_unit(field_name));
    memset(remap_data_field->data_buf, 0, mem_size);

    if (words_are_the_same(decomp_name, "NULL"))
        grided_field_data = new Remap_grid_data_class(NULL, remap_data_field);
    else {
        if (use_full_grid)
            decomp_grid = remap_grid_manager->search_remap_grid_with_grid_name(grid_name);
        else decomp_grid = decomp_grids_mgr->search_decomp_grid_info(decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(grid_name))->get_decomp_grid();
        grided_field_data = new Remap_grid_data_class(decomp_grid, remap_data_field);
        remap_data_field->set_fill_value(NULL);
    }

    strcpy(this->comp_name, comp_name);
    strcpy(this->decomp_name, decomp_name);
    strcpy(this->field_name, field_name);
    strcpy(this->grid_name, grid_name);
    this->buf_type = buf_type;
    is_registered_model_buf = false;
    is_restart_field = false;
	is_field_active = false;
	define_order_count = -1;
    last_define_time = 0x7fffffffffffffff;
}


Field_mem_info::~Field_mem_info()
{
    if (is_registered_model_buf)
        grided_field_data->get_grid_data_field()->data_buf = NULL;
    delete grided_field_data;
}


void Field_mem_info::reset_mem_buf(void * buf, bool is_restart_field)
{
    if (grided_field_data->get_grid_data_field()->data_buf != NULL)
        delete [] grided_field_data->get_grid_data_field()->data_buf;
    EXECUTION_REPORT(REPORT_ERROR, !is_registered_model_buf, "model buf has been registered twice");

    grided_field_data->get_grid_data_field()->data_buf = buf;
    is_registered_model_buf = true;
    this->is_restart_field = is_restart_field;
}


void Field_mem_info::change_datatype_to_double()
{
    grided_field_data->change_datatype_in_application(DATA_TYPE_DOUBLE);
}


void Field_mem_info::define_field_values(bool is_restarting)
{
	if (!is_restarting)
		is_field_active = true;
    last_define_time = timer_mgr->get_current_full_time();
}


void Field_mem_info::use_field_values()
{	
    if (is_registered_model_buf) 
        return;
    
    if (last_define_time == timer_mgr->get_current_full_time())
        return;

    EXECUTION_REPORT(REPORT_ERROR, last_define_time != 0x7fffffffffffffff, "field %s %s is used before define it\n", field_name, decomp_name);
    EXECUTION_REPORT(REPORT_ERROR, last_define_time <= timer_mgr->get_current_full_time(), "C-Coupler error in set_use_field\n");
    is_restart_field = true;
}


bool Field_mem_info::match_field_mem(const char *comp_name, 
                                    const char *decomp_name, 
                                    const char *grid_name,
                                    const char *field_name, 
                                    const int buf_type)
{
    if (!words_are_the_same(decomp_name, "NULL")) {
        decomps_info_mgr->search_decomp_info(decomp_name);
        EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "C-Coupler software error: grid name %s is ilegal when searching field\n", grid_name);
        EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(decomp_name)->get_grid_name())->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(grid_name)),
                     "C-Coupler software error: the grid of decomp %s is not a subset of grid %s when searching field\n", decomp_name, grid_name);
    }
    else EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(grid_name, "NULL"), "C-Coupler software error: the grid name must be \"NULL\" if the decomp name is \"NULL\" when searching field\n");
    if (words_are_the_same(this->comp_name, comp_name) &&
        words_are_the_same(this->decomp_name, decomp_name) &&
        words_are_the_same(this->field_name, field_name)) {
           EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(this->grid_name, grid_name), "conflict in matching grid of field %s: there are two grids (%s and %s) for the same field\n", field_name, grid_name, this->grid_name);
        if (this->buf_type == buf_type)
            return true;
    }

    return false;
}


bool Field_mem_info::match_field_mem(const char *comp_name, 
                                    const char *decomp_name, 
                                    const char *grid_name,
                                    const char *field_name,
                                    const char *data_type,
                                    const int buf_type)
{
    if (!words_are_the_same(decomp_name, "NULL")) {
        decomps_info_mgr->search_decomp_info(decomp_name);
        EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "C-Coupler software error: grid name %s is ilegal when searching field\n", grid_name);
        EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(decomp_name)->get_grid_name())->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(grid_name)),
                     "C-Coupler software error: the grid of decomp %s is not a subset of grid %s when searching field\n", decomp_name, grid_name);
    }
    else EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(grid_name, "NULL"), "C-Coupler software error: the grid name must be \"NULL\" if the decomp name is \"NULL\" when searching field\n");
    if (words_are_the_same(this->comp_name, comp_name) &&
        words_are_the_same(this->decomp_name, decomp_name) &&
        words_are_the_same(this->field_name, field_name) &&
        words_are_the_same(this->get_field_data()->get_grid_data_field()->data_type_in_application, data_type)) {
        EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(this->grid_name, grid_name), "conflict in matching grid of field %s: there are two grids (%s and %s) for the same field\n", field_name, grid_name, this->grid_name);
        if (this->buf_type == buf_type)
            return true;
    }

    return false;
}


bool Field_mem_info::match_field_mem(void *data_buffer)
{
    return this->get_data_buf() == data_buffer;
}


void Field_mem_info::get_field_mem_full_name(char *full_name)
{
    sprintf(full_name, "%s_%s_%s_%d", comp_name, decomp_name, field_name, buf_type);
}


void Field_mem_info::reset_field_name(const char *new_name)
{
    strcpy(field_name, new_name);
}


void Field_mem_info::calculate_field_conservative_sum(Field_mem_info *area_field)
{
#ifdef DEBUG_CCPL
	double partial_sum, total_sum;
    long size;

	EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE), "C-Coupler error in calculate_field_sum");
    size = get_field_data()->get_grid_data_field()->required_data_size;
    partial_sum = 0;
    for (long j = 0; j < size; j ++)
        partial_sum += (((double*) get_data_buf())[j])*(((double*) area_field->get_data_buf())[j]);
	MPI_Allreduce(&partial_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, compset_communicators_info_mgr->get_current_comp_comm_group());
    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
		printf("float sum of field (%s %s) is %0.18lf vs %0.18lf\n", get_comp_name(), get_field_name(), partial_sum, total_sum);
    }
#endif
}


void Field_mem_info::check_field_sum()
{
#ifdef DEBUG_CCPL

    int partial_sum, total_sum;
    long size;


    size = get_data_type_size(get_field_data()->get_grid_data_field()->data_type_in_application)*get_field_data()->get_grid_data_field()->required_data_size/4;
    partial_sum = 0;
    for (long j = 0; j < size; j ++)
        partial_sum += (((int*) get_data_buf())[j]);
    MPI_Allreduce(&partial_sum, &total_sum, 1, MPI_INT, MPI_SUM, compset_communicators_info_mgr->get_current_comp_comm_group());
    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
        EXECUTION_REPORT(REPORT_LOG, true, "check sum of field (%s %s) is %x vs %x", get_comp_name(), get_field_name(), total_sum, partial_sum);

#endif
}


bool Field_mem_info::field_has_been_defined()
{
    return last_define_time != 0x7fffffffffffffff;
}


Memory_mgt::Memory_mgt(const char *model_mem_cfg_file)
{
    FILE *cfg_fp;
    char line[NAME_STR_SIZE];
    char *line_p;
    Registered_field_info *registered_field_info;
    
    
    EXECUTION_REPORT(REPORT_LOG, true, "model registered field info file is %s", model_mem_cfg_file);

	add_registered_field_info("input_orbYear", "NULL", "NULL");
	add_registered_field_info("input_orbEccen", "NULL", "NULL");
	add_registered_field_info("input_orbObliq", "NULL", "NULL");
	add_registered_field_info("input_orbObliqr", "NULL", "NULL");
	add_registered_field_info("input_orbMvelp", "NULL", "NULL");
	add_registered_field_info("input_orbMvelpp", "NULL", "NULL");
	add_registered_field_info("input_orbLambm0", "NULL", "NULL");

    if (words_are_the_same(model_mem_cfg_file, "NULL")) 
        return;
	
    cfg_fp = open_config_file(model_mem_cfg_file);
    while(get_next_line(line, cfg_fp)) {
        line_p = line;
        registered_field_info = new Registered_field_info;
        get_next_attr(registered_field_info->field_name, &line_p);
        get_next_attr(registered_field_info->decomp_name, &line_p);
        get_next_attr(registered_field_info->grid_name, &line_p);
		for (int i = 0; i < registered_fields_info.size(); i ++)
			if (words_are_the_same(registered_fields_info[i]->field_name, registered_field_info->field_name) && words_are_the_same(registered_fields_info[i]->decomp_name, registered_field_info->decomp_name))
				EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(registered_fields_info[i]->grid_name, registered_field_info->grid_name), 
				                 "field %s is registered in %s more than once, there are conflicts among the multiple registration", registered_field_info->field_name, model_mem_cfg_file);
        registered_fields_info.push_back(registered_field_info);
    }
    fclose(cfg_fp);

	field_define_order_counter = 0;
}


void Memory_mgt::add_registered_field_info(const char *field_name, const char *decomp_name, const char *grid_name)
{
	Registered_field_info *registered_field_info;


	registered_field_info = new Registered_field_info;
	strcpy(registered_field_info->field_name, field_name);
	strcpy(registered_field_info->decomp_name, decomp_name);
	strcpy(registered_field_info->grid_name, grid_name);
	registered_fields_info.push_back(registered_field_info);
}


Field_mem_info *Memory_mgt::alloc_mem(const char *comp_name, 
                               const char *decomp_name, 
                               const char *grid_name,
                               const char *field_name, 
                               const char *data_type,
                               const int buf_type,
                               bool use_full_grid,
                               bool is_input_field)
{
    Field_mem_info *field_mem, *pair_field;
    int i;
    bool find_field_in_cfg;


    EXECUTION_REPORT(REPORT_LOG, true, "allocate new memory for field (%s %s %s %d)", comp_name, decomp_name, field_name, buf_type);

	field_define_order_counter ++;

	if (data_type == NULL) {
		EXECUTION_REPORT(REPORT_ERROR, is_input_field, "C-Coupler software error1 in alloc_mem of Memory_mgt");
		pair_field = search_last_define_field(comp_name, decomp_name, grid_name, field_name, buf_type);
		data_type = pair_field->get_field_data()->get_grid_data_field()->data_type_in_application;
	}
	else {
		get_data_type_size(data_type);
		if (is_input_field)
			search_last_define_field(comp_name, decomp_name, grid_name, field_name, buf_type);
	}
	
    if (!words_are_the_same(grid_name, "NULL")) {
        EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "%s is not a grid when allocating memory for field\n", grid_name);
        decomp_grids_mgr->search_decomp_grid_info(decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(grid_name));
    }

    /* If memory buffer has been allocated, return it */
    for (i = 0; i < fields_mem.size(); i ++) 
        if (fields_mem[i]->match_field_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type)) {
            EXECUTION_REPORT(REPORT_LOG, true, "field (%s %s %s %d) uses existing memory at address %lx", comp_name, decomp_name, field_name, buf_type, fields_mem[i]->get_data_buf());
			if (!is_input_field)
				fields_mem[i]->set_define_order_count(field_define_order_counter);
            return fields_mem[i];
        }

    /* Compute the size of the memory buffer and then allocate and return it */
    field_mem = new Field_mem_info(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, use_full_grid);
	if (!is_input_field)
		field_mem->set_define_order_count(field_define_order_counter);	
    fields_mem.push_back(field_mem);

    EXECUTION_REPORT(REPORT_LOG, true, "allocate new memory for field (%s %s %s %d) at address %lx", comp_name, decomp_name, field_name, buf_type, field_mem->get_data_buf());

    return field_mem;
}


Memory_mgt::~Memory_mgt()
{
    for (int i = 0; i < fields_mem.size(); i ++)
        delete fields_mem[i];
}


void Memory_mgt::register_model_data_buf(const char *model_data_decomp_name, const char *model_data_field_name, const char *data_type, void *model_data_buffer, const char *grid_name, void *fill_value, bool is_restart_field)
{
    Field_mem_info *field_mem;
    bool find_field_in_cfg;
    int i, j;
	const char *local_grid_name;


	field_define_order_counter ++;

	if (words_are_the_same(grid_name, "none")) {
	    find_field_in_cfg = false;
	    for (i = 0; i < registered_fields_info.size(); i ++) {
	        if (words_are_the_same(model_data_field_name, registered_fields_info[i]->field_name) &&
	            words_are_the_same(model_data_decomp_name, registered_fields_info[i]->decomp_name)) {
	            find_field_in_cfg = true;
	            break;
	        }
	    }
	    EXECUTION_REPORT(REPORT_ERROR, find_field_in_cfg, "field (%s %s) registered by model %s is not a legal field (not in field_buf_register.cfg)\n",
	                 model_data_field_name, model_data_decomp_name, compset_communicators_info_mgr->get_current_comp_name());
		local_grid_name = registered_fields_info[i]->grid_name;
	}
	else local_grid_name = grid_name;
	
	if (!words_are_the_same(local_grid_name, "NULL")) {
		EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(local_grid_name) != NULL, "%s is not a grid when registering data buffer\n", local_grid_name);
		decomp_grids_mgr->search_decomp_grid_info(model_data_decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(local_grid_name));
	}

    EXECUTION_REPORT(REPORT_LOG, true, "register new memory for field (%s %s %s %d) at address %lx", model_data_decomp_name, model_data_field_name, local_grid_name, 0, model_data_buffer);
	EXECUTION_REPORT(REPORT_ERROR, search_registerred_field(compset_communicators_info_mgr->get_current_comp_name(), model_data_decomp_name, local_grid_name, model_data_field_name, 0) == NULL,
					 "field (name=%s, decomposition=%s, grid=%s has been registerred more than once by component %s)", model_data_field_name, model_data_decomp_name, local_grid_name, compset_communicators_info_mgr->get_current_comp_name());

    for (j = 0; j < fields_mem.size(); j ++)
        if (fields_mem[j]->match_field_mem(compset_communicators_info_mgr->get_current_comp_name(), model_data_decomp_name, local_grid_name, model_data_field_name, 0)) {
            field_mem = fields_mem[j];
            break;
        }
    if (j == fields_mem.size()) {
        field_mem = new Field_mem_info(compset_communicators_info_mgr->get_current_comp_name(), model_data_decomp_name, local_grid_name, model_data_field_name, data_type, 0, false);
        field_mem->reset_mem_buf(model_data_buffer, is_restart_field);
        fields_mem.push_back(field_mem);
    }
    else {
        EXECUTION_REPORT(REPORT_ERROR, !field_mem->get_is_registered_model_buf(), "field <%s,%s> has been registerred more than once in component %s\n", 
                         model_data_field_name, model_data_decomp_name, compset_communicators_info_mgr->get_current_comp_name());
        if (!(words_are_the_same(model_data_field_name, "lon") || words_are_the_same(model_data_field_name, "lat") || words_are_the_same(model_data_field_name, "mask") || words_are_the_same(model_data_field_name, "arear")))
            EXECUTION_REPORT(REPORT_ERROR, false, "field <%s,%s> has been used before model interface of %s registering it\n", 
                         model_data_field_name, model_data_decomp_name, compset_communicators_info_mgr->get_current_comp_name());
        field_mem->reset_mem_buf(model_data_buffer, is_restart_field);
    }

    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(field_mem->get_field_data()->get_grid_data_field()->data_type_in_application, data_type), 
                 "the data type of field %s registered by component %s is %s, which does not match the data type in field configuration table\n",
                 model_data_field_name, compset_communicators_info_mgr->get_current_comp_name(), data_type);

	field_mem->set_define_order_count(field_define_order_counter);
    if (fill_value != NULL)
        field_mem->get_field_data()->get_grid_data_field()->set_fill_value(fill_value);
}


void Memory_mgt::withdraw_model_data_buf(const char *model_data_decomp_name, const char *model_data_field_name, const char *grid_name)
{
    Field_mem_info *field_mem;
    bool find_field_in_cfg;
    int i, j;
	const char *local_grid_name;


	if (words_are_the_same(grid_name, "none")) {
	    find_field_in_cfg = false;
	    for (i = 0; i < registered_fields_info.size(); i ++) {
	        if (words_are_the_same(model_data_field_name, registered_fields_info[i]->field_name) &&
	            words_are_the_same(model_data_decomp_name, registered_fields_info[i]->decomp_name)) {
	            find_field_in_cfg = true;
	            break;
	        }
	    }
	    EXECUTION_REPORT(REPORT_ERROR, find_field_in_cfg, "field (%s %s) registered by model %s is not a legal field (not in field_buf_register.cfg)\n",
	                 model_data_field_name, model_data_decomp_name, compset_communicators_info_mgr->get_current_comp_name());
		local_grid_name = registered_fields_info[i]->grid_name;
	}
	else local_grid_name = grid_name;
	
    EXECUTION_REPORT(REPORT_ERROR, find_field_in_cfg, "field (%s %s) withdrawed by component %s is not a legal field (not in field_buf_register.cfg)\n", model_data_field_name, model_data_decomp_name, compset_communicators_info_mgr->get_current_comp_name());
    if (!words_are_the_same(local_grid_name, "NULL")) {
        EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(local_grid_name) != NULL, "%s is not a grid when registering data buffer\n", local_grid_name);
        decomp_grids_mgr->search_decomp_grid_info(model_data_decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(local_grid_name));
    }

    EXECUTION_REPORT(REPORT_LOG, true, "withdraw field (%s %s %s) at address %lx", model_data_decomp_name, model_data_field_name, local_grid_name, 0);

    for (j = 0; j < fields_mem.size(); j ++)
        if (fields_mem[j]->match_field_mem(compset_communicators_info_mgr->get_current_comp_name(), model_data_decomp_name, local_grid_name, model_data_field_name, 0)) {
            field_mem = fields_mem[j];
            break;
        }
    EXECUTION_REPORT(REPORT_ERROR, j < fields_mem.size(), "withdrawed field (%s %s %s) has not been registered\n", model_data_decomp_name, model_data_field_name);
    EXECUTION_REPORT(REPORT_ERROR, field_mem->get_is_registered_model_buf(), "withdrawed field (%s %s %s) has not been registered\n", model_data_decomp_name, model_data_field_name);
    
    fields_mem.erase(fields_mem.begin()+j);
    delete field_mem;
}


Field_mem_info *Memory_mgt::search_registerred_field(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, int buf_count)
{
    for (int i = 0; i < fields_mem.size(); i ++) 
		if (fields_mem[i]->get_is_registered_model_buf() && fields_mem[i]->match_field_mem(comp_name, decomp_name, grid_name, field_name, buf_count)) 
			return fields_mem[i];

	return NULL;
}


Field_mem_info *Memory_mgt::search_last_define_field(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, int buf_count)
{
	long found_field_index = -1, found_field_define_count = -1;

	
    for (int i = 0; i < fields_mem.size(); i ++) 
		if (fields_mem[i]->match_field_mem(comp_name, decomp_name, grid_name, field_name, buf_count)) {
			if (found_field_define_count < fields_mem[i]->get_define_order_count()) {
				found_field_index = i;
				found_field_define_count = fields_mem[i]->get_define_order_count();
			}
		}

	EXECUTION_REPORT(REPORT_ERROR, found_field_index != -1, "field (comp_name=%s, decomp_name=%s, grid_name=%s, field_name=%s, buf_count=%d) is not defined before using it",
		             comp_name, decomp_name, grid_name, field_name, buf_count);

	return fields_mem[found_field_index];
}


Field_mem_info *Memory_mgt::search_field_via_data_buf(const void *data_buf)
{
    for (int i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->get_data_buf() == data_buf)
            return fields_mem[i];

    EXECUTION_REPORT(REPORT_ERROR, false, "C-Coupler error in search_field_via_data_buf\n");
    return NULL;
}


void Memory_mgt::write_restart_fields()
{
    EXECUTION_REPORT(REPORT_ERROR, timer_mgr->check_is_coupled_run_restart_time(), "C-Coupler software error in Memory_mgt::write_restart_fields\n");
    for (int i = 0; i < fields_mem.size(); i ++) 
        if (fields_mem[i]->get_is_restart_field())
            restart_mgr->write_one_restart_field(fields_mem[i], 0);
}


void Memory_mgt::check_all_restart_fields_have_been_read()
{
    if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"))
        return;
    
    for (int i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->get_is_restart_field())
            EXECUTION_REPORT(REPORT_ERROR, fields_mem[i]->field_has_been_defined(), "restart field (%s %s %s) has not been read from restart data file\n",
                         fields_mem[i]->get_field_name(), fields_mem[i]->get_decomp_name(), fields_mem[i]->get_comp_name());
}


bool Memory_mgt::is_model_data_renewed_in_current_time_step(void *model_data_buffer)
{
    for (int i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->match_field_mem(model_data_buffer)) {
            return fields_mem[i]->get_last_define_time() == timer_mgr->get_current_full_time();
        }

    EXECUTION_REPORT(REPORT_ERROR, false, "address %lx is not starting address of any data buffer registerred by model", model_data_buffer);
    return false;
}


bool Memory_mgt::is_model_data_active_in_coupling(void *model_data_buffer)
{
    for (int i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->match_field_mem(model_data_buffer)) {
            return fields_mem[i]->check_is_field_active();
        }

    EXECUTION_REPORT(REPORT_ERROR, false, "address %lx is not starting address of any data buffer registerred by model", model_data_buffer);
    return false;
}


void Memory_mgt::check_sum_of_all_fields()
{
	for (int i = 0; i < fields_mem.size(); i ++)
		fields_mem[i]->check_field_sum();
}

