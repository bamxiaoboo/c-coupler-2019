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
			sprintf(API_label, "CCPL_end_registration");
			break;
        case API_ID_GRID_MGT_REG_H2D_GRID_ONLINE:
			sprintf(API_label, "CCPL_register_H2D_grid");
			break;
        case API_ID_GRID_MGT_REG_H2D_GRID_VIA_FILE:
			sprintf(API_label, "CCPL_register_H2D_grid");
			break;
        case API_ID_GRID_MGT_REG_1D_GRID_ONLINE:
			sprintf(API_label, "CCPL_register_1D_grid");
			break;
        case API_ID_GRID_MGT_REG_MD_GRID:
			sprintf(API_label, "CCPL_combine_grids");
			break;
        case API_ID_GRID_MGT_REG_GRID_VIA_COR:
			sprintf(API_label, "CCPL_get_CoR_defined_grid");
			break;
        case API_ID_GRID_MGT_REG_GRID_VIA_LOCAL:
			sprintf(API_label, "CCPL_get_local_grid");
			break;
        case API_ID_GRID_MGT_REG_GRID_VIA_REMOTE:
			sprintf(API_label, "CCPL_get_remote_grid");
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
        case API_ID_GRID_MGT_SET_3D_GRID_DYN_REF_FLD:
			sprintf(API_label, "CCPL_set_dynamic_3D_grid_reference_fied");
			break;
        case API_ID_GRID_MGT_SET_3D_GRID_STATIC_REF_FLD:
			sprintf(API_label, "CCPL_set_static_3D_grid_reference_fied");
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
		default:
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "software error1 in get_API_hint %x", API_id);
			break;
	}
}


void synchronize_comp_processes_for_API(int comp_id, int API_id, MPI_Comm comm, const char *hint, const char *annotation)
{
	char API_label_local[NAME_STR_SIZE], API_label_another[NAME_STR_SIZE];
	int local_process_id, num_processes, diff_pos;
	int *API_ids;
	char *annotations;


	get_API_hint(-1, API_id, API_label_local);

	if (comp_id != -1)
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "The component id is wrong when calling the interface \"%s\" for \"%s\". Please check the model code with the annotation \"%s\"", API_label_local, hint, annotation);

	if (hint != NULL)
		EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "Before the MPI_barrier for synchronizing all processes of a communicator for %s at C-Coupler interface \"%s\" with model code annotation \"%s\"", hint, API_label_local, annotation);	
	else EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "Before the MPI_barrier for synchronizing all processes of a communicator at C-Coupler interface \"%s\" with model code annotation \"%s\"", API_label_local, annotation);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Barrier(comm) == MPI_SUCCESS);
	if (hint != NULL)
		EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "After the MPI_barrier for synchronizing all processes of a communicator for %s at C-Coupler interface \"%s\" with model code annotation \"%s\"", hint, API_label_local, annotation);	
	else EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "After the MPI_barrier for synchronizing all processes of a communicator at C-Coupler interface \"%s\" with model code annotation \"%s\"", API_label_local, annotation);
	EXECUTION_REPORT(REPORT_ERROR, -1, MPI_Comm_rank(comm, &local_process_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(comm, &num_processes) == MPI_SUCCESS);
	API_ids = new int [num_processes];
	annotations = new char [num_processes*NAME_STR_SIZE];
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather(&API_id, 1, MPI_INT, API_ids, 1, MPI_INT, 0, comm) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather((void*)annotation, NAME_STR_SIZE, MPI_CHAR, annotations, NAME_STR_SIZE, MPI_CHAR, 0, comm) == MPI_SUCCESS);
	diff_pos = -1;
	if (local_process_id == 0) {
		for (int i = 1; i < num_processes; i ++)
			if (API_id != API_ids[i])
				diff_pos = i;
		if (diff_pos != -1)
			get_API_hint(comp_id, API_ids[diff_pos], API_label_another);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, diff_pos == -1, "different kinds of C-Coupler API calls (\"%s\" and \"%s\") are mapped to the same synchronization. Please check the model code related to the annotations \"%s\" and \"%s\".",
						 API_label_local, API_label_another, annotation, annotations+NAME_STR_SIZE*diff_pos);
	}
	
	delete [] API_ids;
	delete [] annotations;
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

