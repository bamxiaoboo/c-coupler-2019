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


void Compset_communicators_info_mgt::register_component(const char *comp_name, const char *comp_type, MPI_Comm comm)
{
	int i, current_proc_global_id;
	Component_communicator_info *comp_comm_info;
	
	
	MPI_Barrier(comm);
	
    for(i = 0; i < comps_comms_info.size(); i ++)
        EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(comps_comms_info[i]->comp_name, comp_name), "Component name \"%s\" has alread been registered before");

	comp_comm_info = new Component_communicator_info;
	strcpy(comp_comm_info->comp_name, comp_name);
	strcpy(comp_comm_info->comp_type, comp_type);
	comp_comm_info->comm_group = comm;
	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_size(comm, &comp_comm_info->num_comp_procs) == MPI_SUCCESS);
	comp_comm_info->comp_procs_global_ids = new int [comp_comm_info->num_comp_procs];
    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(global_comm_group, &current_proc_global_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR, MPI_Allgather(&current_proc_global_id, 1, MPI_INT, comp_comm_info->comp_procs_global_ids, 1, MPI_INT, comm) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(comm, &comp_comm_info->current_proc_local_id) == MPI_SUCCESS);
	comps_comms_info.push_back(comp_comm_info);
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
	int num_procs, proc_id, num_children;

	
	buffer_node->read_data_from_array_buffer(comp_name, NAME_STR_SIZE);
	buffer_node->read_data_from_array_buffer(comp_type, NAME_STR_SIZE);
	buffer_node->read_data_from_array_buffer(&comm_group, sizeof(MPI_Comm));
	buffer_node->read_data_from_array_buffer(&num_procs, sizeof(int));
	for (int i = 0; i < num_procs; i ++) {
		buffer_node->read_data_from_array_buffer(&proc_id, sizeof(int));
		local_processes_global_ids.push_back(proc_id);
	}
	this->local_node_id = -1;
	this->global_node_id = (global_node_id | TYPE_COMP_GLOBAL_ID_PREFIX);
	global_node_id ++;
	this->parent = parent;
	temp_array_buffer = NULL;
	definition_finalized = true;
	
	buffer_node->read_data_from_array_buffer(&num_children, sizeof(int));
	for (int i = 0; i < num_children; i ++)
		children.push_back(new Comp_comm_group_mgt_global_node(buffer_node, this, global_node_id));
	for (int i = 0; i < num_children; i ++)
		for (int j = i+1; j < num_children; j ++)
			EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(children[i]->comp_name, children[j]->comp_name), 
							 "different children components of the same component cannot have the same name. Please check the registration of the component \"%s\"",
							 children[i]->comp_name);
}


Comp_comm_group_mgt_global_node::Comp_comm_group_mgt_global_node(const char *comp_name, const char *comp_type, int local_node_id, Comp_comm_group_mgt_global_node *parent, MPI_Comm &comm)
{
	char *all_comp_name, *all_comp_type;
	std::vector<char*> unique_comp_name, unique_comp_type;
	int i, j, num_procs, current_proc_global_id, *process_comp_id, *processes_global_id;

	
	strcpy(this->comp_name, comp_name);
	strcpy(this->comp_type, comp_type);
	this->local_node_id = local_node_id;
	this->global_node_id = -1;
	this->parent = parent;
	buffer_content_size = 0;
	buffer_max_size = 1024;
	temp_array_buffer = new char [buffer_max_size];
	definition_finalized = false;
	num_global_children = 0;

	if (comm != -1) {
		comm_group = comm;
		MPI_Barrier(comm);
	}
	else {
		MPI_Barrier(parent->get_comm_group());
		EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_size(parent->get_comm_group(), &num_procs) == MPI_SUCCESS);
		all_comp_name = new char [NAME_STR_SIZE*num_procs];
		all_comp_type = new char [NAME_STR_SIZE*num_procs];
		process_comp_id = new int [num_procs];
		EXECUTION_REPORT(REPORT_ERROR, MPI_Gather((char*)comp_name, NAME_STR_SIZE, MPI_CHAR, all_comp_name, NAME_STR_SIZE, MPI_CHAR, 0, parent->get_comm_group()) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR, MPI_Gather((char*)comp_type, NAME_STR_SIZE, MPI_CHAR, all_comp_type, NAME_STR_SIZE, MPI_CHAR, 0, parent->get_comm_group()) == MPI_SUCCESS);
		if (parent->get_current_proc_local_id() == 0) {
			unique_comp_name.push_back(all_comp_name);
			unique_comp_type.push_back(all_comp_type);
			process_comp_id[0] = 0;
			for (i = 1; i < num_procs; i ++) {
				for (j = 0; j < unique_comp_name.size(); j ++)
					if (words_are_the_same(unique_comp_name[j], all_comp_name+i*NAME_STR_SIZE)) {
						EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(unique_comp_type[j], all_comp_type+i*NAME_STR_SIZE), 
							             "The type of the component \"%s\" is different among processes. Please verify", unique_comp_name[j]);
						break;
					}
				if (j == unique_comp_name.size()) {
					unique_comp_name.push_back(all_comp_name+i*NAME_STR_SIZE);
					unique_comp_type.push_back(all_comp_type+i*NAME_STR_SIZE);
				}
				process_comp_id[i] = j;
			}
		}
		EXECUTION_REPORT(REPORT_ERROR, MPI_Bcast(process_comp_id, num_procs, MPI_INT, 0, parent->get_comm_group())  == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_split(parent->get_comm_group(), process_comp_id[parent->get_current_proc_local_id()], 0, &comm_group) == MPI_SUCCESS);
		delete [] all_comp_name;
		delete [] all_comp_type;
		delete [] process_comp_id;
	}

	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(comm_group, &current_proc_local_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_size(comm_group, &num_procs) == MPI_SUCCESS);
	processes_global_id = new int [num_procs];
	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(MPI_COMM_WORLD, &current_proc_global_id) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR, MPI_Allgather(&current_proc_global_id, 1, MPI_INT, processes_global_id, 1, MPI_INT, comm_group) == MPI_SUCCESS);
	for (i = 0; i < num_procs; i ++)
		local_processes_global_ids.push_back(processes_global_id[i]);
	delete [] processes_global_id;
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
	EXECUTION_REPORT(REPORT_ERROR, data_size <= buffer_content_size, "Software error in Comp_comm_group_mgt_global_node::read_data_into_array_buffer");
	
	for (int i = 0; i < data_size; i ++)
		((char*) data)[i] = temp_array_buffer[buffer_content_size-data_size+i];
	
	buffer_content_size -= data_size;
}


void Comp_comm_group_mgt_global_node::merge_comp_comm_info(bool is_root_node)
{
	int i, num_children_at_root, *num_children_for_all, *counts, *displs;
	char *temp_buffer;
	

	EXECUTION_REPORT(REPORT_ERROR, !definition_finalized, "The registration related to component \"%s\" has been finalized before. It cannot be finalized again. Please verify.");
	definition_finalized = true;
	for (int i = 0; i < children.size(); i ++)
		EXECUTION_REPORT(REPORT_ERROR, children[i]->definition_finalized, 
		                 "The registration related to component \"%s\" has not been finalized yet. So the registration related to its parent component \"%s\" cannot be finalized. Please verify.");

	MPI_Barrier(comm_group);

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

	EXECUTION_REPORT(REPORT_ERROR, MPI_Gather(&num_children_at_root, 1, MPI_INT, num_children_for_all, 1, MPI_INT, 0, comm_group) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_ERROR, MPI_Gather(&buffer_content_size, 1, MPI_INT, counts, 1, MPI_INT, 0, comm_group) == MPI_SUCCESS);

	if (current_proc_local_id == 0) {
		displs[0] = 0;
		for (i = 1; i < local_processes_global_ids.size(); i ++) {
			displs[i] = displs[i-1]+counts[i-1];
			num_children_at_root += num_children_for_all[i];
		}
		temp_buffer = new char [displs[i-1]+counts[i-1]];
	}

    EXECUTION_REPORT(REPORT_ERROR, MPI_Gatherv(temp_array_buffer, buffer_content_size, MPI_CHAR, temp_buffer, counts, displs, MPI_CHAR, 0, comm_group) == MPI_SUCCESS);
	
	if (current_proc_local_id == 0) {		
		delete [] num_children_for_all;
		delete [] counts;
		delete [] displs;
		delete [] temp_array_buffer;
		temp_array_buffer = temp_buffer;
		buffer_content_size = displs[i-1]+counts[i-1];
		buffer_max_size = buffer_content_size;
		write_data_into_array_buffer(&num_children_at_root, sizeof(int));
		transform_node_into_array();
	}

	MPI_Barrier(comm_group);

	if (is_root_node) {
		EXECUTION_REPORT(REPORT_ERROR, MPI_Bcast(&buffer_content_size, 1, MPI_INT, 0, comm_group)  == MPI_SUCCESS);
		if (current_proc_local_id != 0) {
			delete [] temp_array_buffer;
			temp_array_buffer = new char [buffer_content_size];
			buffer_max_size = buffer_content_size;
		}
		EXECUTION_REPORT(REPORT_ERROR, MPI_Bcast(temp_array_buffer, buffer_max_size, MPI_CHAR, 0, comm_group)  == MPI_SUCCESS);
	}
}


Comp_comm_group_mgt_local_node::Comp_comm_group_mgt_local_node(const char *comp_name, const char *comp_type, Comp_comm_group_mgt_local_node *parent, MPI_Comm &comm, int local_id)
{
	self_local_node_id = local_id;
	if (parent != NULL) {
		parent_local_node_id = parent->get_local_node_id();
		global_node = new Comp_comm_group_mgt_global_node(comp_name, comp_type, local_id, parent->get_global_node(), comm);
	}
	else {
		parent_local_node_id = -1; 
		global_node = new Comp_comm_group_mgt_global_node(comp_name, comp_type, local_id, NULL, comm);
	}
	
}


Comp_comm_group_mgt_mgr::Comp_comm_group_mgt_mgr()
{
	MPI_Comm global_comm_group;

	
	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_dup(MPI_COMM_WORLD, &global_comm_group) == MPI_SUCCESS);
	local_nodes.clear();
	Comp_comm_group_mgt_local_node *root_node = new Comp_comm_group_mgt_local_node("ROOT", "ROOT", NULL, global_comm_group, local_nodes.size()|TYPE_COMP_LOCAL_ID_PREFIX);
	local_nodes.push_back(root_node);
	global_node_root = root_node->get_global_node();
	
}


Comp_comm_group_mgt_mgr::~Comp_comm_group_mgt_mgr()
{
	for (int i = 0; i < local_nodes.size(); i ++)
		delete local_nodes[i];

	delete global_node_root;
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
				EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(old_global_node->get_child(i)->get_comp_type(), new_global_node->get_child(j)->get_comp_type()),
								 "Software error1 in Comp_comm_group_mgt_mgr::update_global_nodes");
				EXECUTION_REPORT(REPORT_ERROR, new_global_node->get_child(j)->get_local_node_id() == -1, "Software error2 in Comp_comm_group_mgt_mgr::update_global_nodes");
				new_global_node->get_child(j)->reset_local_node_id(old_global_node->get_child(i)->get_local_node_id());
				local_nodes[old_global_node->get_child(i)->get_local_node_id()]->reset_global_node(new_global_node->get_child(j));
				update_global_nodes(old_global_node->get_child(i), new_global_node->get_child(j));
				break;
			}
		EXECUTION_REPORT(REPORT_ERROR, find_new_global_node, "Software error3 in Comp_comm_group_mgt_mgr::update_global_nodes");
	}
}


bool Comp_comm_group_mgt_mgr::is_legal_local_comp_id(int local_comp_id)
{
	if ((local_comp_id&TYPE_ID_PREFIX_MASK) != TYPE_COMP_LOCAL_ID_PREFIX)
		return false;

	int true_parent_id = (local_comp_id & TYPE_ID_SUFFIX_MASK);
	if (!(true_parent_id >= 0 && true_parent_id < local_nodes.size()))
		return false;

	return true;
}


int Comp_comm_group_mgt_mgr::register_component(const char *comp_name, const char *comp_type, MPI_Comm &comm, int parent_local_id, const char *annotation)
{
	int true_parent_id = (parent_local_id & TYPE_ID_SUFFIX_MASK);
	Comp_comm_group_mgt_local_node *new_comp;

	if (parent_local_id == -1)
		parent_local_id = local_nodes[0]->get_local_node_id();
	
	EXECUTION_REPORT(REPORT_ERROR, is_legal_local_comp_id(parent_local_id), 
		             "For the registration of component (name=\"%s\", type=\"%s\"), the input parameter of the ID of the parent component is wrong",
		             comp_name, comp_type, annotation);

	new_comp = new Comp_comm_group_mgt_local_node(comp_name, comp_type, local_nodes[true_parent_id], comm, local_nodes.size()|TYPE_COMP_LOCAL_ID_PREFIX);

	return new_comp->get_local_node_id();
}


void Comp_comm_group_mgt_mgr::merge_comp_comm_info(int comp_local_id)
{
	Comp_comm_group_mgt_global_node *global_node;
	int true_local_id = (comp_local_id & TYPE_ID_SUFFIX_MASK), global_node_id;


	EXECUTION_REPORT(REPORT_ERROR, is_legal_local_comp_id(comp_local_id), "The comp_local_id is wrong");
	global_node = local_nodes[true_local_id]->get_global_node();
	MPI_Barrier(global_node->get_comm_group());
	global_node->merge_comp_comm_info(true_local_id == 0);

	if (true_local_id == 0) {
		global_node_id = 0;
		Comp_comm_group_mgt_global_node *new_root = new Comp_comm_group_mgt_global_node(global_node, NULL, global_node_id);
		EXECUTION_REPORT(REPORT_ERROR, global_node->get_buffer_content_size() == 0, "Software error1 in Comp_comm_group_mgt_mgr::merge_comp_comm_info");
		update_global_nodes(global_node_root, new_root);
		delete global_node_root;
		global_node_root = new_root;
	}
	MPI_Barrier(global_node->get_comm_group());
}

