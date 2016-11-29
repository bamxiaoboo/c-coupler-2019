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


Comp_comm_group_mgt_node::~Comp_comm_group_mgt_node()
{
	if (temp_array_buffer != NULL)
		delete [] temp_array_buffer;
}


Comp_comm_group_mgt_node::Comp_comm_group_mgt_node(Comp_comm_group_mgt_node *buffer_node, Comp_comm_group_mgt_node *parent, int &global_node_id)
{	
	int num_procs, *proc_id, num_children;


	int old_iter = buffer_content_iter;
	read_data_from_array_buffer(annotation_end, NAME_STR_SIZE, buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	read_data_from_array_buffer(annotation_start, NAME_STR_SIZE, buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	read_data_from_array_buffer(working_dir, NAME_STR_SIZE, buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	read_data_from_array_buffer(full_name, NAME_STR_SIZE, buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	read_data_from_array_buffer(comp_name, NAME_STR_SIZE, buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	read_data_from_array_buffer(&comm_group, sizeof(MPI_Comm), buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	read_data_from_array_buffer(&num_procs, sizeof(int), buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(MPI_COMM_WORLD, &current_proc_global_id) == MPI_SUCCESS);
	proc_id = new int [num_procs];
	for (int i = 0; i < num_procs; i ++)
		read_data_from_array_buffer(&proc_id[num_procs-1-i], sizeof(int), buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	for (int i = 0; i < num_procs; i ++)
		local_processes_global_ids.push_back(proc_id[i]);
	this->comp_id = -1;
	this->comm_group = -1;
	this->current_proc_local_id = -1;
	this->working_dir[0] = '\0';
	global_node_id ++;
	this->parent = parent;
	temp_array_buffer = NULL;
	definition_finalized = true;
	unified_global_id = 0;

	read_data_from_array_buffer(&num_children, sizeof(int), buffer_node->temp_array_buffer, buffer_node->buffer_content_iter);
	for (int i = 0; i < num_children; i ++)
		children.push_back(new Comp_comm_group_mgt_node(buffer_node, this, global_node_id));
}


Comp_comm_group_mgt_node::Comp_comm_group_mgt_node(const char *comp_name, int comp_id, Comp_comm_group_mgt_node *parent, MPI_Comm &comm, const char *annotation)
{
	char *all_comp_name;
	std::vector<char*> unique_comp_name;
	int i, j, num_procs, current_proc_local_id_in_parent, *process_comp_id, *processes_global_id;
	MPI_Comm parent_comm;
	char dir[NAME_STR_SIZE];

	
	strcpy(this->comp_name, comp_name);
	if (parent == NULL || words_are_the_same(parent->get_comp_name(), "ROOT"))
		strcpy(this->full_name, this->comp_name);
	else sprintf(this->full_name, "%s@%s", parent->get_full_name(), this->comp_name);
	strcpy(this->annotation_start, annotation);
	this->annotation_end[0] = '\0';
	this->comp_id = comp_id;
	this->parent = parent;
	this->buffer_content_size = 0;
	this->buffer_max_size = 1024;
	this->temp_array_buffer = new char [buffer_max_size];
	this->definition_finalized = false;	
	this->unified_global_id = 0;


	if (comm != -1)
		comm_group = comm;
	else {
		EXECUTION_REPORT(REPORT_ERROR,-1, parent != NULL, "Software error in Comp_comm_group_mgt_node::Comp_comm_group_mgt_node for checking parent");
		parent_comm = parent->get_comm_group();
		if ((parent->comp_id&TYPE_ID_SUFFIX_MASK) != 0)
			EXECUTION_REPORT(REPORT_PROGRESS, parent->comp_id, true, "Before the MPI_barrier for synchronizing all processes of the parent component \"%s\" for registerring its children components including \"%s\" (the corresponding model code annotation is \"%s\")", parent->get_comp_name(), comp_name, annotation);
		else if (parent->get_current_proc_local_id() == 0) 
			EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "Before the MPI_barrier for synchronizing all processes of the whole coupled model for registerring root components including \"%s\" (the corresponding model code annotation is \"%s\")", comp_name, annotation);	
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Barrier(parent_comm) == MPI_SUCCESS);
		if ((parent->comp_id&TYPE_ID_SUFFIX_MASK) != 0)
			EXECUTION_REPORT(REPORT_PROGRESS, parent->comp_id, true, "After the MPI_barrier for synchronizing all processes of the parent component \"%s\" for registerring its children components including \"%s\" (the corresponding model code annotation is \"%s\")", parent->get_comp_name(), comp_name, annotation);
		else if (parent->get_current_proc_local_id() == 0) 
			EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "After the MPI_barrier for synchronizing all processes of the whole coupled model for registerring root components including \"%s\" (the corresponding model code annotation is \"%s\")", comp_name, annotation);	
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(parent_comm, &num_procs) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(parent_comm, &current_proc_local_id_in_parent) == MPI_SUCCESS);
		all_comp_name = new char [NAME_STR_SIZE*num_procs];
		process_comp_id = new int [num_procs];
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather((char*)comp_name, NAME_STR_SIZE, MPI_CHAR, all_comp_name, NAME_STR_SIZE, MPI_CHAR, 0, parent_comm) == MPI_SUCCESS);
		if (current_proc_local_id_in_parent == 0) {
			unique_comp_name.push_back(all_comp_name);
			process_comp_id[0] = 0;
			for (i = 1; i < num_procs; i ++) {
				for (j = 0; j < unique_comp_name.size(); j ++)
					if (words_are_the_same(unique_comp_name[j], all_comp_name+i*NAME_STR_SIZE)) {
						break;
					}
				if (j == unique_comp_name.size())
					unique_comp_name.push_back(all_comp_name+i*NAME_STR_SIZE);
				process_comp_id[i] = j;
			}
		}
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(process_comp_id, num_procs, MPI_INT, 0, parent_comm)  == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_split(parent_comm, process_comp_id[current_proc_local_id_in_parent], 0, &comm_group) == MPI_SUCCESS);
		delete [] all_comp_name;
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
				                 "The processes of component \"%s\" must be a subset of the processes of its parent component \"%s\". Please check the model code related to the annotations \"%s\" and \"%s\"", 
				                 comp_name, parent->get_comp_name(), annotation_start, parent->annotation_start);
		}
		parent->children.push_back(this);
	}

	if (parent != NULL) {
		if (parent->get_parent() == NULL) {
			EXECUTION_REPORT(REPORT_ERROR, -1, getcwd(working_dir,NAME_STR_SIZE) != NULL, "Cannot get the current working directory");
			strcpy(working_dir+strlen(working_dir), "/../");
		}
		else {
			sprintf(working_dir, "%s/%s\0", parent->working_dir, comp_name);
			create_directory(working_dir, get_current_proc_local_id() == 0);
		}
		sprintf(dir, "%s/CCPL_logs", working_dir, comp_name);
		create_directory(dir, get_current_proc_local_id() == 0);
		sprintf(dir, "%s/CCPL_configs", working_dir, comp_name);
		create_directory(dir, get_current_proc_local_id() == 0);
		sprintf(dir, "%s/data", working_dir, comp_name);
		create_directory(dir, get_current_proc_local_id() == 0);
		MPI_Barrier(get_comm_group());
		char file_name[NAME_STR_SIZE*2];
		sprintf(file_name, "%s/CCPL_logs/%s.CCPL.log.%d", working_dir, get_comp_name(), get_current_proc_local_id());
		FILE *fp = fopen(file_name, "w+");
		fclose(fp);
	}
	else {
		EXECUTION_REPORT(REPORT_ERROR,-1, getcwd(working_dir,NAME_STR_SIZE) != NULL, "Cannot get the current working directory");
		strcpy(working_dir+strlen(working_dir), "/../../../all/");
		sprintf(dir, "%s/CCPL_configs", working_dir, comp_name);
		create_directory(dir, get_current_proc_local_id() == 0);
	}
}


void Comp_comm_group_mgt_node::transform_node_into_array()
{
	int num_procs, proc_id, num_children;


	num_procs = local_processes_global_ids.size();
	for (int i = 0; i < num_procs; i ++) {
		proc_id = local_processes_global_ids[i];
		write_data_into_array_buffer(&proc_id, sizeof(int), &temp_array_buffer, buffer_max_size, buffer_content_size);
	}
	write_data_into_array_buffer(&num_procs, sizeof(int), &temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&comm_group, sizeof(MPI_Comm), &temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(comp_name, NAME_STR_SIZE, &temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(full_name, NAME_STR_SIZE, &temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(working_dir, NAME_STR_SIZE, &temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(annotation_start, NAME_STR_SIZE, &temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(annotation_end, NAME_STR_SIZE, &temp_array_buffer, buffer_max_size, buffer_content_size);
}


void Comp_comm_group_mgt_node::reset_working_dir(const char *new_working_dir)
{
	strcpy(working_dir, new_working_dir);
}


void Comp_comm_group_mgt_node::merge_comp_comm_info(const char *annotation)
{
	int i, num_children_at_root, *num_children_for_all, *counts, *displs;
	char *temp_buffer;
	

	EXECUTION_REPORT(REPORT_ERROR,-1, !definition_finalized, "The registration related to component \"%s\" has been finalized before (the corresponding model code annotation is \"%s\"). It cannot be finalized again at the model code with the annotation \"%s\". Please verify.",
		             comp_name, annotation_end, annotation); 
	this->definition_finalized = true;

	strcpy(this->annotation_end, annotation);

	for (int i = 0; i < children.size(); i ++)
		EXECUTION_REPORT(REPORT_ERROR,-1, children[i]->definition_finalized, 
		                 "The registration related to component \"%s\" has not been finalized yet. So the registration related to its parent component \"%s\" cannot be finalized. Please check the model code related to the annotations \"%s\" and \"%s\".",
		                 children[i]->comp_name, comp_name, children[i]->annotation_start, annotation_end);
	for (i = 0, num_children_at_root = 0; i < children.size(); i ++)
		if (children[i]->current_proc_local_id == 0) {
			num_children_at_root ++;
			write_data_into_array_buffer(children[i]->temp_array_buffer, children[i]->buffer_content_size, &temp_array_buffer, buffer_max_size, buffer_content_size);
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
		write_data_into_array_buffer(&num_children_at_root, sizeof(int), &temp_array_buffer, buffer_max_size, buffer_content_size);
		transform_node_into_array();
		delete [] num_children_for_all;
		delete [] counts;
		delete [] displs;
	}
	
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(&buffer_content_size, 1, MPI_INT, 0, comm_group)  == MPI_SUCCESS);
	if (current_proc_local_id != 0) {
		delete [] temp_array_buffer;
		temp_array_buffer = new char [buffer_content_size];
		buffer_max_size = buffer_content_size;
	}
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(temp_array_buffer, buffer_content_size, MPI_CHAR, 0, comm_group)  == MPI_SUCCESS);
	buffer_content_iter = buffer_content_size;
}


void Comp_comm_group_mgt_node::write_node_into_XML(TiXmlElement *parent_element)
{
	int i, num_segments;
	int *segments_start, *segments_end;
	TiXmlElement * current_element;
	char *string;

	
	current_element = new TiXmlElement("Online_Model");
	parent_element->LinkEndChild(current_element);
	current_element->SetAttribute("name", comp_name);
	current_element->SetAttribute("full_name", full_name);
	current_element->SetAttribute("global_id", unified_global_id);

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


void Comp_comm_group_mgt_node::print_global_nodes()
{
	if (parent != NULL)
		printf("information of global node (%x: %d) (%s) at process %d, parent id is %x, number of children is %d, ", comp_id, current_proc_local_id, comp_name, current_proc_global_id, parent->comp_id, children.size());
	else printf("information of global node (%x: %d) (%s) at process %d, does not have parent, number of children is %d, ", comp_id, current_proc_local_id, comp_name, current_proc_global_id, children.size()); 
	printf("processes include: ");
	for (int i = 0; i < local_processes_global_ids.size(); i ++)
		printf("%d  ", local_processes_global_ids[i]);
	printf("\n\n\n");

	for (int i = 0; i < children.size(); i ++)
		children[i]->print_global_nodes();
}


void Comp_comm_group_mgt_node::update_child(const Comp_comm_group_mgt_node *child_old, Comp_comm_group_mgt_node *child_new)
{
	int i;


	for (i = 0; i < children.size(); i ++)
		if (children[i] == child_old) {
			child_new->parent = this;
			children[i] = child_new;
			break;
		}

	EXECUTION_REPORT(REPORT_ERROR, -1, i < children.size(), "software error in Comp_comm_group_mgt_node::update_child");
}


void Comp_comm_group_mgt_node::transfer_data_buffer(Comp_comm_group_mgt_node *new_node)
{
	if (new_node->temp_array_buffer != NULL)
		delete [] new_node->temp_array_buffer;

	new_node->temp_array_buffer = this->temp_array_buffer;
	new_node->buffer_content_iter = this->buffer_content_iter;
	new_node->buffer_content_size = this->buffer_content_size;
	new_node->buffer_max_size = this->buffer_max_size;
	this->temp_array_buffer = NULL;
}


void Comp_comm_group_mgt_node::confirm_coupling_configuration_active(int API_id, const char *annotation)
{
	char API_label[NAME_STR_SIZE]; 

	get_API_hint(comp_id, API_id, API_label);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, !definition_finalized, "component \"%s\" cannot call the C-Coupler interface \"%s\" at the model code with the annotation \"%s\", because the coupling configuration stage has been ended at the model code with the annotation \"%s\"", 
		             comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"confirm_coupling_configuration_active")->get_comp_name(), API_label, annotation, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"confirm_coupling_configuration_active")->get_annotation_end());
}


int Comp_comm_group_mgt_node::get_local_proc_global_id(int local_indx)
{
    if (local_indx < local_processes_global_ids.size())
        return local_processes_global_ids[local_indx];

	EXECUTION_REPORT(REPORT_ERROR, -1, false, "Software error in Comp_comm_group_mgt_node::get_local_proc_global_id");
	
    return -1;
}


Comp_comm_group_mgt_mgr::Comp_comm_group_mgt_mgr(const char *executable_name, const char *experiment_model, const char *case_name, 
	       const char *case_desc, const char *case_mode, const char *comp_namelist,
           const char *current_config_time, const char *original_case_name, const char *original_config_time)
{
	int i, j;

	
	global_node_array.clear();
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
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(MPI_COMM_WORLD, &num_total_global_procs) == MPI_SUCCESS);

	if (words_are_the_same(case_mode, "initial")) {
		strcpy(this->original_case_name, "none");
		strcpy(this->original_config_time, "none");
	}		
}


Comp_comm_group_mgt_mgr::~Comp_comm_group_mgt_mgr()
{
	for (int i = 0; i < global_node_array.size(); i ++)
		delete global_node_array[i];

	delete [] sorted_comp_ids;
}


void Comp_comm_group_mgt_mgr::transform_global_node_tree_into_array(Comp_comm_group_mgt_node *current_global_node, Comp_comm_group_mgt_node **all_global_nodes, int &global_node_id)
{
	all_global_nodes[global_node_id++] = current_global_node;
	for (int i = 0; i < current_global_node->get_num_children(); i ++)
		transform_global_node_tree_into_array(current_global_node->get_child(i), all_global_nodes, global_node_id);
}


void Comp_comm_group_mgt_mgr::update_global_nodes(Comp_comm_group_mgt_node **all_global_nodes, int num_global_node)
{
	int i, j, old_global_array_size;
	bool find_new_global_node;


	for (i = 0; i < num_global_node; i ++)
		for (j = i+1; j < num_global_node; j ++)
			EXECUTION_REPORT(REPORT_ERROR, -1, !words_are_the_same(all_global_nodes[i]->get_comp_name(), all_global_nodes[j]->get_comp_name()), 
							 "There are at least two components have the same name (\"%s\"), which is not allowed. Please check the model code (%s and %s).", 
							 all_global_nodes[i]->get_comp_name(), all_global_nodes[i]->get_annotation_start(), all_global_nodes[j]->get_annotation_start());
		
	for (i = 0; i < global_node_array.size(); i ++) {
		for (j = 0; j < num_global_node; j ++)
			if (words_are_the_same(all_global_nodes[j]->get_comp_full_name(), global_node_array[i]->get_comp_full_name()))
				break;
		if (j == num_global_node)
			continue;
		all_global_nodes[j]->reset_comm_group(global_node_array[i]->get_comm_group());
		all_global_nodes[j]->reset_current_proc_local_id(global_node_array[i]->get_current_proc_local_id());
		all_global_nodes[j]->reset_local_node_id(global_node_array[i]->get_local_node_id());
		all_global_nodes[j]->reset_working_dir(global_node_array[i]->get_working_dir());
		delete global_node_array[i];
		global_node_array[i] = all_global_nodes[j];
	}
	
	old_global_array_size = global_node_array.size();
	for (i = 0; i < num_global_node; i ++) {
		for (j = 0; j < old_global_array_size; j ++)
			if (all_global_nodes[i] == global_node_array[j])
				break;
			if (j == old_global_array_size) {
				all_global_nodes[i]->reset_local_node_id(global_node_array.size()|TYPE_COMP_LOCAL_ID_PREFIX);
				global_node_array.push_back(all_global_nodes[i]);
			}
	}
}


bool Comp_comm_group_mgt_mgr::is_legal_local_comp_id(int local_comp_id)
{
	if ((local_comp_id&TYPE_ID_PREFIX_MASK) != TYPE_COMP_LOCAL_ID_PREFIX)
		return false;

	int true_parent_id = (local_comp_id & TYPE_ID_SUFFIX_MASK);
	return true_parent_id >= 0 && true_parent_id < global_node_array.size();
}


bool Comp_comm_group_mgt_mgr::is_local_comp_definition_finalized(int local_comp_id)
{
	EXECUTION_REPORT(REPORT_ERROR,-1, is_legal_local_comp_id(local_comp_id), "software error in Comp_comm_group_mgt_mgr::is_local_comp_definition_finalized");

	return global_node_array[(local_comp_id&TYPE_ID_SUFFIX_MASK)]->is_definition_finalized();
}


int Comp_comm_group_mgt_mgr::register_component(const char *comp_name, MPI_Comm &comm, int parent_local_id, const char *annotation)
{
	int i, true_parent_id = 0;
	Comp_comm_group_mgt_node *root_local_node, *new_comp;
	MPI_Comm global_comm = MPI_COMM_WORLD;
	
	
	if (definition_finalized)
		EXECUTION_REPORT(REPORT_ERROR, -1, !definition_finalized, "Cannot register component \"%s\" at the model code with the annotation \"%s\" because the stage of registering coupling configurations of the whole coupled model has been ended at the model code with the annotation \"%s\"", comp_name, annotation, global_node_array[0]->get_annotation_end());
	
	for (i = 0; i < global_node_array.size(); i ++)
		if (words_are_the_same(global_node_array[i]->get_comp_name(), comp_name))
			break;
		
	if (i < global_node_array.size())
		EXECUTION_REPORT(REPORT_ERROR, -1, i == global_node_array.size(),  
						 "A component with the name \"%s\" has already been registered at the model code with the annotation \"%s\". The same name cannot be used for registerring another component at the model code with the annotation \"%s\"", comp_name, global_node_array[i]->get_annotation_start(), annotation);

	if (parent_local_id == -1) {
		root_local_node = new Comp_comm_group_mgt_node("ROOT", global_node_array.size()|TYPE_COMP_LOCAL_ID_PREFIX, NULL, global_comm, annotation);
		global_node_array.push_back(root_local_node);
		global_node_root = root_local_node;
		new_comp = new Comp_comm_group_mgt_node(comp_name, global_node_array.size()|TYPE_COMP_LOCAL_ID_PREFIX, root_local_node, comm, annotation);
		global_node_array.push_back(new_comp);
	}
	else {
		true_parent_id = (parent_local_id & TYPE_ID_SUFFIX_MASK);
		EXECUTION_REPORT(REPORT_ERROR, -1, !global_node_array[true_parent_id]->is_definition_finalized(), 
			             "Cannot register component \"%s\" at the model code with the annotation \"%s\" because the registration corresponding to the parent \"%s\" has been ended at the model code with the annotation \"%s\"", 
			             comp_name, annotation, global_node_array[true_parent_id]->get_comp_name(), global_node_array[true_parent_id]->get_annotation_end()); // add debug information
		new_comp = new Comp_comm_group_mgt_node(comp_name, global_node_array.size()|TYPE_COMP_LOCAL_ID_PREFIX, global_node_array[true_parent_id], comm, annotation);
		global_node_array.push_back(new_comp);
	}

	return new_comp->get_local_node_id();
}


void Comp_comm_group_mgt_mgr::merge_comp_comm_info(int comp_local_id, const char *annotation)
{
	Comp_comm_group_mgt_node *global_node;
	int true_local_id = (comp_local_id & TYPE_ID_SUFFIX_MASK), global_node_id;


	global_node = global_node_array[true_local_id];
	global_node->merge_comp_comm_info(annotation);

	global_node_id = 0;
	Comp_comm_group_mgt_node *new_root = new Comp_comm_group_mgt_node(global_node, NULL, global_node_id);
	EXECUTION_REPORT(REPORT_ERROR, -1, global_node->get_buffer_content_iter() == 0, "Software error1 in Comp_comm_group_mgt_mgr::merge_comp_comm_info");
	Comp_comm_group_mgt_node **all_global_nodes = new Comp_comm_group_mgt_node *[global_node_id];
	global_node_id = 0;
	transform_global_node_tree_into_array(new_root, all_global_nodes, global_node_id);
	if (global_node->get_parent() != NULL)
		global_node->get_parent()->update_child(global_node, new_root);
	global_node->transfer_data_buffer(new_root);
	update_global_nodes(all_global_nodes, global_node_id);
	for (int i = 0; i < global_node_id; i ++)
		if (all_global_nodes[i]->get_local_node_id() == -1) {
			all_global_nodes[i]->reset_local_node_id(global_node_array.size()|TYPE_COMP_LOCAL_ID_PREFIX);
			global_node_array.push_back(all_global_nodes[i]);
		}
	delete [] all_global_nodes;

	global_node = global_node_array[true_local_id];
	MPI_Barrier(global_node->get_comm_group());

	if (true_local_id == 0) {
		EXECUTION_REPORT(REPORT_ERROR, -1, global_node_array.size() == global_node_id, "software error in Comp_comm_group_mgt_mgr::merge_comp_comm_info");
		global_node_root = global_node_array[0];
		definition_finalized = true;
		generate_sorted_comp_ids();
		write_comp_comm_info_into_XML();
		read_comp_comm_info_from_XML();
	}
	if (global_node->get_current_proc_local_id() == 0) {
		printf("dump comps at root comp %s\n", global_node->get_comp_name());
		global_node->print_global_nodes();	
	}

	if (true_local_id == 1)
		merge_comp_comm_info(global_node_array[0]->get_local_node_id(), annotation);
}


void Comp_comm_group_mgt_mgr::generate_sorted_comp_ids()
{
	int i, j, k;
	
	sorted_comp_ids = new int [global_node_array.size()];
	sorted_comp_ids[0] = global_node_array.size();
	for (i = 1; i < global_node_array.size(); i ++)
		sorted_comp_ids[i] = -1;
	for (i = 1; i < global_node_array.size(); i ++) {
		for (k = 0, j = 1; j < global_node_array.size(); j ++) {
			if (i == j)
				continue;
			EXECUTION_REPORT(REPORT_ERROR, -1, !words_are_the_same(global_node_array[i]->get_full_name(), global_node_array[j]->get_full_name()), "in Comp_comm_group_mgt_mgr::generate_sorted_comp_ids");
			if (strcmp(global_node_array[i]->get_full_name(), global_node_array[j]->get_full_name()) > 0)
				k ++;
		}
		EXECUTION_REPORT(REPORT_ERROR, -1, sorted_comp_ids[k+1] == -1, "in Comp_comm_group_mgt_mgr::generate_sorted_comp_ids");		
		sorted_comp_ids[k+1] = global_node_array[i]->get_comp_id();
	}
	for (i = 1; i < global_node_array.size(); i ++) {
		printf("sorted comp %d: \"%s\"\n", i-1, get_global_node_of_local_comp(sorted_comp_ids[i], "")->get_full_name());
		get_global_node_of_local_comp(sorted_comp_ids[i], "")->set_unified_global_id(i);
	}
}


Comp_comm_group_mgt_node *Comp_comm_group_mgt_mgr::get_global_node_of_local_comp(int local_comp_id, const char *annotation)
{	
	EXECUTION_REPORT(REPORT_ERROR,-1, is_legal_local_comp_id(local_comp_id), "The id of component is wrong when getting the management node of a component. Please check the model code with the annotation \"%s\"", annotation); 

	return global_node_array[(local_comp_id&TYPE_ID_SUFFIX_MASK)];
}


MPI_Comm Comp_comm_group_mgt_mgr::get_comm_group_of_local_comp(int local_comp_id, const char *annotation)
{
	get_global_node_of_local_comp(local_comp_id, annotation)->get_comm_group();
}


void Comp_comm_group_mgt_mgr::get_output_data_file_header(int comp_id, char *data_file_header)
{
	int true_comp_id;


	EXECUTION_REPORT(REPORT_ERROR, -1, is_legal_local_comp_id(comp_id), "software error in Comp_comm_group_mgt_mgr::get_data_file_header");
	true_comp_id = (comp_id & TYPE_ID_SUFFIX_MASK);
	printf("before get log_file_name\n");
	fflush(NULL);	
	sprintf(data_file_header, "%s/data/%s", global_node_array[true_comp_id]->get_working_dir(), global_node_array[true_comp_id]->get_comp_name());
	printf("data_file_header is %s\n", data_file_header);
	fflush(NULL);
}


void Comp_comm_group_mgt_mgr::get_log_file_name(int comp_id, char *log_file_name)
{
	int true_comp_id;


	EXECUTION_REPORT(REPORT_ERROR, -1, is_legal_local_comp_id(comp_id), "software error in Comp_comm_group_mgt_mgr::get_log_file_name");
	true_comp_id = (comp_id & TYPE_ID_SUFFIX_MASK);
	printf("before get log_file_name\n");
	fflush(NULL);	
	sprintf(log_file_name, "%s/CCPL_logs/%s.CCPL.log.%d", global_node_array[true_comp_id]->get_working_dir(), global_node_array[true_comp_id]->get_comp_name(), global_node_array[true_comp_id]->get_current_proc_local_id());
	printf("log file name size is %d %d %d\n", strlen(log_file_name), strlen(global_node_array[true_comp_id]->get_working_dir()), strlen(global_node_array[true_comp_id]->get_comp_name()));
	fflush(NULL);
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

	sprintf(XML_file_name, "%s/CCPL_configs/components.xml", global_node_array[0]->get_working_dir());
	
	XML_file->SaveFile(XML_file_name);
}


void Comp_comm_group_mgt_mgr::read_global_node_from_XML(const TiXmlElement *current_element)
{
	Comp_comm_group_mgt_node *global_node;
	const TiXmlNode *child;


	global_node = search_global_node(current_element->Attribute("full_name"));
	EXECUTION_REPORT(REPORT_ERROR, -1, global_node != NULL, "The XML file of component model hierarchy has been illegally modified by others or C-Coupler has software bugs");
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(global_node->get_comp_name(), current_element->Attribute("name")), "The XML file of component model hierarchy has been illegally modified by others or there are software errors");

	for (child = current_element->FirstChild(); child != NULL; child = child->NextSibling())
		read_global_node_from_XML(child->ToElement());
}


void Comp_comm_group_mgt_mgr::read_comp_comm_info_from_XML()
{
	char XML_file_name[NAME_STR_SIZE];


	if (current_proc_global_id != 0)
		return;

	sprintf(XML_file_name, "%s/CCPL_configs/components.xml", global_node_array[0]->get_working_dir());
	TiXmlDocument XML_file(XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, -1, XML_file.LoadFile(), "cannot open the components xml file: %s", XML_file_name);
	
	TiXmlElement *XML_element = XML_file.FirstChildElement();
	TiXmlElement *Online_Model = XML_element->FirstChildElement();
	read_global_node_from_XML(Online_Model);
}


Comp_comm_group_mgt_node *Comp_comm_group_mgt_mgr::search_global_node(const char *full_name)
{
	for (int i = 0; i < global_node_array.size(); i ++)
		if (words_are_the_same(global_node_array[i]->get_full_name(), full_name))
			return global_node_array[i];
		
	return NULL;	
}


Comp_comm_group_mgt_node *Comp_comm_group_mgt_mgr::search_global_node(int global_node_id)
{
	for (int i = 0; i < global_node_array.size(); i ++)
		if (global_node_array[i]->get_local_node_id() == global_node_id)
			return global_node_array[i];
		
	return NULL;
}


int Comp_comm_group_mgt_mgr::get_current_proc_id_in_comp(int comp_id, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_legal_local_comp_id(comp_id), "The component id specified for getting the id of the current process is wrong. Please check the model code with the annotation %s.", annotation); 
	return search_global_node(comp_id)->get_current_proc_local_id();
}


int Comp_comm_group_mgt_mgr::get_num_proc_in_comp(int comp_id, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_legal_local_comp_id(comp_id), "The component id specified for getting the the number of processes is wrong. Please check the model code with the annotation %s.", annotation); 
	EXECUTION_REPORT(REPORT_ERROR, -1, search_global_node(comp_id)->get_current_proc_local_id() != -1, "The component id specified for getting the the number of processes is wrong. Please check the model code with the annotation %s.", annotation);
	return search_global_node(comp_id)->get_num_procs();
}


void Comp_comm_group_mgt_mgr::confirm_coupling_configuration_active(int comp_id, int API_id, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "software error in Comp_comm_group_mgt_mgr::confirm_coupling_configuration_active");
	get_global_node_of_local_comp(comp_id, "in Comp_comm_group_mgt_mgr::confirm_coupling_configuration_active")->confirm_coupling_configuration_active(API_id, annotation);
}


const int *Comp_comm_group_mgt_mgr::get_all_components_ids()
{
	int *all_components_ids = new int [global_node_array.size()];


	for (int i = 1; i < global_node_array.size(); i ++) {
		all_components_ids[i] = global_node_array[i]->get_comp_id();
	}
	all_components_ids[0] = global_node_array.size();

	return all_components_ids;
}

