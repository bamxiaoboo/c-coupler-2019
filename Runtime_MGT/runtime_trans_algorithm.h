/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Cheng Zhang and then
  *  modified by Dr. Cheng Zhang and Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Cheng Zhang via zhangc-cess@tsinghua.edu.cn
  *  or Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef RUNTIME_TRANS
#define RUNTIME_TRANS
#include <vector>
#include "mpi.h"
#include "Runtime_Algorithm_Basis.h"
#include "routing_info_mgt.h"
#include "memory_mgt.h"
#include "timer_mgt.h"


class Runtime_trans_algorithm
{
    private:
		bool send_or_receive;    // true means send and false means receive
		int comp_id;
        int num_transfered_fields;
        Field_mem_info **fields_mem;
        void **fields_data_buffers;
        Routing_info **fields_routers;
        void * data_buf;
        long * tag_buf;
        MPI_Win data_win;
        MPI_Win tag_win;
        int data_buf_size;
        int tag_buf_size;
        int num_remote_procs;
		int num_local_procs;
        int * send_displs_in_remote_procs;
        int * recv_displs_in_current_proc;
        int * transfer_size_with_remote_procs;
		std::vector<int> index_remote_procs_with_common_data;
        int * fields_data_type_sizes;
        long * field_grids_num_lev;
		bool *transfer_process_on;
		long *current_remote_fields_time;
		long last_field_remote_recv_count;
		long current_field_local_recv_count;
		Time_mgt *time_mgr;
		long *last_receive_field_sender_time;
		long *current_receive_field_sender_time;
		long *current_receive_field_usage_time;
		std::vector<bool> history_receive_buffer_status;
		std::vector<long*> history_receive_sender_time;
		std::vector<long*> history_receive_usage_time;
		std::vector<void*> history_receive_data_buffer;
		int last_history_receive_buffer_index;
        Comp_comm_group_mgt_node * local_comp_node;
        Comp_comm_group_mgt_node * remote_comp_node;
	    int current_proc_local_id;
    	int current_proc_global_id;	
        MPI_Comm union_comm;
        int * remote_proc_ranks_in_union_comm;
        int current_proc_id_union_comm;
		bool sender_time_has_matched;

        bool send(bool);
        bool recv(bool);
		long get_receive_data_time();
        bool is_remote_data_buf_ready();
        bool set_remote_tags(bool);
        bool set_local_tags();
        void preprocess();
        void pack_MD_data(int, int, int *);
        void unpack_MD_data(void *, int, int, int *);
        template <class T> void pack_segment_data(T *, T *, int, int, int, int);
        template <class T> void unpack_segment_data(T *, T *, int, int, int, int);

    public:
        Runtime_trans_algorithm(bool, int, Field_mem_info **, Routing_info **, MPI_Comm, int *);
        ~Runtime_trans_algorithm();
        bool run(bool);
        void * get_data_buf() {return data_buf;}
        long * get_tag_buf() {return tag_buf;}
        int get_data_buf_size() {return data_buf_size;}
        int get_tag_buf_size() {return tag_buf_size;}
		void pass_transfer_parameters(std::vector<bool> &, std::vector<long> &);
        void set_data_win(MPI_Win win) {data_win = win;}
        void set_tag_win(MPI_Win win) {tag_win = win;}
		void receve_data_in_temp_buffer();
		long get_history_receive_sender_time(int);
};


#endif
