/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Cheng Zhang and then
  *  modified by Dr. Cheng Zhang and Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Cheng Zhang via zhangc-cess@tsinghua.edu.cn
  *  or Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/
  


#include "runtime_trans_algorithm.h"
#include "global_data.h"
#include <string.h>


template <class T> void Runtime_trans_algorithm::pack_segment_data(T *mpi_buf, T *field_data_buf, int segment_start, int segment_size, int field_2D_size, int num_lev)
{
    int i, j, offset;

    for (i = segment_start, offset = 0; i < segment_size+segment_start; i ++)
        for (j = 0; j < num_lev; j ++)
            mpi_buf[offset++] = field_data_buf[i+j*field_2D_size];
}


template <class T> void Runtime_trans_algorithm::unpack_segment_data(T *mpi_buf, T *field_data_buf, int segment_start, int segment_size, int field_2D_size, int num_lev)
{
    int i, j, offset;


    for (i = segment_start, offset = 0; i < segment_size+segment_start; i ++)
        for (j = 0; j < num_lev; j ++)
            field_data_buf[i+j*field_2D_size] = mpi_buf[offset++];
}


Runtime_trans_algorithm::Runtime_trans_algorithm(int num_src_fields, int num_dst_fields, Field_mem_info ** fields_mem, Routing_info ** routers, Coupling_timer ** timers, MPI_Comm comm, int * ranks)
{
    if (num_src_fields != 0 && num_dst_fields != 0)
        EXECUTION_REPORT(REPORT_ERROR,-1, num_src_fields == num_dst_fields, "Remapping has different number of src and dst fields: number of src fields is %d, but number of dst fields is %d", num_src_fields, num_dst_fields);

    this->num_src_fields = num_src_fields;
    this->num_dst_fields = num_dst_fields;
    num_transfered_fields = num_src_fields + num_dst_fields;
    EXECUTION_REPORT(REPORT_ERROR,-1, num_transfered_fields > 0, "Runtime_trans_algorithm does not have transfer fields");

    data_buf = NULL;
    tag_buf = NULL; 
    data_buf_size = 0;
    tag_buf_size = 0;
    union_comm = comm;
    MPI_Comm_rank(union_comm, &current_proc_id_union_comm);

    this->fields_mem = new Field_mem_info *[num_transfered_fields];
    fields_data_buffers = new void *[num_transfered_fields];
    fields_routers = new Routing_info *[num_transfered_fields];
    fields_timers = new Coupling_timer *[num_transfered_fields];
	transfer_process_on = new bool [num_transfered_fields];
	current_remote_fields_time = new long [num_transfered_fields];
	last_history_receive_buffer_index = -1;
	last_field_remote_recv_count = -1;
	current_field_local_recv_count = 1;

    for (int i = 0; i < num_transfered_fields; i ++) {
        this->fields_mem[i] = fields_mem[i];
        fields_data_buffers[i] = fields_mem[i]->get_data_buf();
        fields_routers[i] = routers[i];
        fields_timers[i] = timers[i];
    }

    int local_comp_id, remote_comp_id;
    if (num_src_fields > 0) {
        local_comp_id = fields_routers[0]->get_src_comp_id();
        remote_comp_id = fields_routers[0]->get_dst_comp_id();
    }
    else {
        local_comp_id = fields_routers[0]->get_dst_comp_id();
        remote_comp_id = fields_routers[0]->get_src_comp_id();
    }
    comp_id = local_comp_id;
    local_comp_node = comp_comm_group_mgt_mgr->search_global_node(local_comp_id);
    remote_comp_node = comp_comm_group_mgt_mgr->search_global_node(remote_comp_id);
    current_proc_local_id = local_comp_node->get_current_proc_local_id();
    current_proc_global_id = comp_comm_group_mgt_mgr->get_current_proc_global_id();
	time_mgr = components_time_mgrs->get_time_mgr(local_comp_id);
    num_remote_procs = remote_comp_node->get_num_procs();
	num_local_procs = local_comp_node->get_num_procs();
    remote_proc_ranks_in_union_comm = new int[num_remote_procs];
    memcpy(remote_proc_ranks_in_union_comm, ranks, num_remote_procs*sizeof(int));
	
    fields_allocated = false;
    initialize_local_data_structures();
    allocate_src_dst_fields("kernel");
}


void Runtime_trans_algorithm::initialize_local_data_structures()
{
    send_size_with_remote_procs = new int[num_remote_procs];
    recv_size_with_remote_procs = new int[num_remote_procs];
    send_displs_in_remote_procs = new int[num_remote_procs];
    recv_displs_in_current_proc = new int[num_remote_procs];
    field_grids_num_lev = new long[num_transfered_fields];
    fields_data_type_sizes = new int[num_transfered_fields];
	last_receive_field_sender_time = new long [num_transfered_fields];
	current_receive_field_sender_time = new long [num_transfered_fields];
	current_receive_field_usage_time = new long [num_transfered_fields];

    memset(send_size_with_remote_procs, 0, sizeof(int)*num_remote_procs);
    memset(recv_size_with_remote_procs, 0, sizeof(int)*num_remote_procs);
    memset(send_displs_in_remote_procs, 0, sizeof(int)*num_remote_procs);
    memset(recv_displs_in_current_proc, 0, sizeof(int)*num_remote_procs);

	for (int j = 0; j < num_remote_procs; j ++) {
	    for (int i = 0; i < num_src_fields; i ++) {
	        fields_data_type_sizes[i] = get_data_type_size(fields_mem[i]->get_data_type());
	        if (fields_routers[i]->get_num_dimensions() == 0) 
	            field_grids_num_lev[i] = 1;
	        else {
	            int grid_id = fields_mem[i]->get_grid_id();
	            if (original_grid_mgr->get_CoR_grid(grid_id)->get_num_dimensions() < 3) 
	                field_grids_num_lev[i] = 1;
	            else field_grids_num_lev[i] = cpl_get_num_levs_in_grid(grid_id);
	        }
            send_size_with_remote_procs[j] += fields_routers[i]->get_num_elements_transferred_with_remote_proc(true, j) * fields_data_type_sizes[i] * field_grids_num_lev[i];
        }
		if (send_size_with_remote_procs[j] > 0)
			send_index_remote_procs_with_common_data.push_back(j);
    }

	for (int j = 0; j < num_remote_procs; j ++) {
	    for (int i = num_src_fields; i < num_transfered_fields; i ++) {
	        fields_data_type_sizes[i] = get_data_type_size(fields_mem[i]->get_data_type());
	        if (fields_routers[i]->get_num_dimensions() == 0) 
	            field_grids_num_lev[i] = 1;
	        else {
	            int grid_id = fields_mem[i]->get_grid_id();
	            if (original_grid_mgr->get_CoR_grid(grid_id)->get_num_dimensions() < 3) 
	                field_grids_num_lev[i] = 1;
	            else field_grids_num_lev[i] = cpl_get_num_levs_in_grid(grid_id);
	        }
            recv_size_with_remote_procs[j] += fields_routers[i]->get_num_elements_transferred_with_remote_proc(false, j) * fields_data_type_sizes[i] * field_grids_num_lev[i];
        }
		if (recv_size_with_remote_procs[j] > 0)
			recv_index_remote_procs_with_common_data.push_back(j);
    }

    int * total_send_size_with_remote_procs = new int[num_local_procs * num_remote_procs];
    int src_fields_size = 0;

    if (num_src_fields > 0) {
        MPI_Allgather(send_size_with_remote_procs, num_remote_procs, MPI_INT, total_send_size_with_remote_procs, num_remote_procs, MPI_INT, local_comp_node->get_comm_group());

        for (int i = 0; i < current_proc_local_id; i ++) {
            for (int j = 0; j < num_remote_procs; j ++) {
                send_displs_in_remote_procs[j] += total_send_size_with_remote_procs[i * num_remote_procs + j];
            }
        }

        if (num_dst_fields > 0) {
            for (int i = 0; i < num_remote_procs; i ++)
                src_fields_size += send_size_with_remote_procs[i];

            for (int i = 0; i < num_remote_procs; i ++)
                send_displs_in_remote_procs[i] += src_fields_size;
        }
    }

    if (num_dst_fields > 0) {
        for (int i = 1; i < num_remote_procs; i ++)
            recv_displs_in_current_proc[i] = recv_displs_in_current_proc[i - 1] + recv_size_with_remote_procs[i - 1];

        if (num_src_fields > 0)
            for (int i = 0; i < num_remote_procs; i ++)
                recv_displs_in_current_proc[i] += src_fields_size;
    }

	for (int i = 0; i < num_transfered_fields; i ++)
		last_receive_field_sender_time[i] = -1;

    delete [] total_send_size_with_remote_procs;
}


Runtime_trans_algorithm::~Runtime_trans_algorithm()
{
    if (num_transfered_fields > 0) {
        delete [] fields_mem;
        delete [] fields_data_buffers;
        delete [] fields_routers;
        delete [] fields_timers;
        delete [] field_grids_num_lev;
        delete [] fields_data_type_sizes;
		delete [] transfer_process_on;
		delete [] current_remote_fields_time;
		delete [] current_receive_field_sender_time;
		delete [] current_receive_field_usage_time;
		delete [] last_receive_field_sender_time;
    }

    if (data_buf != NULL) delete [] data_buf;
    if (tag_buf != NULL) delete [] tag_buf;
    if (num_remote_procs > 0) {
        delete [] send_size_with_remote_procs;
        delete [] recv_size_with_remote_procs;
        delete [] send_displs_in_remote_procs;
        delete [] recv_displs_in_current_proc;
    }
    delete [] remote_proc_ranks_in_union_comm;
}


void Runtime_trans_algorithm::pass_transfer_parameters(std::vector <bool> &transfer_process_on, std::vector <long> &current_remote_fields_time)
{
	for (int i = 0; i < transfer_process_on.size(); i ++) {
		this->transfer_process_on[i] = transfer_process_on[i];
		this->current_remote_fields_time[i] = current_remote_fields_time[i];
	}
}


bool Runtime_trans_algorithm::set_remote_tags()
{
	long current_full_time = ((long)time_mgr->get_current_num_elapsed_day())*100000 + time_mgr->get_current_second();

	
	for (int i = 0; i < num_src_fields; i ++)
		if (transfer_process_on[i]) {
			tag_buf[i] = current_full_time;
			tag_buf[num_src_fields+i] = current_remote_fields_time[i];
		}

    for (int i = 0; i < send_index_remote_procs_with_common_data.size(); i ++) {
       	int remote_proc_id = remote_proc_ranks_in_union_comm[send_index_remote_procs_with_common_data[i]];
        MPI_Win_lock(MPI_LOCK_SHARED, remote_proc_id, 0, tag_win);
		printf("sender put tag %d\n", send_index_remote_procs_with_common_data[i]);
       	MPI_Put(tag_buf, num_src_fields*2, MPI_LONG, remote_proc_id, num_src_fields*current_proc_local_id*2, num_src_fields*2, MPI_LONG, tag_win);
       	MPI_Win_unlock(remote_proc_id, tag_win);
    }

    return true;
}


bool Runtime_trans_algorithm::set_local_tags()
{
    MPI_Win_lock(MPI_LOCK_SHARED, current_proc_id_union_comm, 0, tag_win);
    tag_buf[tag_buf_size-1] = current_field_local_recv_count;
	printf("set local tag %ld\n", current_field_local_recv_count);
	current_field_local_recv_count ++;
    MPI_Win_unlock(current_proc_id_union_comm, tag_win);
    
    return true;
}


bool Runtime_trans_algorithm::is_remote_data_buf_ready()
{
	long temp_field_remote_recv_count = -100;

	
    for (int i = 0; i < send_index_remote_procs_with_common_data.size(); i ++) {
		int remote_proc_index = send_index_remote_procs_with_common_data[i];
        if (send_size_with_remote_procs[remote_proc_index] > 0) {
            int remote_proc_id = remote_proc_ranks_in_union_comm[remote_proc_index];
            MPI_Win_lock(MPI_LOCK_SHARED, remote_proc_id, 0, tag_win);
            MPI_Get(tag_buf+tag_buf_size-1, 1, MPI_LONG, remote_proc_id, num_local_procs*num_src_fields*2, 1, MPI_LONG, tag_win);
            MPI_Win_unlock(remote_proc_id, tag_win);
			if (temp_field_remote_recv_count == -100) 
				temp_field_remote_recv_count = tag_buf[tag_buf_size-1];
			else if (temp_field_remote_recv_count != tag_buf[tag_buf_size-1])
				return false;
        }
    }

	if (temp_field_remote_recv_count == -1) {
		EXECUTION_REPORT(REPORT_ERROR, -1, last_field_remote_recv_count == -1 || last_field_remote_recv_count == 0, "Software error in Runtime_trans_algorithm::is_remote_data_buf_ready");
		if (last_field_remote_recv_count == -1) {
			last_field_remote_recv_count = 0;
			return true;
		}
		return false;
	}

	if (temp_field_remote_recv_count != last_field_remote_recv_count) {
		EXECUTION_REPORT(REPORT_ERROR, -1, temp_field_remote_recv_count == last_field_remote_recv_count + 1, "Software error in Runtime_trans_algorithm::is_remote_data_buf_ready %ld %ld", temp_field_remote_recv_count, last_field_remote_recv_count + 1);
		last_field_remote_recv_count = temp_field_remote_recv_count;
		return true;
	}

	return false;
}


void Runtime_trans_algorithm::receve_data_in_temp_buffer()
{
    bool is_ready = true, has_new_data = false;


    MPI_Win_lock(MPI_LOCK_SHARED, current_proc_id_union_comm, 0, tag_win);
	for (int j = num_src_fields; j < num_dst_fields; j ++)
	    for (int i = 0; i < recv_index_remote_procs_with_common_data.size(); i ++) {
			int remote_proc_index = recv_index_remote_procs_with_common_data[i];
			if (i == 0) {
				current_receive_field_sender_time[j] = tag_buf[remote_proc_index*num_dst_fields*2+j];
				current_receive_field_usage_time[j] = tag_buf[remote_proc_index*num_dst_fields*2+num_dst_fields+j];
			}
			else is_ready = is_ready && (current_receive_field_sender_time[j] == tag_buf[remote_proc_index*num_dst_fields*2+j]);
		}
    MPI_Win_unlock(current_proc_id_union_comm, tag_win);

    if (!is_ready)
		return;
	
	for (int i = num_src_fields; i < num_dst_fields; i ++)
		has_new_data = has_new_data | (last_receive_field_sender_time[i] != current_receive_field_sender_time[i]);

	if (!has_new_data)
		return;

	int empty_history_receive_buffer_index = -1;
	if (last_history_receive_buffer_index != -1) {
		for (int i = 0; i < history_receive_data_buffer.size(); i ++) {
			int index_iter = (last_history_receive_buffer_index+i) % history_receive_data_buffer.size();
			if (!history_receive_buffer_status[index_iter]) {
				empty_history_receive_buffer_index = index_iter;
				break;
			}
		}
	}
	printf("%s empty_history_receive_buffer_index is %d vs %d %d %lx\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), empty_history_receive_buffer_index, history_receive_data_buffer.size(), last_history_receive_buffer_index, this);
	if (empty_history_receive_buffer_index == -1) {
		std::vector<bool> temp_history_receive_buffer_status;
		std::vector<long*> temp_history_receive_sender_time;
		std::vector<long*> temp_history_receive_usage_time;
		std::vector<void*> temp_history_receive_data_buffer;
		for (int i = 0; i < history_receive_data_buffer.size(); i ++) {
			int index_iter = (last_history_receive_buffer_index+i) % history_receive_data_buffer.size();
			temp_history_receive_buffer_status.push_back(history_receive_buffer_status[index_iter]);
			temp_history_receive_sender_time.push_back(history_receive_sender_time[index_iter]);
			temp_history_receive_usage_time.push_back(history_receive_usage_time[index_iter]);
			temp_history_receive_data_buffer.push_back(history_receive_data_buffer[index_iter]);
		}
		history_receive_buffer_status.clear();
		history_receive_sender_time.clear();
		history_receive_usage_time.clear();
		history_receive_data_buffer.clear();
		for (int i = 0; i < temp_history_receive_data_buffer.size(); i ++) {
			history_receive_buffer_status.push_back(temp_history_receive_buffer_status[i]);
			history_receive_sender_time.push_back(temp_history_receive_sender_time[i]);
			history_receive_usage_time.push_back(temp_history_receive_usage_time[i]);
			history_receive_data_buffer.push_back(temp_history_receive_data_buffer[i]);			
		}
		last_history_receive_buffer_index = 0;
		empty_history_receive_buffer_index = history_receive_buffer_status.size();
		history_receive_buffer_status.push_back(false);
		history_receive_sender_time.push_back(new long [num_transfered_fields]);
		history_receive_usage_time.push_back(new long [num_transfered_fields]);
		history_receive_data_buffer.push_back(new char [data_buf_size]);
	}

	history_receive_buffer_status[empty_history_receive_buffer_index] = true;
	for (int i = 0; i < num_transfered_fields; i ++) {
		history_receive_sender_time[empty_history_receive_buffer_index][i] = current_receive_field_sender_time[i];
		history_receive_usage_time[empty_history_receive_buffer_index][i] = current_receive_field_usage_time[i];
		last_receive_field_sender_time[i] = current_receive_field_sender_time[i];
	}
	MPI_Win_lock(MPI_LOCK_SHARED, current_proc_id_union_comm, 0, data_win);
	memcpy(history_receive_data_buffer[empty_history_receive_buffer_index], data_buf, data_buf_size);
	MPI_Win_unlock(current_proc_id_union_comm, data_win);	
	
	printf("%s detect new data in receive buffer %ld vs %ld at %ld: store at %d vs %d\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), last_receive_field_sender_time[0], current_receive_field_sender_time[0], ((long)time_mgr->get_current_num_elapsed_day())*100000 + time_mgr->get_current_second(), empty_history_receive_buffer_index, history_receive_buffer_status.size()); 
	set_local_tags();
}


bool Runtime_trans_algorithm::run(bool is_algorithm_in_kernel_stage)
{
    if (num_src_fields > 0 && num_dst_fields == 0)
        return send(is_algorithm_in_kernel_stage);
    else if (num_src_fields == 0 && num_dst_fields > 0)
        return recv(is_algorithm_in_kernel_stage);
    else
        return sendrecv(is_algorithm_in_kernel_stage);
}


bool Runtime_trans_algorithm::send(bool is_algorithm_in_kernel_stage)
{
    preprocess();

	printf("%s before check remote buf for send at %ld %d\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), ((long)time_mgr->get_current_num_elapsed_day())*100000 + time_mgr->get_current_second(), last_field_remote_recv_count);

    if (!is_remote_data_buf_ready())
		return false;

	printf("%s before MPI_put send at %ld %d\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), ((long)time_mgr->get_current_num_elapsed_day())*100000 + time_mgr->get_current_second(), last_field_remote_recv_count);
    int offset = 0;
    for (int i = 0; i < num_remote_procs; i ++) {
        if (send_size_with_remote_procs[i] == 0) continue;

        int old_offset = offset;

        for (int j = 0; j < num_src_fields; j ++)
            if (transfer_process_on[j])
            {
                if (fields_routers[j]->get_num_dimensions() == 0) {
                    if (fields_routers[j]->get_num_elements_transferred_with_remote_proc(true, i) > 0)
                        MPI_Pack((char *)fields_data_buffers[j], fields_data_type_sizes[j], MPI_CHAR, data_buf, data_buf_size, &offset, union_comm);
                }
                else
                    pack_MD_data(i, j, &offset);
            }

        int remote_proc_id = remote_proc_ranks_in_union_comm[i];
        MPI_Win_lock(MPI_LOCK_SHARED, remote_proc_id, 0, data_win);
        MPI_Put((char *)data_buf + old_offset, send_size_with_remote_procs[i], MPI_CHAR, remote_proc_id,
                send_displs_in_remote_procs[i], send_size_with_remote_procs[i], MPI_CHAR, data_win);
        MPI_Win_unlock(remote_proc_id, data_win);

        EXECUTION_REPORT(REPORT_ERROR, -1, offset - old_offset == send_size_with_remote_procs[i], "C-Coupler software error in send of runtime_trans_algorithm: %d  %d", offset, old_offset == send_size_with_remote_procs[i]);
    }

    set_remote_tags();

	if (fields_data_type_sizes[0] == 4)
		printf("%s check send buffer %f %f vs %f\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), ((float*)data_buf)[0], ((float*)data_buf)[10], ((float*)fields_data_buffers[0])[0]);
	else printf("%s check send buffer %lf %lf vs %lf\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), ((double*)data_buf)[0], ((float*)data_buf)[10], ((double*)fields_data_buffers[0])[0]); 

	printf("After MPI_put send\n");

    return true;
}


bool Runtime_trans_algorithm::recv(bool is_algorithm_in_kernel_stage)
{
	bool received_data_ready = false;
    int offset = 0;


    preprocess();

	while (!received_data_ready) {
		receve_data_in_temp_buffer();
		received_data_ready = last_history_receive_buffer_index != -1 && history_receive_buffer_status[last_history_receive_buffer_index];
	}

	for (int j = num_src_fields; j < num_dst_fields; j ++)
		if (transfer_process_on[j]) {
			printf("last_history_receive_buffer_index is %d: %ld : %ld vs %ld %lx\n", last_history_receive_buffer_index, history_receive_sender_time[last_history_receive_buffer_index][j], history_receive_usage_time[last_history_receive_buffer_index][j], ((long)time_mgr->get_current_num_elapsed_day())*100000 + time_mgr->get_current_second(), this);
			EXECUTION_REPORT(REPORT_ERROR, -1, history_receive_sender_time[last_history_receive_buffer_index][j] == current_remote_fields_time[j], "software error in Runtime_trans_algorithm::recv: %ld vs %ld", history_receive_sender_time[last_history_receive_buffer_index][j], current_remote_fields_time[j]);
			EXECUTION_REPORT(REPORT_ERROR, -1, history_receive_usage_time[last_history_receive_buffer_index][j] == ((long)time_mgr->get_current_num_elapsed_day())*100000 + time_mgr->get_current_second(), "software error in Runtime_trans_algorithm::recv: %ld vs %ld", history_receive_usage_time[last_history_receive_buffer_index][j], ((long)time_mgr->get_current_num_elapsed_day())*100000 + time_mgr->get_current_second());
		}	

    for (int i = 0; i < num_remote_procs; i ++) {
        if (recv_size_with_remote_procs[i] == 0) 
			continue;
        offset = recv_displs_in_current_proc[i];
        for (int j = num_src_fields; j < num_transfered_fields; j ++)
            if (transfer_process_on[j]) {
                if (fields_routers[j]->get_num_dimensions() == 0) {
                    if (fields_routers[j]->get_num_elements_transferred_with_remote_proc(false, i) > 0)
                        MPI_Unpack((char *) history_receive_data_buffer[last_history_receive_buffer_index], data_buf_size, &offset, (char *) fields_data_buffers[j], fields_data_type_sizes[j], MPI_CHAR, union_comm);
                }
                else unpack_MD_data(history_receive_data_buffer[last_history_receive_buffer_index], i, j, &offset);
				fields_mem[j]->define_field_values(false);
            }

        EXECUTION_REPORT(REPORT_ERROR, -1, offset - recv_displs_in_current_proc[i] == recv_size_with_remote_procs[i], "C-Coupler software error in recv of runtime_trans_algorithm.");
    }
    
	for (int j = num_src_fields; j < num_transfered_fields; j ++)
		if (fields_data_type_sizes[j] == 4)
			printf("%s receive field instance with value %f at %d-%05d, %ld\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), ((float*) fields_data_buffers[j])[0], components_time_mgrs->get_time_mgr(comp_id)->get_current_num_elapsed_day(), components_time_mgrs->get_time_mgr(comp_id)->get_current_second(), current_remote_fields_time[j]);
		else printf("%s receive field instance with value %lf at %d-%05d, %ld\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name(), ((double*) fields_data_buffers[j])[0], components_time_mgrs->get_time_mgr(comp_id)->get_current_num_elapsed_day(), components_time_mgrs->get_time_mgr(comp_id)->get_current_second(), current_remote_fields_time[j]);

	history_receive_buffer_status[last_history_receive_buffer_index] = false;
	last_history_receive_buffer_index = (last_history_receive_buffer_index+1) % history_receive_buffer_status.size();

    return true;
}


bool Runtime_trans_algorithm::sendrecv(bool is_algorithm_in_kernel_stage)
{
    send(is_algorithm_in_kernel_stage);
    recv(is_algorithm_in_kernel_stage);
    return true;
}


void Runtime_trans_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
    if (fields_allocated) return;
    fields_allocated = true;
    data_buf_size = 0;

    for (int i = 0; i < num_transfered_fields; i ++)
        data_buf_size += fields_mem[i]->get_size_of_field() * fields_data_type_sizes[i];

    data_buf = (void *) new char [data_buf_size];

    if (num_dst_fields > 0)
		tag_buf_size = 2 * num_remote_procs * num_dst_fields + 1;
	else tag_buf_size = 2 * num_src_fields + 1;
	tag_buf = new long [tag_buf_size];	
	for (int i = 0; i < tag_buf_size; i ++)
		tag_buf[i] = -1;

	printf("buf size is %d vs %d\n", data_buf_size, tag_buf_size);
	fflush(NULL);
}

void Runtime_trans_algorithm::create_win()
{
    int union_size;
    MPI_Comm_size(union_comm, &union_size);
    EXECUTION_REPORT(REPORT_ERROR,-1, union_size == (num_remote_procs + local_comp_node->get_num_procs()), 
            "The two models have common processes");
    
    if (num_dst_fields > 0) {
        MPI_Win_create(data_buf, data_buf_size*sizeof(char), sizeof(char), MPI_INFO_NULL, union_comm, &data_win);
        MPI_Win_create(tag_buf, tag_buf_size*sizeof(long), sizeof(long), MPI_INFO_NULL, union_comm, &tag_win);
    }
    else {
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, union_comm, &data_win);
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, union_comm, &tag_win);
    }
}


void Runtime_trans_algorithm::preprocess()
{
    memset(send_size_with_remote_procs, 0, sizeof(int)*num_remote_procs);
    memset(recv_size_with_remote_procs, 0, sizeof(int)*num_remote_procs);

    for (int i = 0; i < num_src_fields; i ++) {
        if (transfer_process_on[i]) {
	        if (fields_data_type_sizes[i] == 4)
		       	printf("send a field instance: %d vs %d   %f\n", i, num_src_fields, ((float*)fields_data_buffers[i])[0]);
			else printf("send a field instance: %d vs %d   %lf\n", i, num_src_fields, ((double*)fields_data_buffers[i])[0]);
            for (int j = 0; j < num_remote_procs; j ++)
                send_size_with_remote_procs[j] += fields_routers[i]->get_num_elements_transferred_with_remote_proc(true, j) * fields_data_type_sizes[i] * field_grids_num_lev[i];
        }
    }

    for (int i = num_src_fields; i < num_transfered_fields; i ++) {
        if (transfer_process_on[i]) {
            for (int j = 0; j < num_remote_procs; j ++)
                recv_size_with_remote_procs[j] += fields_routers[i]->get_num_elements_transferred_with_remote_proc(false, j) * fields_data_type_sizes[i] * field_grids_num_lev[i];
        }
    }
}


void Runtime_trans_algorithm::pack_MD_data(int remote_proc_index, int field_index, int * offset)
{
    int num_segments;
    int *segment_starts, *num_elements_in_segments;
    int i, j;
	int field_2D_size;


    num_segments = fields_routers[field_index]->get_num_local_indx_segments_with_remote_proc(true, remote_proc_index);
    if (num_segments == 0)
        return;
    
    segment_starts = fields_routers[field_index]->get_local_indx_segment_starts_with_remote_proc(true, remote_proc_index);
    num_elements_in_segments = fields_routers[field_index]->get_local_indx_segment_lengths_with_remote_proc(true, remote_proc_index);
    field_2D_size = fields_routers[field_index]->get_src_decomp_size();
    for (i = 0; i < num_segments; i ++) {
        switch (fields_data_type_sizes[field_index]) {
            case 1:
                pack_segment_data((char*)((char*)data_buf+(*offset)), (char*)fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 2:
                pack_segment_data((short*)((char*)data_buf+(*offset)), (short*)fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 4:
                pack_segment_data((int*)((char*)data_buf+(*offset)), (int*)fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 8:
                pack_segment_data((double*)((char*)data_buf+(*offset)), (double*)fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            default:
                EXECUTION_REPORT(REPORT_ERROR,-1, false, "unsupported data type in runtime transfer algorithm %s. Please verify.", algorithm_cfg_name);
                break;
        }
        (*offset) += num_elements_in_segments[i]*field_grids_num_lev[field_index]*fields_data_type_sizes[field_index];
    }
}


void Runtime_trans_algorithm::unpack_MD_data(void *data_buf, int remote_proc_index, int field_index, int * offset)
{
    int num_segments;
    int *segment_starts, *num_elements_in_segments;
    int i, j;
	int field_2D_size;


    num_segments = fields_routers[field_index]->get_num_local_indx_segments_with_remote_proc(false, remote_proc_index);
    if (num_segments == 0)
        return;
    
    segment_starts = fields_routers[field_index]->get_local_indx_segment_starts_with_remote_proc(false, remote_proc_index);
    num_elements_in_segments = fields_routers[field_index]->get_local_indx_segment_lengths_with_remote_proc(false, remote_proc_index);
    field_2D_size = fields_routers[field_index]->get_dst_decomp_size();
    for (i = 0; i < num_segments; i ++) {
        switch (fields_data_type_sizes[field_index]) {
            case 1:
                unpack_segment_data((char*)((char*)data_buf+(*offset)), (char*)fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 2:
                unpack_segment_data((short*)((char*)data_buf+(*offset)), (short*)fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 4:
                unpack_segment_data((int*)((char*)data_buf+(*offset)), (int*)fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 8:
                unpack_segment_data((double*)((char*)data_buf+(*offset)), (double*)fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            default:
                EXECUTION_REPORT(REPORT_ERROR,-1, false, "unsupported data type in runtime transfer algorithm %s. Please verify.", algorithm_cfg_name);
                break;
        }
        (*offset) += num_elements_in_segments[i]*field_grids_num_lev[field_index]*fields_data_type_sizes[field_index];
    }
}
