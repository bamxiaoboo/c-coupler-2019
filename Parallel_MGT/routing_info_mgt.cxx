/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu and then
  *  modified by Dr. Cheng Zhang and Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn or
  *  Dr. Cheng Zhang via zhangc-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "routing_info_mgt.h"
#include "global_data.h"
#include "cor_global_data.h"
#include "CCPL_api_mgt.h"
#include <stdio.h>
#include <string.h>

Routing_info *Routing_info_mgt::search_or_add_router(const int src_comp_id, const int dst_comp_id, const char *src_decomp_name, const char *dst_decomp_name)
{
    Routing_info *router;

    router = search_router(src_comp_id, dst_comp_id, src_decomp_name, dst_decomp_name);

    if (router != NULL)
        return router;

    router = new Routing_info(src_comp_id, dst_comp_id, src_decomp_name, dst_decomp_name);
    routers.push_back(router);

    return router;
}


Routing_info *Routing_info_mgt::search_or_add_router(const char *local_comp_name, const char *remote_comp_name, const char *local_decomp_name, const char *remote_decomp_name)
{
    Routing_info *router;


    router = search_router(remote_comp_name, local_decomp_name, remote_decomp_name);
    if (router != NULL)
        return router;

    router = new Routing_info(local_comp_name, remote_comp_name, local_decomp_name, remote_decomp_name);
    routers.push_back(router);

    return router;
}


Routing_info_mgt::~Routing_info_mgt()
{
    for (int i = 0; i < routers.size(); i ++)
        delete routers[i];
}


Routing_info *Routing_info_mgt::search_router(const int src_comp_id, const int dst_comp_id, const char *src_decomp_name, const char *dst_decomp_name)
{
    for (int i = 0; i < routers.size(); i ++)
        if (routers[i]->match_router(src_comp_id, dst_comp_id, src_decomp_name, dst_decomp_name))
            return routers[i];

    return NULL;
}


Routing_info *Routing_info_mgt::search_router(const char *remote_comp_name, const char *local_decomp_name, const char *remote_decomp_name)
{
    for (int i = 0; i < routers.size(); i ++) {
        if (routers[i]->match_router(remote_comp_name, local_decomp_name, remote_decomp_name))
            return routers[i];
    }

    return NULL;
}


Routing_info::Routing_info(const int src_comp_id, const int dst_comp_id, const char *src_decomp_name, const char *dst_decomp_name)
{
	src_decomp_info = decomps_info_mgr->search_decomp_info(src_decomp_name, src_comp_id);
	dst_decomp_info = decomps_info_mgr->search_decomp_info(dst_decomp_name, dst_comp_id);
    this->src_comp_id = src_comp_id;
    this->dst_comp_id = dst_comp_id;
    src_comp_node = comp_comm_group_mgt_mgr->search_global_node(src_comp_id);
    dst_comp_node = comp_comm_group_mgt_mgr->search_global_node(dst_comp_id);
	if (dst_decomp_info != NULL)
		dst_comp_node = comp_comm_group_mgt_mgr->search_global_node(dst_decomp_info->get_host_comp_id());
    strcpy(this->src_decomp_name, src_decomp_name);
    strcpy(this->dst_decomp_name, dst_decomp_name);
    src_decomp_size = 0;
    dst_decomp_size = 0;
    is_in_src_comp = false;
    is_in_dst_comp = false;
    current_proc_id_src_comp = src_comp_node->get_current_proc_local_id();
    current_proc_id_dst_comp = dst_comp_node->get_current_proc_local_id();
    if (current_proc_id_src_comp != -1) 
		is_in_src_comp = true;
    if (current_proc_id_dst_comp != -1) 
		is_in_dst_comp = true;

    if (words_are_the_same(src_decomp_name, "NULL")) {
        EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(dst_decomp_name, "NULL"), "for router of scalar variables, the local and remote decompositions must be \"NULL\"\n");
        num_dimensions = 0;
        if (current_proc_id_src_comp != -1) 
			src_decomp_size = 1;
        if (current_proc_id_dst_comp != -1) 
			dst_decomp_size = 1;
    }
    else {
        num_dimensions = 2;
        build_2D_router();
        if (current_proc_id_src_comp != -1) 
			src_decomp_size = src_decomp_info->get_num_local_cells();
        if (current_proc_id_dst_comp != -1) 
			dst_decomp_size = dst_decomp_info->get_num_local_cells();
    }
}


Routing_info::Routing_info(const char *local_comp_name, const char *remote_comp_name, const char *local_decomp_name, const char *remote_decomp_name)
{
	char tmp_remote_decomp_name[NAME_STR_SIZE];
	int remote_root_proc_global_id;
	MPI_Comm global_comm_group;
	MPI_Request send_req, recv_req;
	MPI_Status status;


    strcpy(this->remote_comp_name, remote_comp_name);
    strcpy(this->local_decomp_name, local_decomp_name);
    strcpy(this->remote_decomp_name, remote_decomp_name);
	local_communicator_info = compset_communicators_info_mgr->get_communicator_info_by_name(local_comp_name);
    src_comp_id = local_communicator_info->comp_id;
    dst_comp_id = compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name);
	
    if (words_are_the_same(local_decomp_name, "NULL")) {
        EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(remote_decomp_name, "NULL"), 
                     "for router of scalar variables, the local and remote decompositions must be \"NULL\"\n");
        num_dimensions = 0;
        local_decomp_size = 1;
    }
    else if (src_comp_id != dst_comp_id) {
        num_dimensions = 2;
		if (local_communicator_info->current_proc_local_id == 0) {
			remote_root_proc_global_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(dst_comp_id, 0);
			global_comm_group = compset_communicators_info_mgr->get_global_comm_group();
			MPI_Isend((char*)local_decomp_name, NAME_STR_SIZE, MPI_CHAR, remote_root_proc_global_id, 1000+src_comp_id, global_comm_group, &send_req);
			MPI_Irecv(tmp_remote_decomp_name, NAME_STR_SIZE, MPI_CHAR, remote_root_proc_global_id, 1000+dst_comp_id, global_comm_group, &recv_req);	 
			MPI_Wait(&send_req, &status);
			MPI_Wait(&recv_req, &status);
		}
		MPI_Bcast(tmp_remote_decomp_name, NAME_STR_SIZE, MPI_CHAR, 0, local_communicator_info->comm_group);
		EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(remote_decomp_name, tmp_remote_decomp_name), "the decompositions' names do not match when building router from %s to (%s, %s)\n",
					 local_decomp_name, remote_comp_name, remote_decomp_name);
        build_2D_remote_router(local_decomp_name);
        local_decomp_size = decomps_info_mgr->search_decomp_info(local_decomp_name)->get_num_local_cells();
    }
    else {
        num_dimensions = 2;
        build_2D_self_router(remote_decomp_name, local_decomp_name);
        local_decomp_size = decomps_info_mgr->search_decomp_info(local_decomp_name)->get_num_local_cells();
		remap_decomp_size = decomps_info_mgr->search_decomp_info(remote_decomp_name)->get_num_local_cells();
    }
}


Routing_info::~Routing_info()
{
    for (int i = 0; i < remote_procs_routing_info.size(); i ++) {
        if (remote_procs_routing_info[i]->num_elements_transferred > 0) {
            delete [] remote_procs_routing_info[i]->local_indx_segment_starts;
            delete [] remote_procs_routing_info[i]->local_indx_segment_lengths;
        }
    }
}


bool Routing_info::match_router(const int src_comp_id, const int dst_comp_id, const char *src_decomp_name, const char *dst_decomp_name)
{
    return (this->src_comp_id == src_comp_id && 
            this->dst_comp_id == dst_comp_id &&
            words_are_the_same(this->src_decomp_name, src_decomp_name) &&
            words_are_the_same(this->dst_decomp_name, dst_decomp_name));
}


bool Routing_info::match_router(const char *remote_comp_name, const char *local_decomp_name, const char *remote_decomp_name)
{
    return (words_are_the_same(this->remote_comp_name, remote_comp_name) &&
            words_are_the_same(this->local_decomp_name, local_decomp_name) &&
            words_are_the_same(this->remote_decomp_name, remote_decomp_name));
}


void Routing_info::build_2D_router()
{
    int num_src_procs = src_comp_node->get_num_procs();
    int *num_cells_each_src_proc = new int [num_src_procs];
    int num_dst_procs = dst_comp_node->get_num_procs();
    int * num_cells_each_dst_proc = new int [num_dst_procs];
    int num_local_src_cells, num_local_dst_cells;
    const int * local_dst_cells_global_indx;
    int *cells_indx_each_src_proc = NULL;
    int *cells_indx_each_dst_proc = NULL;
    int src_comp_root_proc_global_id = src_comp_node->get_root_proc_global_id();
    int dst_comp_root_proc_global_id = dst_comp_node->get_root_proc_global_id();


    if (current_proc_id_src_comp != -1) {
		EXECUTION_REPORT(REPORT_ERROR, -1, src_decomp_info != NULL, "Software error in Routing_info::build_2D_router: NULL src decomp info");
        num_local_src_cells = src_decomp_info->get_num_local_cells();
		gather_array_in_one_comp(num_src_procs, current_proc_id_src_comp, (void*)src_decomp_info->get_local_cell_global_indx(), num_local_src_cells, 
			                     sizeof(int), num_cells_each_src_proc, (void**)(&cells_indx_each_src_proc), src_comp_node->get_comm_group());
    }
    if (current_proc_id_dst_comp != -1) {
		EXECUTION_REPORT(REPORT_ERROR, -1, dst_decomp_info != NULL, "Software error in Routing_info::build_2D_router: NULL dst decomp info");
        num_local_dst_cells = dst_decomp_info->get_num_local_cells();
        local_dst_cells_global_indx = dst_decomp_info->get_local_cell_global_indx();
		gather_array_in_one_comp(num_dst_procs, current_proc_id_dst_comp, (void*)dst_decomp_info->get_local_cell_global_indx(), num_local_dst_cells, 
								 sizeof(int), num_cells_each_dst_proc, (void**)(&cells_indx_each_dst_proc), dst_comp_node->get_comm_group());
    }

	int temp_size = num_src_procs*sizeof(int);
	transfer_array_from_one_comp_to_another(current_proc_id_src_comp, src_comp_root_proc_global_id, current_proc_id_dst_comp, dst_comp_root_proc_global_id, dst_comp_node->get_comm_group(), (char**)(&num_cells_each_src_proc), temp_size);
	temp_size = num_dst_procs*sizeof(int);
	transfer_array_from_one_comp_to_another(current_proc_id_dst_comp, dst_comp_root_proc_global_id, current_proc_id_src_comp, src_comp_root_proc_global_id, src_comp_node->get_comm_group(), (char**)(&num_cells_each_dst_proc), temp_size);
	int total_src_cells = 0;
	for (int i = 0; i < num_src_procs; i ++) 
		total_src_cells += num_cells_each_src_proc[i] * sizeof(int);
	int total_dst_cells = 0;
	for (int i = 0; i < num_dst_procs; i ++) 
		total_dst_cells += num_cells_each_dst_proc[i] * sizeof(int);
	transfer_array_from_one_comp_to_another(current_proc_id_src_comp, src_comp_root_proc_global_id, current_proc_id_dst_comp, dst_comp_root_proc_global_id, dst_comp_node->get_comm_group(), (char**)(&cells_indx_each_src_proc), total_src_cells);
	transfer_array_from_one_comp_to_another(current_proc_id_dst_comp, dst_comp_root_proc_global_id, current_proc_id_src_comp, src_comp_root_proc_global_id, src_comp_node->get_comm_group(), (char**)(&cells_indx_each_dst_proc), total_dst_cells);
	
    if (current_proc_id_src_comp != -1) {
        int tmp_displs = 0;
        if (num_local_src_cells > 0)
            for (int i = 0; i < num_dst_procs; i ++) {
                compute_routing_info_between_decomps(num_local_src_cells, src_decomp_info->get_local_cell_global_indx(), num_cells_each_dst_proc[i], cells_indx_each_dst_proc+tmp_displs, 
                    src_decomp_info->get_num_global_cells(), comp_comm_group_mgt_mgr->get_current_proc_global_id(), dst_comp_node->get_local_proc_global_id(i));        
                remote_procs_routing_info[remote_procs_routing_info.size()-1]->send_or_recv = true;
                tmp_displs += num_cells_each_dst_proc[i];
            }
    }

    if (current_proc_id_dst_comp != -1) {
        int tmp_displs = 0;
        if (num_local_dst_cells > 0)
            for (int i = 0; i < num_src_procs; i ++) {
                compute_routing_info_between_decomps(num_local_dst_cells, local_dst_cells_global_indx, num_cells_each_src_proc[i], cells_indx_each_src_proc+tmp_displs, 
                    dst_decomp_info->get_num_global_cells(), comp_comm_group_mgt_mgr->get_current_proc_global_id(), src_comp_node->get_local_proc_global_id(i));        
                remote_procs_routing_info[remote_procs_routing_info.size()-1]->send_or_recv = false;
                tmp_displs += num_cells_each_src_proc[i];
            }
    }
    
    if (cells_indx_each_src_proc != NULL) 
		delete [] cells_indx_each_src_proc;
    if (cells_indx_each_dst_proc != NULL) 
		delete [] cells_indx_each_dst_proc;
    delete [] num_cells_each_src_proc; 
    delete [] num_cells_each_dst_proc;
}


void Routing_info::build_2D_remote_router(const char *decomp_name)
{
    Decomp_info *decomp_info;
    Routing_info_with_one_process *routing_info;
    int i, j;
    int num_global_cells;
    int num_local_cells;
    int *local_cell_global_indx;
    int num_remote_procs;
    int remote_proc_global_id;
    MPI_Comm global_comm_group;
    int *num_cells_each_remote_proc;
    int **cell_indx_each_remote_proc; 
    MPI_Request *send_reqs, *recv_reqs;
    MPI_Status status;
    int local_proc_global_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(src_comp_id, local_communicator_info->current_proc_local_id);


    decomp_info = decomps_info_mgr->search_decomp_info(decomp_name);
    num_local_cells = decomp_info->get_num_local_cells();
    num_global_cells = decomp_info->get_num_global_cells();
	if (num_local_cells > 0)
		local_cell_global_indx = new int [num_local_cells];
	for (i = 0; i < num_local_cells; i ++)
		local_cell_global_indx[i] = decomp_info->get_local_cell_global_indx()[i];
    num_remote_procs = compset_communicators_info_mgr->get_num_procs_in_comp(dst_comp_id);
    global_comm_group = compset_communicators_info_mgr->get_global_comm_group();
    num_cells_each_remote_proc = new int [num_remote_procs];
    cell_indx_each_remote_proc = new int *[num_remote_procs];
    send_reqs = new MPI_Request [num_remote_procs];
    recv_reqs = new MPI_Request [num_remote_procs];

	MPI_Barrier(local_communicator_info->comm_group);
    
    for (i = 0; i < num_remote_procs; i ++) {
        remote_proc_global_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(dst_comp_id, i);
		if (src_comp_id < dst_comp_id) {
	        MPI_Send(&num_local_cells, 1, MPI_INT, remote_proc_global_id, src_comp_id, global_comm_group);
    	    MPI_Recv(&num_cells_each_remote_proc[i], 1, MPI_INT, remote_proc_global_id, dst_comp_id, global_comm_group, &status);    
		}
		else {
    	    MPI_Recv(&num_cells_each_remote_proc[i], 1, MPI_INT, remote_proc_global_id, dst_comp_id, global_comm_group, &status);    
	        MPI_Send(&num_local_cells, 1, MPI_INT, remote_proc_global_id, src_comp_id, global_comm_group);
		}
    }
    for (i = 0; i < num_remote_procs; i ++) {
        remote_proc_global_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(dst_comp_id, i);
        if (num_cells_each_remote_proc[i] > 0)
            cell_indx_each_remote_proc[i] = new int [num_cells_each_remote_proc[i]];
        else cell_indx_each_remote_proc[i] = NULL;
		if (src_comp_id < dst_comp_id) {
	        MPI_Send(local_cell_global_indx, num_local_cells, MPI_INT, remote_proc_global_id, src_comp_id, global_comm_group);
    	    MPI_Recv(cell_indx_each_remote_proc[i], num_cells_each_remote_proc[i], MPI_INT, remote_proc_global_id, dst_comp_id, global_comm_group, &status);    
		}
		else {
    	    MPI_Recv(cell_indx_each_remote_proc[i], num_cells_each_remote_proc[i], MPI_INT, remote_proc_global_id, dst_comp_id, global_comm_group, &status);    
	        MPI_Send(local_cell_global_indx, num_local_cells, MPI_INT, remote_proc_global_id, src_comp_id, global_comm_group);
		}
    }

    if (num_local_cells > 0) {
        for (i = 0; i < num_remote_procs; i ++)
            compute_routing_info_between_decomps(num_local_cells, local_cell_global_indx, num_cells_each_remote_proc[i], cell_indx_each_remote_proc[i], 
                                                                        num_global_cells, local_proc_global_id, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(dst_comp_id, i));        
    }

	if (num_local_cells > 0)
		delete [] local_cell_global_indx;
    for (i = 0; i < num_remote_procs; i ++)
        if (cell_indx_each_remote_proc[i] != NULL)
            delete [] cell_indx_each_remote_proc[i];    
    delete [] num_cells_each_remote_proc;
    delete [] cell_indx_each_remote_proc;
    delete [] send_reqs;
    delete [] recv_reqs;

	MPI_Barrier(local_communicator_info->comm_group);
}


void Routing_info::compute_routing_info_between_decomps(int num_local_cells_local, const int *local_cells_global_indexes_local, 
                                                  int num_local_cells_remote, const int *local_cells_global_indexes_remote, 
                                                  int num_global_cells, int local_proc_id, int remote_proc_id)
{
    Routing_info_with_one_process *routing_info;
    const int *reference_cell_indx;
    int *logical_indx_lookup_table_local, *logical_indx_lookup_table_remote; 
    int num_reference_cells;
    int last_local_logical_indx;
    int j;


    routing_info = new Routing_info_with_one_process;
    routing_info->num_elements_transferred = 0;
    routing_info->num_local_indx_segments = 0;
    routing_info->remote_proc_global_id = remote_proc_id;
    
    /* Determine the reference cell index table according to the table size */
    if (num_local_cells_remote < num_local_cells_local ||
        (num_local_cells_remote == num_local_cells_local && (src_comp_node->get_unified_global_id() < dst_comp_node->get_unified_global_id()))) {
        reference_cell_indx = local_cells_global_indexes_remote;
        num_reference_cells = num_local_cells_remote;  
        //EXECUTION_REPORT(REPORT_LOG,-1, true, "use remote index array in router (%s %s %s)", remote_comp_name, remote_decomp_name, local_decomp_name);
    }
    else {
        reference_cell_indx = local_cells_global_indexes_local;
        num_reference_cells = num_local_cells_local; 
        //EXECUTION_REPORT(REPORT_LOG,-1, true, "use local index array in router (%s %s %s)", remote_comp_name, remote_decomp_name, local_decomp_name);
    }

    logical_indx_lookup_table_remote = new int [num_global_cells];
    logical_indx_lookup_table_local = new int [num_global_cells];
    for (j = 0; j < num_global_cells; j ++) {
        logical_indx_lookup_table_local[j] = -1;
        logical_indx_lookup_table_remote[j] = -1;
    }
    
    for (j = 0; j < num_local_cells_local; j ++)
		if (local_cells_global_indexes_local[j] >= 0)
	    	logical_indx_lookup_table_local[local_cells_global_indexes_local[j]] = j;
    for (j = 0; j < num_local_cells_remote; j ++)
		if (local_cells_global_indexes_remote[j] >= 0)
	        logical_indx_lookup_table_remote[local_cells_global_indexes_remote[j]] = j;

    /* Compute the number of common cells and the number of segments of common cells */
    last_local_logical_indx = -100;
    for (j = 0; j < num_reference_cells; j ++) 
        if (reference_cell_indx[j] >= 0 && logical_indx_lookup_table_local[reference_cell_indx[j]] != -1 && logical_indx_lookup_table_remote[reference_cell_indx[j]] != -1) {
            if (last_local_logical_indx + 1 != logical_indx_lookup_table_local[reference_cell_indx[j]]) 
                routing_info->num_local_indx_segments ++;
            last_local_logical_indx = logical_indx_lookup_table_local[reference_cell_indx[j]];
            routing_info->num_elements_transferred ++;
        }

    //EXECUTION_REPORT(REPORT_LOG,-1, true, "number of segments in router (%s %s %s) is %d", remote_comp_name, remote_decomp_name, local_decomp_name, routing_info->num_local_indx_segments);

    /* Compute the info of segments when there are common cells */
    last_local_logical_indx = -100;
    if (routing_info->num_elements_transferred > 0) {
        routing_info->local_indx_segment_starts = new int [routing_info->num_local_indx_segments];
        routing_info->local_indx_segment_lengths = new int [routing_info->num_local_indx_segments];
        routing_info->num_local_indx_segments = 0;
        for (j = 0; j < num_reference_cells; j ++) 
            if (reference_cell_indx[j] >= 0 && logical_indx_lookup_table_local[reference_cell_indx[j]] != -1 && logical_indx_lookup_table_remote[reference_cell_indx[j]] != -1) {
                if (last_local_logical_indx + 1 != logical_indx_lookup_table_local[reference_cell_indx[j]]) {
                    routing_info->local_indx_segment_starts[routing_info->num_local_indx_segments] = logical_indx_lookup_table_local[reference_cell_indx[j]];
                    routing_info->local_indx_segment_lengths[routing_info->num_local_indx_segments] = 1;
                    routing_info->num_local_indx_segments ++;
                }
                else routing_info->local_indx_segment_lengths[routing_info->num_local_indx_segments - 1] ++;
                last_local_logical_indx = logical_indx_lookup_table_local[reference_cell_indx[j]];
            }
        remote_procs_routing_info.push_back(routing_info);
    }
    else remote_procs_routing_info.push_back(routing_info);

    delete [] logical_indx_lookup_table_remote;
    delete [] logical_indx_lookup_table_local;
}


void Routing_info::build_2D_self_router(const char *decomp_name_remap, const char *decomp_name_local)
{
    Decomp_info *decomp_remap, *decomp_local;
    int *num_local_cells_all_remap, *num_local_cells_all_local;
    int *local_cells_global_index_all_remap, *local_cells_global_index_all_local;
    int *displ_remap, *displ_local;
    int num_local_cells_remap, num_local_cells_local, num_total_cells, num_global_cells;
    int num_comp_procs;
    int i;


	MPI_Barrier(local_communicator_info->comm_group);
    EXECUTION_REPORT(REPORT_LOG,-1, true, "begin building 2D self router");

    decomp_remap = decomps_info_mgr->search_decomp_info(decomp_name_remap);
    decomp_local = decomps_info_mgr->search_decomp_info(decomp_name_local);
    num_global_cells = decomp_remap->get_num_global_cells();

    num_local_cells_remap = decomp_remap->get_num_local_cells();
    num_local_cells_local = decomp_local->get_num_local_cells();
    num_comp_procs = local_communicator_info->num_comp_procs;
    num_local_cells_all_remap = new int [num_comp_procs];
    num_local_cells_all_local = new int [num_comp_procs];
    displ_remap = new int [num_comp_procs];
    displ_local = new int [num_comp_procs];

	EXECUTION_REPORT(REPORT_LOG,-1, true, "in building 2D self router, before allgather");

    MPI_Allgather(&num_local_cells_remap, 1, MPI_INT, num_local_cells_all_remap, 1, MPI_INT, local_communicator_info->comm_group);
    MPI_Allgather(&num_local_cells_local, 1, MPI_INT, num_local_cells_all_local, 1, MPI_INT, local_communicator_info->comm_group);

	EXECUTION_REPORT(REPORT_LOG,-1, true, "in building 2D self router, after allgather");

    num_total_cells = 0;
    for (i = 0; i < num_comp_procs; i ++) {
        displ_remap[i] = num_total_cells;
        num_total_cells += num_local_cells_all_remap[i];
    }
    local_cells_global_index_all_remap = new int [num_total_cells];
    num_total_cells = 0;
    for (i = 0; i < num_comp_procs; i ++) {
        displ_local[i] = num_total_cells;
        num_total_cells += num_local_cells_all_local[i];
    }
    local_cells_global_index_all_local = new int [num_total_cells];
    MPI_Allgatherv((int*)decomp_remap->get_local_cell_global_indx(), num_local_cells_remap, MPI_INT, local_cells_global_index_all_remap, 
                   num_local_cells_all_remap, displ_remap, MPI_INT, local_communicator_info->comm_group);
    MPI_Allgatherv((int*)decomp_local->get_local_cell_global_indx(), num_local_cells_local, MPI_INT, local_cells_global_index_all_local, 
                   num_local_cells_all_local, displ_local, MPI_INT, local_communicator_info->comm_group);

    EXECUTION_REPORT(REPORT_LOG,-1, true, "before computing routing info for self router");

    for (i = 0; i < num_comp_procs; i ++) {
        compute_routing_info_between_decomps(num_local_cells_local, decomp_local->get_local_cell_global_indx(), num_local_cells_all_remap[i], local_cells_global_index_all_remap+displ_remap[i], num_global_cells, local_communicator_info->current_proc_local_id, i);
        remote_procs_routing_info[remote_procs_routing_info.size()-1]->send_or_recv = true;
    }

	EXECUTION_REPORT(REPORT_LOG,-1, true, "finish computing routing info for self router for sending");
	
    for (i = 0; i < num_comp_procs; i ++) {
        compute_routing_info_between_decomps(num_local_cells_remap, decomp_remap->get_local_cell_global_indx(), num_local_cells_all_local[i], local_cells_global_index_all_local+displ_local[i], num_global_cells, local_communicator_info->current_proc_local_id, i);
        remote_procs_routing_info[remote_procs_routing_info.size()-1]->send_or_recv = false;
    }

	MPI_Barrier(local_communicator_info->comm_group);
	EXECUTION_REPORT(REPORT_LOG,-1, true, "finish computing routing info for self router for receiving");
}


int Routing_info::get_true_routing_info_index(bool is_send, int i)
{
    if (is_in_src_comp && is_in_dst_comp && !is_send)
        return i+remote_procs_routing_info.size()/2;

    return i;
}


int Routing_info::get_num_elements_transferred_with_remote_proc(bool is_send, int i) 
{
    if (num_dimensions == 2) {
        if (remote_procs_routing_info.size() <= get_true_routing_info_index(is_send, i))
            return 0;

        return remote_procs_routing_info[get_true_routing_info_index(is_send, i)]->num_elements_transferred; 
    }
    else {
        if (is_send) {
            if (local_communicator_info->current_proc_local_id == 0)
                return 1;
            else return 0;
        }
        else {
            if (get_true_routing_info_index(is_send, i) == 0)
                return 1;
            else return 0;
        }
    }
}

