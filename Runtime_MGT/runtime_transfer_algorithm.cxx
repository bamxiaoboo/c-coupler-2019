/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "runtime_transfer_algorithm.h"
#include "global_data.h"
#include "runtime_config_dir.h"
#include "cor_global_data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


template <class T> void pack_segment_data(T *mpi_buf, T *field_data_buf, int segment_start, int segment_size, int field_2D_size, int num_lev)
{
    int i, j, offset;


    for (i = segment_start, offset = 0; i < segment_size+segment_start; i ++)
        for (j = 0; j < num_lev; j ++)
            mpi_buf[offset++] = field_data_buf[i+j*field_2D_size];
}


template <class T> void unpack_segment_data(T *mpi_buf, T *field_data_buf, int segment_start, int segment_size, int field_2D_size, int num_lev)
{
    int i, j, offset;


    for (i = segment_start, offset = 0; i < segment_size+segment_start; i ++)
        for (j = 0; j < num_lev; j ++)
            field_data_buf[i+j*field_2D_size] = mpi_buf[offset++];
}


Runtime_transfer_algorithm::Runtime_transfer_algorithm(const char *cfg_name)
{
	strcpy(algorithm_cfg_name, cfg_name);
	fields_transfer_info_string = NULL;
	generate_algorithm_info_from_cfg_file();
}


Runtime_transfer_algorithm::Runtime_transfer_algorithm(int num_fields, Field_mem_info **fields_mem, Routing_info *router, Coupling_timer *timer)
{
    num_transfered_fields = num_fields;
    strcpy(remote_comp_name, compset_communicators_info_mgr->get_current_comp_name());
    initialize_local_data_structures();
    num_send_fields = num_fields/2;
    num_recv_fields = num_fields/2;
    comm_tag = 0;
    for (int i = 0; i < num_fields; i ++) {
        transferred_fields_mem[i] = fields_mem[i];
        transferred_fields_data_buffers[i] = fields_mem[i]->get_data_buf();
        fields_data_type_sizes[i] = get_data_type_size(fields_mem[i]->get_field_data()->get_grid_data_field()->data_type_in_application);
        fields_routers[i] = router;
        fields_timers[i] = timer;
		strcpy(comp_names[i], fields_mem[i]->get_comp_name());
		strcpy(field_names[i], fields_mem[i]->get_field_name());
        strcpy(field_local_decomp_names[i], fields_mem[0]->get_decomp_name());
        strcpy(field_remote_decomp_names[i], fields_mem[num_send_fields]->get_decomp_name());
        strcpy(field_grid_names[i], fields_mem[i]->get_grid_name());
    }
	fields_transfer_info_string = NULL;
}


Runtime_transfer_algorithm::~Runtime_transfer_algorithm()
{
    if (mpi_send_buf != NULL)
        delete [] mpi_send_buf;
    if (mpi_recv_buf != NULL)
        delete [] mpi_recv_buf;
    if (num_transfered_fields > 0) {
        delete [] fields_data_type_sizes;
        delete [] currently_transferred_fields_mark;
        delete [] field_grids_num_lev;
        delete [] send_size_with_remote_procs;
        delete [] recv_size_with_remote_procs;
        for (int i = 0; i < num_transfered_fields; i ++) {
			delete [] comp_names[i];
			delete [] field_names[i];
            delete [] field_local_decomp_names[i];
            delete [] field_remote_decomp_names[i];
            delete [] field_grid_names[i];
            if (num_send_fields != num_recv_fields)
                delete fields_timers[i];
        }
		delete [] comp_names;
		delete [] field_names;
        delete [] field_local_decomp_names;
        delete [] field_remote_decomp_names;
        delete [] field_grid_names;
		delete [] buf_marks;
        delete [] fields_timers;
        delete [] fields_routers;
        delete [] transferred_fields_mem;
    }
    if (num_remote_procs > 0) {
        delete [] send_requests;
        delete [] send_statuses;
        delete [] recv_requests;
        delete [] recv_statuses;
    }
	if (fields_transfer_info_string != NULL) {
		delete [] fields_transfer_info_string;
	}
}


void Runtime_transfer_algorithm::check_mpi_error(const char *error_reporter)
{
	EXECUTION_REPORT(REPORT_ERROR, mpi_ierr == MPI_SUCCESS, "%s", error_reporter);
}


void Runtime_transfer_algorithm::preprocess(bool is_algorithm_in_kernel_stage)
{
    int i, j;


	if (num_timer_on_fields == 0)
		return;

#ifdef DEBUG_CCPL
	if (!(num_send_fields > 0 && num_recv_fields > 0))
		check_cfg_info_consistency();
	if (num_send_fields > 0 && num_recv_fields == 0) {
		if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
			for (i = 0; i < num_transfered_fields; i ++)
				if (currently_transferred_fields_mark[i])
					fields_transfer_info_string[i*10] = 1;
				else fields_transfer_info_string[i*10] = 0;
			MPI_Send(fields_transfer_info_string, num_transfered_fields*10, MPI_CHAR, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), 0), 
					 comm_tag, compset_communicators_info_mgr->get_global_comm_group());
		}
	}
	if (num_recv_fields > 0 && num_send_fields == 0) {
		if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
			MPI_Recv(fields_transfer_info_string, num_transfered_fields*10, MPI_CHAR, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), 0), 
				 	 comm_tag, compset_communicators_info_mgr->get_global_comm_group(), &recv_statuses[0]);
			for (i = 0; i < num_transfered_fields; i ++)
				EXECUTION_REPORT(REPORT_ERROR, !currently_transferred_fields_mark[i] && fields_transfer_info_string[i*10] == 0 || currently_transferred_fields_mark[i] && fields_transfer_info_string[i*10] == 1,
				                 "Mismatch happens for transferring field %s between components %s and %s", transferred_fields_mem[i]->get_field_name(), compset_communicators_info_mgr->get_current_comp_name(), remote_comp_name);
		}
	}
#endif

	if (fields_allocated) {
		if (num_send_fields > 0 && mpi_send_buf != NULL) {
			delete [] mpi_send_buf;
			mpi_send_buf = NULL;
		}
		if (num_recv_fields > 0 && mpi_recv_buf != NULL) {
			delete [] mpi_recv_buf;
			mpi_recv_buf = NULL;
		}
	}

    /* Allocate memory buffer for sending or receiving */
    if (mpi_send_buf == NULL && num_send_fields > 0) {
        for (i = 0; i < num_send_fields; i ++) {
            fields_routers[i] = routing_info_mgr->search_or_add_router(remote_comp_name, field_local_decomp_names[i], field_remote_decomp_names[i]);
            if (fields_routers[i]->get_num_dimensions() == 0)
                field_grids_num_lev[i] = 1;
            else {
                if (remap_grid_manager->search_remap_grid_with_grid_name(field_grid_names[i])->get_num_dimensions() == 2)
                    field_grids_num_lev[i] = 1;
                else field_grids_num_lev[i] = cpl_get_num_levs_in_grid(field_grid_names[i]);
            }
        }
        for (i = 0, buffer_size = 0; i < num_send_fields; i ++) 
            for (j = 0; j < num_remote_procs; j ++) 
                buffer_size += fields_data_type_sizes[i]*fields_routers[i]->get_num_elements_transferred_with_remote_proc(true, j)*field_grids_num_lev[i];
        if (buffer_size > 0)
            mpi_send_buf = new char [buffer_size];
		else mpi_send_buf = new char [1];
    }
    if (mpi_recv_buf == NULL && num_recv_fields > 0) {
        for (i = num_send_fields; i < num_transfered_fields; i ++) {
            fields_routers[i] = routing_info_mgr->search_or_add_router(remote_comp_name, field_local_decomp_names[i], field_remote_decomp_names[i]);
            if (fields_routers[i]->get_num_dimensions() == 0)
                field_grids_num_lev[i] = 1;
            else {
                if (remap_grid_manager->search_remap_grid_with_grid_name(field_grid_names[i])->get_num_dimensions() == 2)
                    field_grids_num_lev[i] = 1;
                else field_grids_num_lev[i] = cpl_get_num_levs_in_grid(field_grid_names[i]);
            }
        }
        for (i = num_send_fields, buffer_size = 0; i < num_transfered_fields; i ++) 
            for (j = 0; j < num_remote_procs; j ++) 
                buffer_size += fields_data_type_sizes[i]*fields_routers[i]->get_num_elements_transferred_with_remote_proc(false, j)*field_grids_num_lev[i];
        if (buffer_size > 0)
            mpi_recv_buf = new char [buffer_size];
		else mpi_recv_buf = new char [1];
    }

    for (i = 0; i < num_remote_procs; i ++)
        send_size_with_remote_procs[i] = 0;
    for (i = 0; i < num_send_fields; i ++) {
        if (currently_transferred_fields_mark[i]) {
			EXECUTION_REPORT(REPORT_ERROR, fields_data_type_sizes[i] > 0, "C-Coupler software error1 in preprocess");
            for (j = 0; j < num_remote_procs; j ++) 
                send_size_with_remote_procs[j] += fields_routers[i]->get_num_elements_transferred_with_remote_proc(true, j)*fields_data_type_sizes[i]*field_grids_num_lev[i];
        }
    }

    for (i = 0; i < num_remote_procs; i ++)
        recv_size_with_remote_procs[i] = 0;
    for (i = num_send_fields; i < num_transfered_fields; i ++) {
        if (currently_transferred_fields_mark[i]) {
			EXECUTION_REPORT(REPORT_ERROR, fields_data_type_sizes[i] > 0, "C-Coupler software error2 in preprocess");
            for (j = 0; j < num_remote_procs; j ++)
                recv_size_with_remote_procs[j] += fields_routers[i]->get_num_elements_transferred_with_remote_proc(false, j)*fields_data_type_sizes[i]*field_grids_num_lev[i];
        }
    }

	EXECUTION_REPORT(REPORT_LOG, true, "Finish preprocess for data transfer");
}


void Runtime_transfer_algorithm::run(bool is_algorithm_in_kernel_stage)
{
    if (num_send_fields > 0 && num_recv_fields == 0)
        send_data(is_algorithm_in_kernel_stage);
    else if (num_send_fields == 0 && num_recv_fields > 0)
        recv_data(is_algorithm_in_kernel_stage);
    else
        sendrecv_data(is_algorithm_in_kernel_stage);
}


void Runtime_transfer_algorithm::unpack_MD_data(int remote_proc_index, int field_index, int buffer_size, long field_2D_size, int *offset)
{
    long i, j;
    int *segment_starts, *num_elements_in_segments;
    int num_segments;


    if (fields_routers[field_index]->get_num_elements_transferred_with_remote_proc(false, remote_proc_index) == 0)
        return;

    segment_starts = fields_routers[field_index]->get_local_indx_segment_starts_with_remote_proc(false, remote_proc_index);
    num_elements_in_segments = fields_routers[field_index]->get_local_indx_segment_lengths_with_remote_proc(false, remote_proc_index);
    num_segments = fields_routers[field_index]->get_num_local_indx_segments_with_remote_proc(false, remote_proc_index);
    for (i = 0; i < num_segments; i ++) {     
        switch (fields_data_type_sizes[field_index]) {
            case 1:
                unpack_segment_data((char*)((char*)mpi_recv_buf+(*offset)), (char*)transferred_fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 2:
                unpack_segment_data((short*)((char*)mpi_recv_buf+(*offset)), (short*)transferred_fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 4:
                unpack_segment_data((int*)((char*)mpi_recv_buf+(*offset)), (int*)transferred_fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 8:
                unpack_segment_data((double*)((char*)mpi_recv_buf+(*offset)), (double*)transferred_fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            default:
                EXECUTION_REPORT(REPORT_ERROR, false, "unsupported data type in runtime transfer algorithm\n");
                break;
        }
        (*offset) += num_elements_in_segments[i]*field_grids_num_lev[field_index]*fields_data_type_sizes[field_index];
    }
}


void Runtime_transfer_algorithm::pack_MD_data(long remote_proc_index, long field_index, int *offset, int buffer_size)
{
    long num_segments;
    int *segment_starts, *num_elements_in_segments;
    long i, j;
	long field_2D_size;


    if (fields_routers[field_index]->get_num_elements_transferred_with_remote_proc(true, remote_proc_index) == 0)
        return;
    
    num_segments = fields_routers[field_index]->get_num_local_indx_segments_with_remote_proc(true, remote_proc_index);
    segment_starts = fields_routers[field_index]->get_local_indx_segment_starts_with_remote_proc(true, remote_proc_index);
    num_elements_in_segments = fields_routers[field_index]->get_local_indx_segment_lengths_with_remote_proc(true, remote_proc_index);
    field_2D_size = fields_routers[field_index]->get_local_decomp_size();
    for (i = 0; i < num_segments; i ++) {         
        switch (fields_data_type_sizes[field_index]) {
            case 1:
                pack_segment_data((char*)((char*)mpi_send_buf+(*offset)), (char*)transferred_fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 2:
                pack_segment_data((short*)((char*)mpi_send_buf+(*offset)), (short*)transferred_fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 4:
                pack_segment_data((int*)((char*)mpi_send_buf+(*offset)), (int*)transferred_fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            case 8:
                pack_segment_data((double*)((char*)mpi_send_buf+(*offset)), (double*)transferred_fields_data_buffers[field_index], segment_starts[i], num_elements_in_segments[i], field_2D_size, field_grids_num_lev[field_index]);
                break;
            default:
                EXECUTION_REPORT(REPORT_ERROR, false, "unsupported data type in runtime transfer algorithm\n");
                break;
        }
        (*offset) += num_elements_in_segments[i]*field_grids_num_lev[field_index]*fields_data_type_sizes[field_index];
    }
}


void Runtime_transfer_algorithm::recv_data(bool is_algorithm_in_kernel_stage) 
{
    MPI_Comm global_comm;
    int remote_proc_id;
    int offset;
    int i,m;
	char data_type[NAME_STR_SIZE];

    
    EXECUTION_REPORT(REPORT_LOG, true, "begin receiving data");

	if (num_timer_on_fields == 0) {
		EXECUTION_REPORT(REPORT_LOG, true, "no field is transferred in current time");
		return;
	}

	if (restart_mgr->is_in_restart_read_time_window()) {
		for (i = num_send_fields; i < num_transfered_fields; i ++)
			if (!is_algorithm_in_kernel_stage || fields_timers[i]->is_timer_on()) {
				restart_mgr->get_field_datatype_for_transfer(comp_names[i], field_local_decomp_names[i], field_grid_names[i], field_names[i], buf_marks[i], data_type);
				transferred_fields_mem[i] = alloc_mem(comp_names[i], field_local_decomp_names[i], field_grid_names[i], field_names[i], data_type, buf_marks[i], false);	
				add_runtime_datatype_transformation(transferred_fields_mem[i], false, fields_timers[i]);
				restart_mgr->read_one_restart_field(transferred_fields_mem[i]);
				transferred_fields_mem[i] = NULL;
			}
		EXECUTION_REPORT(REPORT_LOG, true, "transform data receiving to restart read in restart read time window at %ld\n", timer_mgr->get_current_full_time());
		return;
	}

	preprocess(is_algorithm_in_kernel_stage);

    /* Receive data from each remote process */
    global_comm = compset_communicators_info_mgr->get_global_comm_group();
    for(i = 0, offset = 0; i < num_remote_procs; i ++) {
        if (recv_size_with_remote_procs[i] == 0)
            continue;        
        remote_proc_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), i);
        mpi_ierr = MPI_Irecv(((char *)mpi_recv_buf+offset), recv_size_with_remote_procs[i], MPI_CHAR, remote_proc_id, comm_tag, global_comm, (recv_requests+i));        
		check_mpi_error("MPI error of MPI_Irev in recv_data of runtime_transfer_algorithm");
        offset += recv_size_with_remote_procs[i];
    }
	performance_timing_mgr->performance_timing_start(TIMING_TYPE_COMMUNICATION, TIMING_COMMUNICATION_RECV, compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), NULL);
    for(i = 0; i < num_remote_procs; i ++)
        if (recv_size_with_remote_procs[i] > 0) {
            mpi_ierr = MPI_Wait(&recv_requests[i], &recv_statuses[i]);
			check_mpi_error("MPI error of MPI_Wait in recv_data of runtime_transfer_algorithm");
        }

	performance_timing_mgr->performance_timing_stop(TIMING_TYPE_COMMUNICATION, TIMING_COMMUNICATION_RECV, compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), NULL);

	EXECUTION_REPORT(REPORT_LOG, true, "after getting data from MPI");

    /* Unpack the data received*/
    for(i = 0, offset = 0; i < num_remote_procs; i ++) {
        int old_offset = offset;
        if (recv_size_with_remote_procs[i] == 0)
            continue;
        for (m = num_send_fields; m < num_transfered_fields; m ++) 
            if (currently_transferred_fields_mark[m])
                if (fields_routers[m]->get_num_dimensions() == 0) {
                    if (fields_routers[m]->get_num_elements_transferred_with_remote_proc(false, i) > 0)
                        MPI_Unpack((char *)mpi_recv_buf, buffer_size, &offset, (char *)transferred_fields_data_buffers[m], fields_data_type_sizes[m], MPI_CHAR, global_comm);
                }
                else unpack_MD_data(i, m, buffer_size, fields_routers[m]->get_local_decomp_size(), &offset);
        EXECUTION_REPORT(REPORT_ERROR, offset-old_offset == recv_size_with_remote_procs[i], "C-Coupler software error in recv_data\n");
    }

#ifdef DEBUG_CCPL
    exchange_comp_time_info();
#endif
    
    EXECUTION_REPORT(REPORT_LOG, true, "after receiving data");

    for (m = num_send_fields; m < num_transfered_fields; m ++) 
        if (currently_transferred_fields_mark[m]) {
            transferred_fields_mem[m]->check_field_sum();
			transferred_fields_mem[m]->define_field_values(false);
        }

    if (restart_mgr->is_in_restart_write_time_window()) {
        for (i = num_send_fields; i < num_transfered_fields; i ++)
            if (currently_transferred_fields_mark[i])
				restart_mgr->write_one_restart_field(transferred_fields_mem[i], 0);
        EXECUTION_REPORT(REPORT_LOG, true, "must write recved data to restart file at %ld", timer_mgr->get_current_full_time());
    }
}


void Runtime_transfer_algorithm::send_data(bool is_algorithm_in_kernel_stage)
{    
    MPI_Comm global_comm;
    int offset;
    int remote_proc_id;
    long num_segments;
    long i, m;
    int remote_proc_begin_pos_in_buffer;

 
    EXECUTION_REPORT(REPORT_LOG, true, "before sending data at %ld");

	if (num_timer_on_fields == 0) {
		EXECUTION_REPORT(REPORT_LOG, true, "no field is transferred in current time");
		return;
	}

	if (restart_mgr->is_in_restart_read_time_window()) {
		EXECUTION_REPORT(REPORT_LOG, true, "bypass data sending in restart read time window");
		return;
	}

	preprocess(is_algorithm_in_kernel_stage);

    /* For each remote process,  pack the data and then send out the data */
    global_comm = compset_communicators_info_mgr->get_global_comm_group();
    for (i = 0, offset = 0; i < num_remote_procs; i ++) {
        if (send_size_with_remote_procs[i] == 0)
            continue;
        int old_offset = offset;
        remote_proc_begin_pos_in_buffer = offset;
        for (m = 0; m < num_send_fields; m ++)
            if (currently_transferred_fields_mark[m])
                if (fields_routers[m]->get_num_dimensions() == 0) {
                    if (fields_routers[m]->get_num_elements_transferred_with_remote_proc(true, i) > 0)
                        MPI_Pack((char *)transferred_fields_data_buffers[m], fields_data_type_sizes[m], MPI_CHAR, mpi_send_buf, buffer_size, &offset, global_comm);
                }
                else pack_MD_data(i, m, &offset, buffer_size);
        EXECUTION_REPORT(REPORT_ERROR, offset-old_offset == send_size_with_remote_procs[i], "C-Coupler error in send_data\n");        
        remote_proc_id = compset_communicators_info_mgr->get_proc_id_in_global_comm_group(compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), i);
        mpi_ierr = MPI_Isend((char *)mpi_send_buf+remote_proc_begin_pos_in_buffer, send_size_with_remote_procs[i], MPI_CHAR, remote_proc_id, comm_tag, global_comm, (send_requests+i));
		check_mpi_error("MPI error of MPI_Isend in send_data of runtime_transfer_algorithm");
    }    
	performance_timing_mgr->performance_timing_start(TIMING_TYPE_COMMUNICATION, TIMING_COMMUNICATION_SEND, compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), NULL);
    for (i = 0, offset = 0; i < num_remote_procs; i ++) 
        if (send_size_with_remote_procs[i] > 0) {
            mpi_ierr = MPI_Wait(&send_requests[i], &send_statuses[i]);
			check_mpi_error("MPI error of MPI_Wait in send_data of runtime_transfer_algorithm");
        }
	performance_timing_mgr->performance_timing_stop(TIMING_TYPE_COMMUNICATION, TIMING_COMMUNICATION_SEND, compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), NULL);

#ifdef DEBUG_CCPL
    exchange_comp_time_info();
#endif

    for (m = 0; m < num_send_fields; m ++)
        if (currently_transferred_fields_mark[m]) {   
            transferred_fields_mem[m]->check_field_sum();
			transferred_fields_mem[m]->use_field_values();
        }
		
	EXECUTION_REPORT(REPORT_LOG, true, "after sending data at %ld", timer_mgr->get_current_full_time());
}


void Runtime_transfer_algorithm::sendrecv_data(bool is_algorithm_in_kernel_stage)
{
    MPI_Comm local_comm;
    int offset;
    long num_segments;
    long i, m;
    int remote_proc_begin_pos_in_buffer;
    int old_offset;


    EXECUTION_REPORT(REPORT_LOG, true, "before sending/receiving data");

	if (num_timer_on_fields == 0) {
		EXECUTION_REPORT(REPORT_LOG, true, "no field is transferred in current time");
		return;
	}

	preprocess(is_algorithm_in_kernel_stage);

    /* For each remote process,  pack the data and then send out the data */
    local_comm = compset_communicators_info_mgr->get_current_comp_comm_group();
    for (i = 0, offset = 0; i < num_remote_procs; i ++) {
        if (send_size_with_remote_procs[i] == 0)
            continue;
        old_offset = offset;
        remote_proc_begin_pos_in_buffer = offset;
        for (m = 0; m < num_send_fields; m ++)
            if (currently_transferred_fields_mark[m])
                pack_MD_data(i, m, &offset, buffer_size);
        EXECUTION_REPORT(REPORT_ERROR, offset-old_offset == send_size_with_remote_procs[i], "C-Coupler error1 in sendrecv_data\n");        
        mpi_ierr = MPI_Isend((char *)mpi_send_buf+remote_proc_begin_pos_in_buffer, send_size_with_remote_procs[i], MPI_CHAR, i, comm_tag, local_comm, (send_requests+i));
		check_mpi_error("MPI error of MPI_Isend in sendrecv_data of runtime_transfer_algorithm");
    }

    /* Receive data from each remote process */
    for(i = 0, offset = 0; i < num_remote_procs; i ++) {
        if (recv_size_with_remote_procs[i] == 0)
            continue;        
        mpi_ierr = MPI_Irecv(((char *)mpi_recv_buf+offset), recv_size_with_remote_procs[i], MPI_CHAR, i, comm_tag, local_comm, (recv_requests+i));        
		check_mpi_error("MPI error of MPI_Irecv in sendrecv_data of runtime_transfer_algorithm");
        offset += recv_size_with_remote_procs[i];
    }

	performance_timing_mgr->performance_timing_start(TIMING_TYPE_COMMUNICATION, TIMING_COMMUNICATION_SENDRECV, compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), NULL);
    for (i = 0, offset = 0; i < num_remote_procs; i ++) 
        if (send_size_with_remote_procs[i] > 0) {
            mpi_ierr = MPI_Wait(&send_requests[i], &send_statuses[i]);
			check_mpi_error("MPI error of MPI_Wait for sending in sendrecv_data of runtime_transfer_algorithm");
        }
    for(i = 0; i < num_remote_procs; i ++)
        if (recv_size_with_remote_procs[i] > 0) {
            mpi_ierr = MPI_Wait(&recv_requests[i], &recv_statuses[i]);	
			check_mpi_error("MPI error of MPI_Wait for receiving in sendrecv_data of runtime_transfer_algorithm");
        }
	performance_timing_mgr->performance_timing_stop(TIMING_TYPE_COMMUNICATION, TIMING_COMMUNICATION_SENDRECV, compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), NULL);

    /* Unpack the data received*/
    for(i = 0, offset = 0; i < num_remote_procs; i ++) {
        old_offset = offset;
        if (recv_size_with_remote_procs[i] == 0)
            continue;
        for (m = num_send_fields; m < num_transfered_fields; m ++) 
            if (currently_transferred_fields_mark[m])
                unpack_MD_data(i, m, buffer_size, fields_routers[m]->get_remap_decomp_size(), &offset);
        EXECUTION_REPORT(REPORT_ERROR, offset-old_offset == recv_size_with_remote_procs[i], "C-Coupler software error2 in sendrecv_data\n");
    }

    EXECUTION_REPORT(REPORT_LOG, true, "after sending/receiving data at %ld", timer_mgr->get_current_full_time());
}


void Runtime_transfer_algorithm::initialize_local_data_structures()
{
    num_remote_procs = compset_communicators_info_mgr->get_num_procs_in_comp(compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name));
    fields_data_type_sizes = new int [num_transfered_fields];
    transferred_fields_data_buffers = new void* [num_transfered_fields];
    currently_transferred_fields_mark = new bool [num_transfered_fields];
    field_grids_num_lev = new long [num_transfered_fields];
    send_size_with_remote_procs = new int [num_remote_procs];
    recv_size_with_remote_procs = new int [num_remote_procs];
    fields_timers = new Coupling_timer* [num_transfered_fields];
    fields_routers = new Routing_info* [num_transfered_fields];
	comp_names = new char* [num_transfered_fields];
	field_names = new char* [num_transfered_fields];
    field_local_decomp_names = new char* [num_transfered_fields];
    field_remote_decomp_names = new char* [num_transfered_fields];
    field_grid_names = new char* [num_transfered_fields];
	buf_marks = new int [num_transfered_fields];
    transferred_fields_mem = new Field_mem_info *[num_transfered_fields];
    for (int i = 0; i < num_transfered_fields; i ++) {
        currently_transferred_fields_mark[i] = false;
		comp_names[i] = new char [NAME_STR_SIZE];
		field_names[i] = new char [NAME_STR_SIZE];
        field_local_decomp_names[i] = new char [NAME_STR_SIZE];
        field_remote_decomp_names[i] = new char [NAME_STR_SIZE];
        field_grid_names[i] = new char [NAME_STR_SIZE];
		transferred_fields_mem[i] = NULL;
		fields_data_type_sizes[i] = 0;
    }

    send_requests = new MPI_Request[num_remote_procs];
    send_statuses = new MPI_Status[num_remote_procs];
    recv_requests = new MPI_Request[num_remote_procs];
    recv_statuses = new MPI_Status[num_remote_procs];
	
    buffer_size = 0;
    mpi_send_buf = NULL;
    mpi_recv_buf = NULL;
}


void Runtime_transfer_algorithm::check_cfg_info_consistency()
{
    char local_line[NAME_STR_SIZE * 16], remote_line[NAME_STR_SIZE * 16], comp_name[NAME_STR_SIZE];
    FILE *fp_cfg;
    char *local_line_p, *remote_line_p;
    char remote_field_name[NAME_STR_SIZE];
	char frequency_unit[NAME_STR_SIZE], remote_frequency_unit[NAME_STR_SIZE];
	char remote_field_local_decomp_name[NAME_STR_SIZE], remote_field_remote_decomp_names[NAME_STR_SIZE], remote_field_grid_name[NAME_STR_SIZE];
	int frequency_count, remote_frequency_count;
    int i, j, remote_buf_mark;
	int remote_comp_id, remote_num_transfered_fields;
    

	if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() != 0) {
		MPI_Barrier(compset_communicators_info_mgr->get_current_comp_comm_group());
		return;
	}

	EXECUTION_REPORT(REPORT_LOG, true, "begin checking consistency of transfer configuration fields information in %s with remote component %s", transfer_fields_cfg_file, remote_comp_name);

    /* Allocate and initialize the information arrays for runtime transfer algorithm */
    num_transfered_fields = get_num_fields_in_config_file(transfer_fields_cfg_file, RUNTIME_TRANSFER_ALG_DIR);

	/* Check consistency of configuration files between two components */
	fp_cfg = open_config_file(transfer_fields_cfg_file, RUNTIME_TRANSFER_ALG_DIR);
	remote_comp_id = compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name);
	MPI_Sendrecv(&num_transfered_fields, 1, MPI_INT, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comp_id, 0), comm_tag,
			 	 &remote_num_transfered_fields, 1, MPI_INT, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comp_id, 0), comm_tag,
				 compset_communicators_info_mgr->get_global_comm_group(), send_statuses);
	EXECUTION_REPORT(REPORT_ERROR, num_transfered_fields == remote_num_transfered_fields, 
		             "the number of transfered fields in configuration file %s is different in the number in the counterpart configuration file",
		             transfer_fields_cfg_file);
	for(i = 0; i < num_transfered_fields; i ++) {
		get_next_line(local_line, fp_cfg);
		MPI_Sendrecv(local_line, NAME_STR_SIZE*16, MPI_CHAR, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comp_id, 0), comm_tag,
				 	 remote_line, NAME_STR_SIZE*16, MPI_CHAR, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comp_id, 0), comm_tag,
					 compset_communicators_info_mgr->get_global_comm_group(), send_statuses);
		local_line_p = local_line;
		remote_line_p = remote_line;
		get_next_attr(comp_names[i], &local_line_p);
		get_next_attr(field_names[i], &local_line_p);
		get_next_attr(field_local_decomp_names[i], &local_line_p);
		get_next_attr(field_remote_decomp_names[i], &local_line_p);
		get_next_attr(field_grid_names[i], &local_line_p);
		buf_marks[i] = get_next_integer_attr(&local_line_p);
		get_next_attr(frequency_unit, &local_line_p);
		frequency_count = get_next_integer_attr(&local_line_p);
		get_next_attr(comp_name, &remote_line_p);
		get_next_attr(remote_field_name, &remote_line_p);
		get_next_attr(remote_field_local_decomp_name, &remote_line_p);
		get_next_attr(remote_field_remote_decomp_names, &remote_line_p);
		get_next_attr(remote_field_grid_name, &remote_line_p);
		remote_buf_mark = get_next_integer_attr(&remote_line_p);
		get_next_attr(remote_frequency_unit, &remote_line_p);
		remote_frequency_count = get_next_integer_attr(&remote_line_p);
		EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(field_names[i], remote_field_name), 
			             "the field name %s in configuration file %s is not the same as the field name %s in the counterpart configuration file",
			             field_names[i], transfer_fields_cfg_file, remote_field_name);
		EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(field_local_decomp_names[i], remote_field_remote_decomp_names) && words_are_the_same(field_remote_decomp_names[i], remote_field_local_decomp_name), 
			             "the decomposition names %s and %s in configuration file %s are not consistent with the names in the counterpart configuration file",
			             field_names[i], remote_field_name, transfer_fields_cfg_file);
		if (words_are_the_same(field_grid_names[i],"NULL") || words_are_the_same(remote_field_grid_name,"NULL"))
			EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(field_grid_names[i],"NULL") && words_are_the_same(remote_field_grid_name,"NULL"), 
				             "the grids for field %s in configuration file %s and in the counterpart configuration file must both be \"NULL\"",
				             field_names[i], transfer_fields_cfg_file);	
		else EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(field_grid_names[i])->get_grid_size() == remap_grid_manager->search_remap_grid_with_grid_name(remote_field_grid_name)->get_grid_size(), 
			                 "the size of grid %s in configuration file %s is not the same as the size of grid %s in the counterpart configuration file",
			                 field_grid_names[i], transfer_fields_cfg_file, remote_field_grid_name);
		EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(frequency_unit, remote_frequency_unit), 
			             "the frequency unit %s of field %s in configuration file %s is not the same as the frequency unit in the counterpart configuration file",
			             frequency_unit, field_names[i], transfer_fields_cfg_file);
		EXECUTION_REPORT(REPORT_ERROR, frequency_count == remote_frequency_count, 
			             "the frequency count of field %s in configuration file %s is not the same as the frequency count in the counterpart configuration file",
			             field_names[i], transfer_fields_cfg_file);
	}
	fclose(fp_cfg);
	
	EXECUTION_REPORT(REPORT_LOG, true, "finish checking consistency of transfer configuration fields information in %s with remote component %s", transfer_fields_cfg_file, remote_comp_name);

	MPI_Barrier(compset_communicators_info_mgr->get_current_comp_comm_group());
}


void Runtime_transfer_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
    int i, j;


	fields_allocated = false;

	num_timer_on_fields = 0;
    for (i = 0; i < num_transfered_fields; i ++) {
		currently_transferred_fields_mark[i] = (!is_algorithm_in_kernel_stage || fields_timers[i]->is_timer_on());
        if (currently_transferred_fields_mark[i])
			num_timer_on_fields ++;
    }
	if (num_timer_on_fields == 0)
		return;

	if (num_send_fields > 0 && num_recv_fields > 0)
		return;

	if (restart_mgr->is_in_restart_read_time_window())
		return;

	for (i = 0; i < num_transfered_fields; i ++)
		if (currently_transferred_fields_mark[i] && transferred_fields_mem[i] == NULL)
			break;

	if (num_recv_fields > 0) {
		if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
			MPI_Recv(fields_transfer_info_string, num_transfered_fields*10, MPI_CHAR, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), 0), 
				 	 comm_tag, compset_communicators_info_mgr->get_global_comm_group(), &recv_statuses[0]);
			for (i = 0; i < num_transfered_fields; i ++) {
				EXECUTION_REPORT(REPORT_ERROR, !currently_transferred_fields_mark[i] && fields_transfer_info_string[i*10] == 0 || currently_transferred_fields_mark[i] && fields_transfer_info_string[i*10] == 1,
				                 "Please check configuration file %s: mismatch happens between components %s and %s when transferring %dth field", transfer_fields_cfg_file, compset_communicators_info_mgr->get_current_comp_name(), remote_comp_name, i);
			}
		}
		if (i <= num_transfered_fields)
			MPI_Bcast(fields_transfer_info_string, num_transfered_fields*10, MPI_CHAR, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
	}

	/* Allocate the memory buffer for each field */
	if (i <= num_transfered_fields) {
	    for(i = 0; i < num_transfered_fields; i ++) {
			if (!(currently_transferred_fields_mark[i] && transferred_fields_mem[i] == NULL))
				continue;
			if (num_send_fields > 0) {
		        transferred_fields_mem[i] = alloc_mem(comp_names[i], field_local_decomp_names[i], field_grid_names[i], field_names[i], NULL, buf_marks[i], true);
				add_runtime_datatype_transformation(transferred_fields_mem[i], true, fields_timers[i]);
				strcpy(fields_transfer_info_string+10*i+1, transferred_fields_mem[i]->get_field_data()->get_grid_data_field()->data_type_in_application);
			}
			else {
				transferred_fields_mem[i] = alloc_mem(comp_names[i], field_local_decomp_names[i], field_grid_names[i], field_names[i], fields_transfer_info_string+10*i+1, buf_marks[i], false);	
				add_runtime_datatype_transformation(transferred_fields_mem[i], false, fields_timers[i]);
			}
	        fields_data_type_sizes[i] = get_data_type_size(transferred_fields_mem[i]->get_field_data()->get_grid_data_field()->data_type_in_application);
	        transferred_fields_data_buffers[i] =  transferred_fields_mem[i]->get_data_buf();
	    }	    
	}

	if (num_send_fields > 0) {
		if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
			for (i = 0; i < num_transfered_fields; i ++)
				if (currently_transferred_fields_mark[i])
					fields_transfer_info_string[i*10] = 1;
				else fields_transfer_info_string[i*10] = 0;
			MPI_Send(fields_transfer_info_string, num_transfered_fields*10, MPI_CHAR, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name), 0), 
					 comm_tag, compset_communicators_info_mgr->get_global_comm_group());
		}
	}

	fields_allocated = true;
}


void Runtime_transfer_algorithm::generate_algorithm_info_from_cfg_file()
{
    char comm_direction[NAME_STR_SIZE * 16];
    char line[NAME_STR_SIZE * 16];    
    FILE *fp_cfg;
    char *local_line;
    int i, j;
    

    fp_cfg = open_config_file(algorithm_cfg_name, RUNTIME_TRANSFER_ALG_DIR);

    get_next_line(comm_direction, fp_cfg);

    /* Set comm_tag and transfer_fields_cfg_file according to config file */
    get_next_line(remote_comp_name, fp_cfg);
    get_next_line(line, fp_cfg);
    sscanf(line, "%d", &comm_tag);
    get_next_line(transfer_fields_cfg_file, fp_cfg);
    fclose(fp_cfg);
    
    /* Allocate and initialize the information arrays for runtime transfer algorithm */
    num_transfered_fields = get_num_fields_in_config_file(transfer_fields_cfg_file, RUNTIME_TRANSFER_ALG_DIR);

    initialize_local_data_structures();

    fp_cfg = open_config_file(transfer_fields_cfg_file, RUNTIME_TRANSFER_ALG_DIR);

    for(i = 0; i < num_transfered_fields; i ++) {
        get_next_line(line, fp_cfg);
        local_line = line;        
        get_next_attr(comp_names[i], &local_line);
        get_next_attr(field_names[i], &local_line);
        get_next_attr(field_local_decomp_names[i], &local_line);
        get_next_attr(field_remote_decomp_names[i], &local_line);
        get_next_attr(field_grid_names[i], &local_line);
        buf_marks[i] = get_next_integer_attr(&local_line);
		fields_timers[i] = new Coupling_timer(local_line);
    }
    
    fclose(fp_cfg);

	comps_transfer_time_info = timer_mgr->allocate_comp_transfer_time_info(compset_communicators_info_mgr->get_comp_id_by_comp_name(remote_comp_name));

	if (words_are_the_same(comm_direction, "send")) {
		num_send_fields = num_transfered_fields;
		num_recv_fields = 0;
	}
	else if  (words_are_the_same(comm_direction, "recv")) {
		num_send_fields = 0;
		num_recv_fields = num_transfered_fields;
	}
	else EXECUTION_REPORT(REPORT_ERROR, false, "No send or recv in the transfer config file %s\n", algorithm_cfg_name);

	fields_transfer_info_string = new char [num_transfered_fields*10];
}


void Runtime_transfer_algorithm::exchange_comp_time_info()
{
    int i;
    long send_buffer[2], recv_buffer[2];
    long local_comp_time, local_comp_frequency;
    int remote_comp_id;


    for (i = 0; i < num_transfered_fields; i ++)
        if (currently_transferred_fields_mark[i])
            break;
    if (i == num_transfered_fields)
        return;

    local_comp_time = timer_mgr->get_current_full_time();
    local_comp_frequency = timer_mgr->get_comp_frequency();
    send_buffer[0] = local_comp_time;
    send_buffer[1] = local_comp_frequency;

    remote_comp_id = comps_transfer_time_info->remote_comp_id;
    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
        MPI_Sendrecv(send_buffer, 2, MPI_LONG, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comp_id, 0), comm_tag,
                     recv_buffer, 2, MPI_LONG, compset_communicators_info_mgr->get_proc_id_in_global_comm_group(remote_comp_id, 0), comm_tag,
                     compset_communicators_info_mgr->get_global_comm_group(), send_statuses);
    MPI_Bcast(recv_buffer, 2, MPI_LONG, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
    comps_transfer_time_info->remote_comp_time = recv_buffer[0];
    comps_transfer_time_info->remote_comp_frequency = recv_buffer[1];
    comps_transfer_time_info->local_comp_time = timer_mgr->get_current_full_time();
    comps_transfer_time_info->counter ++;

    EXECUTION_REPORT(REPORT_LOG, true, "exchange time info: with remote component %s : %ld (%d) %ld: %d", remote_comp_name, comps_transfer_time_info->remote_comp_time,
                     comps_transfer_time_info->remote_comp_frequency, comps_transfer_time_info->local_comp_time, comps_transfer_time_info->counter);
}

