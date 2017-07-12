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


Connection_field_time_info::Connection_field_time_info(Inout_interface *inout_interface, Coupling_timer *timer, int time_step_in_second, int inst_or_aver)
{
	this->inout_interface = inout_interface;
	this->timer = timer;
	components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->get_current_time(current_year, current_month, current_day, current_second, 0, "CCPL internal");
	current_num_elapsed_days = components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->get_current_num_elapsed_day();
	this->time_step_in_second = time_step_in_second;
	this->inst_or_aver = inst_or_aver;
	if (components_time_mgrs->get_time_mgr(inout_interface->get_comp_id())->is_timer_on(timer->get_frequency_unit(), timer->get_frequency_count(), timer->get_local_lag_count())) {
		last_timer_num_elapsed_days = current_num_elapsed_days;
		last_timer_second = current_second;
	}
	else {
		last_timer_num_elapsed_days = -1;
		last_timer_second = -1;
	}
	next_timer_num_elapsed_days = -1;
	next_timer_second = -1;
	timer->get_time_of_next_timer_on(components_time_mgrs->get_time_mgr(inout_interface->get_comp_id()), current_year, current_month, current_day,current_second, current_num_elapsed_days, time_step_in_second, next_timer_num_elapsed_days, next_timer_second, true);
	if (words_are_the_same(timer->get_frequency_unit(), FREQUENCY_UNIT_SECONDS))
		lag_seconds = timer->get_remote_lag_count();
	else lag_seconds = timer->get_remote_lag_count() * SECONDS_PER_DAY;
}


void Connection_field_time_info::get_time_of_next_timer_on(bool advance)
{
	timer->get_time_of_next_timer_on(components_time_mgrs->get_time_mgr(inout_interface->get_comp_id()), current_year, current_month, current_day,current_second, current_num_elapsed_days, time_step_in_second, next_timer_num_elapsed_days, next_timer_second, advance);
}


Connection_coupling_procedure::Connection_coupling_procedure(Inout_interface *inout_interface, Coupling_connection *coupling_connection)
{
	this->inout_interface = inout_interface;
	this->coupling_connection = coupling_connection; 


	for (int i = 0; i < coupling_connection->fields_name.size(); i ++)
		for (int j=i+1; j < coupling_connection->fields_name.size(); j ++)
			EXECUTION_REPORT(REPORT_ERROR, -1, !words_are_the_same(coupling_connection->fields_name[i], coupling_connection->fields_name[j]), "Software error in Connection_coupling_procedure::Connection_coupling_procedure: duplicated field name \"%s\" in a coonection", coupling_connection->fields_name[i]);

	for (int i = 0; i < coupling_connection->src_fields_info.size(); i ++) {
		runtime_inner_averaging_algorithm.push_back(NULL);
		runtime_inter_averaging_algorithm.push_back(NULL);
		runtime_remap_algorithms.push_back(NULL);
		runtime_unit_transform_algorithms.push_back(NULL);
		runtime_datatype_transform_algorithms.push_back(NULL);
		if (i < coupling_connection->fields_name.size())
			fields_mem_registered.push_back(inout_interface->search_registered_field_instance(coupling_connection->fields_name[i]));
		else fields_mem_registered.push_back(coupling_connection->get_bottom_field(inout_interface->get_import_or_export_or_remap() == 1, i-coupling_connection->fields_name.size()));
		transfer_process_on.push_back(false);
		current_remote_fields_time.push_back(-1);
		last_remote_fields_time.push_back(-1);
		if (inout_interface->get_import_or_export_or_remap() == 1) {
			fields_mem_inner_step_averaged.push_back(memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_AVERAGED_INNER, coupling_connection->connection_id, NULL, inout_interface->get_interface_type() == INTERFACE_TYPE_REGISTER && i < coupling_connection->fields_name.size()));
			fields_mem_inter_step_averaged.push_back(memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_AVERAGED_INTER, coupling_connection->connection_id, NULL, inout_interface->get_interface_type() == INTERFACE_TYPE_REGISTER && i < coupling_connection->fields_name.size()));
		}
		else {
			fields_mem_inner_step_averaged.push_back(NULL);
			fields_mem_inter_step_averaged.push_back(NULL);
		}
		fields_mem_remapped.push_back(NULL);
		fields_mem_datatype_transformed.push_back(NULL);
		fields_mem_unit_transformed.push_back(NULL);
		fields_mem_transfer.push_back(NULL);
		Connection_field_time_info *field_time_info = new Connection_field_time_info(inout_interface, coupling_connection->src_timer, coupling_connection->src_time_step_in_second, -1);
		fields_time_info_src.push_back(field_time_info);
		field_time_info = new Connection_field_time_info(inout_interface, coupling_connection->dst_timer, coupling_connection->dst_time_step_in_second, coupling_connection->dst_inst_or_aver);
		fields_time_info_dst.push_back(field_time_info);
		if (inout_interface->get_import_or_export_or_remap() == 0)
			fields_time_info_src[i]->reset_last_timer_info();
		else fields_time_info_dst[i]->reset_last_timer_info();
		if (inout_interface->get_import_or_export_or_remap() == 1) {
			runtime_inner_averaging_algorithm[i] = new Runtime_cumulate_average_algorithm(fields_mem_registered[i], fields_mem_inner_step_averaged[i]);
			runtime_inter_averaging_algorithm[i] = new Runtime_cumulate_average_algorithm(fields_mem_inner_step_averaged[i], fields_mem_inter_step_averaged[i]);
		}
		const char *transfer_data_type = get_data_type_size(coupling_connection->src_fields_info[i]->data_type) <= get_data_type_size(coupling_connection->dst_fields_info[i]->data_type)? coupling_connection->src_fields_info[i]->data_type : coupling_connection->dst_fields_info[i]->data_type;
		if (inout_interface->get_import_or_export_or_remap() == 1) {
			if (!words_are_the_same(transfer_data_type, coupling_connection->src_fields_info[i]->data_type)) {
				EXECUTION_REPORT(REPORT_LOG, inout_interface->get_comp_id(), true, "for field %s, add data type transformation at src from %s to %s\n", fields_mem_registered[i]->get_field_name(), coupling_connection->src_fields_info[i]->data_type, transfer_data_type);
				fields_mem_datatype_transformed[i] = memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_DATATYPE_TRANS, coupling_connection->connection_id, transfer_data_type, inout_interface->get_interface_type() == INTERFACE_TYPE_REGISTER && i < coupling_connection->fields_name.size());
				runtime_datatype_transform_algorithms[i] = new Runtime_datatype_transformer(fields_mem_inter_step_averaged[i], fields_mem_datatype_transformed[i]);
			}	
		}	
		if (inout_interface->get_import_or_export_or_remap() == 0) {
			if (coupling_connection->dst_fields_info[i]->runtime_remapping_weights == NULL || coupling_connection->dst_fields_info[i]->runtime_remapping_weights->get_parallel_remapping_weights() == NULL)
				fields_mem_transfer[i] = memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_DATA_TRANSFER, coupling_connection->connection_id, transfer_data_type, inout_interface->get_interface_type() == INTERFACE_TYPE_REGISTER && i < coupling_connection->fields_name.size());
			else {
				fields_mem_transfer[i] = memory_manager->alloc_mem(fields_mem_registered[i]->get_field_name(), coupling_connection->dst_fields_info[i]->runtime_remapping_weights->get_src_decomp_info()->get_decomp_id(), coupling_connection->dst_fields_info[i]->runtime_remapping_weights->get_src_original_grid()->get_grid_id(), BUF_MARK_DATA_TRANSFER^coupling_connection->connection_id, transfer_data_type, fields_mem_registered[i]->get_unit(), "internal", inout_interface->get_interface_type() == INTERFACE_TYPE_REGISTER && i < coupling_connection->fields_name.size());
				fields_mem_remapped[i] = memory_manager->alloc_mem(fields_mem_registered[i]->get_field_name(), fields_mem_registered[i]->get_decomp_id(), fields_mem_registered[i]->get_grid_id(), BUF_MARK_REMAP_NORMAL^coupling_connection->connection_id, transfer_data_type, fields_mem_registered[i]->get_unit(), "internal", inout_interface->get_interface_type() == INTERFACE_TYPE_REGISTER && i < coupling_connection->fields_name.size());
				runtime_remap_algorithms[i] = new Runtime_remap_algorithm(coupling_connection->dst_fields_info[i]->runtime_remapping_weights, fields_mem_transfer[i], fields_mem_remapped[i], coupling_connection->connection_id);
			}
			if (!words_are_the_same(transfer_data_type, coupling_connection->dst_fields_info[i]->data_type)) {
				EXECUTION_REPORT(REPORT_LOG, inout_interface->get_comp_id(), true, "for field %s, add data type transformation at dst from %s to %s\n", fields_mem_registered[i]->get_field_name(), transfer_data_type, coupling_connection->dst_fields_info[i]->data_type);
				fields_mem_datatype_transformed[i] = memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_DATATYPE_TRANS, coupling_connection->connection_id, coupling_connection->dst_fields_info[i]->data_type, inout_interface->get_interface_type() == INTERFACE_TYPE_REGISTER && i < coupling_connection->fields_name.size());
				if (fields_mem_remapped[i] == NULL)
					runtime_datatype_transform_algorithms[i] = new Runtime_datatype_transformer(fields_mem_transfer[i], fields_mem_datatype_transformed[i]);
				else runtime_datatype_transform_algorithms[i] = new Runtime_datatype_transformer(fields_mem_remapped[i], fields_mem_datatype_transformed[i]);
			}
		}
		if (inout_interface->get_import_or_export_or_remap() == 1) {
			if (fields_mem_remapped[i] != NULL)
				fields_mem_transfer[i] = fields_mem_remapped[i];
			else if (fields_mem_unit_transformed[i] != NULL)
				fields_mem_transfer[i] = fields_mem_unit_transformed[i];
			else if (fields_mem_datatype_transformed[i] != NULL)
				fields_mem_transfer[i] = fields_mem_datatype_transformed[i];
			else fields_mem_transfer[i] = fields_mem_inter_step_averaged[i];
		}
		else {
			Field_mem_info *last_field_instance = fields_mem_transfer[i];
			if (fields_mem_datatype_transformed[i] != NULL)
				last_field_instance = fields_mem_datatype_transformed[i];
			else if (fields_mem_remapped[i] != NULL)
				last_field_instance = fields_mem_remapped[i];
			else last_field_instance = fields_mem_transfer[i];
			runtime_inter_averaging_algorithm[i] = new Runtime_cumulate_average_algorithm(last_field_instance, fields_mem_registered[i]);
		}
	}
}


void Connection_coupling_procedure::execute(bool bypass_timer)
{
	Time_mgt *time_mgr = components_time_mgrs->get_time_mgr(inout_interface->get_comp_id());
	Connection_field_time_info *local_fields_time_info, *remote_fields_time_info;
	int lag_seconds;

	finish_status = false;
	transfer_data = false;
	
	for (int i = 0; i < fields_time_info_dst.size(); i ++) {
		if (inout_interface->get_import_or_export_or_remap() == 0) {
			local_fields_time_info = fields_time_info_dst[i];
			remote_fields_time_info = fields_time_info_src[i];
			lag_seconds = local_fields_time_info->lag_seconds;
		}
		else {
			local_fields_time_info = fields_time_info_src[i];
			remote_fields_time_info = fields_time_info_dst[i];
			lag_seconds = -remote_fields_time_info->lag_seconds;
		}
		time_mgr->get_current_time(local_fields_time_info->current_year, local_fields_time_info->current_month, local_fields_time_info->current_day, local_fields_time_info->current_second, 0, "CCPL internal");
		local_fields_time_info->current_num_elapsed_days = time_mgr->get_current_num_elapsed_day();
		if (local_fields_time_info->last_timer_num_elapsed_days != -1) 
			EXECUTION_REPORT(REPORT_ERROR, local_fields_time_info->inout_interface->get_comp_id(), ((long)local_fields_time_info->current_num_elapsed_days)*100000+local_fields_time_info->current_second >= ((long)local_fields_time_info->last_timer_num_elapsed_days)*100000+local_fields_time_info->last_timer_second,
			                 "Software error in Connection_coupling_procedure::execute: current time is earlier than last timer time");
		EXECUTION_REPORT(REPORT_ERROR, local_fields_time_info->inout_interface->get_comp_id(), ((long)local_fields_time_info->current_num_elapsed_days)*100000+local_fields_time_info->current_second <= ((long)local_fields_time_info->next_timer_num_elapsed_days)*100000+local_fields_time_info->next_timer_second,
		                 "Please make sure that the import/export interface \"%s\" is called when the timer of any field is on. Please check the model code with the annotation \"%s\"", 
		                 local_fields_time_info->inout_interface->get_interface_name(), annotation_mgr->get_annotation(local_fields_time_info->inout_interface->get_interface_id(), "registering interface"));
		if (time_mgr->is_timer_on(local_fields_time_info->timer->get_frequency_unit(), local_fields_time_info->timer->get_frequency_count(), local_fields_time_info->timer->get_local_lag_count())) {
			if (((long)local_fields_time_info->current_num_elapsed_days)*100000+local_fields_time_info->current_second == ((long)local_fields_time_info->next_timer_num_elapsed_days)*100000+local_fields_time_info->next_timer_second) {
				local_fields_time_info->last_timer_num_elapsed_days = local_fields_time_info->next_timer_num_elapsed_days;
				local_fields_time_info->last_timer_second = local_fields_time_info->next_timer_second;
				local_fields_time_info->get_time_of_next_timer_on(true);
			}
			while((((long)remote_fields_time_info->current_num_elapsed_days)*((long)SECONDS_PER_DAY))+remote_fields_time_info->current_second+lag_seconds <= (((long)local_fields_time_info->current_num_elapsed_days)*((long)SECONDS_PER_DAY)) + local_fields_time_info->current_second) {
				if (remote_fields_time_info->timer->is_timer_on(remote_fields_time_info->current_year, remote_fields_time_info->current_month, remote_fields_time_info->current_day, remote_fields_time_info->current_second, remote_fields_time_info->current_num_elapsed_days, time_mgr->get_start_year(), time_mgr->get_start_month(), time_mgr->get_start_day(), time_mgr->get_start_second(), time_mgr->get_start_num_elapsed_day(), time_mgr->is_time_advanced())) {
					remote_fields_time_info->last_timer_num_elapsed_days = remote_fields_time_info->current_num_elapsed_days;
					remote_fields_time_info->last_timer_second = remote_fields_time_info->current_second;
				}	
				time_mgr->advance_time(remote_fields_time_info->current_year, remote_fields_time_info->current_month, remote_fields_time_info->current_day, remote_fields_time_info->current_second, remote_fields_time_info->current_num_elapsed_days,  remote_fields_time_info->time_step_in_second);
			}			
			remote_fields_time_info->get_time_of_next_timer_on(false);
		}
	}
	
	if (inout_interface->get_import_or_export_or_remap() == 0) { 
		((Runtime_trans_algorithm*)runtime_data_transfer_algorithm)->receve_data_in_temp_buffer();
		for (int i = fields_time_info_dst.size() - 1; i >= 0; i --) {
			if (bypass_timer) {
				transfer_process_on[i] = true;
				current_remote_fields_time[i] = -1;
				if (inout_interface->get_bypass_counter() == 1)
					EXECUTION_REPORT(REPORT_ERROR, inout_interface->get_comp_id(), last_remote_fields_time[i] == -1, "Software error in Connection_coupling_procedure::execute: wrong last_remote_fields_time 1");
				else EXECUTION_REPORT(REPORT_ERROR, inout_interface->get_comp_id(), inout_interface->get_bypass_counter() - 1 == last_remote_fields_time[i] / ((long)10000000000), "Software error in Connection_coupling_procedure::execute: wrong last_remote_fields_time 2");
				transfer_data = true;
				continue;
			}
			if (fields_time_info_dst[i]->current_num_elapsed_days != fields_time_info_dst[i]->last_timer_num_elapsed_days || fields_time_info_dst[i]->current_second != fields_time_info_dst[i]->last_timer_second) {
				transfer_process_on[i] = false;
				continue;
			}
			current_remote_fields_time[i] = ((long)fields_time_info_src[i]->last_timer_num_elapsed_days) * 100000 + fields_time_info_src[i]->last_timer_second; 
			if (!time_mgr->is_time_out_of_execution(current_remote_fields_time[i]) && current_remote_fields_time[i] != last_remote_fields_time[i]) {  // restart related
				EXECUTION_REPORT(REPORT_LOG, inout_interface->get_comp_id(), true, "Will receive remote data at %ld", current_remote_fields_time[i]);
				transfer_process_on[i] = true;
				last_remote_fields_time[i] = current_remote_fields_time[i];
				transfer_data = true;
			}
			else {
				EXECUTION_REPORT(REPORT_LOG, inout_interface->get_comp_id(), true, "Do not redundantly receive remote data at %ld", last_remote_fields_time[i]);
				transfer_process_on[i] = false;
			}
		}
		if (transfer_data) {
			runtime_data_transfer_algorithm->pass_transfer_parameters(transfer_process_on, current_remote_fields_time);
			runtime_data_transfer_algorithm->run(bypass_timer, inout_interface->get_bypass_counter());
			for (int i = fields_time_info_dst.size() - 1; i >= 0; i --) {
				if (transfer_process_on[i]) {
					if (runtime_remap_algorithms[i] != NULL)
						runtime_remap_algorithms[i]->run(true);
					if (runtime_datatype_transform_algorithms[i] != NULL)
						runtime_datatype_transform_algorithms[i]->run(true);								
					runtime_inter_averaging_algorithm[i]->run(true);
				}				
			}
		}
		finish_status = true;
		for (int i = fields_time_info_dst.size() - 1; i >= 0; i --) {
			if (!transfer_process_on[i])
				continue;
			last_remote_fields_time[i] = runtime_data_transfer_algorithm->get_history_receive_sender_time(i);
			long remote_bypass_counter = last_remote_fields_time[i] / ((long)10000000000);
			if (bypass_timer) 
				EXECUTION_REPORT(REPORT_ERROR, inout_interface->get_comp_id(), remote_bypass_counter == inout_interface->get_bypass_counter(), "Error happens when bypassing the timer to call the import interface \"%s\": this interface call does not receive the data from the corresponding timer bypassed call of the export interface \"%s\" from the component model \"%s\". Please verify. ", inout_interface->get_interface_name(), coupling_connection->src_comp_interfaces[0].second, coupling_connection->src_comp_interfaces[0].first);
			else {
				EXECUTION_REPORT(REPORT_ERROR, inout_interface->get_comp_id(), remote_bypass_counter == 0, "Error happens when using the timer to call the import interface \"%s\": this interface call does not receive the data from the corresponding timer non-bypassed call of the export interface \"%s\" from the component model \"%s\". Please verify. ", inout_interface->get_interface_name(), coupling_connection->src_comp_interfaces[0].second, coupling_connection->src_comp_interfaces[0].first);
				EXECUTION_REPORT(REPORT_ERROR, inout_interface->get_comp_id(), runtime_data_transfer_algorithm->get_history_receive_sender_time(i) == current_remote_fields_time[i], "Software error: Error happens when using the timer to call the import interface \"%s\": this interface call does not receive the data from the corresponding export interface \"%s\" from the component model \"%s\" at the right model time (the receiver wants the imported data at %ld but received the imported data at %d). Please verify. ", inout_interface->get_interface_name(), coupling_connection->src_comp_interfaces[0].second, coupling_connection->src_comp_interfaces[0].first, runtime_data_transfer_algorithm->get_history_receive_sender_time(i), current_remote_fields_time[i]);
			}	
		}
		return;
	}
	else {
		for (int i = fields_time_info_src.size() - 1; i >= 0; i --) {
			if (bypass_timer) {
				transfer_process_on[i] = true;
				current_remote_fields_time[i] = -1;
				EXECUTION_REPORT(REPORT_ERROR, -1, last_remote_fields_time[i] == -1, "Software error in Connection_coupling_procedure::execute: wrong last_remote_fields_time");
				transfer_data = true;
				runtime_inner_averaging_algorithm[i]->run(true);
				runtime_inter_averaging_algorithm[i]->run(true);
				if (runtime_datatype_transform_algorithms[i] != NULL)
					runtime_datatype_transform_algorithms[i]->run(true);
			}
			else {
				Coupling_timer *dst_timer = fields_time_info_dst[i]->timer;
				Coupling_timer *src_timer = fields_time_info_src[i]->timer;			
				lag_seconds = -fields_time_info_dst[i]->lag_seconds;
				transfer_process_on[i] = false;
				if (fields_time_info_src[i]->current_num_elapsed_days != fields_time_info_src[i]->last_timer_num_elapsed_days || fields_time_info_src[i]->current_second != fields_time_info_src[i]->last_timer_second) {
					if (fields_time_info_dst[i]->inst_or_aver == USING_AVERAGE_VALUE)
						runtime_inner_averaging_algorithm[i]->run(false);
					continue;
				}
				runtime_inner_averaging_algorithm[i]->run(true);
				if (((long)fields_time_info_src[i]->current_num_elapsed_days)*SECONDS_PER_DAY+fields_time_info_src[i]->current_second == ((long)fields_time_info_dst[i]->last_timer_num_elapsed_days)*SECONDS_PER_DAY+fields_time_info_dst[i]->last_timer_second+lag_seconds) {
					current_remote_fields_time[i] = ((long)fields_time_info_dst[i]->last_timer_num_elapsed_days)*100000 + fields_time_info_dst[i]->last_timer_second;
					EXECUTION_REPORT(REPORT_ERROR, -1, last_remote_fields_time[i] != current_remote_fields_time[i], "Software error in Connection_coupling_procedure::execute: wrong last_remote_fields_time");
					last_remote_fields_time[i] = current_remote_fields_time[i];
					runtime_inter_averaging_algorithm[i]->run(true);
					if (runtime_datatype_transform_algorithms[i] != NULL) 
						runtime_datatype_transform_algorithms[i]->run(false);
					if (!time_mgr->is_time_out_of_execution(current_remote_fields_time[i])) {  // restart related
						transfer_process_on[i] = true;
						transfer_data = true;
					}
					continue;
				}
				if ((((long)fields_time_info_dst[i]->next_timer_num_elapsed_days)*((long)SECONDS_PER_DAY))+fields_time_info_dst[i]->next_timer_second+lag_seconds < (((long)fields_time_info_src[i]->next_timer_num_elapsed_days)*((long)SECONDS_PER_DAY)) + fields_time_info_src[i]->next_timer_second) {
					current_remote_fields_time[i] = ((long)fields_time_info_dst[i]->next_timer_num_elapsed_days)*100000 + fields_time_info_dst[i]->next_timer_second;
					EXECUTION_REPORT(REPORT_ERROR, -1, last_remote_fields_time[i] != current_remote_fields_time[i], "Software error in Connection_coupling_procedure::execute: wrong last_remote_fields_time");
					last_remote_fields_time[i] = current_remote_fields_time[i];
					runtime_inter_averaging_algorithm[i]->run(true);
					if (runtime_datatype_transform_algorithms[i] != NULL) 
						runtime_datatype_transform_algorithms[i]->run(false);
					if (!time_mgr->is_time_out_of_execution(current_remote_fields_time[i])) {  // restart related
						transfer_process_on[i] = true;
						transfer_data = true;
					}
				}
				else {
					if (fields_time_info_dst[i]->inst_or_aver == USING_AVERAGE_VALUE)
						runtime_inter_averaging_algorithm[i]->run(false);
				}	
			}
		}
		if (!transfer_data)
			finish_status = true;
		if (transfer_data)
			((Runtime_trans_algorithm*)runtime_data_transfer_algorithm)->pass_transfer_parameters(transfer_process_on, current_remote_fields_time);
	}
}


void Connection_coupling_procedure::send_fields(bool bypass_timer)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, inout_interface->get_import_or_export_or_remap() == 1 && !finish_status && transfer_data, "Software error in Connection_coupling_procedure::send_fields");
	finish_status = runtime_data_transfer_algorithm->run(bypass_timer, inout_interface->get_bypass_counter());
}


Field_mem_info *Connection_coupling_procedure::get_data_transfer_field_instance(int i)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, fields_mem_transfer[i] != NULL, "Software error in Connection_coupling_procedure::get_data_transfer_field_instance");
	return fields_mem_transfer[i]; 
}


Inout_interface::Inout_interface(const char *temp_array_buffer, int &buffer_content_iter)
{
	int num_interfaces;


	interface_id = 0;
	read_data_from_array_buffer(interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
	read_data_from_array_buffer(comp_full_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
	read_data_from_array_buffer(fixed_remote_interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
	read_data_from_array_buffer(fixed_remote_comp_full_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
	read_data_from_array_buffer(&import_or_export_or_remap, sizeof(int), temp_array_buffer, buffer_content_iter);
	read_data_from_array_buffer(&num_interfaces, sizeof(int), temp_array_buffer, buffer_content_iter);
	for (int i = 0; i < num_interfaces; i ++) {
		fields_name.push_back(temp_array_buffer+buffer_content_iter-NAME_STR_SIZE);
		buffer_content_iter -= NAME_STR_SIZE;
	}
	Comp_comm_group_mgt_node *comp_node = comp_comm_group_mgt_mgr->search_global_node(comp_full_name);
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_node != NULL, "Software error in Inout_interface::Inout_interface");
	comp_id = comp_node->get_local_node_id();
	inversed_dst_fraction = NULL;
}


Inout_interface::Inout_interface(const char *interface_name, int interface_id, int num_fields, int *field_ids_src, int *field_ids_dst, int timer_id, int inst_or_aver, int array_size_src, int array_size_dst, const char *API_label, const char *annotation)
{
	char child_interface_name[NAME_STR_SIZE];


	sprintf(child_interface_name, "%s_child_export", interface_name);
	children_interfaces.push_back(new Inout_interface(child_interface_name, -1, 1, num_fields, field_ids_src, array_size_src, timer_id, inst_or_aver, "field_instance_IDs_source", "", annotation, API_ID_INTERFACE_REG_NORMAL_REMAP, INTERFACE_TYPE_REGISTER));
	sprintf(child_interface_name, "%s_child_import", interface_name);
	children_interfaces.push_back(new Inout_interface(child_interface_name, -1, 0, num_fields, field_ids_dst, array_size_dst, timer_id, inst_or_aver, "field_instance_IDs_target", "", annotation, API_ID_INTERFACE_REG_NORMAL_REMAP, INTERFACE_TYPE_REGISTER));
	initialize_data(interface_name, interface_id, 2, timer_id, inst_or_aver, field_ids_src, INTERFACE_TYPE_REGISTER, annotation);
	fixed_remote_comp_full_name[0] = '\0';
	fixed_remote_interface_name[0] = '\0';
	this->timer->reset_remote_lag_count();
	children_interfaces[0]->timer->reset_remote_lag_count();
	children_interfaces[1]->timer->reset_remote_lag_count();

	bool same_field_name = true;
	for (int i = 0; i < num_fields; i ++) 
		same_field_name = same_field_name && words_are_the_same(memory_manager->get_field_instance(field_ids_src[i])->get_field_name(),memory_manager->get_field_instance(field_ids_dst[i])->get_field_name());	
	EXECUTION_REPORT(REPORT_ERROR, comp_id, same_field_name, "Error happens when calling API \"%s\" to register an interface named \"%s\": the field instances specified by the parameter \"field_instance_IDs_source\" are not consistent with the field instances specified by the parameter \"field_instance_IDs_target\" (the ith source field instance must have the same field name with the ith target field instance). Please check the model code with the annotation \"%s\".", API_label, interface_name, annotation);	
}


Inout_interface::Inout_interface(const char *interface_name, int interface_id, int import_or_export_or_remap, int num_fields, int *field_ids, int array_size, int timer_id, int inst_or_aver, const char *field_ids_parameter_name, const char *interface_tag, const char *annotation, int API_id, int interface_type)
{	
	common_checking_for_interface_registration(num_fields, field_ids, array_size, timer_id, inst_or_aver, import_or_export_or_remap, interface_name, API_id, interface_type, field_ids_parameter_name, annotation);
	initialize_data(interface_name, interface_id, import_or_export_or_remap, timer_id, inst_or_aver, field_ids, interface_type, annotation);
	fixed_remote_comp_full_name[0] = '\0';
	fixed_remote_interface_name[0] = '\0';
	if (strlen(interface_tag) > 0) {			
		char XML_file_name[NAME_STR_SIZE];
		sprintf(XML_file_name, "%s/all/redirection_configs/%s.import.redirection.xml", comp_comm_group_mgt_mgr->get_config_root_dir(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "")->get_full_name());
		if (!comp_comm_group_mgt_mgr->search_coupling_interface_tag(comp_id,interface_tag,fixed_remote_comp_full_name,fixed_remote_interface_name))
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "Error happens when registering an import/export interface \"%s\": cannot find the corresponding entry in the XML configuration file \"%s\" according to the parameter \"interface_tag\". Please check the model code with the annotation \"%s\" and the XML file. ", interface_name, XML_file_name, annotation);		
		EXECUTION_REPORT(REPORT_ERROR, comp_id, strlen(fixed_remote_comp_full_name) > 0 && strlen(fixed_remote_interface_name) > 0, "Error happens when registering an import/export interface \"%s\": the parameter of \"interface_tag\" cannot be seperated into a component model full name and an interface name when it contains a special character \"$\". Please check the model code with the annotation \"%s\"", interface_name, annotation);
	}

	for (int i = 0; i < num_fields; i ++)
		fields_mem_registered.push_back(memory_manager->get_field_instance(field_ids[i]));
}


void Inout_interface::initialize_data(const char *interface_name, int interface_id, int import_or_export_or_remap, int timer_id, int inst_or_aver, int *field_ids, int interface_type, const char *annotation)
{
	this->interface_id = interface_id;
	this->import_or_export_or_remap = import_or_export_or_remap;
	this->execution_checking_status = 0;
	this->last_execution_time = -1;
	Coupling_timer *existing_timer = timer_mgr->get_timer(timer_id);
	this->timer = new Coupling_timer(existing_timer->get_comp_id(), -1, existing_timer);
	this->comp_id = this->timer->get_comp_id();
	this->interface_type = interface_type;
	this->inversed_dst_fraction = NULL;
	strcpy(this->interface_name, interface_name);
	strcpy(this->comp_full_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Inout_interface::initialize_data")->get_full_name());
	this->inst_or_aver = inst_or_aver;
	annotation_mgr->add_annotation(interface_id, "registering interface", annotation);
	time_mgr = components_time_mgrs->get_time_mgr(comp_id);
	this->bypass_counter = 0;
}


Inout_interface::~Inout_interface()
{
	if (inversed_dst_fraction != NULL)
		delete [] inversed_dst_fraction;
}


void Inout_interface::common_checking_for_interface_registration(int num_fields, int *field_ids, int array_size, int timer_id, int inst_or_aver, int import_or_export_or_remap, const char *interface_name, int API_id, int interface_type, const char *field_ids_parameter_name, const char *annotation)
{
	int comp_id = -1;
	char str[NAME_STR_SIZE], API_label[NAME_STR_SIZE];


	get_API_hint(-1, API_id, API_label);
	
	EXECUTION_REPORT(REPORT_ERROR, -1, num_fields > 0, "Error happens when calling API \"%s\" to register an interface named \"%s\": the parameter \"num_field_instances\" cannot be smaller than 1. Please verify the model code with the annotation \"%s\".", API_label, interface_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, -1, num_fields <= array_size, "Error happens when calling API \"%s\" to register an interface named \"%s\": the array size of parameter \"%s\" cannot be smaller than the parameter \"num_field_instances\". Please verify the model code with the annotation \"%s\".", API_label, interface_name, field_ids_parameter_name, annotation);
	for (int i = 0; i < num_fields; i ++) {
		if (interface_type != INTERFACE_TYPE_IO_WRITE)
			EXECUTION_REPORT(REPORT_ERROR, -1, memory_manager->check_is_legal_field_instance_id(field_ids[i]) && memory_manager->get_field_instance(field_ids[i])->get_is_registered_model_buf(), "Error happens when calling API \"%s\" to register an interface named \"%s\": the parameter \"%s\" contains wrong field instance ID. Please verify the model code related to the annotation \"%s\"", API_label, interface_name, field_ids_parameter_name, annotation);
		if (i == 0)
			comp_id = memory_manager->get_field_instance(field_ids[i])->get_comp_id();
		EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_id == memory_manager->get_field_instance(field_ids[i])->get_comp_id(), "Error happens when calling API \"%s\" to register an interface named \"%s\": the field instances specified via the parameter \"%s\" cannot correspond to different component models. Please verify the model code with the annotation \"%s\".", API_label, interface_name, field_ids_parameter_name, annotation);
	}
	EXECUTION_REPORT(REPORT_ERROR, comp_id, timer_mgr->check_is_legal_timer_id(timer_id), "Error happens when calling API \"%s\" to register an interface named \"%s\": the parameter \"timer_ID\" is not the legal ID of a timer. Please verify the model code related to the annotation \"%s\"", API_label, interface_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_id == timer_mgr->get_timer(timer_id)->get_comp_id(), "Error happens when calling API \"%s\" to register an interface named \"%s\": the parameter \"timer_ID\" and the parameter \"%s\" do not correspond to the same component model. Please verify the model code related to the annotation \"%s\"", API_label, interface_name, field_ids_parameter_name, annotation);
	if (interface_type != INTERFACE_TYPE_IO_WRITE && import_or_export_or_remap == 1)
		for (int i = 0; i < num_fields; i ++) 
			for (int j = i+1; j < num_fields; j ++)
				EXECUTION_REPORT(REPORT_ERROR, comp_id, !words_are_the_same(memory_manager->get_field_instance(field_ids[i])->get_field_name(),memory_manager->get_field_instance(field_ids[j])->get_field_name()), "Error happens when calling API \"%s\" to register an interface named \"%s\": the parameter \"%s\" is not allowed to include more than one instance of the field \"%s\". Please verify the model code related to the annotation \"%s\"", API_label, interface_name, field_ids_parameter_name, memory_manager->get_field_instance(field_ids[i])->get_field_name(), annotation);			

	sprintf(str, "registerring an interface named \"%s\"", interface_name);
	synchronize_comp_processes_for_API(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Inout_interface::Inout_interface"), str, annotation);
	
	if (interface_type == INTERFACE_TYPE_REGISTER)
		comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(comp_id, API_id, annotation);	
	check_and_verify_name_format_of_string_for_API(comp_id, interface_name, API_id, "the interface", annotation);
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in Inout_interface::Inout_interface"), NULL, num_fields, "num_field_instances", annotation);
	sprintf(str, "\"%s\" (the information of the field instances)", field_ids_parameter_name);
	for (int i = 0; i < num_fields; i ++)
		check_API_parameter_field_instance(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Inout_interface::Inout_interface"), "registerring an interface", field_ids[i], str, annotation);
	check_API_parameter_timer(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Inout_interface::Inout_interface"), "registerring an interface", timer_id, "timer_ID (the information of the timer)", annotation);
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Inout_interface::Inout_interface"), "registerring an interface", inst_or_aver, "inst_or_aver (the tag for using instantaneous or time averaged field value)", annotation);
}


void Inout_interface::report_common_field_instances(const Inout_interface *another_interface)
{
	if (this->interface_type != INTERFACE_TYPE_REGISTER || another_interface->interface_type != INTERFACE_TYPE_REGISTER)
		return;

	if (this->import_or_export_or_remap == 0 && another_interface->import_or_export_or_remap == 0) {
		for (int i = 0; i < this->fields_mem_registered.size(); i ++)
			for (int j = 0; j < another_interface->fields_mem_registered.size(); j ++)
				EXECUTION_REPORT(REPORT_ERROR, comp_id, this->fields_mem_registered[i]->get_data_buf() != another_interface->fields_mem_registered[j]->get_data_buf(), "Two import interfaces (\"%s\" and \"%s\") share the same data buffer of the field (field name is \"%s\") which is not allowed. Please check the model code with the annotation \"%s\" and \"%s\".",
				                 this->interface_name, another_interface->interface_name, fields_mem_registered[i]->get_field_name(), annotation_mgr->get_annotation(this->interface_id, "registering interface"), annotation_mgr->get_annotation(another_interface->interface_id, "registering interface"));		
		return;
	}

	if (this->import_or_export_or_remap != 1 || another_interface->import_or_export_or_remap != 1)
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


void Inout_interface::transform_interface_into_array(char **temp_array_buffer, int &buffer_max_size, int &buffer_content_size)
{
	int temp_int;

	
	for (int i = fields_mem_registered.size()-1; i >= 0 ; i --)
		write_data_into_array_buffer(fields_mem_registered[i]->get_field_name(), NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
	temp_int = fields_mem_registered.size();
	write_data_into_array_buffer(&temp_int, sizeof(int), temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&import_or_export_or_remap, sizeof(int), temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(fixed_remote_comp_full_name, NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(fixed_remote_interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(comp_full_name, NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_max_size, buffer_content_size);
}


void Inout_interface::add_coupling_procedure(Connection_coupling_procedure *coupling_procedure)
{
	coupling_procedures.push_back(coupling_procedure);
}


void Inout_interface::preprocessing_for_frac_based_remapping()
{
	for (int i = 0; i < fields_mem_registered.size()-1; i ++) {
		if (words_are_the_same(fields_mem_registered[i]->get_data_type(), DATA_TYPE_FLOAT))
			if (words_are_the_same(fields_mem_registered[fields_mem_registered.size()-1]->get_data_type(), DATA_TYPE_FLOAT))
				arrays_multiplication_template((float*)fields_mem_registered[i]->get_data_buf(), (float*)fields_mem_registered[fields_mem_registered.size()-1]->get_data_buf(), (float*)children_interfaces[0]->fields_mem_registered[i]->get_data_buf(), fields_mem_registered[i]->get_size_of_field());
			else arrays_multiplication_template((float*)fields_mem_registered[i]->get_data_buf(), (double*)fields_mem_registered[fields_mem_registered.size()-1]->get_data_buf(), (float*)children_interfaces[0]->fields_mem_registered[i]->get_data_buf(), fields_mem_registered[i]->get_size_of_field());
		else if (words_are_the_same(fields_mem_registered[fields_mem_registered.size()-1]->get_data_type(), DATA_TYPE_FLOAT))
			arrays_multiplication_template((double*)fields_mem_registered[i]->get_data_buf(), (float*)fields_mem_registered[fields_mem_registered.size()-1]->get_data_buf(), (double*)children_interfaces[0]->fields_mem_registered[i]->get_data_buf(), fields_mem_registered[i]->get_size_of_field());
		else arrays_multiplication_template((double*)fields_mem_registered[i]->get_data_buf(), (double*)fields_mem_registered[fields_mem_registered.size()-1]->get_data_buf(), (double*)children_interfaces[0]->fields_mem_registered[i]->get_data_buf(), fields_mem_registered[i]->get_size_of_field());
		children_interfaces[0]->fields_mem_registered[i]->define_field_values(false);
	}	
}


void Inout_interface::postprocessing_for_frac_based_remapping(bool bypass_timer)
{
	Field_mem_info *dst_value_field, *dst_frac_field;

	
	if (!timer->is_timer_on() && !bypass_timer)
		return;

	dst_frac_field = children_interfaces[1]->fields_mem_registered[fields_mem_registered.size()-1];

	if (words_are_the_same(dst_frac_field->get_data_type(), DATA_TYPE_FLOAT)) {
		float *dst_frac_buf = (float*)dst_frac_field->get_data_buf();
		for (int i = dst_frac_field->get_size_of_field()-1; i >= 0; i --)
			if (dst_frac_buf[i] == (float) 0.0)
				((float*) inversed_dst_fraction)[i] = dst_frac_buf[i];
			else ((float*) inversed_dst_fraction)[i] = ((float)1.0) / dst_frac_buf[i];	
	}
	else {
		double *dst_frac_buf = (double*)dst_frac_field->get_data_buf();
		for (int i = dst_frac_field->get_size_of_field()-1; i >= 0; i --)
			if (dst_frac_buf[i] == (double) 0.0)
				((double*) inversed_dst_fraction)[i] = dst_frac_buf[i];
			else ((double*) inversed_dst_fraction)[i] = ((double)1.0) / dst_frac_buf[i];	
	}
	
	for (int i = 0; i < fields_mem_registered.size()-1; i ++) {
		dst_value_field = children_interfaces[1]->fields_mem_registered[i];
		if (words_are_the_same(dst_value_field->get_data_type(), DATA_TYPE_FLOAT))
			if (words_are_the_same(dst_frac_field->get_data_type(), DATA_TYPE_FLOAT))
				arrays_multiplication_template((float*)dst_value_field->get_data_buf(), (float*)inversed_dst_fraction, (float*)dst_value_field->get_data_buf(), dst_frac_field->get_size_of_field());
			else arrays_multiplication_template((float*)dst_value_field->get_data_buf(), (double*)inversed_dst_fraction, (float*)dst_value_field->get_data_buf(), dst_frac_field->get_size_of_field());
		else if (words_are_the_same(dst_frac_field->get_data_type(), DATA_TYPE_FLOAT))
			arrays_multiplication_template((double*)dst_value_field->get_data_buf(), (float*)inversed_dst_fraction, (double*)dst_value_field->get_data_buf(), dst_frac_field->get_size_of_field());
		else arrays_multiplication_template((double*)dst_value_field->get_data_buf(), (double*)inversed_dst_fraction, (double*)dst_value_field->get_data_buf(), dst_frac_field->get_size_of_field());
	}	
}


void Inout_interface::execute(bool bypass_timer, const char *annotation)
{
	bool at_first_normal_step = false;

	
	if (bypass_timer)
		bypass_counter ++;
	if (time_mgr->check_is_model_run_finished()) {
		EXECUTION_REPORT(REPORT_WARNING, comp_id, false, "The import/export interface \"%s\" (corresponding to the model code annotation \"%s\") will not execute at time %08d-%05d because the model run has finished",
			             interface_name, annotation_mgr->get_annotation(interface_id, "registering interface"), time_mgr->get_current_date(), time_mgr->get_current_second());
		return;
	}

	EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_comm_group_mgt_mgr->get_is_definition_finalized() || coupling_procedures.size() > 0, "Cannot execute the interface \"%s\" corresponding to the model code with the annotation \"%s\" because the coupling procedures of this interface have not been generated. Please verify.", interface_name, annotation);
	if (bypass_timer && (execution_checking_status & 0x2) != 0)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "The timers of the import/export interface \"%s\" cannot be bypassed again (the corresponding annotation of the model code is \"%s\") because the timers have been bypassed before", interface_name, annotation, annotation_mgr->get_annotation(interface_id, "using timer"));
	if ((execution_checking_status & 0x1) == 0 && bypass_timer || (execution_checking_status & 0x2) == 0 && !bypass_timer) {
		synchronize_comp_processes_for_API(comp_id, API_ID_INTERFACE_EXECUTE, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "software error")->get_comm_group(), "executing an import/export interface", annotation);
		check_API_parameter_string(comp_id, API_ID_INTERFACE_EXECUTE, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"executing an import/export interface"), "executing an import/export interface", interface_name, "the corresponding interface name", annotation);
		int bypass_timer_int;
		if (bypass_timer)
			bypass_timer_int = 0;
		else bypass_timer_int = 1;
		check_API_parameter_int(comp_id, API_ID_INTERFACE_EXECUTE, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"executing an import/export interface"), NULL, bypass_timer_int, "the value for specifying whether bypass timers", annotation);
		if (bypass_timer) {
			execution_checking_status = execution_checking_status | 0x1;
			annotation_mgr->add_annotation(interface_id, "bypassing timer", annotation);
		}
		else {
			at_first_normal_step = (execution_checking_status & 0x2) == 0;
			execution_checking_status = execution_checking_status | 0x2;
			annotation_mgr->add_annotation(interface_id, "using timer", annotation);
		}
	}

	long current_execution_time = ((long)time_mgr->get_current_num_elapsed_day())*100000 + components_time_mgrs->get_time_mgr(comp_id)->get_current_second();
	if (current_execution_time == last_execution_time && !bypass_timer && !at_first_normal_step) {
		int current_year, current_month, current_day, current_second;
		components_time_mgrs->get_time_mgr(comp_id)->get_current_time(current_year, current_month, current_day, current_second, 0, "CCPL internal");
		EXECUTION_REPORT(REPORT_WARNING, comp_id, false, "The import/export interface \"%s\", which is called at the model code with the annotation \"%s\", will not be executed again at the time step %04d-%02d-%02d-%05d, because it has been executed at the same time step before.",
			             interface_name, annotation, current_year, current_month, current_day, current_second);
		return;
	}

	last_execution_time = current_execution_time;

	if (import_or_export_or_remap >= 2) {
		if (import_or_export_or_remap == 3)
			preprocessing_for_frac_based_remapping();
		children_interfaces[0]->execute(bypass_timer, annotation);
		children_interfaces[1]->execute(bypass_timer, annotation);
		if (import_or_export_or_remap == 3)
			postprocessing_for_frac_based_remapping(bypass_timer);
		return;
	}

	if (import_or_export_or_remap == 1) {
		for (int i = 0; i < fields_mem_registered.size(); i ++)
			fields_mem_registered[i]->check_field_sum("before executing an export interface");
	}
	
	for (int i = 0; i < coupling_procedures.size(); i ++)
		coupling_procedures[i]->execute(bypass_timer);

	if (import_or_export_or_remap == 1) {
		bool all_finish = false;
		while (!all_finish) {
			all_finish = true;
			for (int i = 0; i < coupling_procedures.size(); i ++) {
				if (!coupling_procedures[i]->get_finish_status())
					coupling_procedures[i]->send_fields(bypass_timer);
				all_finish = all_finish && coupling_procedures[i]->get_finish_status();
			}	
		}
	}

	if (import_or_export_or_remap == 0) {
		for (int i = 0; i < fields_mem_registered.size(); i ++) {
			if (words_are_the_same(fields_mem_registered[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT))
				EXECUTION_REPORT(REPORT_LOG, comp_id, true, "import interface %s get field instance (%s) with value %f : %f\n", interface_name, fields_mem_registered[i]->get_field_name(), ((float*) fields_mem_registered[i]->get_data_buf())[0], ((float*) fields_mem_registered[i]->get_data_buf())[fields_mem_registered[i]->get_size_of_field()-1]);
			else if (words_are_the_same(fields_mem_registered[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE))
				EXECUTION_REPORT(REPORT_LOG, comp_id, true, "import interface %s get field instance (%s) with value %lf : %lf\n", interface_name, fields_mem_registered[i]->get_field_name(), ((double*) fields_mem_registered[i]->get_data_buf())[0], ((double*) fields_mem_registered[i]->get_data_buf())[fields_mem_registered[i]->get_size_of_field()-1]);
			else if (words_are_the_same(fields_mem_registered[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_INT))
				EXECUTION_REPORT(REPORT_LOG, comp_id, true, "import interface %s get field instance (%s) with value %d : %d\n", interface_name, fields_mem_registered[i]->get_field_name(), ((int*) fields_mem_registered[i]->get_data_buf())[0], ((int*) fields_mem_registered[i]->get_data_buf())[fields_mem_registered[i]->get_size_of_field()-1]);		
			fields_mem_registered[i]->check_field_sum("after executing an export interface");
		}
	}
}


Inout_interface *Inout_interface::get_child_interface(int i)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, i >= 0 && i < children_interfaces.size(), "Software error in Inout_interface::get_child_interface");

	return children_interfaces[i];
}


void Inout_interface::add_remappling_fraction_processing(void *frac_src, void *frac_dst, int size_frac_src, int size_frac_dst, const char *frac_data_type, const char *API_label, const char *annotation)
{
	for (int j = 0; j < 2; j ++) {
		EXECUTION_REPORT(REPORT_ERROR, -1, children_interfaces[j]->coupling_procedures.size() == 1 && children_interfaces[j]->coupling_procedures[0]->get_num_runtime_remap_algorithms() == children_interfaces[j]->fields_mem_registered.size(), "Software error in Inout_interface::add_remappling_fraction_processing");
		for (int i = 0; i < children_interfaces[j]->fields_mem_registered.size(); i ++) {
			Original_grid_info *field_grid = original_grid_mgr->get_original_grid(children_interfaces[j]->fields_mem_registered[i]->get_grid_id());
			EXECUTION_REPORT(REPORT_ERROR, comp_id, field_grid != NULL && field_grid->is_H2D_grid(), "Error happens when calling the API \"%s\" to register an interface named \"%s\": field \"%s\" is not on an horizontal grid. Please verify	the model code model with the annotation \"%s\"", API_label, interface_name, children_interfaces[j]->fields_mem_registered[i]->get_field_name(), annotation);
			EXECUTION_REPORT(REPORT_ERROR, comp_id, children_interfaces[j]->coupling_procedures[0]->get_runtime_remap_algorithm(i) == NULL && children_interfaces[j]->coupling_procedures[0]->get_runtime_remap_algorithm(0) == NULL || children_interfaces[j]->coupling_procedures[0]->get_runtime_remap_algorithm(i) != NULL && children_interfaces[j]->coupling_procedures[0]->get_runtime_remap_algorithm(0) != NULL && children_interfaces[j]->coupling_procedures[0]->get_runtime_remap_algorithm(i)->get_runtime_remapping_weights() == children_interfaces[j]->coupling_procedures[0]->get_runtime_remap_algorithm(0)->get_runtime_remapping_weights(), 
				             "Error happens when calling the API \"%s\" to register an interface named \"%s\": The fields to be remapped do not share the same remapping weights. Please verify the model code model with the annotation \"%s\".", API_label, interface_name, annotation);
		}
	}
	Field_mem_info *template_field_src = children_interfaces[0]->fields_mem_registered[0];
	Field_mem_info *template_field_dst = children_interfaces[1]->fields_mem_registered[0];
	EXECUTION_REPORT(REPORT_ERROR, comp_id, template_field_src->get_size_of_field() == size_frac_src, "Error happens when calling the API \"%s\" to register an interface named \"%s\": the array size of the parameter \"frac_src\" is different from the size of each source field instance. Please verify the model code model with the annotation \"%s\".", API_label, interface_name, annotation);
	int has_frac_dst = size_frac_dst == -1? 0 : 1;
	check_API_parameter_int(comp_id, API_ID_INTERFACE_REG_FRAC_REMAP, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in add_remappling_fraction_processing")->get_comm_group(), "specification (or not)", has_frac_dst, "frac_dst", annotation);
	if (size_frac_dst != -1)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, template_field_dst->get_size_of_field() == size_frac_dst, "Error happens when calling the API \"%s\" to register an interface named \"%s\": the array size of the parameter \"frac_dst\" is different from the size of each target field instance. Please verify	the model code model with the annotation \"%s\".", API_label, interface_name, annotation);
	Field_mem_info *frac_field_src = memory_manager->alloc_mem("remap_frac", template_field_src->get_decomp_id(), template_field_src->get_comp_or_grid_id(), BUF_MARK_REMAP_FRAC ^ coupling_generator->get_latest_connection_id(), frac_data_type, "unitless", "source fraction for remapping", false);
	frac_field_src->reset_mem_buf(frac_src, true);
	Field_mem_info *frac_field_dst = memory_manager->alloc_mem("remap_frac", template_field_dst->get_decomp_id(), template_field_dst->get_comp_or_grid_id(), BUF_MARK_REMAP_FRAC ^ coupling_generator->get_latest_connection_id(), frac_data_type, "unitless", "target fraction for remapping", false);
	if (size_frac_dst != -1) 
		frac_field_dst->reset_mem_buf(frac_dst, true);

	import_or_export_or_remap = 3;
	EXECUTION_REPORT(REPORT_ERROR, -1, fields_mem_registered.size() == 0, "Software error in Inout_interface::add_remappling_fraction_processing");
	for (int i = 0; i < children_interfaces[0]->fields_mem_registered.size(); i ++) {
		fields_mem_registered.push_back(children_interfaces[0]->fields_mem_registered[i]);
		children_interfaces[0]->fields_mem_registered[i] = memory_manager->alloc_mem(fields_mem_registered[i], BUF_MARK_REMAP_FRAC, coupling_generator->get_latest_connection_id(), fields_mem_registered[i]->get_data_type(), true);
	}
	fields_mem_registered.push_back(frac_field_src);
	
	children_interfaces[0]->fields_mem_registered.push_back(frac_field_src);
	children_interfaces[1]->fields_mem_registered.push_back(frac_field_dst);
	delete children_interfaces[0]->coupling_procedures[0];
	delete children_interfaces[1]->coupling_procedures[0];
	children_interfaces[0]->coupling_procedures.clear();
	children_interfaces[1]->coupling_procedures.clear();
	int num_fields = children_interfaces[0]->fields_mem_registered.size();
	int *field_ids_src = new int [num_fields];
	for (int i = 0; i < num_fields; i ++)
		field_ids_src[i] = children_interfaces[0]->fields_mem_registered[i]->get_field_instance_id();
	inout_interface_mgr->generate_remapping_interface_connection(this, num_fields, field_ids_src, true);
	delete [] field_ids_src;

	if (frac_field_dst->get_size_of_field() > 0)
		inversed_dst_fraction = new char [frac_field_dst->get_size_of_field()*get_data_type_size(frac_field_dst->get_data_type())];
}


Inout_interface_mgt::Inout_interface_mgt(const char *temp_array_buffer, int buffer_content_iter)
{
	this->temp_array_buffer = NULL;
	while (buffer_content_iter > 0)
		interfaces.push_back(new Inout_interface(temp_array_buffer, buffer_content_iter));
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


void Inout_interface_mgt::generate_remapping_interface_connection(Inout_interface *new_interface, int num_fields, int *field_ids_src, bool has_frac_remapping) 
{
	Coupling_connection *coupling_connection = new Coupling_connection(coupling_generator->apply_connection_id());
	Inout_interface *child_interface_export = new_interface->get_child_interface(0);
	Inout_interface *child_interface_import = new_interface->get_child_interface(1);
	std::pair<char[NAME_STR_SIZE],char[NAME_STR_SIZE]> src_comp_interface;

	interfaces.push_back(new_interface);
	strcpy(coupling_connection->dst_comp_full_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(new_interface->get_comp_id(), "in Inout_interface_mgt::register_normal_remap_interface")->get_full_name());
	strcpy(coupling_connection->dst_interface_name, child_interface_import->get_interface_name());
	for (int i = 0; i < num_fields; i ++)
		coupling_connection->fields_name.push_back(strdup(memory_manager->get_field_instance(field_ids_src[i])->get_field_name()));
	strcpy(src_comp_interface.first, coupling_connection->dst_comp_full_name);
	strcpy(src_comp_interface.second, child_interface_export->get_interface_name());
	coupling_connection->src_comp_interfaces.push_back(src_comp_interface);

	interfaces.push_back(child_interface_export);
	interfaces.push_back(child_interface_import);

	coupling_connection->generate_a_coupling_procedure(has_frac_remapping);
	delete coupling_connection;

	interfaces.erase(interfaces.begin()+interfaces.size()-1);
	interfaces.erase(interfaces.begin()+interfaces.size()-1);
}


int Inout_interface_mgt::register_normal_remap_interface(const char *interface_name, int num_fields, int *field_ids_src, int *field_ids_dst, int timer_id, int inst_or_aver, int array_size_src, int array_size_dst, const char *API_label, const char *annotation)
{
	Inout_interface *new_interface = new Inout_interface(interface_name, get_next_interface_id(), num_fields, field_ids_src, field_ids_dst, timer_id, inst_or_aver, array_size_src, array_size_dst, API_label, annotation);
	Inout_interface *existing_interface = get_interface(new_interface->get_comp_id(), interface_name);
	if (existing_interface != NULL)
		EXECUTION_REPORT(REPORT_ERROR, new_interface->get_comp_id(), existing_interface == NULL, "Error happens when calling API \"%s\" to register an interface named \"%s\" at the model code model with the annotation \"%s\": an interface with the same name has already been registered at the model code with the annotation \"%s\". Please verify.", API_label, interface_name, annotation, annotation_mgr->get_annotation(existing_interface->get_interface_id(), "registering interface"));
	generate_remapping_interface_connection(new_interface, num_fields, field_ids_src, false);
	
	return new_interface->get_interface_id();
}


int Inout_interface_mgt::register_frac_based_remap_interface(const char *interface_name, int num_fields, int *field_ids_src, int *field_ids_dst, int timer_id, int inst_or_aver, int array_size_src, int array_size_dst, void *frac_src, void *frac_dst, int size_frac_src, int size_frac_dst, const char *frac_data_type, const char *API_label, const char *annotation)
{
	int new_remap_interface_id = register_normal_remap_interface(interface_name, num_fields, field_ids_src, field_ids_dst, timer_id, inst_or_aver, array_size_src, array_size_dst, API_label, annotation);
	Inout_interface *new_remap_interface = get_interface(new_remap_interface_id);
	new_remap_interface->add_remappling_fraction_processing(frac_src, frac_dst, size_frac_src, size_frac_dst, frac_data_type, API_label, annotation);
	return new_remap_interface->get_interface_id();
}


int Inout_interface_mgt::register_inout_interface(const char *interface_name, int import_or_export_or_remap, int num_fields, int *field_ids, int array_size, int timer_id, int inst_or_aver, const char *interface_tag, const char *annotation, int interface_type)
{
	int API_id = import_or_export_or_remap == 0? API_ID_INTERFACE_REG_IMPORT : API_ID_INTERFACE_REG_EXPORT;
	Inout_interface *new_interface = new Inout_interface(interface_name, get_next_interface_id(), import_or_export_or_remap, num_fields, field_ids, array_size, timer_id, inst_or_aver, "field_instance_IDs", interface_tag, annotation, API_id, interface_type);
	for (int i = 0; i < interfaces.size(); i ++) {
		if (new_interface->get_comp_id() != interfaces[i]->get_comp_id())
			continue;
		EXECUTION_REPORT(REPORT_ERROR, new_interface->get_comp_id(), !words_are_the_same(interface_name, interfaces[i]->get_interface_name()), 
		                 "cannot register the import/export interface named \"%s\" at the model code with the annotation \"%s\" because an interface with the same name has been registerred at the model code with the annotation \"%s\"",
		                 interface_name, annotation, annotation_mgr->get_annotation(interfaces[i]->get_interface_id(), "registering interface"));
		//new_interface->report_common_field_instances(interfaces[i]);
		
	}
	interfaces.push_back(new_interface);
	return new_interface->get_interface_id();
}


int Inout_interface_mgt::get_next_interface_id()
{
	return TYPE_INOUT_INTERFACE_ID_PREFIX|interfaces.size();
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


void Inout_interface_mgt::merge_unconnected_inout_interface_fields_info(int comp_id)
{
	MPI_Comm comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in merge_unconnected_inout_interface_fields_info");
	int local_proc_id = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_id, "in merge_unconnected_inout_interface_fields_info");
	int num_local_procs = comp_comm_group_mgt_mgr->get_num_proc_in_comp(comp_id, "in merge_unconnected_inout_interface_fields_info");
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
			if (interfaces[i]->get_comp_id() == comp_id && interfaces[i]->get_num_coupling_procedures() == 0)
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
		if (interfaces[i]->get_comp_id() == comp_id && interfaces[i]->get_import_or_export_or_remap() == 0)
			import_interfaces.push_back(interfaces[i]);
}


void Inout_interface_mgt::get_all_unconnected_fixed_interfaces(std::vector<Inout_interface*> &import_interfaces, int comp_id, int import_or_export, const char *remote_comp_name)
{
	for (int i = 0; i < interfaces.size(); i ++)
		if ((comp_id == -1 || interfaces[i]->get_comp_id() == comp_id) && interfaces[i]->get_import_or_export_or_remap() == import_or_export && interfaces[i]->get_num_coupling_procedures() == 0 && 
			 strlen(interfaces[i]->get_fixed_remote_comp_full_name()) > 0 && (remote_comp_name == NULL || words_are_the_same(remote_comp_name, interfaces[i]->get_fixed_remote_comp_full_name())))
			import_interfaces.push_back(interfaces[i]);
}


void Inout_interface_mgt::write_all_interfaces_fields_info()
{
	const char *field_name;

	
 	for (int i = 0; i < interfaces.size(); i ++) {
		if (interfaces[i]->get_import_or_export_or_remap() == 0)
			printf("import interface \"%s\" of component \"%s\" has %d fields\n", interfaces[i]->get_interface_name(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(interfaces[i]->get_comp_id(),"in write_all_interfaces_fields_info")->get_comp_full_name(), interfaces[i]->get_num_fields());
		else printf("export interface \"%s\" of component \"%s\" has %d fields\n", interfaces[i]->get_interface_name(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(interfaces[i]->get_comp_id(),"in write_all_interfaces_fields_info")->get_comp_full_name(), interfaces[i]->get_num_fields());
		for (int j = 0; (field_name = interfaces[i]->get_field_name(j)) != NULL; j ++) {
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
	EXECUTION_REPORT(REPORT_LOG, inout_interface->get_comp_id(), true, "Begin to execute interface \"%s\" (model code annotation is \"%s\")", inout_interface->get_interface_name(), annotation);	
	inout_interface->execute(bypass_timer, annotation);
	EXECUTION_REPORT(REPORT_LOG, inout_interface->get_comp_id(), true, "Finishing executing interface \"%s\" (model code annotation is \"%s\")", inout_interface->get_interface_name(), annotation);
}


void Inout_interface_mgt::execute_interface(int comp_id, const char *interface_name, bool bypass_timer, const char *annotation)
{
	Inout_interface *inout_interface;

	
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "0x%x is not an legal ID of a component. Please check the model code with the annotation \"%s\"", comp_id, annotation);
	inout_interface = get_interface(comp_id, interface_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, inout_interface != NULL, "Registered interface of this component does not contain an import/export interface named \"%s\". Please check the model code with the annotation \"%s\"", interface_name, annotation);
	EXECUTION_REPORT(REPORT_LOG, inout_interface->get_comp_id(), true, "Begin to execute interface \"%s\" (model code annotation is \"%s\")", inout_interface->get_interface_name(), annotation);	
	inout_interface->execute(bypass_timer, annotation);
	EXECUTION_REPORT(REPORT_LOG, inout_interface->get_comp_id(), true, "Finishing executing interface \"%s\" (model code annotation is \"%s\")", inout_interface->get_interface_name(), annotation);
}


void Inout_interface_mgt::runtime_receive_algorithms_receive_data()
{
	for (int i = 0; i < all_runtime_receive_algorithms.size(); i ++)
		all_runtime_receive_algorithms[i]->receve_data_in_temp_buffer();
}

