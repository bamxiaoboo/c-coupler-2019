/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "inout_interface_mgt.h"


Inout_interface::Inout_interface(const char *temp_array_buffer, int &buffer_content_iter)
{
	char comp_long_name[NAME_STR_SIZE];
	int num_interfaces;


	interface_id = 0;
	read_data_from_array_buffer(comp_long_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
	read_data_from_array_buffer(interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
	read_data_from_array_buffer(&import_or_export, sizeof(int), temp_array_buffer, buffer_content_iter);
	read_data_from_array_buffer(&num_interfaces, sizeof(int), temp_array_buffer, buffer_content_iter);
	for (int i = 0; i < num_interfaces; i ++) {
		fields_name.push_back(temp_array_buffer+buffer_content_iter-NAME_STR_SIZE);
		buffer_content_iter -= NAME_STR_SIZE;
	}
	Comp_comm_group_mgt_node *comp_node = comp_comm_group_mgt_mgr->search_global_node(comp_long_name);
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_node != NULL, "Software error in Inout_interface::Inout_interface");
	comp_id = comp_node->get_local_node_id();
	printf("comp full name is %s: %x\n", comp_long_name, comp_id);
}


Inout_interface::Inout_interface(const char *interface_name, int interface_id, int import_or_export, int num_fields, int *field_ids, int *timer_ids, const char *annotation)
{
	int API_id;

	
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
	comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(comp_id, API_id, annotation);	
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
	strcpy(this->interface_name, interface_name);
	check_and_verify_name_format_of_string_for_API(this->comp_id, this->interface_name, API_id, "the interface", annotation);
}


void Inout_interface::report_common_field_instances(const Inout_interface *another_interface)
{
	if (this->import_or_export != another_interface->import_or_export)
		return;

	for (int i = 0; i < this->fields_mem.size(); i ++)
		for (int j = 0; j < another_interface->fields_mem.size(); j ++)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, !words_are_the_same(this->fields_mem[i]->get_field_name(), another_interface->fields_mem[j]->get_field_name()), "Two import interfaces use the same field instance (field name is \"%s\") which is not allowed. Please check the model code with the annotation \"%s\" and \"%s\".",
			                 fields_mem[i]->get_field_name(), annotation_mgr->get_annotation(this->interface_id, "registering interface"), annotation_mgr->get_annotation(another_interface->interface_id, "registering interface"));
}


void Inout_interface::get_fields_name(std::vector<const char*> *fields_name)
{
	for (int i = 0; i < this->fields_mem.size(); i ++)
		fields_name->push_back(this->fields_mem[i]->get_field_name());
}


const char *Inout_interface::get_field_name(int number)
{
	if (number >= fields_name.size() && number >= fields_mem.size())
		return NULL;

	if (number < fields_name.size())
		return fields_name[number];

	return fields_mem[number]->get_field_name();
}


int Inout_interface::get_num_fields()
{
	if (fields_name.size() >= fields_mem.size())
		return fields_name.size();
	return fields_mem.size();
}


void Inout_interface::transform_interface_into_array(char **temp_array_buffer, int &buffer_max_size, int &buffer_content_size)
{
	int temp_int;

	
	for (int i = 0; i < fields_mem.size(); i ++)
		write_data_into_array_buffer(fields_mem[i]->get_field_name(), NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
	temp_int = fields_mem.size();
	write_data_into_array_buffer(&temp_int, sizeof(int), temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&import_or_export, sizeof(int), temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Inout_interface::transform_interface_into_array")->get_full_name(), NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
}


Inout_interface_mgt::Inout_interface_mgt(const char *temp_array_buffer, int buffer_content_iter)
{
	this->temp_array_buffer = NULL;
	while (buffer_content_iter > 0) {
		printf("iter is %d\n", buffer_content_iter);
		interfaces.push_back(new Inout_interface(temp_array_buffer, buffer_content_iter));
	}
}


Inout_interface_mgt::Inout_interface_mgt()
{
	temp_array_buffer = NULL;
	buffer_content_size = 0;
	buffer_content_iter = 0;
	buffer_max_size = 0;
}


Inout_interface_mgt::~Inout_interface_mgt()
{
	for (int i = 0; i < interfaces.size(); i ++)
		delete interfaces[i];

	if (temp_array_buffer != NULL)
		delete [] temp_array_buffer;
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


bool Inout_interface_mgt::is_interface_id_legal(int interface_id)
{
	if ((interface_id & TYPE_ID_PREFIX_MASK) != TYPE_INOUT_INTERFACE_ID_PREFIX)
		return false;

	return (interface_id&TYPE_ID_SUFFIX_MASK) < interfaces.size();
}


Inout_interface *Inout_interface_mgt::get_interface(int interface_id)
{
	if (!is_interface_id_legal(interface_id))
		return NULL;

	return interfaces[interface_id&TYPE_ID_SUFFIX_MASK];
}


Inout_interface *Inout_interface_mgt::get_interface(const char *comp_full_name, const char *interface_name)
{
	if (comp_comm_group_mgt_mgr->search_global_node(comp_full_name) == NULL)
		return NULL;

	int comp_id = comp_comm_group_mgt_mgr->search_global_node(comp_full_name)->get_comp_id();
	for (int i = 0; i < interfaces.size(); i ++)
		if (interfaces[i]->get_comp_id() == comp_id && words_are_the_same(interfaces[i]->get_interface_name(), interface_name))
			return interfaces[i];

	return NULL;;
}


void Inout_interface_mgt::merge_inout_interface_fields_info(int comp_id)
{
	MPI_Comm comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in merge_inout_interface_fields_info");
	int local_proc_id = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_id, "in merge_inout_interface_fields_info");
	int num_local_procs = comp_comm_group_mgt_mgr->get_num_proc_in_comp(comp_id, "in merge_inout_interface_fields_info");
	int *counts = new int [num_local_procs];
	int *displs = new int [num_local_procs];
	char *temp_buffer;


	MPI_Gather(&buffer_content_size, 1, MPI_INT, counts, 1, MPI_INT, 0, comm);
	if (local_proc_id == 0) {
		displs[0] = 0;
		for (int i = 1; i < num_local_procs; i ++)
			displs[i] = displs[i-1]+counts[i-1];
		buffer_max_size = displs[num_local_procs-1]+counts[num_local_procs-1] + 100;
		temp_buffer = new char [buffer_max_size];
		
	}
	MPI_Gatherv(temp_array_buffer, buffer_content_size, MPI_CHAR, temp_buffer, counts, displs, MPI_CHAR, 0, comm);
	if (temp_array_buffer != NULL) {
		delete [] temp_array_buffer;
		temp_array_buffer = NULL;
	}

	if (local_proc_id == 0) {
		temp_array_buffer = temp_buffer;
		buffer_content_size = displs[num_local_procs-1]+counts[num_local_procs-1];
		for (int i = 0; i < interfaces.size(); i ++)
			if (interfaces[i]->get_comp_id() == comp_id)
				interfaces[i]->transform_interface_into_array(&temp_array_buffer, buffer_max_size, buffer_content_size);
	}
	else {
		buffer_max_size = 0;
		buffer_content_size = 0;
	}
	
	delete [] counts;
	delete [] displs;
}


void Inout_interface_mgt::write_all_interfaces_fields_info()
{
	const char *field_name;

	
 	for (int i = 0; i < interfaces.size(); i ++) {
		if (interfaces[i]->get_import_or_export() == 0)
			printf("import interface \"%s\" of component \"%s\" has %d fields\n", interfaces[i]->get_interface_name(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(interfaces[i]->get_comp_id(),"in write_all_interfaces_fields_info")->get_comp_full_name(), interfaces[i]->get_num_fields());
		else printf("export interface \"%s\" of component \"%s\" has %d fields\n", interfaces[i]->get_interface_name(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(interfaces[i]->get_comp_id(),"in write_all_interfaces_fields_info")->get_comp_full_name(), interfaces[i]->get_num_fields());
		for (int i = 0; (field_name = interfaces[i]->get_field_name(i)) != NULL; i ++) {
			printf("            %s\n", field_name);
		}
		printf("\n\n");
	}
}

