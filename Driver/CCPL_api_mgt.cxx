/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "CCPL_api_mgt.h"
#include "global_data.h"


void get_API_hint(int comp_id, int API_id, char *API_label)
{
	switch(API_id) {
        case API_ID_FINALIZE:
			sprintf(API_label, "CCPL_finalize");
			break;
        case API_ID_COMP_MGT_REG_ROOT_COMP:
			sprintf(API_label, "CCPL_register_root_component");
			break;
        case API_ID_COMP_MGT_REG_COMP:
			sprintf(API_label, "CCPL_register_component");
			break;
        case API_ID_COMP_MGT_END_COMP_REG:
			sprintf(API_label, "CCPL_end_coupling_configuration");
			break;
		case API_ID_COMP_MGT_GET_COMP_ID:
			sprintf(API_label, "CCPL_get_component_id");
			break;
		case API_ID_COMP_MGT_GET_CURRENT_PROC_ID_IN_COMP:
			sprintf(API_label, "CCPL_get_current_process_id_in_component");
			break;
		case API_ID_COMP_MGT_GET_NUM_PROC_IN_COMP:
			sprintf(API_label, "CCPL_get_num_process_in_component");
			break;	
        case API_ID_GRID_MGT_REG_H2D_GRID_VIA_MODEL_DATA:
			sprintf(API_label, "CCPL_register_H2D_grid_via_model_data");
			break;
        case API_ID_GRID_MGT_REG_H2D_GRID_VIA_FILE:
			sprintf(API_label, "CCPL_register_H2D_grid_via_data_file");
			break;
        case API_ID_GRID_MGT_REG_1D_GRID_ONLINE:
			sprintf(API_label, "CCPL_register_1D_grid");
			break;
        case API_ID_GRID_MGT_REG_GRID_VIA_COR:
			sprintf(API_label, "CCPL_get_CoR_defined_grid");
			break;
		case API_ID_GRID_MGT_GET_GRID_SIZE:
			sprintf(API_label, "CCPL_get_grid_size");
			break;			
        case API_ID_GRID_MGT_REG_GRID_VIA_LOCAL:
			sprintf(API_label, "CCPL_get_local_grid");
			break;
        case API_ID_GRID_MGT_REG_H2D_GRID_VIA_COMP:
			sprintf(API_label, "CCPL_register_H2D_grid_from_another_component");
			break;
        case API_ID_GRID_MGT_CMP_GRID_VIA_REMOTE:
			sprintf(API_label, "CCPL_compare_to_remote_grid");
			break;
        case API_ID_GRID_MGT_GET_GRID_ID:
			sprintf(API_label, "CCPL_get_grid_id");
			break;
        case API_ID_GRID_MGT_SET_GRID_DATA:
			sprintf(API_label, "CCPL_set_grid_data");
			break;
        case API_ID_GRID_MGT_SET_3D_GRID_DYN_BOT_FLD:
			sprintf(API_label, "CCPL_set_3D_grid_dynamic_bottom_field");
			break;
        case API_ID_GRID_MGT_SET_3D_GRID_STATIC_BOT_FLD:
			sprintf(API_label, "CCPL_set_3D_grid_static_bottom_field");
			break;
		case API_ID_GRID_MGT_SET_3D_GRID_EXTERNAL_BOT_FLD:
			sprintf(API_label, "CCPL_set_3D_grid_external_bottom_field");
			break;			
        case API_ID_GRID_MGT_GET_GRID_GLOBAL_DATA:
			sprintf(API_label, "CCPL_get_global_grid_data");
			break;
        case API_ID_GRID_MGT_GET_GRID_LOCAL_DATA:
			sprintf(API_label, "CCPL_get_local_grid_data");
			break;
        case API_ID_GRID_MGT_GET_MID_LAYER_GRID:
			sprintf(API_label, "CCPL_get_mid_layer_grid");
			break;
		case API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA:
			sprintf(API_label, "CCPL_register_V1D_grid_via_model_data");
			break;		
		case API_ID_GRID_MGT_REG_MD_GRID_VIA_MULTI_GRIDS:
			sprintf(API_label, "CCPL_register_MD_grid_via_multi_grids");
			break;			
		case API_ID_DECOMP_MGT_REG_DECOMP:
			sprintf(API_label, "CCPL_register_parallel_decomp");
			break;
		case API_ID_FIELD_MGT_REG_FIELD_INST:
			sprintf(API_label, "CCPL_register_field_instance");
			break;
		case API_ID_TIME_MGT_SET_TIME_STEP:
			sprintf(API_label, "CCPL_set_time_step");
			break;
		case API_ID_TIME_MGT_ADVANCE_TIME:
			sprintf(API_label, "CCPL_advance_time");
			break;
		case API_ID_TIME_MGT_GET_CURRENT_NUM_DAYS_IN_YEAR:
			sprintf(API_label, "CCPL_get_current_num_days_in_year");
			break;			
		case API_ID_TIME_MGT_GET_CURRENT_YEAR:
			sprintf(API_label, "CCPL_get_current_year");
			break;						
		case API_ID_TIME_MGT_GET_CURRENT_DATE:
			sprintf(API_label, "CCPL_get_current_date");
			break;						
		case API_ID_TIME_MGT_GET_CURRENT_SECOND:
			sprintf(API_label, "CCPL_get_current_second");
			break;						
		case API_ID_TIME_MGT_GET_START_TIME:
			sprintf(API_label, "CCPL_get_start_time");
			break;						
		case API_ID_TIME_MGT_GET_STOP_TIME:
			sprintf(API_label, "CCPL_get_stop_time");
			break;			
		case API_ID_TIME_MGT_GET_PREVIOUS_TIME:
			sprintf(API_label, "CCPL_get_previous_time");
			break;						
		case API_ID_TIME_MGT_GET_CURRENT_TIME:
			sprintf(API_label, "CCPL_get_current_time");
			break;			
		case API_ID_TIME_MGT_GET_ELAPSED_DAYS_FROM_REF:
			sprintf(API_label, "CCPL_get_num_elapsed_days_from_reference");
			break;			
		case API_ID_TIME_MGT_GET_ELAPSED_DAYS_FROM_START:
			sprintf(API_label, "CCPL_get_num_elapsed_days_from_start");
			break;			
		case API_ID_TIME_MGT_IS_END_CURRENT_DAY:
			sprintf(API_label, "CCPL_is_end_current_day");
			break;			
		case API_ID_TIME_MGT_IS_END_CURRENT_MONTH:
			sprintf(API_label, "CCPL_is_end_current_month");
			break;			
		case API_ID_TIME_MGT_GET_CURRENT_CAL_TIME:
			sprintf(API_label, "CCPL_get_current_calendar_time");
			break;											
		case API_ID_TIME_MGT_IS_FIRST_STEP:
			sprintf(API_label, "CCPL_is_first_step");
			break;
		case API_ID_TIME_MGT_GET_NUM_CURRENT_STEP:
			sprintf(API_label, "CCPL_get_number_of_current_step");
			break;
		case API_ID_TIME_MGT_GET_NUM_TOTAL_STEPS:
			sprintf(API_label, "CCPL_get_number_of_total_steps");
			break;
		case API_ID_TIME_MGT_GET_TIME_STEP:
			sprintf(API_label, "CCPL_get_time_step");
			break;
		case API_ID_TIME_MGT_CHECK_CURRENT_TIME:
			sprintf(API_label, "CCPL_check_current_time");
			break;
		case API_ID_TIME_MGT_IS_TIMER_ON:
			sprintf(API_label, "CCPL_is_timer_on");
			break;
		case API_ID_TIME_MGT_IS_MODEL_RUN_ENDED:
			sprintf(API_label, "CCPL_is_model_run_ended");
			break;
		case API_ID_INTERFACE_REG_IMPORT:
			sprintf(API_label, "CCPL_register_import_interface");
			break;
		case API_ID_INTERFACE_REG_EXPORT:
			sprintf(API_label, "CCPL_register_export_interface");
			break;
		case API_ID_INTERFACE_EXECUTE:
			sprintf(API_label, "CCPL_execute_interface");
			break;
		case API_ID_TIME_MGT_DEFINE_SINGLE_TIMER:
			sprintf(API_label, "CCPL_define_single_timer");
			break;
		case API_ID_TIME_MGT_DEFINE_COMPLEX_TIMER:
			sprintf(API_label, "CCPL_define_complex_timer");
			break;
		case API_ID_FIELD_MGT_REG_IO_FIELD:
			sprintf(API_label, "CCPL_register_IO_field");
			break;
		default:
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "software error1 in get_API_hint %x", API_id);
			break;
	}
}


void synchronize_comp_processes_for_API(int comp_id, int API_id, MPI_Comm comm, const char *hint, const char *annotation)
{
	char API_label_local[NAME_STR_SIZE], API_label_another[NAME_STR_SIZE];
	int local_process_id, num_processes;
	int *API_ids, *comp_ids;
	char *annotations, *comp_names;


	get_API_hint(-1, API_id, API_label_local);

	if (comp_id != -1)
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "The component id is wrong when calling the interface \"%s\" for \"%s\". Please check the model code with the annotation \"%s\"", API_label_local, hint, annotation);

	if (comm == -1)
		comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in synchronize_comp_processes_for_API");

	if (hint != NULL)
		EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "Before the MPI_barrier for synchronizing all processes of a communicator for %s at C-Coupler interface \"%s\" with model code annotation \"%s\" %d  %x", hint, API_label_local, annotation, comm, comp_id);	
	else EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "Before the MPI_barrier for synchronizing all processes of a communicator at C-Coupler interface \"%s\" with model code annotation \"%s\" %d  %x", API_label_local, annotation, comm, comp_id);
	MPI_Barrier(comm);
	if (hint != NULL)
		EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "After the MPI_barrier for synchronizing all processes of a communicator for %s at C-Coupler interface \"%s\" with model code annotation \"%s\"", hint, API_label_local, annotation);	
	else EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "After the MPI_barrier for synchronizing all processes of a communicator at C-Coupler interface \"%s\" with model code annotation \"%s\"", API_label_local, annotation);
	EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Comm_rank(comm, &local_process_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(comm, &num_processes) == MPI_SUCCESS);
	API_ids = new int [num_processes];
	annotations = new char [num_processes*NAME_STR_SIZE];
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather(&API_id, 1, MPI_INT, API_ids, 1, MPI_INT, 0, comm) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "annotation is \"%s\"", annotation);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather((void*)annotation, NAME_STR_SIZE, MPI_CHAR, annotations, NAME_STR_SIZE, MPI_CHAR, 0, comm) == MPI_SUCCESS);
	if (local_process_id == 0) {
		for (int i = 1; i < num_processes; i ++) {
			get_API_hint(comp_id, API_ids[i], API_label_another);
			EXECUTION_REPORT(REPORT_ERROR, comp_id, API_id == API_ids[i], "different kinds of C-Coupler API calls (\"%s\" and \"%s\") are mapped to the same synchronization. Please check the model code related to the annotations \"%s\" and \"%s\".",
							 API_label_local, API_label_another, annotation, annotations+NAME_STR_SIZE*i);			
		}	
	}
	comp_ids = new int [num_processes];
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather(&comp_id, 1, MPI_INT, comp_ids, 1, MPI_INT, 0, comm) == MPI_SUCCESS);
	if (local_process_id == 0) {
		for (int i = 1; i < num_processes; i ++)
			EXECUTION_REPORT(REPORT_ERROR, -1, comp_ids[0] == -1 && comp_ids[i] == -1 || comp_ids[0] != -1 && comp_ids[i] != -1, "It is wrong that different components take part in the same API (\"%s\"). Please check the model code related to the annotations \"%s\" and \"%s\".", 
						     API_label_local, annotation, annotations+NAME_STR_SIZE*i);
	}
	if (comp_id != -1) {
		comp_names = new char [num_processes*NAME_STR_SIZE];
		EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Gather((void*)comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"C-Coupler gets component node in synchronize_comp_processes_for_API")->get_comp_name(), NAME_STR_SIZE, MPI_CHAR, comp_names, NAME_STR_SIZE, MPI_CHAR, 0, comm) == MPI_SUCCESS);
		if (local_process_id == 0) {
			for (int i = 1; i < num_processes; i ++)
				EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(comp_names, comp_names+NAME_STR_SIZE*i), "It is wrong that two different components (\"%s\" and \"%s\") take part in the same API (\"%s\"). Please check the model code related to the annotations \"%s\" and \"%s\".",
							 comp_names, comp_names+NAME_STR_SIZE*i, API_label_local, annotation, annotations+NAME_STR_SIZE*i);		
		}
		delete [] comp_names;
	}
	
	delete [] API_ids;
	delete [] annotations;
	delete [] comp_ids;
}


template <class T> void check_API_parameter_scalar(int comp_id, int API_id, MPI_Comm comm, const char *hint, T value, const char *parameter_name, const char *annotation)
{
	int i, local_process_id, num_processes;
	T *values;
	char API_label[NAME_STR_SIZE];
	

	EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Comm_rank(comm, &local_process_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Comm_size(comm, &num_processes) == MPI_SUCCESS);	
	values = new T [num_processes];
	if (sizeof(T) == 1)
		EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Gather(&value, 1, MPI_CHAR, values, 1, MPI_CHAR, 0, comm) == MPI_SUCCESS);
	else if (sizeof(T) == 2)
		EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Gather(&value, 1, MPI_SHORT, values, 1, MPI_SHORT, 0, comm) == MPI_SUCCESS);
	else if (sizeof(T) == 4)
		EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Gather(&value, 1, MPI_INT, values, 1, MPI_INT, 0, comm) == MPI_SUCCESS);
	else if (sizeof(T) == 8)
		EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Gather(&value, 1, MPI_DOUBLE, values, 1, MPI_DOUBLE, 0, comm) == MPI_SUCCESS);
	else EXECUTION_REPORT(REPORT_ERROR, comp_id, true, "software error in check_API_parameter_scalar");
	if (local_process_id == 0) {
		get_API_hint(comp_id, API_id, API_label);
		for (i = 1; i < num_processes; i ++)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, values[0] == values[i], "Error happens when calling API \"%s\" for %s: parameter \"%s\" is not consistent among processes of component \"%s\". Please check the model code related to the annotation \"%s\"",
			                  API_label, hint, parameter_name, comp_comm_group_mgt_mgr->search_global_node(comp_id)->get_comp_name(), annotation);
	}

	delete [] values;
}


void check_API_parameter_data_array(int comp_id, int API_id, MPI_Comm comm, const char *hint, int array_size, const char *array_value, const char *parameter_name, const char *annotation)
{
	long temp_checksum = 0, total_checksum = 0;


	if (array_size == 0)
		return;
	
	for (int i = 0; i < array_size/sizeof(long); i ++)
		total_checksum += ((const long*) array_value)[i] * (i+1);
	for (int i = (array_size/sizeof(long))*sizeof(long); i < array_size; i ++) 
		temp_checksum = (temp_checksum << 8) | array_value[i];
	total_checksum += temp_checksum * ((array_size/sizeof(long))+1);

	check_API_parameter_long(comp_id, API_id, comm, hint, total_checksum, parameter_name, annotation);
}


void check_API_parameter_int(int comp_id, int API_id, MPI_Comm comm, const char *hint, int value, const char *parameter_name, const char *annotation)
{
	check_API_parameter_scalar(comp_id, API_id, comm, hint, value, parameter_name, annotation);
}


void check_API_parameter_long(int comp_id, int API_id, MPI_Comm comm, const char *hint, long value, const char *parameter_name, const char *annotation)
{
	check_API_parameter_scalar(comp_id, API_id, comm, hint, value, parameter_name, annotation);
}


void check_API_parameter_timer(int comp_id, int API_id, MPI_Comm comm, const char *hint, int timer_id, const char *parameter_name, const char *annotation)
{
	Coupling_timer *timer;

	
	EXECUTION_REPORT(REPORT_ERROR, comp_id, timer_mgr->check_is_legal_timer_id(timer_id), "Software error in check_API_parameter_timer");
	timer = timer_mgr->get_timer(timer_id);
	check_API_parameter_int(comp_id, API_id, comm, hint, timer->get_frequency_count(), parameter_name, annotation);
	check_API_parameter_int(comp_id, API_id, comm, hint, timer->get_lag_count(), parameter_name, annotation);
	check_API_parameter_string(comp_id, API_id, comm, hint, timer->get_frequency_unit(), parameter_name, annotation);
}


void check_API_parameter_field_instance(int comp_id, int API_id, MPI_Comm comm, const char *hint, int field_id, const char *parameter_name, const char *annotation)
{
	Field_mem_info *field_instance;
	int decomp_class, is_registerred;


	EXECUTION_REPORT(REPORT_ERROR, comp_id, memory_manager->check_is_legal_field_instance_id(field_id), "Software error in check_API_parameter_field_instance");
	field_instance = memory_manager->get_field_instance(field_id);
	if (field_instance->get_decomp_id() == -1)
		decomp_class = -1;
	else decomp_class = 0;
	check_API_parameter_int(comp_id, API_id, comm, hint, decomp_class, parameter_name, annotation);	
	check_API_parameter_string(comp_id, API_id, comm, hint, field_instance->get_field_name(), parameter_name, annotation);
	if (field_instance->get_decomp_id() != -1) {
		check_API_parameter_string(comp_id, API_id, comm, hint, decomps_info_mgr->get_decomp_info(field_instance->get_decomp_id())->get_decomp_name(), parameter_name, annotation);
		check_API_parameter_string(comp_id, API_id, comm, hint, original_grid_mgr->get_name_of_grid(field_instance->get_grid_id()), parameter_name, annotation);
	}
	if (field_instance->get_is_registered_model_buf())
		is_registerred = 1;
	else is_registerred = 0;
	check_API_parameter_int(comp_id, API_id, comm, hint, is_registerred, parameter_name, annotation);	
	if (field_instance->get_is_registered_model_buf())
		check_API_parameter_int(comp_id, API_id, comm, hint, field_instance->get_buf_mark(), parameter_name, annotation);
}


void check_API_parameter_string(int comp_id, int API_id, MPI_Comm comm, const char *hint, const char *string, const char *parameter_name, const char *annotation)
{
	int local_process_id, num_processes, local_string_size, *all_string_size;
	char API_label[NAME_STR_SIZE], *all_string_para;


	EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Comm_rank(comm, &local_process_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Comm_size(comm, &num_processes) == MPI_SUCCESS);
	local_string_size = strlen(string);
	all_string_size = new int [num_processes];
	
	EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Gather(&local_string_size, 1, MPI_INT, all_string_size, 1, MPI_INT, 0, comm) == MPI_SUCCESS);
	if (local_process_id == 0) {
		get_API_hint(comp_id, API_id, API_label);
		for (int i = 1; i < num_processes; i ++)
			if (comp_id != -1)
				EXECUTION_REPORT(REPORT_ERROR, comp_id, all_string_size[0] == all_string_size[i], "Error happens when calling API \"%s\" for %s: parameter \"%s\" is not consistent among processes of component \"%s\". Please check the model code related to the annotation \"%s\"",
				                 API_label, hint, parameter_name, comp_comm_group_mgt_mgr->search_global_node(comp_id)->get_comp_name(), annotation);
			else EXECUTION_REPORT(REPORT_ERROR, comp_id, all_string_size[0] == all_string_size[i], "Error happens when calling API \"%s\" for %s: parameter \"%s\" is not consistent among processes. Please check the model code related to the annotation \"%s\"",
				                  API_label, hint, parameter_name, annotation);
	}
	all_string_para = new char [local_string_size*num_processes];
	EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Gather((void*)string, local_string_size, MPI_CHAR, all_string_para, local_string_size, MPI_CHAR, 0, comm) == MPI_SUCCESS);
			
	if (local_process_id == 0) {
		for (int i = 1; i < num_processes; i ++)
			if (comp_id != -1)
				EXECUTION_REPORT(REPORT_ERROR, comp_id, strncmp(all_string_para, all_string_para+local_string_size*i, local_string_size) == 0, 
				                 "Error happens when calling API \"%s\" for %s: parameter \"%s\" is not consistent among processes of component \"%s\". Please check the model code related to the annotation \"%s\"",
				                 API_label, hint, parameter_name, comp_comm_group_mgt_mgr->search_global_node(comp_id)->get_comp_name(), annotation);
			else EXECUTION_REPORT(REPORT_ERROR, comp_id, strncmp(all_string_para, all_string_para+local_string_size*i, local_string_size) == 0, 
				                  "Error happens when calling API \"%s\" for %s: parameter \"%s\" is not consistent among processes. Please check the model code related to the annotation \"%s\"",
				                  API_label, hint, parameter_name, annotation);			
	}
	
	delete [] all_string_size;
	delete [] all_string_para;
}


bool check_and_verify_name_format_of_string(const char *string)
{
	for (int i = 0; i < strlen(string); i ++)
		if (!((string[i] >= 'a' && string[i] <= 'z') || (string[i] >= 'A' && string[i] <= 'Z') || (string[i] >= '0' && string[i] <= '9') || string[i] == '_' || string[i] == '-' || string[i] == '.'))
			return false;

	return true;
}


void check_and_verify_name_format_of_string_for_API(int comp_id, const char *string, int API_id, const char *name_owner, const char *annotation)
{
	char API_label[NAME_STR_SIZE];
	int i;


	get_API_hint(comp_id, API_id, API_label);
	
	EXECUTION_REPORT(REPORT_ERROR, comp_id, check_and_verify_name_format_of_string(string),
					 "When calling the C-Coupler interface \"%s\", the format of the name of %s (currently is \"%s\") is wrong. Each character in the name can only be '-', '_', 'a-z', 'A-Z', '0-9', or '.'. Please check the model code with the annotation \"%s\"",
					 API_label, name_owner, string, annotation);
}


void check_and_verify_name_format_of_string_for_XML(int comp_id, const char *string, const char *name_owner, const char *XML_file_name, int line_number)
{
	EXECUTION_REPORT(REPORT_ERROR, comp_id, check_and_verify_name_format_of_string(string),
					 "When reading the XML file \"%s\", the format of the name of %s (currently is \"%s\") is wrong. Each character in the name can only be '-', '_', 'a-z', 'A-Z', '0-9', or '.'. Please check the XML file arround the line number %d",
					 XML_file_name, name_owner, string, line_number);
}


bool is_XML_setting_on(int comp_id, TiXmlElement *XML_element, const char *XML_file_name, const char *attribute_annotation, const char *XML_file_annotation)
{
	int line_number;
	const char *status = get_XML_attribute(comp_id, XML_element, "status", XML_file_name, line_number, attribute_annotation, XML_file_annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(status, "on") || words_are_the_same(status, "off"), "In the XML file \"%s\" that is for %s, the value of %s is wrong (must be \"on\" or \"off\"). Please verify the XML file arround the line number %d.",
		             XML_file_name, XML_file_annotation, attribute_annotation, line_number);
	return words_are_the_same(status, "on");
}


const char *get_XML_attribute(int comp_id, TiXmlElement *XML_element, const char *attribute_keyword, const char *XML_file_name, int &line_number, const char *attribute_annotation, const char *XML_file_annotation)
{
	const char *attribute_value = XML_element->Attribute(attribute_keyword, &line_number);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, attribute_value != NULL, "In the XML file \"%s\" that is for %s, %s (the keyword is \"%s\") has not been specified. Please verify the XML file arround the line number %d.", 
		             XML_file_name, XML_file_annotation, attribute_annotation, attribute_keyword, XML_element->Row());
	return attribute_value;
}


void transfer_array_from_one_comp_to_another(int current_proc_local_id_src_comp, int root_proc_global_id_src_comp, int current_proc_local_id_dst_comp, 
	                                         int root_proc_global_id_dst_comp, MPI_Comm comm_dst_comp, char **array, int &array_size)
{
	MPI_Status status;

	
	if (current_proc_local_id_src_comp == 0 && current_proc_local_id_dst_comp != 0) {
		MPI_Send(&array_size, 1, MPI_INT, root_proc_global_id_dst_comp, 0, MPI_COMM_WORLD);
		if (array_size > 0) {
			EXECUTION_REPORT(REPORT_ERROR, -1, *array != NULL, "software error in transfer_array_from_one_comp_to_another");
			MPI_Send(*array, array_size, MPI_CHAR, root_proc_global_id_dst_comp, 0, MPI_COMM_WORLD);
		}
	}
	if (current_proc_local_id_src_comp != 0 && current_proc_local_id_dst_comp == 0) {
		MPI_Recv(&array_size, 1, MPI_INT, root_proc_global_id_src_comp, 0, MPI_COMM_WORLD, &status);
		if (array_size > 0) {
			if (*array == NULL)
				*array = new char [array_size];
			MPI_Recv(*array, array_size, MPI_CHAR, root_proc_global_id_src_comp, 0, MPI_COMM_WORLD, &status);
		}
	}

	if (current_proc_local_id_dst_comp != -1) {
		MPI_Bcast(&array_size, 1, MPI_INT, 0, comm_dst_comp);
		if (array_size > 0) {
			if (*array == NULL)
				*array = new char [array_size];
			MPI_Bcast(*array, array_size, MPI_CHAR, 0, comm_dst_comp);
		}
	}		
}


void gather_array_in_one_comp(int num_total_local_proc, int current_proc_local_id, void *local_array, int local_array_size, 
	                          int data_type_size, int *all_array_size, void **global_array, MPI_Comm comm)
{
    int *displs = new int [num_total_local_proc];
	int *counts = new int [num_total_local_proc];

	
    MPI_Gather(&local_array_size, 1, MPI_INT, all_array_size, 1, MPI_INT, 0, comm);
    if (current_proc_local_id == 0) {
        displs[0] = 0;
		counts[0] = all_array_size[0] * data_type_size;
        for (int i = 1; i < num_total_local_proc; i ++) {
			counts[i] = all_array_size[i] * data_type_size;
            displs[i] = displs[i-1] + counts[i-1];
        }
        *global_array = new char [displs[num_total_local_proc-1]+counts[num_total_local_proc-1]];
    }
    MPI_Gatherv(local_array, local_array_size*data_type_size, MPI_CHAR, *global_array, counts, displs, MPI_CHAR, 0, comm);

    delete [] displs;
    delete [] counts;
}

