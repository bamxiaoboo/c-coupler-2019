/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "inout_interface_mgt.h"


Inout_interface::Inout_interface(const char *interface_name, int interface_id, int import_or_export, int num_fields, int *field_ids, int *timer_ids, const char *annotation)
{
	int API_id;

	
	strcpy(this->interface_name, interface_name);
	this->interface_id = interface_id;
	this->import_or_export = import_or_export;
	this->comp_id = -1;
	EXECUTION_REPORT(REPORT_ERROR, -1, num_fields > 0, "The number of fields for registering an import/export interface is wrong (negative). Please verify the model code related to the annotation \"%s\"", annotation);
	for (int i = 0; i < num_fields; i ++) {
		EXECUTION_REPORT(REPORT_ERROR, -1, timer_mgr2->check_is_legal_timer_id(timer_ids[i]), "Wrong timer id is detected when registering a import/export interface. Please verify the model code related to the annotation \"%s\"", annotation);
		printf("field is check is %d\n", field_ids[i]);
		EXECUTION_REPORT(REPORT_ERROR, -1, memory_manager->check_is_legal_field_instance_id(field_ids[i]) && memory_manager->get_field_instance(field_ids[i])->get_is_registered_model_buf(), "Wrong field instance id is detected when registering a import/export interface. Please verify the model code related to the annotation \"%s\"", annotation);
		if (comp_id == -1)
			comp_id = timer_mgr2->get_timer(timer_ids[i])->get_comp_id();
		printf("comp id is %x %x at %lx\n", timer_ids[i], comp_id, timer_mgr2->get_timer(timer_ids[i]));
		EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_id == timer_mgr2->get_timer(timer_ids[i])->get_comp_id(), "Inconsistency is detected when registering an import/export interface. All timers and field instances must belong to the same component (the two different components are \"%s\" and \"%s\"). Please verify the model code related to the annotation \"%s\"", 
			             comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Inout_interface::Inout_interface 1")->get_comp_name(),comp_comm_group_mgt_mgr->get_global_node_of_local_comp(timer_mgr2->get_timer(timer_ids[i])->get_comp_id(),"in Inout_interface::Inout_interface 2")->get_comp_name(), annotation);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_id == memory_manager->get_field_instance(field_ids[i])->get_comp_id(), "Inconsistency is detected when registering an import/export interface. All timers and field instances must belong to the same component (the two different components are \"%s\" and \"%s\"). Please verify the model code related to the annotation \"%s\"", 
						 comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Inout_interface::Inout_interface 3")->get_comp_name(),comp_comm_group_mgt_mgr->get_global_node_of_local_comp(memory_manager->get_field_instance(field_ids[i])->get_comp_id(),"in Inout_interface::Inout_interface 4")->get_comp_name(), annotation);
	}
	for (int i = 0; i < num_fields; i ++)
		for (int j = i+1; j < num_fields; j ++)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, field_ids[i] != field_ids[j], "There are multiple copies of the same field instance when registering an import/export interface. Each field instance can have only one copy. Please verify the model code related to the annotation \"%s\"", annotation);
	if (import_or_export == 0) {
		synchronize_comp_processes_for_API(comp_id, API_ID_INTERFACE_REG_IMPORT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Inout_interface::Inout_interface"), "registerring an interface for importing field instances", annotation);
		API_id = API_ID_INTERFACE_REG_IMPORT;
	}
	else {
		synchronize_comp_processes_for_API(comp_id, API_ID_INTERFACE_REG_EXPORT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Inout_interface::Inout_interface"), "registerring an interface for exporting field instances", annotation);
		API_id = API_ID_INTERFACE_REG_EXPORT;
	}
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in Inout_interface::Inout_interface"), "registerring an interface for importing (or exporting) field instances", num_fields, "num_fields", annotation);
	for (int i = 0; i < num_fields; i ++) {
		check_API_parameter_timer(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Inout_interface::Inout_interface"), "registerring an interface for importing (or exporting) field instances", timer_ids[i], "timer ids (the information of the timers)", annotation);
		check_API_parameter_field_instance(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Inout_interface::Inout_interface"), "registerring an interface for importing (or exporting) field instances", field_ids[i], "field instances ids (the information of the field instances)", annotation);
	}
	for (int i = 0; i < num_fields; i ++) {
		timers.push_back(timer_mgr2->get_timer(timer_ids[i]));
		fields_mem.push_back(memory_manager->get_field_instance(field_ids[i]));
	}
	annotation_mgr->add_annotation(interface_id, "registering interface", annotation);
}


void Inout_interface::report_common_field_instances(const Inout_interface *another_interface)
{
	if (this->import_or_export == 1 && another_interface->import_or_export == 1)
		return;

	for (int i = 0; i < this->fields_mem.size(); i ++)
		for (int j = 0; j < another_interface->fields_mem.size(); j ++)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, this->fields_mem[i] != another_interface->fields_mem[j], "Two import/export interfaces use the same field instance (field name is \"%s\") which is not allowed. Please check the model code with the annotation \"%s\" and \"%s\".",
			                 fields_mem[i]->get_field_name(), annotation_mgr->get_annotation(this->interface_id, "registering interface"), annotation_mgr->get_annotation(another_interface->interface_id, "registering interface"));
}


Inout_interface_mgt::~Inout_interface_mgt()
{
	for (int i = 0; i < interfaces.size(); i ++)
		delete interfaces[i];
}


int Inout_interface_mgt::register_inout_interface(const char *interface_name, int import_or_export, int num_fields, int *field_ids, int *timer_ids, const char *annotation)
{
	Inout_interface *new_interface = new Inout_interface(interface_name, TYPE_INOUT_INTERFACE_ID_PREFIX|interfaces.size(), import_or_export, num_fields, field_ids, timer_ids, annotation);
	for (int i = 0; i < interfaces.size(); i ++) {
		if (new_interface->get_comp_id() != interfaces[i]->get_comp_id())
			continue;
		EXECUTION_REPORT(REPORT_ERROR, new_interface->get_comp_id(), !words_are_the_same(interface_name, interfaces[i]->get_interface_name()), 
		                 "cannot register the import/export interface named \"%s\" at the model code with the annotation \"%s\" because an interface with the same name has been registerred at the model code with the annotation \"%s\"",
		                 interface_name, annotation, annotation_mgr->get_annotation(interfaces[i]->get_interface_id(), "registering interface"));
		new_interface->report_common_field_instances(interfaces[i]);
		
	}
	interfaces.push_back(new_interface);
	return new_interface->get_interface_id();
}

