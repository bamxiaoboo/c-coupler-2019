/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef ROUTER_MGT_H
#define ROUTER_MGT_H


#include "common_utils.h" 
#include "decomp_info_mgt.h"
#include <vector>


struct Routing_info_with_one_process
{
    int remote_proc_global_id;
    int num_elements_transferred;
    int num_local_indx_segments;
    int *local_indx_segment_starts;
    int *local_indx_segment_lengths;
    bool send_or_recv;                           // true is send and false is recv
};


class Routing_info
{
    private: 
        char remote_comp_name[NAME_STR_SIZE];
        char local_decomp_name[NAME_STR_SIZE];
        char remote_decomp_name[NAME_STR_SIZE];
        int num_dimensions;
        int local_comm_id;
        int remote_comm_id;
        int total_num_transferred_cells;
        long local_decomp_size;
		long remap_decomp_size;
        std::vector<Routing_info_with_one_process *> remote_procs_routing_info;

    public:
        Routing_info(const char *, const char *, const char*);
        ~Routing_info();
        int get_true_routing_info_index(bool, int);
        int get_num_remote_procs() { return remote_procs_routing_info.size(); }
        int get_num_elements_transferred_with_remote_proc(bool, int);
        int *get_local_indx_segment_starts_with_remote_proc(bool is_send, int i) { return remote_procs_routing_info[get_true_routing_info_index(is_send,i)]->local_indx_segment_starts; }
        int *get_local_indx_segment_lengths_with_remote_proc(bool is_send, int i) { return remote_procs_routing_info[get_true_routing_info_index(is_send,i)]->local_indx_segment_lengths; }
        int get_num_local_indx_segments_with_remote_proc(bool is_send, int i) { return remote_procs_routing_info[get_true_routing_info_index(is_send,i)]->num_local_indx_segments; }
        bool match_router(const char*, const char*, const char*);
        int get_num_dimensions() { return num_dimensions; }
        long get_local_decomp_size() { return local_decomp_size; }
		long get_remap_decomp_size() { return remap_decomp_size; }
        
    private:
        void build_2D_remote_router(const char*);
        void build_2D_self_router(const char*, const char*);
        void compute_routing_info_between_decomps(int, const int*, int, const int*, int, int, int);
};


class Routing_info_mgt
{
    private:
        std::vector<Routing_info *> routers;
    
    public:
        Routing_info_mgt() {}
        ~Routing_info_mgt();
        Routing_info *search_router(const char*, const char*, const char*);
        Routing_info *search_or_add_router(const char*, const char*, const char*);
};

#endif

