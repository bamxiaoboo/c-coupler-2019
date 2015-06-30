/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "compset_communicators_info_mgt.h"
#include <stdio.h>
#include <string.h>
#include "global_data.h"
#include "cor_global_data.h"
#include <unistd.h>


Compset_communicators_info_mgt::Compset_communicators_info_mgt(const char *experiment_model, const char *current_comp_name, const char *compset_filename,
           const char *case_name, const char *case_desc, const char *case_mode, const char *comp_namelist,
           const char *current_config_time, const char *original_case_name, const char *original_config_time)
{
    load_in_compset(current_comp_name, compset_filename);
	gethostname(host_name_current_computing_node, NAME_STR_SIZE);
    build_compset_communicators_info();

	strcpy(this->experiment_model, experiment_model);
    strcpy(current_case_name, case_name);
    strcpy(current_case_desc, case_desc);
    strcpy(running_case_mode, case_mode);
    strcpy(comp_namelist_filename, comp_namelist);
	strcpy(this->current_config_time, current_config_time);
	strcpy(this->original_case_name, original_case_name);
	strcpy(this->original_config_time, original_config_time);

	if (words_are_the_same(case_mode, "initial")) {
		strcpy(this->original_case_name, "none");
		strcpy(this->original_config_time, "none");
	}	
}


Compset_communicators_info_mgt::~Compset_communicators_info_mgt()
{
    for (int i = 0; i < comps_comms_info.size(); i ++) {
        delete [] comps_comms_info[i]->comp_procs_global_ids;
        delete comps_comms_info[i];
    }
}


void Compset_communicators_info_mgt::write_case_info(IO_netcdf *netcdf_file)
{
	netcdf_file->put_global_text("experiment model", experiment_model);
    netcdf_file->put_global_text("current case name", current_case_name);
    netcdf_file->put_global_text("current case description", current_case_desc);
    netcdf_file->put_global_text("running mode of case", running_case_mode);
	netcdf_file->put_global_text("configuration time", current_config_time);
    if (words_are_the_same(running_case_mode, "restart"))
        netcdf_file->put_global_text("original case name", original_case_name);
}


void Compset_communicators_info_mgt::load_in_compset(const char *current_comp_name, const char *compset_filename)
{
    FILE *cfg_fp;
    char line[NAME_STR_SIZE*6];
    char *local_line;
    Component_communicator_info *comp_comm_info;
    

    cfg_fp = open_config_file(compset_filename);

    /* read component infomation from all_comp_names config file */
    while (get_next_line(line, cfg_fp)) {
        local_line = line;
        comp_comm_info = new Component_communicator_info;
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(comp_comm_info->comp_name, &local_line), "C-Coupler error1 in Compset_communicators_info_mgt::load_in_compset");
		EXECUTION_REPORT(REPORT_ERROR, get_next_attr(comp_comm_info->comp_type, &local_line), "C-Coupler error2 in Compset_communicators_info_mgt::load_in_compset");
        comp_comm_info->comp_id = comps_comms_info.size() + 1;
        comps_comms_info.push_back(comp_comm_info);
        EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_ATM) || words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_OCN) ||
                     words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_LND) || words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_SEA_ICE) ||
                     words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_CPL) || words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_WAVE) ||
                     words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_CESM) || words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_ATM_CHEM),
                     "%s is a wrong componet type\n", comp_comm_info->comp_type);
    }
    
    fclose(cfg_fp); 

    current_comp_id = get_comp_id_by_comp_name(current_comp_name);
}


void Compset_communicators_info_mgt::build_compset_communicators_info()
{
    int i, j;
    int current_proc_global_id, current_proc_computing_node_id;
    int num_global_procs;
    int *all_procs_global_ids;
    int *all_procs_local_ids;
    int *num_all_comps_procs;
    int *all_comp_root_procs_global_ids;
    int *all_procs_comp_ids;
	char *host_name_all_computing_nodes, *host_name_distinct_computing_nodes;
	int *host_name_ids;
	int num_distinct_computing_nodes;
	

    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_dup(MPI_COMM_WORLD, &global_comm_group) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(global_comm_group, &current_proc_global_id) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_size(global_comm_group, &num_global_procs) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_split(global_comm_group, current_comp_id, 0, &current_comp_comm_group) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(current_comp_comm_group, &current_proc_local_id) == MPI_SUCCESS);

    /* Gather all components' communication info */
    all_procs_global_ids = new int [num_global_procs];
    all_procs_local_ids = new int [num_global_procs];
    all_procs_comp_ids = new int [num_global_procs];
    num_all_comps_procs = new int [comps_comms_info.size()];
    all_comp_root_procs_global_ids = new int [comps_comms_info.size()];
	if (num_global_procs == 1) {
		all_procs_global_ids[0] = current_proc_global_id;
		all_procs_comp_ids[0] = current_comp_id;
		all_procs_local_ids[0] = current_proc_local_id;
	}
	else {
	    EXECUTION_REPORT(REPORT_ERROR, MPI_Allgather(&current_proc_global_id, 1, MPI_INT, all_procs_global_ids, 1, MPI_INT, global_comm_group) == MPI_SUCCESS);
	    EXECUTION_REPORT(REPORT_ERROR, MPI_Allgather(&current_comp_id, 1, MPI_INT, all_procs_comp_ids, 1, MPI_INT, global_comm_group) == MPI_SUCCESS);
	    EXECUTION_REPORT(REPORT_ERROR, MPI_Allgather(&current_proc_local_id, 1, MPI_INT, all_procs_local_ids, 1, MPI_INT, global_comm_group) == MPI_SUCCESS);
	}
    for (i = 0; i < comps_comms_info.size(); i ++)
        num_all_comps_procs[i] = 0;
    for (i = 0; i < num_global_procs; i ++) {
        num_all_comps_procs[all_procs_comp_ids[i]] ++;
        if (all_procs_local_ids[i] == 0)
            all_comp_root_procs_global_ids[all_procs_comp_ids[i]] = all_procs_global_ids[i];
    }

    /* Build the data structure of communication info */
    for (i = 0; i < comps_comms_info.size(); i ++) {
        comps_comms_info[i]->num_comp_procs = num_all_comps_procs[i];
        comps_comms_info[i]->comp_procs_global_ids = new int [num_all_comps_procs[i]];
    }
    for (i = 0; i < num_global_procs; i ++) 
        comps_comms_info[all_procs_comp_ids[i]]->comp_procs_global_ids[all_procs_local_ids[i]] = all_procs_global_ids[i];

	/* Gather host name of computing nodes */
	host_name_all_computing_nodes = new char [num_all_comps_procs[current_comp_id]*NAME_STR_SIZE];
	host_name_distinct_computing_nodes = new char [num_all_comps_procs[current_comp_id]*NAME_STR_SIZE];
	host_name_ids = new int [num_all_comps_procs[current_comp_id]];
	if (num_global_procs == 1)
		strcpy(host_name_distinct_computing_nodes, host_name_current_computing_node);
	else EXECUTION_REPORT(REPORT_ERROR, MPI_Allgather(host_name_current_computing_node, NAME_STR_SIZE, MPI_CHAR, host_name_all_computing_nodes, NAME_STR_SIZE, MPI_CHAR, current_comp_comm_group) == MPI_SUCCESS);
	for (i = 0; i < num_all_comps_procs[current_comp_id]; i ++)
		host_name_ids[i] = -1;
	num_distinct_computing_nodes = 0;
	for (i = 0; i < num_all_comps_procs[current_comp_id]; i ++) {
		if (host_name_ids[i] >= 0)
			continue;
		host_name_ids[i] = num_distinct_computing_nodes;
		for (j = i + 1; j < num_all_comps_procs[current_comp_id]; j ++) {
			if (host_name_ids[j] >= 0)
				continue;
			if (words_are_the_same(host_name_all_computing_nodes+NAME_STR_SIZE*i, host_name_all_computing_nodes+NAME_STR_SIZE*j))
				host_name_ids[j] = num_distinct_computing_nodes;
		}
		num_distinct_computing_nodes ++;
	}

	EXECUTION_REPORT(REPORT_LOG, true, "%d computing nodes are used in this experiment", num_distinct_computing_nodes);
	
    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_split(current_comp_comm_group, host_name_ids[current_proc_local_id], 0, &computing_node_comp_group) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(computing_node_comp_group, &current_proc_computing_node_id) == MPI_SUCCESS);
	is_master_process_in_computing_node = (current_proc_computing_node_id == 0);
	EXECUTION_REPORT(REPORT_ERROR, current_proc_computing_node_id < 64, "current version of C-Coupler only supports 64 processes in a computing node");

    delete [] all_procs_global_ids;
    delete [] all_procs_local_ids;
    delete [] num_all_comps_procs;               
    delete [] all_comp_root_procs_global_ids;
    delete [] all_procs_comp_ids;
	delete [] host_name_all_computing_nodes;
	delete [] host_name_distinct_computing_nodes;
	delete [] host_name_ids;
}


int Compset_communicators_info_mgt::get_comp_id_by_comp_name(const char *name)
{
    for(int i = 0; i < comps_comms_info.size(); i ++)
        if (strcmp(comps_comms_info[i]->comp_name, name) == 0)    
            return i;

    EXECUTION_REPORT(REPORT_ERROR, false, "the component list does not include the name of current component %s\n", name);
    return -1;
}


int Compset_communicators_info_mgt::get_proc_id_in_global_comm_group(int cid, int local_proc_rank)
{
    EXECUTION_REPORT(REPORT_ERROR, local_proc_rank < comps_comms_info[cid]->num_comp_procs);
    return comps_comms_info[cid]->comp_procs_global_ids[local_proc_rank];
}


