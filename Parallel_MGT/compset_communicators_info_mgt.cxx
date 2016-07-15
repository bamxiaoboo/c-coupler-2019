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
#include "quick_sort.h"
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
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(comp_comm_info->comp_name, &local_line), "C-Coupler error1 in Compset_communicators_info_mgt::load_in_compset");
		EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(comp_comm_info->comp_type, &local_line), "C-Coupler error2 in Compset_communicators_info_mgt::load_in_compset");
        comp_comm_info->comp_id = comps_comms_info.size();
        comps_comms_info.push_back(comp_comm_info);
        EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_ATM) || words_are_the_same(comp_comm_info->comp_type, COMP_TYPE_OCN) ||
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
	

    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_dup(MPI_COMM_WORLD, &global_comm_group) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(global_comm_group, &current_proc_global_id) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(global_comm_group, &num_global_procs) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_split(global_comm_group, current_comp_id, 0, &current_comp_comm_group) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(current_comp_comm_group, &current_proc_local_id) == MPI_SUCCESS);
	comps_comms_info[current_comp_id]->comm_group = current_comp_comm_group;
	comps_comms_info[current_comp_id]->current_proc_local_id = current_proc_local_id;

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
	    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(&current_proc_global_id, 1, MPI_INT, all_procs_global_ids, 1, MPI_INT, global_comm_group) == MPI_SUCCESS);
	    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(&current_comp_id, 1, MPI_INT, all_procs_comp_ids, 1, MPI_INT, global_comm_group) == MPI_SUCCESS);
	    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(&current_proc_local_id, 1, MPI_INT, all_procs_local_ids, 1, MPI_INT, global_comm_group) == MPI_SUCCESS);
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
	else EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(host_name_current_computing_node, NAME_STR_SIZE, MPI_CHAR, host_name_all_computing_nodes, NAME_STR_SIZE, MPI_CHAR, current_comp_comm_group) == MPI_SUCCESS);
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

	EXECUTION_REPORT(REPORT_LOG,-1, true, "%d computing nodes are used in this experiment", num_distinct_computing_nodes);
	
    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_split(current_comp_comm_group, host_name_ids[current_proc_local_id], 0, &computing_node_comp_group) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(computing_node_comp_group, &current_proc_computing_node_id) == MPI_SUCCESS);
	is_master_process_in_computing_node = (current_proc_computing_node_id == 0);
	EXECUTION_REPORT(REPORT_ERROR,-1, current_proc_computing_node_id < 64, "current version of C-Coupler only supports 64 processes in a computing node");

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

    EXECUTION_REPORT(REPORT_ERROR,-1, false, "the component list does not include the name of current component %s\n", name);
    return -1;
}


int Compset_communicators_info_mgt::get_proc_id_in_global_comm_group(int cid, int local_proc_rank)
{
    EXECUTION_REPORT(REPORT_ERROR,-1, local_proc_rank < comps_comms_info[cid]->num_comp_procs);
    return comps_comms_info[cid]->comp_procs_global_ids[local_proc_rank];
}


void Compset_communicators_info_mgt::register_component(const char *comp_name, const char *comp_type, MPI_Comm comm)
{
	int i, current_proc_global_id;
	Component_communicator_info *comp_comm_info;
	
	
	MPI_Barrier(comm);
	
    for(i = 0; i < comps_comms_info.size(); i ++)
        EXECUTION_REPORT(REPORT_ERROR,-1, !words_are_the_same(comps_comms_info[i]->comp_name, comp_name), "Component name \"%s\" has alread been registered before");

	comp_comm_info = new Component_communicator_info;
	strcpy(comp_comm_info->comp_name, comp_name);
	strcpy(comp_comm_info->comp_type, comp_type);
	comp_comm_info->comm_group = comm;
	comp_comm_info->comp_id = comps_comms_info.size();
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(comm, &comp_comm_info->num_comp_procs) == MPI_SUCCESS);
	comp_comm_info->comp_procs_global_ids = new int [comp_comm_info->num_comp_procs];
    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(global_comm_group, &current_proc_global_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(&current_proc_global_id, 1, MPI_INT, comp_comm_info->comp_procs_global_ids, 1, MPI_INT, comm) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(comm, &comp_comm_info->current_proc_local_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(MPI_COMM_WORLD, &current_proc_global_id) == MPI_SUCCESS);
	comps_comms_info.push_back(comp_comm_info);
}


Component_communicator_info *Compset_communicators_info_mgt::get_communicator_info_by_name(const char *comp_name)
{
	int i;
	

	for (i = 0; i < comps_comms_info.size(); i ++)
		if (words_are_the_same(comps_comms_info[i]->comp_name, comp_name))
			break;

	EXECUTION_REPORT(REPORT_ERROR,-1, i < comps_comms_info.size(), "Component with name \"%s\" has not been registered. Please verify", comp_name);
	
	return comps_comms_info[i];
}


Comp_comm_group_mgt_global_node::~Comp_comm_group_mgt_global_node()
{
	for (int i = 0; i < children.size(); i ++)
		delete children[i];
	if (temp_array_buffer != NULL)
		delete [] temp_array_buffer;
}


Comp_comm_group_mgt_global_node::Comp_comm_group_mgt_global_node(Comp_comm_group_mgt_global_node *buffer_node, Comp_comm_group_mgt_global_node *parent, int &global_node_id)
{	
	int num_procs, *proc_id, num_children;


	buffer_node->read_data_from_array_buffer(annotation_end, NAME_STR_SIZE);
	buffer_node->read_data_from_array_buffer(annotation_start, NAME_STR_SIZE);
	buffer_node->read_data_from_array_buffer(comp_name, NAME_STR_SIZE);
	buffer_node->read_data_from_array_buffer(comp_type, NAME_STR_SIZE);
	buffer_node->read_data_from_array_buffer(&comm_group, sizeof(MPI_Comm));
	buffer_node->read_data_from_array_buffer(&num_procs, sizeof(int));
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(MPI_COMM_WORLD, &current_proc_global_id) == MPI_SUCCESS);
	proc_id = new int [num_procs];
	for (int i = 0; i < num_procs; i ++)
		buffer_node->read_data_from_array_buffer(&proc_id[num_procs-1-i], sizeof(int));
	for (int i = 0; i < num_procs; i ++)
		local_processes_global_ids.push_back(proc_id[i]);
	this->local_node_id = -1;
	this->comm_group = -1;
	this->current_proc_local_id = -1;
	this->global_node_id = (global_node_id | TYPE_COMP_GLOBAL_ID_PREFIX);
	global_node_id ++;
	this->parent = parent;
	temp_array_buffer = NULL;
	definition_finalized = true;
	
	buffer_node->read_data_from_array_buffer(&num_children, sizeof(int));
	for (int i = 0; i < num_children; i ++)
		children.push_back(new Comp_comm_group_mgt_global_node(buffer_node, this, global_node_id));
}


Comp_comm_group_mgt_global_node::Comp_comm_group_mgt_global_node(const char *comp_name, const char *comp_type, int local_node_id, Comp_comm_group_mgt_global_node *parent, MPI_Comm &comm)
{
	char *all_comp_name, *all_comp_type;
	std::vector<char*> unique_comp_name, unique_comp_type;
	int i, j, num_procs, current_proc_local_id_in_parent, *process_comp_id, *processes_global_id;
	MPI_Comm parent_comm;

	
	strcpy(this->comp_name, comp_name);
	strcpy(this->comp_type, comp_type);
	get_annotation(this->annotation_start);
	annotation_end[0] = '\0';
	this->local_node_id = local_node_id;
	this->global_node_id = -1;
	this->parent = parent;
	buffer_content_size = 0;
	buffer_max_size = 1024;
	temp_array_buffer = new char [buffer_max_size];
	definition_finalized = false;

	if (comm != -1) {
		char temp_comp_name[NAME_STR_SIZE], temp_comp_type[NAME_STR_SIZE];
		int proc_local_id;
		comm_group = comm;
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(comm, &proc_local_id) == MPI_SUCCESS);	 
		if (parent != NULL && (parent->local_node_id&TYPE_ID_SUFFIX_MASK) != 0)
			EXECUTION_REPORT(REPORT_PROGRESS, parent->local_node_id, true, "Before the MPI_barrier for synchronizing all processes of a communicator for registerring component \"%s\" %s", comp_name, annotation_start);	
		else if (proc_local_id == 0)
			EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "Before the MPI_barrier for synchronizing all processes of a communicator for registerring component \"%s\" %s", comp_name, annotation_start);				
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Barrier(comm) == MPI_SUCCESS);
		if (parent != NULL && (parent->local_node_id&TYPE_ID_SUFFIX_MASK) != 0)
			EXECUTION_REPORT(REPORT_PROGRESS, parent->local_node_id, true, "After the MPI_barrier for synchronizing all processes of a communicator for registerring component \"%s\" %s", comp_name, annotation_start);	
		else if (proc_local_id == 0) 
			EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "After the MPI_barrier for synchronizing all processes of a communicator for registerring component \"%s\" %s", comp_name, annotation_start);	
		strcpy(temp_comp_name, comp_name);
		strcpy(temp_comp_type, comp_type);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(temp_comp_name, NAME_STR_SIZE, MPI_CHAR, 0, comm) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(temp_comp_type, NAME_STR_SIZE, MPI_CHAR, 0, comm) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(temp_comp_type, comp_type) && words_are_the_same(temp_comp_name, comp_name), "When registerring a component with an available communicator, the component name and type must be the same among all processes, while the name is different (<\"%s\", \"%s\"> VS <\"%s\", \"%s\">) %s",
			             temp_comp_name, temp_comp_type, comp_name, comp_type, annotation_start);
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR,-1, parent != NULL, "Software error in Comp_comm_group_mgt_global_node::Comp_comm_group_mgt_global_node for checking parent");
		parent_comm = parent->get_comm_group();
		if (parent != NULL && (parent->local_node_id&TYPE_ID_SUFFIX_MASK) != 0)
			EXECUTION_REPORT(REPORT_PROGRESS, parent->local_node_id, true, "Before the MPI_barrier for synchronizing all processes of the parent component \"%s\" %s for registerring its children components %s", parent->get_comp_name(), parent->annotation_start, annotation_start);
		else if (parent->get_current_proc_local_id() == 0) 
			EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "Before the MPI_barrier for synchronizing all processes of the parent component \"%s\" %s for registerring its children components %s", parent->get_comp_name(), parent->annotation_start, annotation_start);	
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Barrier(parent_comm) == MPI_SUCCESS);
		if (parent != NULL && (parent->local_node_id&TYPE_ID_SUFFIX_MASK) != 0)
			EXECUTION_REPORT(REPORT_PROGRESS, parent->local_node_id, true, "After the MPI_barrier for synchronizing all processes of the parent component \"%s\" %s for registerring its children components %s", parent->get_comp_name(), parent->annotation_start, annotation_start);
		else if (parent->get_current_proc_local_id() == 0) 
			EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "After the MPI_barrier for synchronizing all processes of the parent component \"%s\" %s for registerring its children components %s", parent->get_comp_name(), parent->annotation_start, annotation_start);	
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(parent_comm, &num_procs) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(parent_comm, &current_proc_local_id_in_parent) == MPI_SUCCESS);
		all_comp_name = new char [NAME_STR_SIZE*num_procs];
		all_comp_type = new char [NAME_STR_SIZE*num_procs];
		process_comp_id = new int [num_procs];
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather((char*)comp_name, NAME_STR_SIZE, MPI_CHAR, all_comp_name, NAME_STR_SIZE, MPI_CHAR, 0, parent_comm) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather((char*)comp_type, NAME_STR_SIZE, MPI_CHAR, all_comp_type, NAME_STR_SIZE, MPI_CHAR, 0, parent_comm) == MPI_SUCCESS);
		if (current_proc_local_id_in_parent == 0) {
			unique_comp_name.push_back(all_comp_name);
			unique_comp_type.push_back(all_comp_type);
			process_comp_id[0] = 0;
			for (i = 1; i < num_procs; i ++) {
				for (j = 0; j < unique_comp_name.size(); j ++)
					if (words_are_the_same(unique_comp_name[j], all_comp_name+i*NAME_STR_SIZE)) {
						EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(unique_comp_type[j], all_comp_type+i*NAME_STR_SIZE), 
							             "The type of the component is different (\"%s\" vs \"%s\") among the processes of component \"%s\" %s. Please verify.", 
							             unique_comp_type[j], all_comp_type+i*NAME_STR_SIZE, unique_comp_name[j], annotation_start);
						break;
					}
				if (j == unique_comp_name.size()) {
					unique_comp_name.push_back(all_comp_name+i*NAME_STR_SIZE);
					unique_comp_type.push_back(all_comp_type+i*NAME_STR_SIZE);
				}
				process_comp_id[i] = j;
			}
		}
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(process_comp_id, num_procs, MPI_INT, 0, parent_comm)  == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_split(parent_comm, process_comp_id[current_proc_local_id_in_parent], 0, &comm_group) == MPI_SUCCESS);
		delete [] all_comp_name;
		delete [] all_comp_type;
		delete [] process_comp_id;
		comm = comm_group;
	}

	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(comm_group, &current_proc_local_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(MPI_COMM_WORLD, &current_proc_global_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(comm_group, &num_procs) == MPI_SUCCESS);
	processes_global_id = new int [num_procs];
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(&current_proc_global_id, 1, MPI_INT, processes_global_id, 1, MPI_INT, comm_group) == MPI_SUCCESS);
	for (i = 0; i < num_procs; i ++)
		local_processes_global_ids.push_back(processes_global_id[i]);
	delete [] processes_global_id;

	if (parent != NULL) {
		for (i = 0; i < local_processes_global_ids.size(); i ++) {
			for (j = 0; j < parent->local_processes_global_ids.size(); j ++)
				if (local_processes_global_ids[i] == parent->local_processes_global_ids[j])
					break;
			if (current_proc_local_id == 0)
				EXECUTION_REPORT(REPORT_ERROR,-1, j < parent->local_processes_global_ids.size(), 
				                 "The processes of component \"%s\" %s must be a subset of the processes of its parent \"%s\" %s", 
				                 comp_name, annotation_start, parent->get_comp_name(), parent->annotation_start);
		}
		parent->children.push_back(this);
	}
}


void Comp_comm_group_mgt_global_node::transform_node_into_array()
{
	int num_procs, proc_id, num_children;


	num_procs = local_processes_global_ids.size();
	for (int i = 0; i < num_procs; i ++) {
		proc_id = local_processes_global_ids[i];
		write_data_into_array_buffer(&proc_id, sizeof(int));
	}
	write_data_into_array_buffer(&num_procs, sizeof(int));
	write_data_into_array_buffer(&comm_group, sizeof(MPI_Comm));
	write_data_into_array_buffer(comp_type, NAME_STR_SIZE);
	write_data_into_array_buffer(comp_name, NAME_STR_SIZE);
	write_data_into_array_buffer(annotation_start, NAME_STR_SIZE);
	write_data_into_array_buffer(annotation_end, NAME_STR_SIZE);
}


void Comp_comm_group_mgt_global_node::write_data_into_array_buffer(const void *data, int data_size)
{
	if (buffer_max_size < buffer_content_size+data_size) {
		buffer_max_size = (buffer_content_size+data_size) * 2;
		char *temp_buffer = new char [buffer_max_size];
		for (int i = 0; i < buffer_content_size; i ++)
			temp_buffer[i] = temp_array_buffer[i];
		delete [] temp_array_buffer;
		temp_array_buffer = temp_buffer;
	}

	for (int i = 0; i < data_size; i ++)
		temp_array_buffer[buffer_content_size++] = ((char*)data)[i];
}


void Comp_comm_group_mgt_global_node::read_data_from_array_buffer(void *data, int data_size)
{
	EXECUTION_REPORT(REPORT_ERROR,-1, data_size <= buffer_content_size, "Software error in Comp_comm_group_mgt_global_node::read_data_into_array_buffer");
	
	for (int i = 0; i < data_size; i ++)
		((char*) data)[i] = temp_array_buffer[buffer_content_size-data_size+i];
	
	buffer_content_size -= data_size;
}


void Comp_comm_group_mgt_global_node::merge_comp_comm_info(bool is_root_node)
{
	int i, num_children_at_root, *num_children_for_all, *counts, *displs;
	char *temp_buffer;
	

	if (strlen(current_annotation) != 0)
		EXECUTION_REPORT(REPORT_ERROR,-1, !definition_finalized, "The registration related to component \"%s\" has been finalized before %s. It cannot be finalized again (corresponding to the annotation \"%s\"). Please verify.",
			             comp_name, annotation_end, current_annotation);
	else EXECUTION_REPORT(REPORT_ERROR,-1, !definition_finalized, "The registration related to component \"%s\" has been finalized before %s. It cannot be finalized again. Please verify.",
			              comp_name, annotation_end); 
	definition_finalized = true;

	get_annotation(this->annotation_end);

	for (int i = 0; i < children.size(); i ++)
		EXECUTION_REPORT(REPORT_ERROR,-1, children[i]->definition_finalized, 
		                 "The registration related to component \"%s\" %s has not been finalized yet. So the registration related to its parent component \"%s\" cannot be finalized. Please verify.",
		                 children[i]->comp_name, children[i]->annotation_start, comp_name, annotation_end);

	for (i = 0, num_children_at_root = 0; i < children.size(); i ++)
		if (children[i]->current_proc_local_id == 0) {
			num_children_at_root ++;
			write_data_into_array_buffer(children[i]->temp_array_buffer, children[i]->buffer_content_size);
		}
	
	if (current_proc_local_id == 0) {
		num_children_for_all = new int [local_processes_global_ids.size()];
		counts = new int [local_processes_global_ids.size()];
		displs = new int [local_processes_global_ids.size()];
	}

	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather(&num_children_at_root, 1, MPI_INT, num_children_for_all, 1, MPI_INT, 0, comm_group) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather(&buffer_content_size, 1, MPI_INT, counts, 1, MPI_INT, 0, comm_group) == MPI_SUCCESS);

	if (current_proc_local_id == 0) {
		displs[0] = 0;
		for (i = 1; i < local_processes_global_ids.size(); i ++) {
			displs[i] = displs[i-1]+counts[i-1];
			num_children_at_root += num_children_for_all[i];
		}
		temp_buffer = new char [displs[i-1]+counts[i-1]+100];
	}

    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gatherv(temp_array_buffer, buffer_content_size, MPI_CHAR, temp_buffer, counts, displs, MPI_CHAR, 0, comm_group) == MPI_SUCCESS);
	
	if (current_proc_local_id == 0) {		
		delete [] temp_array_buffer;
		temp_array_buffer = temp_buffer;
		buffer_content_size = displs[i-1]+counts[i-1];
		buffer_max_size = buffer_content_size+100;
		write_data_into_array_buffer(&num_children_at_root, sizeof(int));
		transform_node_into_array();
		delete [] num_children_for_all;
		delete [] counts;
		delete [] displs;
	}
	
//	MPI_Barrier(comm_group);

	if (is_root_node) {
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(&buffer_content_size, 1, MPI_INT, 0, comm_group)  == MPI_SUCCESS);
		if (current_proc_local_id != 0) {
			delete [] temp_array_buffer;
			temp_array_buffer = new char [buffer_content_size];
			buffer_max_size = buffer_content_size;
		}
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(temp_array_buffer, buffer_content_size, MPI_CHAR, 0, MPI_COMM_WORLD)  == MPI_SUCCESS);
	}
}


Comp_comm_group_mgt_global_node *Comp_comm_group_mgt_global_node::search_global_node(int global_node_id)
{
	if (global_node_id == this->global_node_id)
		return this;

	for (int i = 0; i < children.size(); i ++) {
		Comp_comm_group_mgt_global_node *global_node = children[i]->search_global_node(global_node_id);
		if (global_node != NULL)
			return global_node;
	}

	return NULL;
}


void Comp_comm_group_mgt_global_node::write_node_into_XML(TiXmlElement *parent_element)
{
	int i, num_segments;
	int *segments_start, *segments_end;
	TiXmlElement * current_element;
	char *string;

	
	current_element = new TiXmlElement("Online_Model");
	parent_element->LinkEndChild(current_element);
	current_element->SetAttribute("name", comp_name);
	current_element->SetAttribute("type", comp_type);
	current_element->SetAttribute("global_id", global_node_id);

	segments_start = new int [local_processes_global_ids.size()];
	segments_end = new int [local_processes_global_ids.size()];
	segments_start[0] = local_processes_global_ids[0];
	for (i = 1, num_segments = 1; i < local_processes_global_ids.size(); i ++) {
		if (local_processes_global_ids[i] != local_processes_global_ids[i-1]+1) {
			segments_end[num_segments-1] = local_processes_global_ids[i-1];
			segments_start[num_segments] = local_processes_global_ids[i];
			num_segments ++;
		}
	}
	segments_end[num_segments-1] = local_processes_global_ids[local_processes_global_ids.size()-1];
	string = new char [num_segments*(8*2+1)];
	string[0] = '\0';
	for (i = 0; i < num_segments; i ++) {
		if (segments_start[i] != segments_end[i])
			sprintf(string+strlen(string), " %d~%d", segments_start[i], segments_end[i]);
		else sprintf(string+strlen(string), " %d", segments_start[i]);
	}

	current_element->SetAttribute("processes", string);
	
	delete [] segments_start;
	delete [] segments_end;
	delete [] string;

	for (i = 0; i < children.size(); i ++)
		children[i]->write_node_into_XML(current_element);
}


void Comp_comm_group_mgt_global_node::print_global_nodes()
{
	if (parent != NULL)
		printf("information of global node %x (%x: %d) (%s) at process %d, parent id is %x, number of children is %d, ", global_node_id, local_node_id, current_proc_local_id, comp_name, current_proc_global_id, parent->global_node_id, children.size());
	else printf("information of global node %x (%x: %d) (%s) at process %d, does not have parent, number of children is %d, ", global_node_id, local_node_id, current_proc_local_id, comp_name, current_proc_global_id, children.size()); 
	printf("processes include: ");
	for (int i = 0; i < local_processes_global_ids.size(); i ++)
		printf("%d  ", local_processes_global_ids[i]);
	printf("\n\n\n");

	for (int i = 0; i < children.size(); i ++)
		children[i]->print_global_nodes();
}


Comp_comm_group_mgt_local_node::Comp_comm_group_mgt_local_node(const char *comp_name, const char *comp_type, Comp_comm_group_mgt_local_node *parent, MPI_Comm &comm, int local_id)
{
	char dir[NAME_STR_SIZE];

	
	self_local_node_id = local_id;
	if (parent != NULL) {
		parent_local_node_id = parent->get_local_node_id();
		global_node = new Comp_comm_group_mgt_global_node(comp_name, comp_type, local_id, parent->get_global_node(), comm);
		if (parent->get_global_node()->get_parent() == NULL) {
			EXECUTION_REPORT(REPORT_ERROR,-1, getcwd(working_dir,NAME_STR_SIZE) != NULL, "Cannot get the current working directory");
			strcpy(working_dir+strlen(working_dir), "/../");
		}
		else {
			sprintf(working_dir, "%s/%s\0", parent->working_dir, comp_name);
			create_directory(working_dir, global_node->get_current_proc_local_id() == 0);
		}
		sprintf(dir, "%s/CCPL_logs", working_dir, comp_name);
		create_directory(dir, global_node->get_current_proc_local_id() == 0);
		sprintf(dir, "%s/CCPL_configs", working_dir, comp_name);
		create_directory(dir, global_node->get_current_proc_local_id() == 0);
		MPI_Barrier(global_node->get_comm_group());
		char file_name[NAME_STR_SIZE*2];
		sprintf(file_name, "%s/CCPL_logs/%s.CCPL.log.%d", working_dir, global_node->get_comp_name(), global_node->get_current_proc_local_id());
		FILE *fp = fopen(file_name, "w+");
		fclose(fp);
	}
	else {
		parent_local_node_id = -1; 
		global_node = new Comp_comm_group_mgt_global_node(comp_name, comp_type, local_id, NULL, comm);
		EXECUTION_REPORT(REPORT_ERROR,-1, getcwd(working_dir,NAME_STR_SIZE) != NULL, "Cannot get the current working directory");
		strcpy(working_dir+strlen(working_dir), "/../../../all/");
		sprintf(dir, "%s/CCPL_configs", working_dir, comp_name);
		create_directory(dir, global_node->get_current_proc_local_id() == 0);
	}	
}


Comp_comm_group_mgt_mgr::Comp_comm_group_mgt_mgr(const char *executable_name, const char *experiment_model, const char *case_name, 
	       const char *case_desc, const char *case_mode, const char *comp_namelist,
           const char *current_config_time, const char *original_case_name, const char *original_config_time)
{
	int i, j;

	
	local_nodes.clear();
	global_node_root = NULL;
	definition_finalized = false;

	strcpy(this->experiment_model, experiment_model);
    strcpy(current_case_name, case_name);
    strcpy(current_case_desc, case_desc);
    strcpy(running_case_mode, case_mode);
    strcpy(comp_namelist_filename, comp_namelist);
	strcpy(this->current_config_time, current_config_time);
	strcpy(this->original_case_name, original_case_name);
	strcpy(this->original_config_time, original_config_time);

	for (i = strlen(executable_name)-1; i >= 0; i --)
		if (executable_name[i] == '/')
			break;
	i ++;
	EXECUTION_REPORT(REPORT_ERROR,-1, i < strlen(executable_name), "Software error1 in Comp_comm_group_mgt_mgr::Comp_comm_group_mgt_mgr");
	strcpy(this->executable_name, executable_name+i);

	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(MPI_COMM_WORLD, &current_proc_global_id) == MPI_SUCCESS);

	if (words_are_the_same(case_mode, "initial")) {
		strcpy(this->original_case_name, "none");
		strcpy(this->original_config_time, "none");
	}		
}


Comp_comm_group_mgt_mgr::~Comp_comm_group_mgt_mgr()
{
	for (int i = 0; i < local_nodes.size(); i ++)
		delete local_nodes[i];

	delete global_node_root;
}


void Comp_comm_group_mgt_mgr::transform_global_node_tree_into_array(Comp_comm_group_mgt_global_node *current_global_node, Comp_comm_group_mgt_global_node **all_global_nodes, int &global_node_id)
{
	all_global_nodes[global_node_id++] = current_global_node;
	for (int i = 0; i < current_global_node->get_num_children(); i ++)
		transform_global_node_tree_into_array(current_global_node->get_child(i), all_global_nodes, global_node_id);
}


void Comp_comm_group_mgt_mgr::update_global_nodes(Comp_comm_group_mgt_global_node *old_global_node, Comp_comm_group_mgt_global_node *new_global_node)
{
	int i, j;
	bool find_new_global_node;


	for (i = 0; i < old_global_node->get_num_children(); i ++) {
		find_new_global_node = false;
		for (j = 0; j < new_global_node->get_num_children(); j ++)
			if (words_are_the_same(old_global_node->get_child(i)->get_comp_name(), new_global_node->get_child(j)->get_comp_name())) {
				find_new_global_node = true;
				EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(old_global_node->get_child(i)->get_comp_type(), new_global_node->get_child(j)->get_comp_type()),
								 "Software error1 in Comp_comm_group_mgt_mgr::update_global_nodes");
				EXECUTION_REPORT(REPORT_ERROR,-1, new_global_node->get_child(j)->get_local_node_id() == -1, "Software error2 in Comp_comm_group_mgt_mgr::update_global_nodes");
				new_global_node->get_child(j)->reset_local_node_id(old_global_node->get_child(i)->get_local_node_id());
				new_global_node->get_child(j)->reset_comm_group(old_global_node->get_child(i)->get_comm_group());
				new_global_node->get_child(j)->reset_current_proc_local_id(old_global_node->get_current_proc_local_id());
				local_nodes[old_global_node->get_child(i)->get_local_node_id()&TYPE_ID_SUFFIX_MASK]->reset_global_node(new_global_node->get_child(j));
				update_global_nodes(old_global_node->get_child(i), new_global_node->get_child(j));
				break;
			}
		EXECUTION_REPORT(REPORT_ERROR,-1, find_new_global_node, "Software error3 in Comp_comm_group_mgt_mgr::update_global_nodes");
	}
}


bool Comp_comm_group_mgt_mgr::is_legal_local_comp_id(int local_comp_id)
{
	if ((local_comp_id&TYPE_ID_PREFIX_MASK) != TYPE_COMP_LOCAL_ID_PREFIX)
		return false;

	int true_parent_id = (local_comp_id & TYPE_ID_SUFFIX_MASK);
	return true_parent_id >= 0 && true_parent_id < local_nodes.size();
}


bool Comp_comm_group_mgt_mgr::is_local_comp_definition_finalized(int local_comp_id)
{
	EXECUTION_REPORT(REPORT_ERROR,-1, is_legal_local_comp_id(local_comp_id), "software error in Comp_comm_group_mgt_mgr::is_local_comp_definition_finalized");

	return local_nodes[(local_comp_id&TYPE_ID_SUFFIX_MASK)]->get_global_node()->is_definition_finalized();
}


int Comp_comm_group_mgt_mgr::register_component(const char *comp_name, const char *comp_type, MPI_Comm &comm, int parent_local_id)
{
	int i, true_parent_id = 0;
	Comp_comm_group_mgt_local_node *root_local_node, *new_comp;
	MPI_Comm global_comm = MPI_COMM_WORLD;
	char annotation[NAME_STR_SIZE];



	get_annotation(annotation);
	
	if (definition_finalized)
		EXECUTION_REPORT(REPORT_ERROR,-1, !definition_finalized, "Cannot register component \"%s\" %s because the stage of registering coupling configurations of the whole coupled model has been ended %s", comp_name, annotation, local_nodes[0]->get_global_node()->get_annotation_end());
	
	for (i = 0; i < local_nodes.size(); i ++)
		if (words_are_the_same(local_nodes[i]->get_global_node()->get_comp_name(), comp_name))
			break;
		
	if (i < local_nodes.size())
		EXECUTION_REPORT(REPORT_ERROR,-1, i == local_nodes.size(),  
						 "A component with the name \"%s\" has already been registered %s. The same name cannot be used for registerring another component %s", comp_name, local_nodes[i]->get_global_node()->get_annotation_start(), annotation);

	if (parent_local_id == -1) {
		root_local_node = new Comp_comm_group_mgt_local_node("ROOT", "ROOT", NULL, global_comm, local_nodes.size()|TYPE_COMP_LOCAL_ID_PREFIX);
		local_nodes.push_back(root_local_node);
		global_node_root = root_local_node->get_global_node();
		new_comp = new Comp_comm_group_mgt_local_node(comp_name, comp_type, root_local_node, comm, local_nodes.size()|TYPE_COMP_LOCAL_ID_PREFIX);
		local_nodes.push_back(new_comp);
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR,-1, is_legal_local_comp_id(parent_local_id), 
			             "For the registration of component (name=\"%s\", type=\"%s\"), the input parameter of the ID of the parent component is wrong %s",
			             comp_name, comp_type, annotation);
		true_parent_id = (parent_local_id & TYPE_ID_SUFFIX_MASK);
		EXECUTION_REPORT(REPORT_ERROR,-1, !local_nodes[true_parent_id]->get_global_node()->is_definition_finalized(), 
			             "Cannot register component %s %s because the registration corresponding to the parent \"%s\" %s has been ended", 
			             comp_name, annotation, local_nodes[true_parent_id]->get_global_node()->get_comp_name(), local_nodes[true_parent_id]->get_global_node()->get_annotation_end()); // add debug information
		new_comp = new Comp_comm_group_mgt_local_node(comp_name, comp_type, local_nodes[true_parent_id], comm, local_nodes.size()|TYPE_COMP_LOCAL_ID_PREFIX);
		local_nodes.push_back(new_comp);
	}

	return new_comp->get_local_node_id();
}


void Comp_comm_group_mgt_mgr::merge_comp_comm_info(int comp_local_id)
{
	Comp_comm_group_mgt_global_node *global_node;
	int true_local_id = (comp_local_id & TYPE_ID_SUFFIX_MASK), global_node_id;
	char annotation[NAME_STR_SIZE];


	get_annotation(annotation);


	EXECUTION_REPORT(REPORT_ERROR,-1, is_legal_local_comp_id(comp_local_id), "The comp_local_id \"%x\" for ending the coupling registration is wrong. Please check %s", comp_local_id, annotation);
	global_node = local_nodes[true_local_id]->get_global_node();
	if (true_local_id != 0)
		EXECUTION_REPORT(REPORT_PROGRESS, comp_local_id, true, "Before MPI_barrier of ending the registration of component \"%s\" %s", global_node->get_comp_name(), annotation);
	else if (global_node->get_current_proc_local_id() == 0)
		EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "Before MPI_barrier of ending the registration of component \"%s\" %s", global_node->get_comp_name(), annotation);	
	MPI_Barrier(global_node->get_comm_group());
	if (true_local_id != 0)
		EXECUTION_REPORT(REPORT_PROGRESS, comp_local_id, true, "After MPI_barrier of ending the registration of component \"%s\" %s", global_node->get_comp_name(), annotation);
	else if (global_node->get_current_proc_local_id() == 0)
		EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "After MPI_barrier of ending the registration of component \"%s\" %s", global_node->get_comp_name(), annotation);
	
	global_node->merge_comp_comm_info(true_local_id == 0);

	if (true_local_id == 1)
		merge_comp_comm_info(local_nodes[0]->get_local_node_id());

	if (true_local_id == 0) {
		global_node_id = 0;
		Comp_comm_group_mgt_global_node *new_root = new Comp_comm_group_mgt_global_node(global_node, NULL, global_node_id);
		EXECUTION_REPORT(REPORT_ERROR,-1, global_node->get_buffer_content_size() == 0, "Software error1 in Comp_comm_group_mgt_mgr::merge_comp_comm_info");
		Comp_comm_group_mgt_global_node **all_global_nodes = new Comp_comm_group_mgt_global_node *[global_node_id];
		global_node_id = 0;
		transform_global_node_tree_into_array(new_root, all_global_nodes, global_node_id);
		for (int i = 0; i < global_node_id; i ++)
			for (int j = i+1; j < global_node_id; j ++)
				EXECUTION_REPORT(REPORT_ERROR, -1, !words_are_the_same(all_global_nodes[i]->get_comp_name(), all_global_nodes[j]->get_comp_name()), 
				                 "There are at least two components have the same name (\"%s\"), which is not allowed. Please check the model code (%s and %s).", 
				                 all_global_nodes[i]->get_comp_name(), all_global_nodes[i]->get_annotation_start(), all_global_nodes[j]->get_annotation_start());
		new_root->reset_local_node_id(global_node_root->get_local_node_id());
		new_root->reset_comm_group(global_node_root->get_comm_group());
		new_root->reset_current_proc_local_id(global_node_root->get_current_proc_local_id());
		local_nodes[global_node_root->get_local_node_id()&TYPE_ID_SUFFIX_MASK]->reset_global_node(new_root);
		update_global_nodes(global_node_root, new_root);
		delete global_node_root;
		global_node_root = new_root;
		definition_finalized = true;
		if (current_proc_global_id == 0)
			new_root->print_global_nodes();
		write_comp_comm_info_into_XML();
		read_comp_comm_info_from_XML();
		delete [] all_global_nodes;
	}
	global_node = local_nodes[true_local_id]->get_global_node();
	MPI_Barrier(global_node->get_comm_group());
}


Comp_comm_group_mgt_local_node *Comp_comm_group_mgt_mgr::get_local_node_of_local_comp(int local_comp_id)
{
	char annotation[NAME_STR_SIZE];


	get_annotation(annotation);
	
	EXECUTION_REPORT(REPORT_ERROR,-1, is_legal_local_comp_id(local_comp_id), "The id of component is wrong. Please check the model code with the annotation \"%s\".", annotation); 

	return local_nodes[(local_comp_id&TYPE_ID_SUFFIX_MASK)];
}


Comp_comm_group_mgt_global_node *Comp_comm_group_mgt_mgr::get_global_node_of_local_comp(int local_comp_id)
{
	return get_local_node_of_local_comp(local_comp_id)->get_global_node();
}


Comp_comm_group_mgt_global_node *Comp_comm_group_mgt_mgr::get_global_node_of_global_comp(int global_comp_id)
{
	EXECUTION_REPORT(REPORT_ERROR,-1, definition_finalized, "Software error in Comp_comm_group_mgt_mgr::get_global_node_of_global_comp");
		
	Comp_comm_group_mgt_global_node *global_node = global_node_root->search_global_node(global_comp_id);
	
	EXECUTION_REPORT(REPORT_ERROR,-1, global_node != NULL, "Software error in Comp_comm_group_mgt_mgr::get_global_node_of_global_comp");  // add debug information
	
	return global_node;
}


MPI_Comm Comp_comm_group_mgt_mgr::get_comm_group_of_local_comp(int local_comp_id)
{
	get_global_node_of_local_comp(local_comp_id)->get_comm_group();
}


MPI_Comm Comp_comm_group_mgt_mgr::get_comm_group_of_global_comp(int global_comp_id)
{
	get_global_node_of_global_comp(global_comp_id)->get_comm_group();
}


void Comp_comm_group_mgt_mgr::get_log_file_name(int comp_id, char *log_file_name)
{
	int true_comp_id;


	EXECUTION_REPORT(REPORT_ERROR, -1, is_legal_local_comp_id(comp_id), "software error in Comp_comm_group_mgt_mgr::get_log_file_name");
	true_comp_id = (comp_id & TYPE_ID_SUFFIX_MASK);
	sprintf(log_file_name, "%s/CCPL_logs/%s.CCPL.log.%d", local_nodes[true_comp_id]->get_working_dir(), local_nodes[true_comp_id]->get_global_node()->get_comp_name(), local_nodes[true_comp_id]->get_global_node()->get_current_proc_local_id());
}

void Comp_comm_group_mgt_mgr::write_comp_comm_info_into_XML()
{
	char XML_file_name[NAME_STR_SIZE];

	
	TiXmlDocument *XML_file;
	TiXmlDeclaration *XML_declaration;
	TiXmlElement * root_element;


	if (current_proc_global_id != 0)
		return;

	XML_file = new TiXmlDocument;
	XML_declaration = new TiXmlDeclaration(("1.0"),(""),(""));
	EXECUTION_REPORT(REPORT_ERROR, -1, XML_file != NULL, "Software error: cannot create an xml file");
	
	XML_file->LinkEndChild(XML_declaration);
	root_element = new TiXmlElement("Components");
	XML_file->LinkEndChild(root_element);
	global_node_root->write_node_into_XML(root_element);

	sprintf(XML_file_name, "%s/CCPL_configs/components.xml", local_nodes[0]->get_working_dir());
	
	XML_file->SaveFile(XML_file_name);
}


void Comp_comm_group_mgt_mgr::read_global_node_from_XML(const TiXmlElement *current_element)
{
	int global_node_id;
	Comp_comm_group_mgt_global_node *global_node;
	const TiXmlNode *child;


	global_node_id = atoi(current_element->Attribute("global_id"));
	global_node = search_global_node(global_node_id);
	EXECUTION_REPORT(REPORT_ERROR, -1, global_node != NULL, "The XML file of component model hierarchy has been illegally modified by others or C-Coupler has software bugs");
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(global_node->get_comp_name(), current_element->Attribute("name")), "The XML file of component model hierarchy has been illegally modified by others or there are software errors");
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(global_node->get_comp_type(), current_element->Attribute("type")), "The XML file of component model hierarchy has been illegally modified by others or there are software errors");

	for (child = current_element->FirstChild(); child != NULL; child = child->NextSibling())
		read_global_node_from_XML(child->ToElement());
}


void Comp_comm_group_mgt_mgr::read_comp_comm_info_from_XML()
{
	char XML_file_name[NAME_STR_SIZE];


	if (current_proc_global_id != 0)
		return;

	sprintf(XML_file_name, "%s/CCPL_configs/components.xml", local_nodes[0]->get_working_dir());
	TiXmlDocument XML_file(XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, -1, XML_file.LoadFile(), "cannot open the components xml file: %s", XML_file_name);
	
	TiXmlElement *XML_element = XML_file.FirstChildElement();
	TiXmlElement *Online_Model = XML_element->FirstChildElement();
	read_global_node_from_XML(Online_Model);
}


Comp_comm_group_mgt_global_node *Comp_comm_group_mgt_mgr::search_global_node(int global_node_id)
{
	return global_node_root->search_global_node(global_node_id);
}

