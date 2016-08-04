/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef ORIGINAL_GRID_MGT
#define ORIGINAL_GRID_MGT


#include "original_grid_mgt.h"
#include "remap_grid_class.h"
#include <vector>


class Original_grid_info
{
	private: 
		int comp_id;
		int grid_id;
		char grid_name[NAME_STR_SIZE];
		Remap_grid_class *CoR_grid;
		
	public:
		Original_grid_info(int, int, const char*, const char*, Remap_grid_class*);
		const char *get_annotation();
		const char *get_grid_name() const { return grid_name; }
		int get_local_grid_id() const { return grid_id; }
		~Original_grid_info() {}
		int get_comp_id() const { return comp_id; }
		Remap_grid_class *get_CoR_grid() { return CoR_grid; }
};


class Original_grid_mgt
{
	private:
		std::vector<Original_grid_info*> original_grids;
		char CoR_script_name[NAME_STR_SIZE];
		Remap_mgt *CoR_grids;

	public:
		Original_grid_mgt(const char*);
		~Original_grid_mgt();
		void check_for_grid_definition(int, const char*, const char*);
		int get_CoR_defined_grid(int, const char*, const char*, const char*);
		Original_grid_info *search_grid_info(const char*);
		Original_grid_info *search_grid_info(int);
		bool is_grid_id_legal(int) const;		
		int get_comp_id_of_grid(int);
		const char *get_name_of_grid(int) const;
};




#endif

