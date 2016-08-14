/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef DECOMP_INFO
#define DECOMP_INFO

#include "common_utils.h"
#include "remap_grid_class.h"
#include <vector>


class Decomp_info
{
    private:
        char grid_name[NAME_STR_SIZE];
        char model_name[NAME_STR_SIZE];
        int num_global_cells;
        int num_local_cells;
        int *local_cell_global_indx;
		bool is_registered;

		char decomp_name[NAME_STR_SIZE];
		int comp_id;
		int grid_id;
		int decomp_id;

    public:
		Decomp_info(const char *, int, int, int, const int *, const char *);
        Decomp_info(const char *, const char*, const char*, int, const int *);
        ~Decomp_info();
        int get_num_local_cells() { return num_local_cells; }
        const int *get_local_cell_global_indx() { return local_cell_global_indx; }
        const char *get_grid_name() { return grid_name; }
        const char *get_decomp_name() { return decomp_name; }
		int get_grid_id() { return grid_id; }
		int get_comp_id() { return comp_id; }
		int get_decomp_id() { return decomp_id; }
        void gen_decomp_grid_data();
        int get_num_global_cells() { return num_global_cells; }
        const char *get_model_name() { return model_name; }
        bool get_is_fully_grid_decomp() { return num_local_cells == num_global_cells; }
		bool is_registered_decomp() { return is_registered; }
		void set_decomp_registered() { is_registered = true; };
		void check_local_cell_global_indx();
};


class Decomp_info_mgt
{
    private:
        std::vector<Decomp_info *> decomps_info;
        
    public:
        Decomp_info_mgt() {}
        ~Decomp_info_mgt();
        Decomp_info *search_decomp_info(const char *);
        void add_decomps_from_cfg_file(const char*);
        void add_decomp_from_model_interface(const char*, const char*, const char*, int, int*);
        Decomp_info *generate_fully_decomp(const char*, const char*);
        Decomp_info *generate_remap_weights_src_decomp(const char*, const char*, const char*);
		int search_decomp_info(const char*, int);
		int register_H2D_parallel_decomposition(const char *, int, int, const int *, const char *);
		bool is_decomp_id_legal(int) const;
		int get_comp_id_of_decomp(int) const;
		Remap_grid_class *get_CoR_grid_of_decomp(int) const;
		Decomp_info *get_decomp_info(int);
};


#endif
