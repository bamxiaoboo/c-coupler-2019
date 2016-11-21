/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "IO_field_mgt.h"


IO_field::IO_field(int IO_field_id, int field_instance_id, int timer_id, const char *field_IO_name, const char *annotation)
{
	this->IO_field_id = IO_field_id;
	if (timer_id == -1)
		this->timer = NULL;
	else {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, timer_mgr2->check_is_legal_timer_id(timer_id), "The parameter of timer id when calling the CCPL interface \"CCPL_register_IO_field\" is wrong. Please verify the model code with the annotation \"%s\"", annotation);
		this->timer = new Coupling_timer(timer_mgr2->get_timer(timer_id));
		this->timer->reset_lag_count();
	}

	Field_mem_info *field_inst = memory_manager->get_field_instance(field_instance_id);
	this->comp_id = field_inst->get_comp_id();
	this->field_instance_id = field_instance_id;
	if (strlen(field_IO_name) > 0)
		strcpy(this->field_IO_name, field_IO_name);
	else strcpy(this->field_IO_name, field_inst->get_field_name());
	strcpy(this->field_long_name, fields_info->search_field_info(field_inst->get_field_name())->field_long_name);
	strcpy(this->field_unit, field_inst->get_unit());
}


IO_field::IO_field(int IO_field_id, int timer_id, int comp_or_grid_id, int decomp_id, int field_size, void *data_buffer, const char * field_IO_name, const char *long_name, const char *unit, const char *data_type, const char * annotation)
{
	Field_mem_info *field_mem;


	this->IO_field_id = IO_field_id;
	strcpy(this->field_IO_name, field_IO_name);
	strcpy(this->field_unit, unit);
	strcpy(this->field_long_name, long_name);

	if (decomp_id == -1) {
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_or_grid_id), "The parameter of component id when calling the CCPL interface \"CCPL_register_IO_field\" for registering IO field \"%s\" is wrong. Please verify the model code with the annotation \"%s\"", field_IO_name, annotation);
		this->comp_id = comp_or_grid_id;
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR, -1, decomps_info_mgr->is_decomp_id_legal(decomp_id), "The parameter of decomposition id when calling the CCPL interface \"CCPL_register_IO_field\" for registering IO field \"%s\" is wrong. Please verify the model code with the annotation \"%s\"", field_IO_name, annotation);
		this->comp_id = decomps_info_mgr->get_decomp_info(decomp_id)->get_comp_id();
		EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid_mgr->is_grid_id_legal(comp_or_grid_id), "The parameter of grid id when calling the CCPL interface \"CCPL_register_IO_field\" for registering IO field \"%s\" is wrong. Please verify the model code with the annotation \"%s\"", field_IO_name, annotation);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid_mgr->get_comp_id_of_grid(comp_or_grid_id), "The parameters of grid id and decomposition id when calling the CCPL interface \"CCPL_register_IO_field\" for registering IO field \"%s\" do not belong to the same component. Please verify the model code with the annotation \"%s\"", field_IO_name, annotation);
	}

	EXECUTION_REPORT(REPORT_ERROR, comp_id, strlen(field_IO_name) > 0, "The parameter of field I/O name when calling the CCPL interface \"CCPL_register_IO_field\" is empty. Please verify the model code with the annotation \"%s\"", annotation);

	if (timer_id == -1)
		this->timer = NULL;
	else {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, timer_mgr2->check_is_legal_timer_id(timer_id), "The parameter of timer id when calling the CCPL interface \"CCPL_register_IO_field\" is wrong. Please verify the model code with the annotation \"%s\"", annotation);
		this->timer = new Coupling_timer(timer_mgr2->get_timer(timer_id));
		this->timer->reset_lag_count();
	}

	field_instance_id = memory_manager->register_external_field_instance(field_IO_name, data_buffer, field_size, decomp_id, comp_or_grid_id, BUF_MARK_IO_FIELD, unit, data_type, annotation);

	EXECUTION_REPORT(REPORT_ERROR, comp_id, strlen(field_IO_name) > 0, "The parameter of field I/O name when calling the CCPL interface \"CCPL_register_IO_field\" cannot be an empty string. Please verify the model code with the annotation \"%s\"", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, strlen(long_name) > 0, "The parameter of long name of the I/O field \"%s\" when calling the CCPL interface \"CCPL_register_IO_field\" cannot be an empty string. Please verify the model code with the annotation \"%s\"", field_IO_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, strlen(unit) > 0, "The parameter of unit of the I/O field \"%s\" when calling the CCPL interface \"CCPL_register_IO_field\" cannot be an empty string. Please verify the model code with the annotation \"%s\"", field_IO_name, annotation);
}


IO_field *IO_field_mgt::search_IO_field(int comp_id, const char *field_IO_name)
{
	for (int i = 0; i < IO_fields.size(); i ++)
		if (IO_fields[i]->get_comp_id() == comp_id && words_are_the_same(IO_fields[i]->get_field_IO_name(), field_IO_name))
			return IO_fields[i];

	return NULL;
}


void IO_field_mgt::check_for_registering_IO_field(IO_field *new_IO_field, const char *annotation)
{
	IO_field *existing_field = search_IO_field(new_IO_field->get_comp_id(), new_IO_field->get_field_IO_name());
	if (existing_field != NULL)
		EXECUTION_REPORT(REPORT_ERROR, new_IO_field->get_comp_id(), false, "IO field \"%s\" has been registered before (the corresponding model code annotation is \"%s\"). It cannot be registered again at the model code with the annotation \"%s\"",
		                 new_IO_field->get_field_IO_name(), annotation_mgr->get_annotation(existing_field->get_IO_field_id(), "registering I/O field"), annotation);
	annotation_mgr->add_annotation(new_IO_field->get_IO_field_id(), "registering I/O field", annotation);
	synchronize_comp_processes_for_API(new_IO_field->get_comp_id(), API_ID_FIELD_MGT_REG_IO_FIELD, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(new_IO_field->get_comp_id(),""), "registering an I/O field", annotation);
	IO_fields.push_back(new_IO_field);
}


int IO_field_mgt::register_IO_field(int field_instance_id, int timer_id, const char *field_IO_name, const char *annotation)
{
	int IO_field_id = TYPE_IO_FIELD_PREFIX | IO_fields.size();
	IO_field *new_IO_field = new IO_field(IO_field_id, field_instance_id, timer_id, field_IO_name, annotation);
	check_for_registering_IO_field(new_IO_field, annotation);
	check_API_parameter_field_instance(new_IO_field->get_comp_id(), API_ID_FIELD_MGT_REG_IO_FIELD, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(new_IO_field->get_comp_id(),""), "registering an I/O field", new_IO_field->get_field_instance_id(), "field instance id", annotation);
	return IO_field_id;
}


int IO_field_mgt::register_IO_field(int timer_id, int comp_or_grid_id, int decomp_id, int field_size, void *data_buffer, const char * field_IO_name, const char *long_name, const char *unit, const char *data_type, const char * annotation)
{
	int IO_field_id = TYPE_IO_FIELD_PREFIX | IO_fields.size();
	IO_field *new_IO_field = new IO_field(IO_field_id, timer_id, comp_or_grid_id, decomp_id, field_size, data_buffer, field_IO_name, long_name, unit, data_type, annotation);
	check_for_registering_IO_field(new_IO_field, annotation);
	return IO_field_id;
}


