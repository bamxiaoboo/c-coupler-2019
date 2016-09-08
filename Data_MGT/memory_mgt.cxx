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

Field_mem_info *alloc_mem(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, const char *data_type, const int buf_type, bool is_input_field, const char *cfg_name)
{
    return memory_manager->alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, false, is_input_field, cfg_name);
}


Field_mem_info *alloc_mem(const char *field_name, int decomp_id, int comp_or_grid_id, int buf_mark, const char *data_type, const char *field_unit, const char *annotation)
{
	return memory_manager->alloc_mem(field_name, decomp_id, comp_or_grid_id, buf_mark, data_type, field_unit, annotation);
}


Field_mem_info *alloc_full_grid_mem(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, const char *data_type, const int buf_type, bool is_input_field, const char *cfg_name)
{
    return memory_manager->alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, true, is_input_field, cfg_name);
}
	

Field_mem_info::Field_mem_info(const char *field_name, int decomp_id, int comp_or_grid_id, 
	                           int buf_mark, const char *unit, const char *data_type, const char *annotation)
{
	int comp_id = -1, mem_size;
	Remap_grid_class *remap_grid_grid = NULL, *remap_grid_decomp = NULL;
	bool grid_match;
    Remap_data_field *remap_data_field;


	if (decomp_id == -1) {
		grid_match = true;
		comp_id = comp_or_grid_id;
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_or_grid_id), "Software error1 in new Field_mem_info");
		mem_size = get_data_type_size(data_type);
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR, -1, original_grid_mgr->is_grid_id_legal(comp_or_grid_id), "Software error2 in new Field_mem_info");
		EXECUTION_REPORT(REPORT_ERROR, -1, decomps_info_mgr->is_decomp_id_legal(decomp_id), "Software error3 in new Field_mem_info");
		comp_id = original_grid_mgr->get_comp_id_of_grid(comp_or_grid_id);
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_id == decomps_info_mgr->get_comp_id_of_decomp(decomp_id), 
			             "Software error4 in new Field_mem_info");
		
		remap_grid_decomp = decomps_info_mgr->get_CoR_grid_of_decomp(decomp_id);
		remap_grid_grid = original_grid_mgr->get_CoR_grid(comp_or_grid_id);
		grid_match = remap_grid_decomp->is_subset_of_grid(remap_grid_grid);
		mem_size = decomps_info_mgr->get_decomp_info(decomp_id)->get_num_local_cells() * get_data_type_size(data_type) * remap_grid_grid->get_grid_size()/remap_grid_decomp->get_grid_size();
	}
	EXECUTION_REPORT(REPORT_ERROR, comp_id, grid_match, "When registering an instance of coupling field of \"%s\", the parameters of grid ID and decomposition ID do not match each other: the grid corresponding to the decomposition should be a subset of the grid corresponding to the grid ID. Please check the model code with the annotation \"%s\"",
		             field_name, annotation);
	
	EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_info->search_field_info(field_name) != NULL,
		             "When trying to register an instance of a coupling field, the field name \"%s\" is unknown (has not been registered). Please check the model code with the annotation \"%s\"",
		             field_name, annotation);

	// check the field unit

	strcpy(this->field_name, field_name);
	this->decomp_id = decomp_id;
	this->comp_or_grid_id = comp_or_grid_id;
	this->buf_mark = buf_mark;
    this->buf_type = buf_type;
    is_registered_model_buf = false;
    is_restart_field = false;
	is_field_active = false;
	define_order_count = -1;
    last_define_time = 0x7fffffffffffffff;

    remap_data_field = new Remap_data_field;
    strcpy(remap_data_field->field_name_in_application, field_name);
    strcpy(remap_data_field->field_name_in_IO_file, field_name);
    strcpy(remap_data_field->data_type_in_application, data_type);
    remap_data_field->required_data_size = mem_size / get_data_type_size(data_type);
    remap_data_field->read_data_size = remap_data_field->required_data_size;
    remap_data_field->data_buf = new char [mem_size];
	EXECUTION_REPORT(REPORT_ERROR,-1, fields_info->get_field_long_name(field_name) != NULL, "Cannot register the field \"%s\" because its basic information has not been registered. Please check model code with the annotation \"%s\"", field_name, annotation);
    remap_data_field->set_field_long_name(fields_info->get_field_long_name(field_name));
    remap_data_field->set_field_unit(unit);   // to complete: when strlen(unit) is 0, use default unit of the field
    memset(remap_data_field->data_buf, 0, mem_size);

    if (decomp_id == -1)
		grided_field_data = new Remap_grid_data_class(NULL, remap_data_field);
    else {
        Remap_grid_class *decomp_grid = decomp_grids_mgr->search_decomp_grid_info(decomp_id, remap_grid_grid, false)->get_decomp_grid();
        grided_field_data = new Remap_grid_data_class(decomp_grid, remap_data_field);
        remap_data_field->set_fill_value(NULL);
    }	
}


Field_mem_info::Field_mem_info(const char *comp_name, 
                                  const char *decomp_name, 
                                  const char *grid_name,
                                  const char *field_name, 
                                  const char *data_type,
                                  const int buf_type,
                                  bool use_full_grid,
                                  const char *cfg_name)
{
    Remap_data_field *remap_data_field;
    Decomp_info *decomp_info;
    Remap_grid_class *decomp_grid;
    long mem_size;
    

	if (!words_are_the_same(grid_name, "NULL"))
		EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "\"%s\" is not a legal grid when allocating memory for the field instance of name \"%s\". Please check the configuration file \"%s\"", grid_name, field_name, cfg_name);
    if (words_are_the_same(decomp_name, "NULL")) {
        mem_size = get_data_type_size(data_type);
		if (!words_are_the_same(grid_name, "NULL"))
			mem_size *= remap_grid_manager->search_remap_grid_with_grid_name(grid_name)->get_grid_size();
    }
    else {
        decomp_info = decomps_info_mgr->search_decomp_info(decomp_name);
        mem_size = decomp_info->get_num_local_cells() * get_data_type_size(data_type) *
                   remap_grid_manager->search_remap_grid_with_grid_name(grid_name)->get_grid_size()/remap_grid_manager->search_remap_grid_with_grid_name(decomp_info->get_grid_name())->get_grid_size();
		EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(decomp_info->get_grid_name())->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(grid_name)),
						 "Grid \"%s\" and parallel decomposition \"%s\" do not match with each other when allocating memory for the field instance of name \"%s\". Please check the configuration file \"%s\"", grid_name, decomp_name, field_name, cfg_name);
    }

    remap_data_field = new Remap_data_field;
    strcpy(remap_data_field->field_name_in_application, field_name);
    strcpy(remap_data_field->field_name_in_IO_file, field_name);
    strcpy(remap_data_field->data_type_in_application, data_type);
    remap_data_field->required_data_size = mem_size / get_data_type_size(data_type);
    remap_data_field->read_data_size = remap_data_field->required_data_size;
    remap_data_field->data_buf = new char [mem_size];
	EXECUTION_REPORT(REPORT_ERROR,-1, fields_info->get_field_long_name(field_name) != NULL, "The attributes of field \"%s\" has not been defined. Please check the configuration file \"%s\"", field_name, cfg_name);
    remap_data_field->set_field_long_name(fields_info->get_field_long_name(field_name));
    remap_data_field->set_field_unit(fields_info->get_field_unit(field_name));
    memset(remap_data_field->data_buf, 0, mem_size);

    if (words_are_the_same(decomp_name, "NULL"))
		if (words_are_the_same(grid_name, "NULL"))
			grided_field_data = new Remap_grid_data_class(NULL, remap_data_field);
		else grided_field_data = new Remap_grid_data_class(remap_grid_manager->search_remap_grid_with_grid_name(grid_name), remap_data_field);
    else {
        if (use_full_grid)
            decomp_grid = remap_grid_manager->search_remap_grid_with_grid_name(grid_name);
        else decomp_grid = decomp_grids_mgr->search_decomp_grid_info(decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(grid_name), false)->get_decomp_grid();
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
	
    grided_field_data->get_grid_data_field()->data_buf = buf;
    is_registered_model_buf = true;
    this->is_restart_field = is_restart_field;
	printf("label registered buff %lx\n", this);
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


void Field_mem_info::use_field_values(const char *cfg_name)
{	
    if (is_registered_model_buf) 
        return;
    
    if (last_define_time == timer_mgr->get_current_full_time())
        return;

    EXECUTION_REPORT(REPORT_ERROR,-1, last_define_time != 0x7fffffffffffffff, "field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\", bufmark=%d) is used before define it. Please check the configuration file %s", field_name, decomp_name, grid_name, buf_type, cfg_name);
    EXECUTION_REPORT(REPORT_ERROR,-1, last_define_time <= timer_mgr->get_current_full_time(), "C-Coupler error in set_use_field\n");
    is_restart_field = true;
}


bool Field_mem_info::match_field_mem(const char *comp_name, 
                                    const char *decomp_name, 
                                    const char *grid_name,
                                    const char *field_name, 
                                    const int buf_type,
                                    const char *cfg_name)
{
    if (!words_are_the_same(decomp_name, "NULL")) {
        decomps_info_mgr->search_decomp_info(decomp_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "C-Coupler software error: grid name %s is ilegal when searching field\n", grid_name);
		if (cfg_name != NULL)
	        EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(decomp_name)->get_grid_name())->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(grid_name)),
                             "The grid of decomp \"%s\" is not a subset of grid \"%s\" when registerring, allocating or searching field \"%s\". Please verify the configuration file %s", decomp_name, grid_name, field_name, cfg_name);
		else EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(decomp_name)->get_grid_name())->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(grid_name)),
                             "The grid of decomp \"%s\" is not a subset of grid \"%s\" when registerring, allocating or searching field \"%s\". Please check the corresponding model code.", decomp_name, grid_name, field_name);
    }
    if (words_are_the_same(this->comp_name, comp_name) &&
        words_are_the_same(this->decomp_name, decomp_name) &&
        words_are_the_same(this->field_name, field_name)) {
//        EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(this->grid_name, grid_name), "conflict in matching grid of field %s: there are two grids (%s and %s) for the same field\n", field_name, grid_name, this->grid_name);
    }

	if (words_are_the_same(this->comp_name, comp_name) &&
		words_are_the_same(this->decomp_name, decomp_name) &&
		words_are_the_same(this->field_name, field_name) &&
		words_are_the_same(this->grid_name, grid_name) && 
		this->buf_type == buf_type)
			return true;

    return false;
}


bool Field_mem_info::match_field_mem(const char *comp_name, 
                                    const char *decomp_name, 
                                    const char *grid_name,
                                    const char *field_name,
                                    const char *data_type,
                                    const int buf_type,
                                    const char *cfg_name)
{
    if (!words_are_the_same(decomp_name, "NULL")) {
        decomps_info_mgr->search_decomp_info(decomp_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "C-Coupler software error: grid name %s is ilegal when searching field\n", grid_name);
		if (cfg_name != NULL)
	        EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(decomp_name)->get_grid_name())->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(grid_name)),
                             "The grid of decomp \"%s\" is not a subset of grid \"%s\" when registerring, allocating or searching field \"%s\". Please verify the configuration file %s", decomp_name, grid_name, field_name, cfg_name);
		else EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(decomp_name)->get_grid_name())->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(grid_name)),
                             "The grid of decomp \"%s\" is not a subset of grid \"%s\" when registerring, allocating or searching field \"%s\". Please check the corresponding model code.", decomp_name, grid_name, field_name);
    }
    if (words_are_the_same(this->comp_name, comp_name) &&
        words_are_the_same(this->decomp_name, decomp_name) &&
        words_are_the_same(this->field_name, field_name) &&
        words_are_the_same(this->get_field_data()->get_grid_data_field()->data_type_in_application, data_type)) {
//        EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(this->grid_name, grid_name), "conflict in matching grid of field %s: there are two grids (%s and %s) for the same field\n", field_name, grid_name, this->grid_name);
    }

	if (words_are_the_same(this->comp_name, comp_name) &&
		words_are_the_same(this->decomp_name, decomp_name) &&
		words_are_the_same(this->field_name, field_name) &&
		words_are_the_same(this->get_field_data()->get_grid_data_field()->data_type_in_application, data_type) &&
		words_are_the_same(this->grid_name, grid_name) && 
		this->buf_type == buf_type)
			return true;

    return false;
}


bool Field_mem_info::match_field_instance(const char *field_name, int decomp_id, int comp_or_grid_id, int buf_mark)
{
    return words_are_the_same(this->field_name, field_name) && this->decomp_id == decomp_id && this->comp_or_grid_id == comp_or_grid_id && this->buf_mark == buf_mark;
}


bool Field_mem_info::match_field_mem(void *data_buffer)
{
    return this->get_data_buf() == data_buffer;
}


void Field_mem_info::get_field_mem_full_name(char *full_name)
{
    sprintf(full_name, "%s_%s_%s_%s_%d", comp_name, decomp_name, grid_name, field_name, buf_type);
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

	EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE), "C-Coupler error in calculate_field_sum");
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
        EXECUTION_REPORT(REPORT_LOG,-1, true, "check sum of field (%s %s) is %x vs %x", get_comp_name(), get_field_name(), total_sum, partial_sum);

#endif
}


bool Field_mem_info::field_has_been_defined()
{
    return last_define_time != 0x7fffffffffffffff;
}


long Field_mem_info::get_size_of_field()
{
	return grided_field_data->get_grid_data_field()->read_data_size;
}


int Field_mem_info::get_comp_id()
{
	if (decomp_id == -1)
		return comp_or_grid_id;
	return original_grid_mgr->get_comp_id_of_grid(comp_or_grid_id);
}


int Field_mem_info::get_grid_id() 
{
	EXECUTION_REPORT(REPORT_ERROR, -1, decomp_id != -1, "Software error is reported in Field_mem_info::get_grid_id");
	return comp_or_grid_id; 
}


void Field_mem_info::set_field_instance_id(int field_instance_id, const char *annotation)
{
	this->field_instance_id = field_instance_id;
	annotation_mgr->add_annotation(field_instance_id, "allocate field instance", annotation);
}


Memory_mgt::Memory_mgt(const char *model_mem_cfg_file)
{
    FILE *cfg_fp;
    char line[NAME_STR_SIZE];
    char *line_p;
    Registered_field_info *registered_field_info;
	int i = 0;
    
    
    EXECUTION_REPORT(REPORT_LOG,-1, true, "model registered field info file is %s", model_mem_cfg_file);
	strcpy(field_register_cfg_file, model_mem_cfg_file);

    if (words_are_the_same(model_mem_cfg_file, "NULL")) 
        return;
	
    cfg_fp = open_config_file(model_mem_cfg_file);
    while(get_next_line(line, cfg_fp)) {
        line_p = line;
		i ++;
        registered_field_info = new Registered_field_info;
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(registered_field_info->field_name, &line_p), "Please specify the name of the %dth registered field instance by component \"%s\" in the configuration file \"%s\".",
						 i, compset_communicators_info_mgr->get_current_comp_name(), model_mem_cfg_file);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(registered_field_info->decomp_name, &line_p), "Please specify the parallel decomposition of the %dth registered field instance by component \"%s\" in the configuration file \"%s\".",
						 i, compset_communicators_info_mgr->get_current_comp_name(), model_mem_cfg_file);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(registered_field_info->grid_name, &line_p), "Please specify the grid of the %dth registered field instance by component \"%s\" in the configuration file \"%s\".",
						 i, compset_communicators_info_mgr->get_current_comp_name(), model_mem_cfg_file);
		for (int j = 0; j < registered_fields_info.size(); j ++)
			if (words_are_the_same(registered_fields_info[j]->field_name, registered_field_info->field_name) && words_are_the_same(registered_fields_info[j]->decomp_name, registered_field_info->decomp_name))
				EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(registered_fields_info[j]->grid_name, registered_field_info->grid_name),
				                 "field \"%s\" is registered by the configuration file \"%s\" more than once. There are conflicts among the multiple registrations.", 
				                 registered_field_info->field_name, model_mem_cfg_file);
        registered_fields_info.push_back(registered_field_info);
    }
    fclose(cfg_fp);

	add_registered_field_info("input_orbYear", "NULL", "NULL");
	add_registered_field_info("input_orbEccen", "NULL", "NULL");
	add_registered_field_info("input_orbObliq", "NULL", "NULL");
	add_registered_field_info("input_orbObliqr", "NULL", "NULL");
	add_registered_field_info("input_orbMvelp", "NULL", "NULL");
	add_registered_field_info("input_orbMvelpp", "NULL", "NULL");
	add_registered_field_info("input_orbLambm0", "NULL", "NULL");

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


void Memory_mgt::add_field_instance(Field_mem_info *field_instance, const char *cfg_name)
{
	for (int i = 0; i < fields_mem.size(); i ++)
		if (words_are_the_same(field_instance->get_field_name(), fields_mem[i]->get_field_name())) {
			bool check_right = false;
			if (words_are_the_same(field_instance->get_grid_name(), "NULL") || words_are_the_same(fields_mem[i]->get_grid_name(), "NULL"))
				check_right = words_are_the_same(field_instance->get_grid_name(), "NULL") && words_are_the_same(fields_mem[i]->get_grid_name(), "NULL");
			else check_right = remap_grid_manager->search_remap_grid_with_grid_name(field_instance->get_grid_name())->get_num_dimensions() == remap_grid_manager->search_remap_grid_with_grid_name(fields_mem[i]->get_grid_name())->get_num_dimensions();
			if (cfg_name != NULL)
				EXECUTION_REPORT(REPORT_ERROR,-1, check_right,
								 "An instance of field \"%s\" has been allocated but its grid \"%s\" conflicts with the grid \"%s\" of the current field instance (The dimension numbers of the two grids are different). Please check the configuration file \"%s\".", 
								 field_instance->get_field_name(), fields_mem[i]->get_grid_name(), field_instance->get_grid_name(), cfg_name);
			else EXECUTION_REPORT(REPORT_ERROR,-1, check_right,
								  "An instance of field \"%s\" has been allocated but its grid \"%s\" conflicts with the grid \"%s\" of the current field instance (The dimension numbers of the two grids are different). Please check the model code of the component \"%s\".", 
								  field_instance->get_field_name(), fields_mem[i]->get_grid_name(), field_instance->get_grid_name(), field_instance->get_comp_name());
		}
	fields_mem.push_back(field_instance);
}


Field_mem_info *Memory_mgt::alloc_mem(const char *field_name, int decomp_id, int comp_or_grid_id, int buf_mark, const char *data_type, const char *field_unit, const char *annotation)
{
    Field_mem_info *field_mem, *pair_field;
    int i, comp_id;
    bool find_field_in_cfg;


	EXECUTION_REPORT(REPORT_ERROR, -1, buf_mark < 0, "Software error in Memory_mgt::alloc_mem: wrong value of buffer mark");
	EXECUTION_REPORT(REPORT_ERROR, -1, fields_info->search_field_info(field_name) != NULL, "Software error in Memory_mgt::alloc_mem: field name is undefined");
	EXECUTION_REPORT(REPORT_ERROR, -1, data_type != NULL, "Software error in Memory_mgt::alloc_mem: data type is NULL");
	get_data_type_size(data_type);
	if (decomp_id != -1) {
		EXECUTION_REPORT(REPORT_ERROR, -1, decomps_info_mgr->is_decomp_id_legal(decomp_id), "Software error in Memory_mgt::alloc_mem: wrong decomposition id");
		EXECUTION_REPORT(REPORT_ERROR, -1, original_grid_mgr->is_grid_id_legal(comp_or_grid_id), "Software error in Memory_mgt::alloc_mem: wrong grid id");
		comp_id = original_grid_mgr->search_grid_info(comp_or_grid_id)->get_comp_id();
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_or_grid_id), "Software error in Memory_mgt::alloc_mem: wrong component id");
		comp_id = comp_or_grid_id;
	}

	

    /* If memory buffer has been allocated, return it */
    for (i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->match_field_instance(field_name, decomp_id, comp_or_grid_id, buf_mark)) {
            // EXECUTION_REPORT(REPORT_ERROR, comp_id, field_unitһ��, ...);
			// EXECUTION_REPORT(REPORT_ERROR, comp_id, data typeһ��, ...);
            return fields_mem[i];
        }

    /* Compute the size of the memory buffer and then allocate and return it */
    field_mem = new Field_mem_info(field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, data_type, annotation);
	field_mem->set_field_instance_id(TYPE_FIELD_INST_ID_PREFIX|fields_mem.size(), annotation);
	fields_mem.push_back(field_mem);

    return field_mem;
}


Field_mem_info *Memory_mgt::alloc_mem(const char *comp_name, 
                               const char *decomp_name, 
                               const char *grid_name,
                               const char *field_name, 
                               const char *data_type,
                               const int buf_type,
                               bool use_full_grid,
                               bool is_input_field,
                               const char *cfg_name)
{
    Field_mem_info *field_mem, *pair_field;
    int i;
    bool find_field_in_cfg;


    EXECUTION_REPORT(REPORT_LOG,-1, true, "allocate new memory for field (%s %s %s %d)", comp_name, decomp_name, field_name, buf_type);

	field_define_order_counter ++;

	compset_communicators_info_mgr->get_comp_id_by_comp_name(comp_name);

	if (data_type == NULL) {
		EXECUTION_REPORT(REPORT_ERROR,-1, is_input_field, "C-Coupler software error1 in alloc_mem of Memory_mgt");
		pair_field = search_last_define_field(comp_name, decomp_name, grid_name, field_name, buf_type, true, cfg_name);
		data_type = pair_field->get_field_data()->get_grid_data_field()->data_type_in_application;
	}
	else {
		get_data_type_size(data_type);
		if (is_input_field)
			search_last_define_field(comp_name, decomp_name, grid_name, field_name, buf_type, true, cfg_name);
	}
	
    if (!words_are_the_same(grid_name, "NULL")) {
        EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "\"%s\" has not been defined as a grid when allocating memory for the corresponding field instance. Please check the configuration file \"%s\"", grid_name, cfg_name);
        decomp_grids_mgr->search_decomp_grid_info(decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(grid_name),false);
    }

    /* If memory buffer has been allocated, return it */
    for (i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->match_field_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, cfg_name)) {
            EXECUTION_REPORT(REPORT_LOG,-1, true, "field (%s %s %s %d) uses existing memory at address %lx", comp_name, decomp_name, field_name, buf_type, fields_mem[i]->get_data_buf());
			if (!is_input_field)
				fields_mem[i]->set_define_order_count(field_define_order_counter);
            return fields_mem[i];
        }

    /* Compute the size of the memory buffer and then allocate and return it */
    field_mem = new Field_mem_info(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, use_full_grid, cfg_name);
	if (!is_input_field)
		field_mem->set_define_order_count(field_define_order_counter);	
    add_field_instance(field_mem, cfg_name);

    EXECUTION_REPORT(REPORT_LOG,-1, true, "allocate new memory for field (%s %s %s %d) at address %lx", comp_name, decomp_name, field_name, buf_type, field_mem->get_data_buf());

    return field_mem;
}


Memory_mgt::~Memory_mgt()
{
    for (int i = 0; i < fields_mem.size(); i ++)
        delete fields_mem[i];
}


void Memory_mgt::export_field_data(void *model_buf, const char *field_name, const char *decomp_name, const char *grid_name, const char *data_type, int data_size)
{
	Field_mem_info *field_instance;


	if (!words_are_the_same(decomp_name,"NULL"))
		field_instance = search_last_define_field(decomps_info_mgr->search_decomp_info(decomp_name)->get_model_name(), decomp_name, grid_name, field_name, 0, false, NULL);
	else field_instance = search_last_define_field(compset_communicators_info_mgr->get_current_comp_name(), decomp_name, grid_name, field_name, 0, false, NULL);
	EXECUTION_REPORT(REPORT_ERROR,-1, field_instance != NULL, "the field instance (\"%s\", \"%s\", \"%s\") does not exist and cannot export it to a data buffer. Please check.", field_name, decomp_name, grid_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, field_instance->get_size_of_field() == data_size, "the size of the field instance (\"%s\", \"%s\", \"%s\") does not equal the size of the data buffer for exporting. Please verify.", field_name, decomp_name, grid_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(field_instance->get_field_data()->get_grid_data_field()->data_type_in_application, data_type), 
		             "the data type of the field instance (\"%s\", \"%s\", \"%s\") is \"%s\", different from the data type of the data buffer for exporting. Please verify.", 
		             field_name, decomp_name, grid_name, field_instance->get_field_data()->get_grid_data_field()->data_type_in_application);

	memcpy(model_buf, field_instance->get_data_buf(), get_data_type_size(data_type)*field_instance->get_size_of_field());
}


void Memory_mgt::register_model_data_buf(const char *model_data_decomp_name, const char *model_data_field_name, const char *data_type, void *model_data_buffer, const char *grid_name, void *fill_value, bool is_restart_field, int data_size)
{
    Field_mem_info *field_mem;
    bool find_field_in_cfg;
    int i, j;
	char local_grid_name[NAME_STR_SIZE];


	if (model_data_buffer != NULL) {
		field_mem = search_field_via_data_buf(model_data_buffer, false);
		if (field_mem != NULL)
			EXECUTION_REPORT(REPORT_WARNING, -1, words_are_the_same(field_mem->get_decomp_name(), model_data_decomp_name) && words_are_the_same(field_mem->get_field_name(), model_data_field_name) && words_are_the_same(field_mem->get_grid_name(), grid_name),
							 "Model data buffer currently registered for field (name=\"%s\", grid=\"%s\", decomp=\"%s\") has been registered for field (name=\"%s\", grid=\"%s\", decomp=\"%s\")",
							 model_data_field_name, grid_name, model_data_decomp_name, field_mem->get_field_name(), field_mem->get_grid_name(), field_mem->get_decomp_name());
	}
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
		if (!words_are_the_same(model_data_decomp_name,"NULL")) {
	    	EXECUTION_REPORT(REPORT_ERROR,-1, find_field_in_cfg, "field (%s %s) registered by model %s is not a legal field (not in the configuration file \"%s\". Please check the model code of component \"%s\".",
	        		         model_data_field_name, model_data_decomp_name, decomps_info_mgr->search_decomp_info(model_data_decomp_name)->get_model_name(), field_register_cfg_file, decomps_info_mgr->search_decomp_info(model_data_decomp_name)->get_model_name());
			strcpy(local_grid_name, registered_fields_info[i]->grid_name);
		}
		else strcpy(local_grid_name, "NULL");
	}
	else strcpy(local_grid_name, grid_name);
	
	if (!words_are_the_same(local_grid_name, "NULL")) {
		EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(local_grid_name) != NULL, "\"%s\" has not been defined as a grid when allocating memory for the corresponding field instance. Please check the model code of component \"%s\".", 
			             grid_name, decomps_info_mgr->search_decomp_info(model_data_decomp_name)->get_model_name());
		decomp_grids_mgr->search_decomp_grid_info(model_data_decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(local_grid_name), false);
	}

    EXECUTION_REPORT(REPORT_LOG,-1, true, "register new memory for field (%s %s %s %d) at address %lx", model_data_decomp_name, model_data_field_name, local_grid_name, 0, model_data_buffer);

	if (!words_are_the_same(model_data_decomp_name,"NULL"))	
		EXECUTION_REPORT(REPORT_ERROR,-1, search_registerred_field(decomps_info_mgr->search_decomp_info(model_data_decomp_name)->get_model_name(), model_data_decomp_name, local_grid_name, model_data_field_name, 0) == NULL,
						 "field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\") has been registerred more than once by component \"%s\"). Please check the model code. ", 
						 model_data_field_name, model_data_decomp_name, local_grid_name, decomps_info_mgr->search_decomp_info(model_data_decomp_name)->get_model_name());
	else EXECUTION_REPORT(REPORT_ERROR,-1, search_registerred_field(compset_communicators_info_mgr->get_current_comp_name(), model_data_decomp_name, local_grid_name, model_data_field_name, 0) == NULL,
						  "field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\") has been registerred more than once by component \"%s\"). Please check the model code. ", 
						  model_data_field_name, model_data_decomp_name, local_grid_name, compset_communicators_info_mgr->get_current_comp_name());

    for (j = 0; j < fields_mem.size(); j ++) {
		if (!words_are_the_same(model_data_decomp_name,"NULL"))	{
	        if (fields_mem[j]->match_field_mem(decomps_info_mgr->search_decomp_info(model_data_decomp_name)->get_model_name(), model_data_decomp_name, local_grid_name, model_data_field_name, data_type, 0, NULL)) {
    	        field_mem = fields_mem[j];
        	    break;
        	}
		}
		else {
	        if (fields_mem[j]->match_field_mem(compset_communicators_info_mgr->get_current_comp_name(), model_data_decomp_name, local_grid_name, model_data_field_name, data_type, 0, NULL)) {
    	        field_mem = fields_mem[j];
        	    break;
        	}
		}
    }
    if (j == fields_mem.size()) {
		if (!words_are_the_same(model_data_decomp_name,"NULL"))
	        field_mem = new Field_mem_info(decomps_info_mgr->search_decomp_info(model_data_decomp_name)->get_model_name(), model_data_decomp_name, local_grid_name, model_data_field_name, data_type, 0, false, field_register_cfg_file);
		else field_mem = new Field_mem_info(compset_communicators_info_mgr->get_current_comp_name(), model_data_decomp_name, local_grid_name, model_data_field_name, data_type, 0, false, field_register_cfg_file);
		if (field_mem->get_size_of_field() != 0)
			EXECUTION_REPORT(REPORT_ERROR,-1, model_data_buffer != NULL,
					 		 "Model data buffer currently registered for field (name=\"%s\", grid=\"%s\", decomp=\"%s\") is not allocated by the model program",
					 		 model_data_field_name, grid_name, model_data_decomp_name);	
        field_mem->reset_mem_buf(model_data_buffer, is_restart_field);
        add_field_instance(field_mem, NULL);
    }
    else {
        EXECUTION_REPORT(REPORT_ERROR,-1, !field_mem->get_is_registered_model_buf(), "field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\") has been registerred more than once by component \"%s\"). Please check the model code. ",
                         model_data_field_name, model_data_decomp_name, local_grid_name, compset_communicators_info_mgr->get_current_comp_name());
        if (!(words_are_the_same(model_data_field_name, "lon") || words_are_the_same(model_data_field_name, "lat") || words_are_the_same(model_data_field_name, "mask") || words_are_the_same(model_data_field_name, "arear")))
            EXECUTION_REPORT(REPORT_ERROR,-1, false, "field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\") has been used before registering it. Please check the model code of component \"%s\"", 
                         model_data_field_name, model_data_decomp_name, local_grid_name, compset_communicators_info_mgr->get_current_comp_name());
		if (field_mem->get_size_of_field() != 0)
			EXECUTION_REPORT(REPORT_ERROR,-1, model_data_buffer != NULL,
					 		 "Model data buffer currently registered for field (name=\"%s\", grid=\"%s\", decomp=\"%s\") is not allocated by the model program",
					 		 model_data_field_name, grid_name, model_data_decomp_name);	
        field_mem->reset_mem_buf(model_data_buffer, is_restart_field);
    }

	field_mem->set_define_order_count(field_define_order_counter);
    if (fill_value != NULL)
        field_mem->get_field_data()->get_grid_data_field()->set_fill_value(fill_value);

/*
	if (field_mem->get_size_of_field() > 0)
		EXECUTION_REPORT(REPORT_ERROR,-1, data_size == field_mem->get_size_of_field(), 
			             "The size of the data buffer registered by the component model for field (field_name=\"%s\", decomposition=\"%s\", grid=\"%s\") is %d, which is not consistent with the size (%d) determined by the decomposition and grid\n", model_data_field_name, model_data_decomp_name, grid_name, data_size, field_mem->get_size_of_field());
*/

	if (field_mem->get_size_of_field() > 0)
		EXECUTION_REPORT(REPORT_ERROR,-1, model_data_buffer != NULL, "The data buffer for registered field (field_name=\"%s\", decomposition=\"%s\", grid=\"%s\") may not have been allocated. Please check the corresponding model code\n", model_data_field_name, model_data_decomp_name, grid_name);
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
	    EXECUTION_REPORT(REPORT_ERROR,-1, find_field_in_cfg, "field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\") that is registered by the component \"%s\" is not legal (not in the configuration file \"%s\").",
	                 model_data_field_name, model_data_decomp_name, grid_name, compset_communicators_info_mgr->get_current_comp_name(), field_register_cfg_file);
		local_grid_name = registered_fields_info[i]->grid_name;
	}
	else local_grid_name = grid_name;
	
    EXECUTION_REPORT(REPORT_ERROR,-1, find_field_in_cfg, "field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\") withdrawn by the component \"%s\" is not legal (not in the configuration file \"%s\")", 
		             model_data_field_name, model_data_decomp_name, grid_name, compset_communicators_info_mgr->get_current_comp_name(), field_register_cfg_file);
    if (!words_are_the_same(local_grid_name, "NULL")) {
        EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(local_grid_name) != NULL, "\"%s\" has not been defined as a grid when withdrawing the field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\"). Please check the model code of the component \"%s\"", 
			             local_grid_name, model_data_field_name, model_data_decomp_name, grid_name, compset_communicators_info_mgr->get_current_comp_name());
        decomp_grids_mgr->search_decomp_grid_info(model_data_decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(local_grid_name), false);
    }

    EXECUTION_REPORT(REPORT_LOG,-1, true, "withdraw field (%s %s %s) at address %lx", model_data_decomp_name, model_data_field_name, local_grid_name, 0);

    for (j = 0; j < fields_mem.size(); j ++)
		if (!words_are_the_same(model_data_decomp_name,"NULL"))	{
	        if (fields_mem[j]->match_field_mem(decomps_info_mgr->search_decomp_info(model_data_decomp_name)->get_model_name(), model_data_decomp_name, local_grid_name, model_data_field_name, 0, NULL)) {
	            field_mem = fields_mem[j];
	            break;
	        }
		}
		else {
			if (fields_mem[j]->match_field_mem(compset_communicators_info_mgr->get_current_comp_name(), model_data_decomp_name, local_grid_name, model_data_field_name, 0, NULL)) {
	            field_mem = fields_mem[j];
	            break;
	        }
		}
    EXECUTION_REPORT(REPORT_ERROR,-1, j < fields_mem.size() || field_mem->get_is_registered_model_buf(), "The field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\") to be withdrawn has not been registered. Please check the model code of component \"%s\"", model_data_field_name, model_data_decomp_name, grid_name, compset_communicators_info_mgr->get_current_comp_name());
    
    fields_mem.erase(fields_mem.begin()+j);
    delete field_mem;
}


Field_mem_info *Memory_mgt::search_registerred_field(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, int buf_count)
{
    for (int i = 0; i < fields_mem.size(); i ++) 
		if (fields_mem[i]->get_is_registered_model_buf() && fields_mem[i]->match_field_mem(comp_name, decomp_name, grid_name, field_name, buf_count, NULL)) 
			return fields_mem[i];

	return NULL;
}


Field_mem_info *Memory_mgt::search_last_define_field(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, int buf_count, bool diag, const char *cfg_name)
{
	long found_field_index = -1, found_field_define_count = -1;

	
    for (int i = 0; i < fields_mem.size(); i ++) 
		if (fields_mem[i]->match_field_mem(comp_name, decomp_name, grid_name, field_name, buf_count, cfg_name)) {
			if (found_field_define_count < fields_mem[i]->get_define_order_count()) {
				found_field_index = i;
				found_field_define_count = fields_mem[i]->get_define_order_count();
			}
		}

	if (diag)
		EXECUTION_REPORT(REPORT_ERROR,-1, found_field_index != -1, "field instance (comp_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\", field_name=\"%s\", buf_count=%d) is not defined before using it. Please check the configuration file \"%s\"",
			             comp_name, decomp_name, grid_name, field_name, buf_count, cfg_name);

	if (found_field_index == -1)
		return NULL;
	
	return fields_mem[found_field_index];
}


Field_mem_info *Memory_mgt::search_field_via_data_buf(const void *data_buf, bool diag)
{
    for (int i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->get_data_buf() == data_buf)
            return fields_mem[i];

	if (diag)
	    EXECUTION_REPORT(REPORT_ERROR,-1, false, "C-Coupler error in search_field_via_data_buf\n");
    return NULL;
}


void Memory_mgt::write_restart_fields()
{
    EXECUTION_REPORT(REPORT_ERROR,-1, timer_mgr->check_is_coupled_run_restart_time(), "C-Coupler software error in Memory_mgt::write_restart_fields\n");
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
            EXECUTION_REPORT(REPORT_ERROR,-1, fields_mem[i]->field_has_been_defined(), "restart field instance (field_name=\"%s\", decomp_name=\"%s\", grid_name=\"%s\") has not been read from restart data file. Please check the corresponding restart data file",
                             fields_mem[i]->get_field_name(), fields_mem[i]->get_decomp_name(), fields_mem[i]->get_grid_name());
}


bool Memory_mgt::is_model_data_renewed_in_current_time_step(void *model_data_buffer)
{
    for (int i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->match_field_mem(model_data_buffer)) {
            return fields_mem[i]->get_last_define_time() == timer_mgr->get_current_full_time();
        }

    EXECUTION_REPORT(REPORT_ERROR,-1, false, "memory address %lx is not starting address of any data buffer registerred by model. Please check the model code of component \"%s\"", 
		             model_data_buffer, compset_communicators_info_mgr->get_current_comp_name());

    return false;
}


bool Memory_mgt::is_model_data_active_in_coupling(void *model_data_buffer)
{
    for (int i = 0; i < fields_mem.size(); i ++)
        if (fields_mem[i]->match_field_mem(model_data_buffer)) {
            return fields_mem[i]->check_is_field_active();
        }

    EXECUTION_REPORT(REPORT_ERROR,-1, false, "memory address %lx is not starting address of any data buffer registerred by model. Please check the model code of component \"%s\"", 
		             model_data_buffer, compset_communicators_info_mgr->get_current_comp_name());

    return false;
}


void Memory_mgt::check_sum_of_all_fields()
{
	for (int i = 0; i < fields_mem.size(); i ++)
		fields_mem[i]->check_field_sum();
}


int Memory_mgt::get_field_size(void *data_buf, const char *annotation)
{
	Field_mem_info *field = search_field_via_data_buf(data_buf, false);

	EXECUTION_REPORT(REPORT_ERROR,-1, field != NULL, "Detect a memory buffer that is not managed by C-Coupler. Please verify the model code according to annotation \"%s\"", annotation);

	return field->get_size_of_field();
}


Field_mem_info *Memory_mgt::search_field_instance(const char *field_name, int decomp_id, int comp_or_grid_id, int buf_mark)
{
	for (int i = 0; i < fields_mem.size(); i ++)
		if (fields_mem[i]->match_field_instance(field_name, decomp_id, comp_or_grid_id, buf_mark))
			return fields_mem[i];

	return NULL;
}


int Memory_mgt::register_external_field_instance(const char *field_name, void *data_buffer, int field_size, int decomp_id, int comp_or_grid_id, 
	                                             int buf_mark, const char *unit, const char *data_type, const char *annotation)
{
	int comp_id;
	Field_mem_info *existing_field_instance, *new_field_instance;

	
	if (decomp_id == -1) {
		comp_id = comp_or_grid_id;
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_or_grid_id), "The parameter of component ID for registering an instance of coupling field of \"%s\" is wrong. Please check the model code with the annotation \"%s\"",
			             field_name, annotation);
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR, -1, original_grid_mgr->is_grid_id_legal(comp_or_grid_id), 
			             "The parameter of grid ID for registering an instance of coupling field of \"%s\" is wrong. Please check the model code with the annotation \"%s\"",
			             field_name, annotation);
		EXECUTION_REPORT(REPORT_ERROR, -1, decomps_info_mgr->is_decomp_id_legal(decomp_id), 
						 "The parameter of decomposition ID for registering an instance of coupling field of \"%s\" is wrong. Please check the model code with the annotation \"%s\"",
			             field_name, annotation);
		comp_id = original_grid_mgr->get_comp_id_of_grid(comp_or_grid_id);
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_id == decomps_info_mgr->get_comp_id_of_decomp(decomp_id), 
			             "When registering an instance of coupling field of \"%s\", the parameters of grid ID and decomposition ID do not match each other: they belong to different components. Please check the model code with the annotation \"%s\"",
			             field_name, annotation);
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "Software error in Memory_mgt::register_external_field_instance: illegal component id from grid id");
	}
	synchronize_comp_processes_for_API(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", annotation);
	comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, annotation);
	check_API_parameter_string(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", field_name, "field name", annotation);
	check_API_parameter_string(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node")->get_comp_name(), "the component name specified by the corresponding ID", annotation);
	check_API_parameter_int(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", buf_mark, "the mark of the field instance", annotation);
	check_API_parameter_string(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", data_type, "the data type (such as integer, float, and double) of the field instance", annotation);
	check_API_parameter_string(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", unit, "the unit of the field instance", annotation);
	if (decomp_id == -1) {
		check_API_parameter_int(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", decomp_id, "the ID of the parallel decomposition", annotation);
	}
	else {
		check_API_parameter_string(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", decomps_info_mgr->get_decomp_info(decomp_id)->get_decomp_name(), "the parallel decomposition name specified by the corresponding ID", annotation);
		check_API_parameter_string(comp_id, API_ID_FIELD_MGT_REG_FIELD_INST, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"C-Coupler code in register_external_field_instance for getting component management node"), "registering a field instance by a component", original_grid_mgr->get_name_of_grid(comp_or_grid_id), "the grid name specified by the corresponding ID", annotation);
	}

	EXECUTION_REPORT(REPORT_ERROR, comp_id, buf_mark >= 0, "When registering an instance of coupling field of \"%s\", the parameter of the mark of the field instance cannot be a negative integer. Please check the model code with the annotation \"%s\"",
			         field_name, annotation);

	existing_field_instance = search_field_instance(field_name, decomp_id, comp_or_grid_id, buf_mark);
	if (existing_field_instance != NULL)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "Cannot register an instance of coupling field of \"%s\" again (the corresponding annotation is \"%s\") because this field instance has been registered before (the corresponding annotation is \"%s\")", 
						 field_name, annotation, annotation_mgr->get_annotation(existing_field_instance->get_field_instance_id(), "allocate field instance"));

	new_field_instance = new Field_mem_info(field_name, decomp_id, comp_or_grid_id, 
	                           buf_mark, unit, data_type, annotation);
	printf("qiguaiqiguai %x: %d %d\n", comp_id, field_size, new_field_instance->get_size_of_field());
	EXECUTION_REPORT(REPORT_ERROR, comp_id, field_size == new_field_instance->get_size_of_field(), "Fail to register an instance of coupling field of \"%s\" because the size of the model data buffer is different from the size determined by the parallel decomposition and grid. Please check the model code with the annotation \"%s\"",
					 field_name, annotation);
	new_field_instance->reset_mem_buf(data_buffer, true);
	new_field_instance->set_field_instance_id(TYPE_FIELD_INST_ID_PREFIX|fields_mem.size(), annotation);
	fields_mem.push_back(new_field_instance);

	printf("okokokqiguai %lx %lx\n", new_field_instance, get_field_instance(new_field_instance->get_field_instance_id()));
	return new_field_instance->get_field_instance_id();
}


bool Memory_mgt::check_is_legal_field_instance_id(int field_instance_id)
{
	printf("check field %x vs %x: %d vs %d: %lx\n", (field_instance_id&TYPE_ID_PREFIX_MASK), TYPE_FIELD_INST_ID_PREFIX, field_instance_id&TYPE_ID_SUFFIX_MASK, fields_mem.size(), fields_mem[field_instance_id&TYPE_ID_SUFFIX_MASK]);
	if ((field_instance_id&TYPE_ID_PREFIX_MASK) != TYPE_FIELD_INST_ID_PREFIX)
		return false;

	if (fields_mem[field_instance_id&TYPE_ID_SUFFIX_MASK]->get_is_registered_model_buf())
		printf("find a register field instance\n");

	return (field_instance_id&TYPE_ID_SUFFIX_MASK) < fields_mem.size();
}


Field_mem_info *Memory_mgt::get_field_instance(int field_instance_id)
{
	if (!check_is_legal_field_instance_id(field_instance_id))
		return NULL;

	return fields_mem[field_instance_id&TYPE_ID_SUFFIX_MASK];
}

