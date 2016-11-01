/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "runtime_cumulate_average_algorithm.h"
#include "runtime_datatype_transformer.h"
#include "inout_interface_mgt.h"


Connection_coupling_procedure::Connection_coupling_procedure(Inout_interface *inout_interface, Coupling_connection *coupling_connection)
{
	this->inout_interface = inout_interface;
	this->coupling_connection = coupling_connection; 

	for (int i = 0; i < coupling_connection->fields_name.size(); i ++) {
		runtime_inner_averaging_algorithm.push_back(NULL);
		runtime_inter_averaging_algorithm.push_back(NULL);
		runtime_remap_algorithms.push_back(NULL);
		runtime_unit_transform_algorithms.push_back(NULL);
		runtime_datatype_transform_algorithms.push_back(NULL);
		runtime_data_transfer_algorithms.push_back(NULL);
		fields_mem_registered.push_back(inout_interface->search_registered_field_instance(coupling_connection->fields_name[i]));
		transfer_process_on.push_back(false);
		if (inout_interface->get_import_or_export() == 1) {
			fields_mem_inner_step_averaged.push_back(memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_AVERAGED_INNER, coupling_connection->connection_id, NULL));
			fields_mem_inter_step_averaged.push_back(memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_AVERAGED_INTER, coupling_connection->connection_id, NULL));
		}
		else {
			fields_mem_inner_step_averaged.push_back(NULL);
			fields_mem_inter_step_averaged.push_back(NULL);
		}
		fields_mem_remapped.push_back(NULL);
		fields_mem_datatype_transformed.push_back(NULL);
		fields_mem_transfer.push_back(NULL);
		Connection_field_time_info *field_time_info = new Connection_field_time_info;
		field_time_info->timer = coupling_connection->src_fields_info[i]->timer;
		components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->get_current_time(field_time_info->current_year, field_time_info->current_month, field_time_info->current_day, field_time_info->current_second, 0);
		field_time_info->num_elapsed_days = components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->calculate_elapsed_day(field_time_info->current_year, field_time_info->current_month, field_time_info->current_day);
		field_time_info->time_step_in_second = coupling_connection->src_fields_info[i]->time_step_in_second;
		fields_time_info_src.push_back(field_time_info);
		field_time_info = new Connection_field_time_info;
		field_time_info->timer = coupling_connection->dst_fields_info[i]->timer;
		field_time_info->time_step_in_second = coupling_connection->dst_fields_info[i]->time_step_in_second;
		components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->get_current_time(field_time_info->current_year, field_time_info->current_month, field_time_info->current_day, field_time_info->current_second, 0);
		field_time_info->num_elapsed_days = components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->calculate_elapsed_day(field_time_info->current_year, field_time_info->current_month, field_time_info->current_day);
		fields_time_info_dst.push_back(field_time_info);
		if (inout_interface->get_import_or_export() == 1) {
			runtime_inner_averaging_algorithm[i] = new Runtime_cumulate_average_algorithm(fields_mem_registered[i], fields_mem_inner_step_averaged[i]);
			runtime_inter_averaging_algorithm[i] = new Runtime_cumulate_average_algorithm(fields_mem_inner_step_averaged[i], fields_mem_inter_step_averaged[i]);
		}
		if (!words_are_the_same(coupling_connection->src_fields_info[i]->data_type, coupling_connection->dst_fields_info[i]->data_type)) {
			if (get_data_type_size(coupling_connection->src_fields_info[i]->data_type) > get_data_type_size(coupling_connection->dst_fields_info[i]->data_type)) {
				if (inout_interface->get_import_or_export() == 1) {
					printf("for field %s, add data type transformation at src from %s to %s\n", coupling_connection->fields_name[i], coupling_connection->src_fields_info[i]->data_type, coupling_connection->dst_fields_info[i]->data_type);
					fields_mem_datatype_transformed[i] = memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_DATATYPE_TRANS, coupling_connection->connection_id, coupling_connection->dst_fields_info[i]->data_type);
					runtime_datatype_transform_algorithms[i] = new Runtime_datatype_transformer(fields_mem_inter_step_averaged[i], fields_mem_datatype_transformed[i]);
				}
			}
			else {
				if (inout_interface->get_import_or_export() == 0) {
					printf("for field %s, add data type transformation at dst from %s to %s\n", coupling_connection->fields_name[i], coupling_connection->src_fields_info[i]->data_type, coupling_connection->dst_fields_info[i]->data_type);
					fields_mem_datatype_transformed[i] = memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_DATATYPE_TRANS, coupling_connection->connection_id, coupling_connection->src_fields_info[i]->data_type);
//					runtime_data_transfer_algorithms[i] = new Runtime_datatype_transformer(..., ...);
				}
			}

		}
	}
}


void Connection_coupling_procedure::execute(bool bypass_timer)
{
	for (int i = 0; i < fields_time_info_dst.size(); i ++) {
		if (inout_interface->get_import_or_export() == 0) {
			components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->get_current_time(fields_time_info_dst[i]->current_year, fields_time_info_dst[i]->current_month, fields_time_info_dst[i]->current_day, fields_time_info_dst[i]->current_second, 0);
			fields_time_info_dst[i]->num_elapsed_days = components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->get_current_num_elapsed_day();				
		}
		else {
			components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->get_current_time(fields_time_info_src[i]->current_year, fields_time_info_src[i]->current_month, fields_time_info_src[i]->current_day, fields_time_info_src[i]->current_second, 0);
			fields_time_info_src[i]->num_elapsed_days = components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->get_current_num_elapsed_day();
		}
	}

	if (bypass_timer) {
		for (int i = 0; i < fields_time_info_dst.size(); i ++) {
			transfer_process_on[i] = true;
			if (runtime_inner_averaging_algorithm[i] != NULL)
				runtime_inner_averaging_algorithm[i]->run(true);
			if (runtime_inter_averaging_algorithm[i] != NULL)
				runtime_inter_averaging_algorithm[i]->run(true);
			if (runtime_unit_transform_algorithms[i] != NULL)
				runtime_unit_transform_algorithms[i]->run(true);
			if (runtime_datatype_transform_algorithms[i] != NULL)
				runtime_datatype_transform_algorithms[i]->run(true);
			if (runtime_remap_algorithms[i] != NULL)
				runtime_remap_algorithms[i]->run(true);
		}
	}
	else {
		for (int i = 0; i < fields_time_info_dst.size(); i ++) {
			Time_mgt *time_mgr = components_time_mgrs->get_time_mgr(inout_interface->get_comp_id());
			Coupling_timer *dst_timer = fields_time_info_dst[i]->timer;
			Coupling_timer *src_timer = fields_time_info_src[i]->timer;
			if (inout_interface->get_import_or_export() == 0) {
				transfer_process_on[i] = time_mgr->is_timer_on(dst_timer->get_frequency_unit(), dst_timer->get_frequency_count(), dst_timer->get_delay_count());
				continue;
			}			
			if (!time_mgr->is_timer_on(src_timer->get_frequency_unit(), src_timer->get_frequency_count(), src_timer->get_delay_count())) {
				transfer_process_on[i] = false;
				runtime_inner_averaging_algorithm[i]->run(false);			
				printf("src accum/aver %d: inner accumulate field %s: %f\n", time_mgr->get_current_step_id(), fields_mem_registered[i]->get_field_name(), ((double*)fields_mem_inner_step_averaged[i]->get_data_buf())[0]);
				continue;
			}
			runtime_inner_averaging_algorithm[i]->run(true);
			printf("src accum/aver %d: inner average field %s: %f\n", time_mgr->get_current_step_id(), fields_mem_registered[i]->get_field_name(), ((double*)fields_mem_inner_step_averaged[i]->get_data_buf())[0]);
			if (time_mgr->is_timer_on(dst_timer->get_frequency_unit(), dst_timer->get_frequency_count(), dst_timer->get_delay_count())) {
				transfer_process_on[i] = true;
				runtime_inter_averaging_algorithm[i]->run(true);
				printf("src accum/aver %d: inter average field normal %s: %f\n", time_mgr->get_current_step_id(), fields_mem_registered[i]->get_field_name(), ((double*)fields_mem_inter_step_averaged[i]->get_data_buf())[0]);
				if (runtime_datatype_transform_algorithms[i] != NULL) {
					runtime_datatype_transform_algorithms[i]->run(false);
					printf("after data type transformation %f\n", ((float*)fields_mem_datatype_transformed[i]->get_data_buf())[0]);
				}
				continue;
			}
			while((((long)fields_time_info_dst[i]->num_elapsed_days)*((long)86400))+fields_time_info_dst[i]->current_second < (((long)fields_time_info_src[i]->num_elapsed_days)*((long)86400)) + fields_time_info_src[i]->current_second) 
				time_mgr->advance_time(fields_time_info_dst[i]->current_year, fields_time_info_dst[i]->current_month, fields_time_info_dst[i]->current_day, fields_time_info_dst[i]->current_second, fields_time_info_dst[i]->num_elapsed_days,  fields_time_info_dst[i]->time_step_in_second);				
			while(!dst_timer->is_timer_on(fields_time_info_dst[i]->current_year, fields_time_info_dst[i]->current_month, fields_time_info_dst[i]->current_day, fields_time_info_dst[i]->current_second, fields_time_info_dst[i]->num_elapsed_days, time_mgr->get_start_year(), time_mgr->get_start_month(), time_mgr->get_start_day(), time_mgr->get_start_second(), time_mgr->get_start_num_elapsed_day())) {
				printf("qiguaiqiguai %ld %ld : %d : %d\n", (((long)fields_time_info_dst[i]->num_elapsed_days)*((long)86400))+fields_time_info_dst[i]->current_second, (((long)fields_time_info_src[i]->num_elapsed_days)*((long)86400)) + fields_time_info_src[i]->current_second, fields_time_info_dst[i]->num_elapsed_days, time_mgr->get_start_num_elapsed_day());
				printf("dst timer info: %s %d %d\n", dst_timer->get_frequency_unit(), dst_timer->get_frequency_count(), dst_timer->get_delay_count());
				time_mgr->advance_time(fields_time_info_dst[i]->current_year, fields_time_info_dst[i]->current_month, fields_time_info_dst[i]->current_day, fields_time_info_dst[i]->current_second, fields_time_info_dst[i]->num_elapsed_days,  fields_time_info_dst[i]->time_step_in_second);
			}
			printf("special1 case current (%d %d %d %d) : %d, remote (%d %d %d %d): %d\n", fields_time_info_src[i]->current_year, fields_time_info_src[i]->current_month, fields_time_info_src[i]->current_day, fields_time_info_src[i]->current_second, fields_time_info_src[i]->num_elapsed_days,
				fields_time_info_dst[i]->current_year, fields_time_info_dst[i]->current_month, fields_time_info_dst[i]->current_day, fields_time_info_dst[i]->current_second, fields_time_info_dst[i]->num_elapsed_days);
			do {
				time_mgr->advance_time(fields_time_info_src[i]->current_year, fields_time_info_src[i]->current_month, fields_time_info_src[i]->current_day, fields_time_info_src[i]->current_second, fields_time_info_src[i]->num_elapsed_days,  fields_time_info_src[i]->time_step_in_second);				
			} while(!src_timer->is_timer_on(fields_time_info_src[i]->current_year, fields_time_info_src[i]->current_month, fields_time_info_src[i]->current_day, fields_time_info_src[i]->current_second, fields_time_info_src[i]->num_elapsed_days, time_mgr->get_start_year(), time_mgr->get_start_month(), time_mgr->get_start_day(), time_mgr->get_start_second(), time_mgr->get_start_num_elapsed_day()));
			printf("special2 case current (%d %d %d %d) : %d, remote (%d %d %d %d): %d\n", fields_time_info_src[i]->current_year, fields_time_info_src[i]->current_month, fields_time_info_src[i]->current_day, fields_time_info_src[i]->current_second, fields_time_info_src[i]->num_elapsed_days,
				fields_time_info_dst[i]->current_year, fields_time_info_dst[i]->current_month, fields_time_info_dst[i]->current_day, fields_time_info_dst[i]->current_second, fields_time_info_dst[i]->num_elapsed_days);
			if ((((long)fields_time_info_dst[i]->num_elapsed_days)*((long)86400))+fields_time_info_dst[i]->current_second < (((long)fields_time_info_src[i]->num_elapsed_days)*((long)86400)) + fields_time_info_src[i]->current_second) {
				transfer_process_on[i] = true;
				runtime_inter_averaging_algorithm[i]->run(true);
				printf("src accum/aver %d: inter average field special %s: %f\n", time_mgr->get_current_step_id(), fields_mem_registered[i]->get_field_name(), ((double*)fields_mem_inter_step_averaged[i]->get_data_buf())[0]);
				if (runtime_datatype_transform_algorithms[i] != NULL) {
					runtime_datatype_transform_algorithms[i]->run(false);
					printf("after data type transformation %f\n", ((float*)fields_mem_datatype_transformed[i]->get_data_buf())[0]);
				}				
			}
			else {
				runtime_inter_averaging_algorithm[i]->run(false);
				printf("src accum/aver %d: inter accumulate field %s: %f\n", time_mgr->get_current_step_id(), fields_mem_registered[i]->get_field_name(), ((double*)fields_mem_inter_step_averaged[i]->get_data_buf())[0]);
			}
		}
	}

	for (int i = 0; i < runtime_algorithms.size(); i ++)
		runtime_algorithms[i]->run(true);
}


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
	this->execution_checking_status = 0;
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
		fields_mem_registered.push_back(memory_manager->get_field_instance(field_ids[i]));
	}
	annotation_mgr->add_annotation(interface_id, "registering interface", annotation);
	strcpy(this->interface_name, interface_name);
	check_and_verify_name_format_of_string_for_API(this->comp_id, this->interface_name, API_id, "the interface", annotation);
}


void Inout_interface::report_common_field_instances(const Inout_interface *another_interface)
{
	if (this->import_or_export == 0 && another_interface->import_or_export == 0) {
		for (int i = 0; i < this->fields_mem_registered.size(); i ++)
			for (int j = 0; j < another_interface->fields_mem_registered.size(); j ++)
				EXECUTION_REPORT(REPORT_ERROR, comp_id, this->fields_mem_registered[i]->get_data_buf() != another_interface->fields_mem_registered[j]->get_data_buf(), "Two import interfaces (\"%s\" and \"%s\") share the same data buffer of the field (field name is \"%s\") which is not allowed. Please check the model code with the annotation \"%s\" and \"%s\".",
				                 this->interface_name, another_interface->interface_name, fields_mem_registered[i]->get_field_name(), annotation_mgr->get_annotation(this->interface_id, "registering interface"), annotation_mgr->get_annotation(another_interface->interface_id, "registering interface"));		
		return;
	}

	if (this->import_or_export != 1 || another_interface->import_or_export != 1)
		return;

	for (int i = 0; i < this->fields_mem_registered.size(); i ++)
		for (int j = 0; j < another_interface->fields_mem_registered.size(); j ++)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, !words_are_the_same(this->fields_mem_registered[i]->get_field_name(), another_interface->fields_mem_registered[j]->get_field_name()), "Two export interfaces (\"%s\" and \"%s\") provide the same field (field name is \"%s\") which is not allowed. Please check the model code with the annotation \"%s\" and \"%s\".",
			                 this->interface_name, another_interface->interface_name, fields_mem_registered[i]->get_field_name(), annotation_mgr->get_annotation(this->interface_id, "registering interface"), annotation_mgr->get_annotation(another_interface->interface_id, "registering interface"));
}


void Inout_interface::get_fields_name(std::vector<const char*> *fields_name)
{
	if (this->fields_mem_registered.size() > 0) {
		for (int i = 0; i < this->fields_mem_registered.size(); i ++)
			fields_name->push_back(this->fields_mem_registered[i]->get_field_name());
	}
	else {
		for (int i = 0; i < this->fields_name.size(); i ++)
			fields_name->push_back(this->fields_name[i]);
	}
}


const char *Inout_interface::get_field_name(int number)
{
	if (number >= fields_name.size() && number >= fields_mem_registered.size())
		return NULL;

	if (number < fields_name.size())
		return fields_name[number];

	return fields_mem_registered[number]->get_field_name();
}


int Inout_interface::get_num_fields()
{
	if (fields_name.size() >= fields_mem_registered.size())
		return fields_name.size();
	return fields_mem_registered.size();
}


Field_mem_info *Inout_interface::search_registered_field_instance(const char *field_name)
{
	for (int i = 0; i < fields_mem_registered.size(); i ++)
		if (words_are_the_same(fields_mem_registered[i]->get_field_name(), field_name))
			return fields_mem_registered[i];

	return NULL;
}


Coupling_timer *Inout_interface::search_a_timer(const char *field_name)
{
	for (int i = 0; i < fields_mem_registered.size(); i ++)
		if (words_are_the_same(fields_mem_registered[i]->get_field_name(), field_name))
			return timers[i];

	return NULL;
}


void Inout_interface::transform_interface_into_array(char **temp_array_buffer, int &buffer_max_size, int &buffer_content_size)
{
	int temp_int;

	
	for (int i = 0; i < fields_mem_registered.size(); i ++)
		write_data_into_array_buffer(fields_mem_registered[i]->get_field_name(), NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
	temp_int = fields_mem_registered.size();
	write_data_into_array_buffer(&temp_int, sizeof(int), temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&import_or_export, sizeof(int), temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Inout_interface::transform_interface_into_array")->get_full_name(), NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
}


void Inout_interface::add_coupling_procedure(Connection_coupling_procedure *coupling_procedure)
{
	coupling_procedures.push_back(coupling_procedure);
}


void Inout_interface::execute(bool bypass_timer, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_comm_group_mgt_mgr->get_is_definition_finalized(), "Cannot execute the interface \"%s\" corresponding to the model code with the annotation \"%s\" because the coupling procedures of interfaces have not been generated. Please call API \"CCPL_end_coupling_configuration\" of all components before executing an import/export interface", interface_name, annotation);
	if ((execution_checking_status & 0x1) == 0 && bypass_timer || (execution_checking_status & 0x2) == 0 && !bypass_timer) {
		synchronize_comp_processes_for_API(comp_id, API_ID_INTERFACE_EXECUTE, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "software error")->get_comm_group(), "executing an import/export interface", annotation);
		check_API_parameter_string(comp_id, API_ID_INTERFACE_EXECUTE, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"executing an import/export interface"), "executing an import/export interface", interface_name, "the corresponding interface name", annotation);
		int bypass_timer_int;
		if (bypass_timer)
			bypass_timer_int = 0;
		else bypass_timer_int = 1;
		check_API_parameter_int(comp_id, API_ID_INTERFACE_EXECUTE, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"executing an import/export interface"), "executing an import/export interface", bypass_timer_int, "the value for specifying whether bypass timers", annotation);
		if (bypass_timer) {
			execution_checking_status = execution_checking_status | 0x1;
			annotation_mgr->add_annotation(interface_id, "bypassing timer", annotation);
		}
		else {
			execution_checking_status = execution_checking_status | 0x2;
			annotation_mgr->add_annotation(interface_id, "using timer", annotation);
		}
	}

	if (bypass_timer)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, (execution_checking_status | 0x2) == 0, "Cannot bypass the timers when executing import/export interface \"%s\" (the corrsponding code annotation is \"%s\") because this interface has been executed without bypassing timers before (the corrsponding code annotation is \"%s\")",
		                 interface_name, annotation, annotation_mgr->get_annotation(interface_id, "using timer"));

	for (int i = 0; i < coupling_procedures.size(); i ++)
		coupling_procedures[i]->execute(bypass_timer);
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

	return NULL;
}


Inout_interface *Inout_interface_mgt::get_interface(int comp_id, const char *interface_name)
{
	for (int i = 0; i < interfaces.size(); i ++)
		if (interfaces[i]->get_comp_id() == comp_id && words_are_the_same(interfaces[i]->get_interface_name(), interface_name))
			return interfaces[i];

	return NULL;
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



void Inout_interface_mgt::get_all_import_interfaces_of_a_component(std::vector<Inout_interface*> &import_interfaces, int comp_id)
{
	import_interfaces.clear();

	for (int i = 0; i < interfaces.size(); i ++)
		if (interfaces[i]->get_comp_id() == comp_id && interfaces[i]->get_import_or_export() == 0)
			import_interfaces.push_back(interfaces[i]);
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


void Inout_interface_mgt::execute_interface(int interface_id, bool bypass_timer, const char *annotation)
{
	Inout_interface *inout_interface;

	
	EXECUTION_REPORT(REPORT_ERROR, -1, is_interface_id_legal(interface_id), "0x%x is not an legal ID of an import/export interface. Please check the model code with the annotation \"%s\"", interface_id, annotation);
	inout_interface = get_interface(interface_id);
	EXECUTION_REPORT(REPORT_ERROR, -1, inout_interface != NULL, "0x%x should be the ID of import/export interface. However, it is wrong as the corresponding interface is not found. Please check the model code with the annotation \"%s\"", interface_id, annotation);
	inout_interface->execute(bypass_timer, annotation);
}


void Inout_interface_mgt::execute_interface(int comp_id, const char *interface_name, bool bypass_timer, const char *annotation)
{
	Inout_interface *inout_interface;

	
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "0x%x is not an legal ID of a component. Please check the model code with the annotation \"%s\"", comp_id, annotation);
	inout_interface = get_interface(comp_id, interface_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, inout_interface != NULL, "Registered interface of this component does not contain an import/export interface named \"%s\". Please check the model code with the annotation \"%s\"", interface_name, annotation);
	inout_interface->execute(bypass_timer, annotation);
}

