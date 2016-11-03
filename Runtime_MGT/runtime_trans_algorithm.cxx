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


Runtime_trans_algorithm::Runtime_trans_algorithm(int num_src_fields, int num_dst_fields, Field_mem_info ** fields_mem, Routing_info ** routers, Coupling_timer ** timers)
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

    num_remote_procs = routers[0]->get_num_remote_procs();

    this->fields_mem = new Field_mem_info *[num_transfered_fields];
    fields_data_buffers = new void *[num_transfered_fields];
    fields_routers = new Routing_info *[num_transfered_fields];
    fields_timers = new Coupling_timer *[num_transfered_fields];
	transfer_process_on = new bool [num_transfered_fields];

    for (int i = 0; i < num_transfered_fields; i ++) {
        this->fields_mem[i] = fields_mem[i];
        fields_data_buffers[i] = fields_mem[i]->get_data_buf();
        fields_routers[i] = routers[i];
        fields_timers[i] = timers[i];
    }

    int local_comp_id = fields_routers[0]->get_local_comp_id();
    local_comp_node = comp_comm_group_mgt_mgr->search_global_node(local_comp_id);
    int remote_comp_id = fields_routers[0]->get_remote_comp_id();
    remote_comp_node = comp_comm_group_mgt_mgr->search_global_node(remote_comp_id);
    
    fields_allocated = false;
    initialize_local_data_structures();
    allocate_src_dst_fields("kernel");

    //To be modified in future;
    pre_tag = 0;
    current_tag = 1; 
    next_tag = 2;

    //To be deleted in future;
    if (num_dst_fields > 0) {
        memset(tag_buf, 0, sizeof(int) * tag_buf_size);
        for (int i = 1; i < 2 * num_remote_procs; i += 2)
            tag_buf[i] = 1;
    }
}


void Runtime_trans_algorithm::initialize_local_data_structures()
{
    send_size_with_remote_procs = new int[num_remote_procs];
    recv_size_with_remote_procs = new int[num_remote_procs];
    send_displs_in_remote_procs = new int[num_remote_procs];
    recv_displs_in_current_proc = new int[num_remote_procs];
    field_grids_num_lev = new long[num_transfered_fields];
    fields_data_type_sizes = new int[num_transfered_fields];

    memset(send_size_with_remote_procs, 0, sizeof(int)*num_remote_procs);
    memset(recv_size_with_remote_procs, 0, sizeof(int)*num_remote_procs);
    memset(send_displs_in_remote_procs, 0, sizeof(int)*num_remote_procs);
    memset(recv_displs_in_current_proc, 0, sizeof(int)*num_remote_procs);

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

        for (int j = 0; j < num_remote_procs; j ++) {
            send_size_with_remote_procs[j] += fields_routers[i]->get_num_elements_transferred_with_remote_proc(true, j) * fields_data_type_sizes[i] * field_grids_num_lev[i];
        }
    }

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

        for (int j = 0; j < num_remote_procs; j ++) {
            recv_size_with_remote_procs[j] += fields_routers[i]->get_num_elements_transferred_with_remote_proc(false, j) * fields_data_type_sizes[i] * field_grids_num_lev[i];
        }
    }

    int local_comp_size = local_comp_node->get_num_procs();
    int current_proc_local_id = local_comp_node->get_current_proc_local_id();
    int * total_send_size_with_remote_procs = new int[local_comp_size * num_remote_procs];
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
    }

    if (data_buf != NULL) delete [] data_buf;
    if (tag_buf != NULL) delete [] tag_buf;
    if (num_remote_procs > 0) {
        delete [] send_size_with_remote_procs;
        delete [] recv_size_with_remote_procs;
        delete [] send_displs_in_remote_procs;
        delete [] recv_displs_in_current_proc;
    }
}


void Runtime_trans_algorithm::pass_transfer_status(std::vector <bool> &transfer_process_on)
{
	for (int i = 0; i < transfer_process_on.size(); i ++)
		this->transfer_process_on[i] = transfer_process_on[i];
}


bool Runtime_trans_algorithm::set_remote_tags()
{
    if (num_src_fields <= 0) return true;

    int current_proc_local_id = local_comp_node->get_current_proc_local_id();

    for (int i = 0; i < num_remote_procs; i ++) {
        int remote_proc_global_id = remote_comp_node->get_local_proc_global_id(i);
        MPI_Win_lock(MPI_LOCK_SHARED, remote_proc_global_id, 0, tag_win);
		printf("sender put tag %d\n", current_tag);
        MPI_Put(&current_tag, 1, MPI_INT, remote_proc_global_id, 2 * current_proc_local_id, 1, MPI_INT, tag_win);
        MPI_Win_unlock(remote_proc_global_id, tag_win);
    }

    return true;
}


bool Runtime_trans_algorithm::set_local_tags()
{
    if (num_dst_fields <= 0) return true;

    int current_proc_global_id = comp_comm_group_mgt_mgr->get_current_proc_global_id();

    MPI_Win_lock(MPI_LOCK_SHARED, current_proc_global_id, 0, tag_win);
    for (int i = 1; i < 2 * num_remote_procs; i += 2)
        tag_buf[i] = next_tag;
    MPI_Win_unlock(current_proc_global_id, tag_win);

	printf("receiver set tag %d\n", next_tag);
    
    return true;
}


//To be modified in future
void Runtime_trans_algorithm::advance_step()
{
    pre_tag = current_tag;
    current_tag = next_tag;
    next_tag += 1;
}


bool Runtime_trans_algorithm::is_remote_data_buf_ready()
{
    if (num_src_fields <= 0) return true;

    int current_proc_local_id = local_comp_node->get_current_proc_local_id();
    int current_proc_global_id = comp_comm_group_mgt_mgr->get_current_proc_global_id();

    bool is_ready = true;
    int remote_tag;

    for (int i = 0; i < num_remote_procs; i ++) {
        if (send_size_with_remote_procs[i] > 0) 
        {
            int remote_proc_global_id = remote_comp_node->get_local_proc_global_id(i);
            MPI_Win_lock(MPI_LOCK_SHARED, remote_proc_global_id, 0, tag_win);
            MPI_Get(&remote_tag, 1, MPI_INT, remote_proc_global_id, 2 * current_proc_local_id + 1, 1, MPI_INT, tag_win);
            MPI_Win_unlock(remote_proc_global_id, tag_win);
			printf("remote tag is %d vs %d\n", remote_tag, pre_tag);
            if (remote_tag == pre_tag) {
                is_ready = false;
                break;
            }
            else 
                EXECUTION_REPORT(REPORT_ERROR,-1, remote_tag == current_tag, "The coupling components %s and %s are nonsynchronous: %d %d in is_remote_data_buf_ready.",
                        local_comp_node->get_comp_full_name(), remote_comp_node->get_comp_full_name(), current_tag, remote_tag);
        }
    }

    return is_ready;
}


bool Runtime_trans_algorithm::is_local_data_buf_ready()
{
    if (num_dst_fields <= 0) return true;

    bool is_ready = true;

    int current_proc_global_id = comp_comm_group_mgt_mgr->get_current_proc_global_id();
    MPI_Win_lock(MPI_LOCK_SHARED, current_proc_global_id, 0, tag_win);
    for (int i = 0; i < num_remote_procs; i ++) {
        if (recv_size_with_remote_procs[i] > 0) {
            if (tag_buf[2 * i] == pre_tag) {
                is_ready = false;
                break;
            }
            else
                EXECUTION_REPORT(REPORT_ERROR,-1, tag_buf[2 * i] == current_tag, "The coupling components %s and %s are nonsynchronous: %d %d in is_local_data_buf_ready.", 
                        local_comp_node->get_comp_full_name(), remote_comp_node->get_comp_full_name(), current_tag, tag_buf[2 * i]);
        }
    }
    MPI_Win_unlock(current_proc_global_id, tag_win);

    return is_ready;
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

    while (! is_remote_data_buf_ready());  // to be modified

	printf("before MPI_put send\n");
    int offset = 0;
    for (int i = 0; i < num_remote_procs; i ++) {
        if (send_size_with_remote_procs[i] == 0) continue;

        int old_offset = offset;

        for (int j = 0; j < num_src_fields; j ++)
            if (transfer_process_on[j])
            {
                if (fields_routers[j]->get_num_dimensions() == 0) {
                    if (fields_routers[j]->get_num_elements_transferred_with_remote_proc(true, i) > 0)
                        MPI_Pack((char *)fields_data_buffers[j], fields_data_type_sizes[j], MPI_CHAR, data_buf, data_buf_size, &offset, MPI_COMM_WORLD);
                }
                else
                    pack_MD_data(i, j, &offset);
            }

        int remote_proc_global_id = remote_comp_node->get_local_proc_global_id(i);
        MPI_Win_lock(MPI_LOCK_SHARED, remote_proc_global_id, 0, data_win);
        MPI_Put((char *)data_buf + old_offset, send_size_with_remote_procs[i], MPI_CHAR, remote_proc_global_id,
                send_displs_in_remote_procs[i], send_size_with_remote_procs[i], MPI_CHAR, data_win);
        MPI_Win_unlock(remote_proc_global_id, data_win);

        EXECUTION_REPORT(REPORT_ERROR, -1, offset - old_offset == send_size_with_remote_procs[i], "C-Coupler software error in send of runtime_trans_algorithm: %d  %d", offset, old_offset == send_size_with_remote_procs[i]);
    }

    set_remote_tags();
    advance_step();

	if (fields_data_type_sizes[0] == 4)
		printf("check send buffer %f %f vs %f\n", ((float*)data_buf)[0], ((float*)data_buf)[10], ((float*)fields_data_buffers[0])[0]);
	else printf("check send buffer %lf %lf vs %lf\n", ((double*)data_buf)[0], ((float*)data_buf)[10], ((double*)fields_data_buffers[0])[0]); 

	printf("After MPI_put send\n");

    return true;
}


bool Runtime_trans_algorithm::recv(bool is_algorithm_in_kernel_stage)
{
    preprocess();

    while (!is_local_data_buf_ready());

    int current_proc_global_id = comp_comm_group_mgt_mgr->get_current_proc_global_id();
    int offset = 0;

    //MPI_Win_lock(MPI_LOCK_SHARED, current_proc_global_id, 0, data_win);
    for (int i = 0; i < num_remote_procs; i ++) {
        if (recv_size_with_remote_procs[i] == 0) 
			continue;
        offset = recv_displs_in_current_proc[i];
        for (int j = num_src_fields; j < num_transfered_fields; j ++)
            if (fields_timers[j]->is_timer_on())
            {
                if (fields_routers[j]->get_num_dimensions() == 0) {
                    if (fields_routers[j]->get_num_elements_transferred_with_remote_proc(false, i) > 0)
                        MPI_Unpack((char *) data_buf, data_buf_size, &offset, (char *) fields_data_buffers[j], fields_data_type_sizes[j], MPI_CHAR, MPI_COMM_WORLD);
                }
                else unpack_MD_data(i, j, &offset);
				fields_mem[j]->define_field_values(false);
            }

        EXECUTION_REPORT(REPORT_ERROR, -1, offset - recv_displs_in_current_proc[i] == recv_size_with_remote_procs[i], "C-Coupler software error in recv of runtime_trans_algorithm.");
    }
    //MPI_Win_unlock(current_proc_global_id, data_win);
	for (int j = num_src_fields; j < num_transfered_fields; j ++)
		if (fields_data_type_sizes[j] == 4)
			printf("receive field instance with value %f : %d vs %d\n", ((float*) fields_data_buffers[j])[0], tag_buf[2 * j], pre_tag);
		else printf("receive field instance with value %lf : %d vs %d \n", ((double*) fields_data_buffers[j])[0], tag_buf[2 * j], pre_tag);

    set_local_tags();
    advance_step();
    
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

    if (num_dst_fields > 0) tag_buf_size = 2 * num_remote_procs;

    if (tag_buf_size > 0) tag_buf = new int[tag_buf_size];
}

void Runtime_trans_algorithm::create_win()
{
    if (num_dst_fields > 0) {
        MPI_Win_create(data_buf, data_buf_size*sizeof(char), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &data_win);
        MPI_Win_create(tag_buf, tag_buf_size*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &tag_win);
    }
    else {
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &data_win);
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &tag_win);
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
        if (fields_timers[i]->is_timer_on()) {
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
    field_2D_size = fields_routers[field_index]->get_local_decomp_size();
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


void Runtime_trans_algorithm::unpack_MD_data(int remote_proc_index, int field_index, int * offset)
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
    field_2D_size = fields_routers[field_index]->get_local_decomp_size();
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
