/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "routing_info_mgt.h"
#include "global_data.h"
#include "cor_global_data.h"
#include <stdio.h>
#include <string.h>


Routing_info *Routing_info_mgt::search_or_add_router(const char *remote_comp_name, const char *local_decomp_name, const char *remote_decomp_name)
{
    Routing_info *router;


    router = search_router(remote_comp_name, local_decomp_name, remote_decomp_name);
    if (router != NULL)
        return router;

    router = new Routing_info(remote_comp_name, local_decomp_name, remote_decomp_name);
    routers.push_back(router);

    return router;
}


Routing_info_mgt::~Routing_info_mgt()
{
    for (int i = 0; i < routers.size(); i ++)
        delete routers[i];
}


Routing_info *Routing_info_mgt::search_router(const char *remote_comp_name, const char *local_decomp_name, const char *remote_decomp_name)
{
    for (int i = 0; i < routers.size(); i ++) {
        if (routers[i]->match_router(remote_comp_name, local_decomp_name, remote_decomp_name))
            return routers[i];
    }

    return NULL;
}


Routing_info::Routing_info(const char *remote_comp_name, const char *local_decomp_name, const char *remote_decomp_name)
{
	char tmp_remote_decomp_name[NAME_STR_SIZE];
	int remote_root_proc_global_id;
	MPI_Comm global_comm_group;
	MPI_Request send_req, recv_req;
	MPI_Status status;


    strcpy(this->remote_comp_name, remote_comp_name);
    strcpy(this->local_decomp_name, local_decomp_name);
    strcpy(this->remote_decomp_name, remote_decomp_name);
    local_comm_id = compset_communicators_info_mgr->get_current_comp_id();
    remote_comm_id = compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name);
	
    if (words_are_the_same(local_decomp_name, "NULL")) {
        EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(remote_decomp_name, "NULL"), 
                     "for router of scalar variables, the local and remote decompositions must be \"NULL\"\n");
        num_dimensions = 0;
        local_decomp_size = 1;
    }
    else if (local_comm_id != remote_comm_id) {
        num_dimensions = 2;
		if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
			remote_root_proc_global_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comm_id, 0);
			global_comm_group = compset_communicators_info_mgr->get_global_comm_group();
			MPI_Isend((char*)local_decomp_name, NAME_STR_SIZE, MPI_CHAR, remote_root_proc_global_id, 1000+local_comm_id, global_comm_group, &send_req);
			MPI_Irecv(tmp_remote_decomp_name, NAME_STR_SIZE, MPI_CHAR, remote_root_proc_global_id, 1000+remote_comm_id, global_comm_group, &recv_req);	 
			MPI_Wait(&send_req, &status);
			MPI_Wait(&recv_req, &status);
		}
		MPI_Bcast(tmp_remote_decomp_name, NAME_STR_SIZE, MPI_CHAR, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
		EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(remote_decomp_name, tmp_remote_decomp_name), "the decompositions' names do not match when building router from %s to (%s, %s)\n",
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


bool Routing_info::match_router(const char *remote_comp_name, const char *local_decomp_name, const char *remote_decomp_name)
{
    return (words_are_the_same(this->remote_comp_name, remote_comp_name) &&
            words_are_the_same(this->local_decomp_name, local_decomp_name) &&
            words_are_the_same(this->remote_decomp_name, remote_decomp_name));
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
    int local_proc_global_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(local_comm_id, compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group());


    decomp_info = decomps_info_mgr->search_decomp_info(decomp_name);
    num_local_cells = decomp_info->get_num_local_cells();
    num_global_cells = decomp_info->get_num_global_cells();
	if (num_local_cells > 0)
		local_cell_global_indx = new int [num_local_cells];
	for (i = 0; i < num_local_cells; i ++)
		local_cell_global_indx[i] = decomp_info->get_local_cell_global_indx()[i];
    num_remote_procs = compset_communicators_info_mgr->get_num_procs_in_comp(remote_comm_id);
    global_comm_group = compset_communicators_info_mgr->get_global_comm_group();
    num_cells_each_remote_proc = new int [num_remote_procs];
    cell_indx_each_remote_proc = new int *[num_remote_procs];
    send_reqs = new MPI_Request [num_remote_procs];
    recv_reqs = new MPI_Request [num_remote_procs];

	MPI_Barrier(compset_communicators_info_mgr->get_current_comp_comm_group());
    
    for (i = 0; i < num_remote_procs; i ++) {
        remote_proc_global_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comm_id, i);
		if (local_comm_id < remote_comm_id) {
	        MPI_Send(&num_local_cells, 1, MPI_INT, remote_proc_global_id, local_comm_id, global_comm_group);
    	    MPI_Recv(&num_cells_each_remote_proc[i], 1, MPI_INT, remote_proc_global_id, remote_comm_id, global_comm_group, &status);    
		}
		else {
    	    MPI_Recv(&num_cells_each_remote_proc[i], 1, MPI_INT, remote_proc_global_id, remote_comm_id, global_comm_group, &status);    
	        MPI_Send(&num_local_cells, 1, MPI_INT, remote_proc_global_id, local_comm_id, global_comm_group);
		}
    }
    for (i = 0; i < num_remote_procs; i ++) {
        remote_proc_global_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comm_id, i);
        if (num_cells_each_remote_proc[i] > 0)
            cell_indx_each_remote_proc[i] = new int [num_cells_each_remote_proc[i]];
        else cell_indx_each_remote_proc[i] = NULL;
		if (local_comm_id < remote_comm_id) {
	        MPI_Send(local_cell_global_indx, num_local_cells, MPI_INT, remote_proc_global_id, local_comm_id, global_comm_group);
    	    MPI_Recv(cell_indx_each_remote_proc[i], num_cells_each_remote_proc[i], MPI_INT, remote_proc_global_id, remote_comm_id, global_comm_group, &status);    
		}
		else {
    	    MPI_Recv(cell_indx_each_remote_proc[i], num_cells_each_remote_proc[i], MPI_INT, remote_proc_global_id, remote_comm_id, global_comm_group, &status);    
	        MPI_Send(local_cell_global_indx, num_local_cells, MPI_INT, remote_proc_global_id, local_comm_id, global_comm_group);
		}
    }    

    if (num_local_cells > 0) {
        for (i = 0; i < num_remote_procs; i ++)
            compute_routing_info_between_decomps(num_local_cells, local_cell_global_indx, num_cells_each_remote_proc[i], cell_indx_each_remote_proc[i], 
                                                                        num_global_cells, local_proc_global_id, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comm_id, i));        
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

	MPI_Barrier(compset_communicators_info_mgr->get_current_comp_comm_group());
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
        (num_local_cells_remote == num_local_cells_local && (local_comm_id < remote_comm_id))) {
        reference_cell_indx = local_cells_global_indexes_remote;
        num_reference_cells = num_local_cells_remote;  
        EXECUTION_REPORT(REPORT_LOG, true, "use remote index array in router (%s %s %s)", remote_comp_name, remote_decomp_name, local_decomp_name);
    }
    else {
        reference_cell_indx = local_cells_global_indexes_local;
        num_reference_cells = num_local_cells_local; 
        EXECUTION_REPORT(REPORT_LOG, true, "use local index array in router (%s %s %s)", remote_comp_name, remote_decomp_name, local_decomp_name);
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

    EXECUTION_REPORT(REPORT_LOG, true, "number of segments in router (%s %s %s) is %d", remote_comp_name, remote_decomp_name, local_decomp_name, routing_info->num_local_indx_segments);

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


	MPI_Barrier(compset_communicators_info_mgr->get_current_comp_comm_group());
    EXECUTION_REPORT(REPORT_LOG, true, "begin building 2D self router");

    decomp_remap = decomps_info_mgr->search_decomp_info(decomp_name_remap);
    decomp_local = decomps_info_mgr->search_decomp_info(decomp_name_local);
    num_global_cells = decomp_remap->get_num_global_cells();

    num_local_cells_remap = decomp_remap->get_num_local_cells();
    num_local_cells_local = decomp_local->get_num_local_cells();
    num_comp_procs = compset_communicators_info_mgr->get_num_procs_in_comp(compset_communicators_info_mgr->get_current_comp_id());
    num_local_cells_all_remap = new int [num_comp_procs];
    num_local_cells_all_local = new int [num_comp_procs];
    displ_remap = new int [num_comp_procs];
    displ_local = new int [num_comp_procs];

	EXECUTION_REPORT(REPORT_LOG, true, "in building 2D self router, before allgather");

    MPI_Allgather(&num_local_cells_remap, 1, MPI_INT, num_local_cells_all_remap, 1, MPI_INT, compset_communicators_info_mgr->get_current_comp_comm_group());
    MPI_Allgather(&num_local_cells_local, 1, MPI_INT, num_local_cells_all_local, 1, MPI_INT, compset_communicators_info_mgr->get_current_comp_comm_group());

	EXECUTION_REPORT(REPORT_LOG, true, "in building 2D self router, after allgather");

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
                   num_local_cells_all_remap, displ_remap, MPI_INT, compset_communicators_info_mgr->get_current_comp_comm_group());
    MPI_Allgatherv((int*)decomp_local->get_local_cell_global_indx(), num_local_cells_local, MPI_INT, local_cells_global_index_all_local, 
                   num_local_cells_all_local, displ_local, MPI_INT, compset_communicators_info_mgr->get_current_comp_comm_group());

    EXECUTION_REPORT(REPORT_LOG, true, "before computing routing info for self router");

    for (i = 0; i < num_comp_procs; i ++) {
        compute_routing_info_between_decomps(num_local_cells_local, decomp_local->get_local_cell_global_indx(), num_local_cells_all_remap[i], local_cells_global_index_all_remap+displ_remap[i], num_global_cells, compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group(), i);
        remote_procs_routing_info[remote_procs_routing_info.size()-1]->send_or_recv = true;
    }

	EXECUTION_REPORT(REPORT_LOG, true, "finish computing routing info for self router for sending");
	
    for (i = 0; i < num_comp_procs; i ++) {
        compute_routing_info_between_decomps(num_local_cells_remap, decomp_remap->get_local_cell_global_indx(), num_local_cells_all_local[i], local_cells_global_index_all_local+displ_local[i], num_global_cells, compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group(), i);
        remote_procs_routing_info[remote_procs_routing_info.size()-1]->send_or_recv = false;
    }

	MPI_Barrier(compset_communicators_info_mgr->get_current_comp_comm_group());
	EXECUTION_REPORT(REPORT_LOG, true, "finish computing routing info for self router for receiving");
}


int Routing_info::get_true_routing_info_index(bool is_send, int i)
{
    if (local_comm_id == remote_comm_id && !is_send)
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
            if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
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

