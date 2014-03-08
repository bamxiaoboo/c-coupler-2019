/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef COMM_MGT_H
#define COMM_MGT_H

#include "common_utils.h"
#include "io_netcdf.h"
#include <vector>


#define COMP_TYPE_CPL              "cpl"
#define COMP_TYPE_ATM              "atm"
#define COMP_TYPE_OCN              "ocn"
#define COMP_TYPE_LND              "lnd"
#define COMP_TYPE_SEA_ICE          "sice"
#define COMP_TYPE_WAVE             "wave"


struct Component_communicator_info
{
    char comp_name[NAME_STR_SIZE];                // The name of component
    char comp_type[NAME_STR_SIZE];
    int comp_id;                                    // The id of component. Each component has a unique id
    int num_comp_procs;                            // The number of processes to run component
    int *comp_procs_global_ids;                    // The id of each process of component in global communication group
};


class Compset_communicators_info_mgt
{
    private:
        int global_comm_group;                    // The global communication group
        int current_comp_comm_group;            // The communication group of current component
        int current_comp_id;                        // The component id of current component
        int current_proc_local_id;                    // The id of current process in the communication group of current component
        std::vector<Component_communicator_info *> comps_comms_info;    // The array to record the infomation of all components
        char experiment_model[NAME_STR_SIZE];
        char original_case_name[NAME_STR_SIZE];
        char current_case_name[NAME_STR_SIZE];
        char current_case_desc[NAME_STR_SIZE];
        char running_case_mode[NAME_STR_SIZE];
        char comp_namelist_filename[NAME_STR_SIZE];
		char current_config_time[NAME_STR_SIZE];
		char original_config_time[NAME_STR_SIZE];

        void load_in_compset(const char*, const char*);
        void build_compset_communicators_info();

    public:
        Compset_communicators_info_mgt(const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*);
        ~Compset_communicators_info_mgt();

        int get_comp_id_by_comp_name(const char*);
		int get_global_comm_group() { return global_comm_group; }
        int get_current_comp_id() { return current_comp_id;}
        int get_current_proc_id_in_comp_comm_group() { return current_proc_local_id;}
        int get_current_comp_comm_group() {return current_comp_comm_group;}
        int get_proc_id_in_global_comm_group(int, int);
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
        void write_case_info(IO_netcdf*);
};

#endif
