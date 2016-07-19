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
#define COMP_TYPE_SEA_ICE          "sice"
#define COMP_TYPE_WAVE             "wave"
#define COMP_TYPE_CESM             "cesm"
#define NULL_COMM                  ((int)-1)


class Comp_comm_group_mgt_node
{
	private:
		int comp_id;
	    char comp_name[NAME_STR_SIZE];                // The name of component
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

	public:
		Comp_comm_group_mgt_node(const char*, const char*, int, Comp_comm_group_mgt_node*, MPI_Comm&);
		Comp_comm_group_mgt_node(Comp_comm_group_mgt_node*, Comp_comm_group_mgt_node*, int &);
		~Comp_comm_group_mgt_node();
		MPI_Comm get_comm_group() const { return comm_group; }
		int get_current_proc_local_id() const { return current_proc_local_id; }
		void transform_node_into_array();
		void write_data_into_array_buffer(const void*, int);
		void read_data_from_array_buffer(void*, int);
		void merge_comp_comm_info();
		int get_buffer_content_size() { return buffer_content_size; }
		int get_buffer_content_iter() { return buffer_content_iter; }
		const char *get_comp_name() const { return comp_name; }
		const char *get_comp_type() const { return comp_type; }
		int get_num_children() { return children.size(); }
		int get_local_node_id() { return comp_id; }
		Comp_comm_group_mgt_node *get_child(int indx) { return children[indx]; }
		void reset_local_node_id(int new_id) { comp_id = new_id; }
		void reset_comm_group(int new_comm_group) { comm_group = new_comm_group; }
		void reset_current_proc_local_id(int new_current_proc_local_id) { current_proc_local_id = new_current_proc_local_id; }
		void reset_working_dir(const char *);
		bool is_definition_finalized() { return definition_finalized; }
		void print_global_nodes();
		const char *get_annotation_start() { return annotation_start; }
		const char *get_annotation_end() { return annotation_end; }
		Comp_comm_group_mgt_node *get_parent() const { return parent; }
		void write_node_into_XML(TiXmlElement *);
		const char *get_working_dir() const { return working_dir; }
		void update_child(const Comp_comm_group_mgt_node*, Comp_comm_group_mgt_node*);
		void transfer_data_buffer(Comp_comm_group_mgt_node*);
};


class Comp_comm_group_mgt_mgr
{
	private:
		std::vector<Comp_comm_group_mgt_node*> global_node_array;
		Comp_comm_group_mgt_node *global_node_root;
		bool definition_finalized;
		int current_proc_global_id;
        char experiment_model[NAME_STR_SIZE];
        char original_case_name[NAME_STR_SIZE];
        char current_case_name[NAME_STR_SIZE];
        char current_case_desc[NAME_STR_SIZE];
        char running_case_mode[NAME_STR_SIZE];
        char comp_namelist_filename[NAME_STR_SIZE];
		char current_config_time[NAME_STR_SIZE];
		char original_config_time[NAME_STR_SIZE];
		char executable_name[NAME_STR_SIZE];

	public:
		Comp_comm_group_mgt_mgr(const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*);
		~Comp_comm_group_mgt_mgr();
		int register_component(const char*, const char*, MPI_Comm&, int);
		void merge_comp_comm_info(int);
		bool is_legal_local_comp_id(int);
		bool is_local_comp_definition_finalized(int);
		void update_global_nodes(Comp_comm_group_mgt_node**, int);
		void transform_global_node_tree_into_array(Comp_comm_group_mgt_node*, Comp_comm_group_mgt_node**, int&);
		Comp_comm_group_mgt_node *get_global_node_of_local_comp(int);
		MPI_Comm get_comm_group_of_local_comp(int);
		const char *get_executable_name() { return executable_name; }
		const char *get_annotation_start() { return global_node_array[0]->get_annotation_start(); }
		void get_log_file_name(int, char*);
		Comp_comm_group_mgt_node *search_global_node(int);
		Comp_comm_group_mgt_node *search_global_node(const char*);
		void read_global_node_from_XML(const TiXmlElement*);
		void write_comp_comm_info_into_XML();
		void read_comp_comm_info_from_XML();
		bool get_is_definition_finalized() { return definition_finalized; }
};


struct Component_communicator_info
{
    char comp_name[NAME_STR_SIZE];                // The name of component
    char comp_type[NAME_STR_SIZE];
    int comp_id;                                    // The id of component. Each component has a unique id
    int num_comp_procs;                            // The number of processes to run component
    int *comp_procs_global_ids;                    // The id of each process of component in global communication group
    MPI_Comm comm_group;
	int current_proc_local_id;
};


class Compset_communicators_info_mgt
{
    private:
        MPI_Comm global_comm_group;                    // The global communication group
        MPI_Comm current_comp_comm_group;              // The communication group of current component
        MPI_Comm computing_node_comp_group;            // The communication group for the processes of the same component sharing on the same computing node
        int current_comp_id;                      // The component id of current component
        int current_proc_local_id;                // The id of current process in the communication group of current component
        std::vector<Component_communicator_info *> comps_comms_info;    // The array to record the infomation of all components
        char experiment_model[NAME_STR_SIZE];
        char original_case_name[NAME_STR_SIZE];
        char current_case_name[NAME_STR_SIZE];
        char current_case_desc[NAME_STR_SIZE];
        char running_case_mode[NAME_STR_SIZE];
        char comp_namelist_filename[NAME_STR_SIZE];
		char current_config_time[NAME_STR_SIZE];
		char original_config_time[NAME_STR_SIZE];
		char host_name_current_computing_node[NAME_STR_SIZE];

        void load_in_compset(const char*, const char*);
        void build_compset_communicators_info();

    public:
        Compset_communicators_info_mgt(const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*);
        ~Compset_communicators_info_mgt();

        int get_comp_id_by_comp_name(const char*);
		MPI_Comm get_global_comm_group() { return global_comm_group; }
        int get_current_comp_id() { return current_comp_id;}
        int get_current_proc_id_in_comp_comm_group() { return current_proc_local_id;}
        MPI_Comm get_current_comp_comm_group() {return current_comp_comm_group;}
        int get_proc_id_in_global_comm_group(int, int);
		int get_num_components() { return comps_comms_info.size(); }
        int get_num_procs_in_comp(int cid) { return comps_comms_info[cid]->num_comp_procs;}
        const char *get_current_comp_name() { return comps_comms_info[current_comp_id]->comp_name; }
        const char *get_comp_name_by_id(int cid) { return comps_comms_info[cid]->comp_name; }
        const char *get_current_comp_type() { return comps_comms_info[current_comp_id]->comp_type; }
        const char *get_current_case_name() { return current_case_name; }
		const char *get_original_case_name() { return original_case_name; }
        const char *get_current_case_desc() { return current_case_desc; }
        const char *get_running_case_mode() { return running_case_mode; }
		const char *get_current_config_time() { return current_config_time; }
		const char *get_original_config_time() { return original_config_time; }
		const char *get_host_computing_node_name() { return host_name_current_computing_node; }
		MPI_Comm get_computing_node_comp_group() { return computing_node_comp_group; }
        void write_case_info(IO_netcdf*);
		void register_component(const char*, const char*, MPI_Comm);
		Component_communicator_info *get_communicator_info_by_name(const char*);
};

#endif
