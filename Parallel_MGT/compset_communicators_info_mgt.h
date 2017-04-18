/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef COMM_MGT_H
#define COMM_MGT_H


#include <mpi.h>
#include "common_utils.h"
#include "io_netcdf.h"
#include "tinyxml.h"
#include <vector>


#define COMP_TYPE_CPL              "cpl"
#define COMP_TYPE_ATM              "atm"
#define COMP_TYPE_ATM_CHEM         "atm_chem"
#define COMP_TYPE_OCN              "ocn"
#define COMP_TYPE_LND              "lnd"
#define COMP_TYPE_SEA_ICE          "sea_ice"
#define COMP_TYPE_WAVE             "wave"
#define COMP_TYPE_ROOT             "ROOT"
#define NULL_COMM                  ((int)-1)


class Comp_comm_group_mgt_node
{
	private:
		int comp_id;
		int unified_global_id;
	    char comp_name[NAME_STR_SIZE];                // The name of component	
	    char full_name[NAME_STR_SIZE];
		char comp_type[NAME_STR_SIZE];
		char annotation_start[NAME_STR_SIZE];
		char annotation_end[NAME_STR_SIZE];
		Comp_comm_group_mgt_node *parent;
		std::vector<Comp_comm_group_mgt_node*> children;
		MPI_Comm comm_group;
		std::vector<int> local_processes_global_ids;
		int current_proc_local_id;
		int current_proc_global_id;
		char *temp_array_buffer;
		int buffer_content_size;
		int buffer_content_iter;
		int buffer_max_size;
		bool definition_finalized;
		char working_dir[NAME_STR_SIZE];
		char config_all_dir[NAME_STR_SIZE];
		char config_comp_dir[NAME_STR_SIZE];

	public:
		Comp_comm_group_mgt_node(const char*, const char*, int, Comp_comm_group_mgt_node*, MPI_Comm&, const char*);
		Comp_comm_group_mgt_node(Comp_comm_group_mgt_node*, Comp_comm_group_mgt_node*, int &);
		~Comp_comm_group_mgt_node();
		MPI_Comm get_comm_group() const { return comm_group; }
		int get_comp_id() const { return comp_id; }
		int get_unified_global_id() const { return unified_global_id;}
		void set_unified_global_id(int id) { unified_global_id = id; }
		int get_current_proc_local_id() const { return current_proc_local_id; }
		void transform_node_into_array();
		void merge_comp_comm_info(const char*);
		int get_buffer_content_size() { return buffer_content_size; }
		int get_buffer_content_iter() { return buffer_content_iter; }
		const char *get_comp_name() const { return comp_name; }
		const char *get_comp_type() const { return comp_type; }
		const char *get_comp_full_name() const { return full_name; }
		int get_num_children() { return children.size(); }
		int get_local_node_id() { return comp_id; }
		Comp_comm_group_mgt_node *get_child(int indx) { return children[indx]; }
		void reset_local_node_id(int new_id) { comp_id = new_id; }
		void reset_comm_group(int new_comm_group) { comm_group = new_comm_group; }
		void reset_current_proc_local_id(int new_current_proc_local_id) { current_proc_local_id = new_current_proc_local_id; }
		void reset_dir(Comp_comm_group_mgt_node *);
		bool is_definition_finalized() { return definition_finalized; }
		const char *get_annotation_start() { return annotation_start; }
		const char *get_annotation_end() { return annotation_end; }
		Comp_comm_group_mgt_node *get_parent() const { return parent; }
		void write_node_into_XML(TiXmlElement *);
		const char *get_working_dir() const { return working_dir; }
		const char *get_config_all_dir() const { return config_all_dir; }
		const char *get_config_comp_dir() const { return config_comp_dir; }
		void update_child(const Comp_comm_group_mgt_node*, Comp_comm_group_mgt_node*);
		void transfer_data_buffer(Comp_comm_group_mgt_node*);
		int get_num_procs() const { return local_processes_global_ids.size(); }
		int get_root_proc_global_id() const { return local_processes_global_ids[0]; }
		int get_local_proc_global_id(int);
		void confirm_coupling_configuration_active(int, const char*);
		const char *get_full_name() { return full_name; }
};


class Comp_comm_group_mgt_mgr
{
	private:
		std::vector<Comp_comm_group_mgt_node*> global_node_array;
		Comp_comm_group_mgt_node *global_node_root;
		bool definition_finalized;
		int current_proc_global_id;
		int num_total_global_procs;
        char experiment_model[NAME_STR_SIZE];
        char original_case_name[NAME_STR_SIZE];
        char current_case_name[NAME_STR_SIZE];
        char current_case_desc[NAME_STR_SIZE];
        char running_case_mode[NAME_STR_SIZE];
        char comp_namelist_filename[NAME_STR_SIZE];
		char current_config_time[NAME_STR_SIZE];
		char original_config_time[NAME_STR_SIZE];
		char executable_name[NAME_STR_SIZE];
		int *sorted_comp_ids;

	public:
		Comp_comm_group_mgt_mgr(const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*);
		~Comp_comm_group_mgt_mgr();
		int register_component(const char*, const char*, MPI_Comm&, int, const char*);
		void merge_comp_comm_info(int, const char*);
		bool is_legal_local_comp_id(int);
		void update_global_nodes(Comp_comm_group_mgt_node**, int);
		void transform_global_node_tree_into_array(Comp_comm_group_mgt_node*, Comp_comm_group_mgt_node**, int&);
		Comp_comm_group_mgt_node *get_global_node_of_local_comp(int, const char*);
		MPI_Comm get_comm_group_of_local_comp(int, const char*);
		const char *get_executable_name() { return executable_name; }
		const char *get_annotation_start() { return global_node_array[0]->get_annotation_start(); }
		void get_log_file_name(int, char*);
		void get_output_data_file_header(int, char*);
		Comp_comm_group_mgt_node *search_global_node(int);
		Comp_comm_group_mgt_node *search_global_node(const char*);
		void read_global_node_from_XML(const TiXmlElement*);
		void write_comp_comm_info_into_XML();
		void read_comp_comm_info_from_XML();
		bool get_is_definition_finalized() { return definition_finalized; }
		int get_current_proc_id_in_comp(int, const char *);
		int get_num_proc_in_comp(int, const char *);
		int get_current_proc_global_id() { return current_proc_global_id; }
		int get_num_total_global_procs() { return num_total_global_procs; }
		const char *get_root_working_dir() { return global_node_array[0]->get_working_dir(); }
		const char *get_config_all_dir() { return global_node_array[0]->get_config_all_dir(); }
		const char *get_config_root_comp_dir() { return global_node_array[1]->get_config_comp_dir(); }
		void confirm_coupling_configuration_active(int, int, const char*);
		const int *get_all_components_ids();
		void generate_sorted_comp_ids();
		const int *get_sorted_comp_ids() { return sorted_comp_ids; }
		Comp_comm_group_mgt_node *get_global_node_root() { return global_node_root; }
		bool has_comp_ended_configuration(const char*);
};


#endif
