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


class Runtime_trans_algorithm: public Runtime_algorithm_basis
{
    private:
        int num_transfered_fields;
        Field_mem_info **fields_mem;
        void **fields_data_buffers;
        Coupling_timer **fields_timers;
        Routing_info **fields_routers;
        void * data_buf;
        long * tag_buf;
        MPI_Win data_win;
        MPI_Win tag_win;
        int data_buf_size;
        int tag_buf_size;
        int num_remote_procs;
        bool fields_allocated;
        int * send_displs_in_remote_procs;
        int * recv_displs_in_current_proc;
        int * send_size_with_remote_procs;
        int * recv_size_with_remote_procs;
        int * fields_data_type_sizes;
        long * field_grids_num_lev;
		bool *transfer_process_on;
		long *current_remote_fields_time;
		long *last_remote_fields_time;
		Time_mgt *time_mgr;

        Comp_comm_group_mgt_node * local_comp_node;
        Comp_comm_group_mgt_node * remote_comp_node;
	    int current_proc_local_id;
    	int current_proc_global_id;	

        MPI_Comm union_comm;
        int * remote_proc_ranks_in_union_comm;
        int current_proc_id_union_comm;

        bool send(bool);
        bool recv(bool);
        bool sendrecv(bool);
        void initialize_local_data_structures();
        bool is_remote_data_buf_ready();
        bool is_local_data_buf_ready();
        bool set_remote_tags();
        bool set_local_tags();
        void preprocess();
        void pack_MD_data(int, int, int *);
        void unpack_MD_data(int, int, int *);
        template <class T> void pack_segment_data(T *, T *, int, int, int, int);
        template <class T> void unpack_segment_data(T *, T *, int, int, int, int);

    public:
        Runtime_trans_algorithm(int, int, Field_mem_info **, Routing_info **, Coupling_timer **, MPI_Comm, int *);
        ~Runtime_trans_algorithm();
        bool run(bool);
        void allocate_src_dst_fields(bool);
        MPI_Win * get_data_win() {return &data_win;}
        MPI_Win * get_tag_win() {return &tag_win;}
        void * get_data_buf() {return data_buf;}
        long * get_tag_buf() {return tag_buf;}
        int get_data_buf_size() {return data_buf_size;}
        int get_tag_buf_size() {return tag_buf_size;}
        void create_win();
		void pass_transfer_parameters(std::vector<bool> &, std::vector<long> &);
        void set_data_win(MPI_Win win) {data_win = win;}
        void set_tag_win(MPI_Win win) {tag_win = win;}
};


#endif
