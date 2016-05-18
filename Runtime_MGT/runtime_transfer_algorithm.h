/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef RUNTIME_TRANSFER
#define RUNTIME_TRANSFER

#include <mpi.h>
#include "Runtime_Algorithm_Basis.h"
#include "routing_info_mgt.h"
#include "remap_grid_class.h"
#include "timer_mgt.h"
#include "memory_mgt.h"


class Runtime_transfer_algorithm: public Runtime_algorithm_basis
{
    private:
        int *fields_data_type_sizes;
        int comm_tag;
        Coupling_timer **fields_timers;
        bool *currently_transferred_fields_mark;
        void *mpi_send_buf;
        void *mpi_recv_buf;
        char local_transfer_fields_cfg_file[NAME_STR_SIZE];
		char remote_transfer_fields_cfg_file[NAME_STR_SIZE];
        char remote_comp_name[NAME_STR_SIZE];
        MPI_Request *send_requests;
        MPI_Status *send_statuses;
        MPI_Request *recv_requests;
        MPI_Status *recv_statuses;
        char **field_remote_decomp_names;
        Routing_info **fields_routers;
        long *field_grids_num_lev;
        int *send_size_with_remote_procs;
        int *recv_size_with_remote_procs;
        int num_remote_procs;
        int buffer_size;
        Comps_transfer_time_info *comps_transfer_time_info;
        int num_transfered_fields;
        Field_mem_info **transferred_fields_mem;
        void **transferred_fields_data_buffers;
		int num_timer_on_fields;
		int mpi_ierr;
		char *fields_transfer_info_string;
		bool is_remapping_rearrange_algorithm;

        void pack_MD_data(long, long, int*, int);
        void unpack_MD_data(int, int, int, long, int*);
        void send_data(bool);
        void recv_data(bool);
        void sendrecv_data(bool);
        void initialize_local_data_structures();
        void generate_algorithm_info_from_cfg_file();
        void exchange_comp_time_info();
		void preprocess(bool);
		void check_mpi_error(const char*);
		void check_cfg_info_consistency();
        
    public:
        Runtime_transfer_algorithm(const char *);
        Runtime_transfer_algorithm(int, Field_mem_info**, Routing_info*, Coupling_timer*);
        ~Runtime_transfer_algorithm();
        void run(bool);
		void allocate_src_dst_fields(bool);
};

#endif

