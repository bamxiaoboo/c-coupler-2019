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


#define EDGE_TYPE_LONLAT          "lon_lat"
#define EDGE_TYPE_GREAT_ARC       "great_arc"
#define EDGE_TYPE_HYBRID          "hybrid"


class Original_grid_info
{
	private: 
		int local_comp_id;
		int local_grid_id;
		int global_grid_id;
		char grid_name[NAME_STR_SIZE];
		char annotation[NAME_STR_SIZE];
		Remap_grid_class *CoR_grid;
		
	public:
		Original_grid_info(int, int, const char*, const char*, Remap_grid_class*);
		const char *get_annotation() const { return annotation;  }
		const char *get_grid_name() const { return grid_name; }
		int get_local_grid_id() { return local_grid_id; }
		~Original_grid_info() {}
};


class Original_grid_mgt
{
	private:
		std::vector<Original_grid_info*> original_grids;
		char last_annotation[NAME_STR_SIZE];   // to improve
		char CoR_script_name[NAME_STR_SIZE];
		Remap_mgt *CoR_grids;

	public:
		Original_grid_mgt(const char*);
		~Original_grid_mgt();
		void check_for_grid_definition(int, const char*, const char*);
		int get_CoR_defined_grid(int, const char*, const char*, const char*);
		Original_grid_info *search_grid_info(const char*);
		
};




#endif

