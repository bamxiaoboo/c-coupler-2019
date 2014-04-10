/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef DECOMP_GRID_MGT
#define DECOMP_GRID_MGT


#include "decomp_info_mgt.h"
#include "common_utils.h"
#include "cor_global_data.h"
#include <vector>


class Decomp_grid_info
{
    private:
        char decomp_name[NAME_STR_SIZE];
        Remap_grid_class *decomp_grid;
        Remap_grid_class *original_grid;

    public:
        Decomp_grid_info(const char*, Remap_grid_class*);
        ~Decomp_grid_info();
        Remap_grid_class *get_decomp_grid() { return decomp_grid; }
		Remap_grid_class *get_original_grid() { return original_grid; }
        bool match(const char*, Remap_grid_class*);
        void register_decomp_grid_fields();
};


class Decomp_grid_mgt
{
    private:
        std::vector<Decomp_grid_info*> decomp_grids_info;

    public:
        Decomp_grid_mgt() {}
        Decomp_grid_info *search_decomp_grid_info(const char*, Remap_grid_class*);
		void bcast_grid_area_or_volumn_intra_computing_node(Remap_grid_class*);
        ~Decomp_grid_mgt();
};

#endif
