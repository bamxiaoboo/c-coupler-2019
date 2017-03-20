/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef ORIGINAL_GRID_MGT
#define ORIGINAL_GRID_MGT


#include "remap_grid_class.h"
#include "remap_mgt.h"
#include <vector>


#define BOTTOM_FIELD_VARIATION_UNSET             ((int)-1)
#define BOTTOM_FIELD_VARIATION_STATIC            ((int) 0)
#define BOTTOM_FIELD_VARIATION_DYNAMIC           ((int) 1)
#define BOTTOM_FIELD_VARIATION_EXTERNAL          ((int) 2)


class Original_grid_info
{
	private: 
		int comp_id;
		int grid_id;
		char grid_name[NAME_STR_SIZE];
		char bottom_field_name[NAME_STR_SIZE];
		Remap_grid_class *original_CoR_grid;
		Remap_grid_class *H2D_sub_CoR_grid;
		Remap_grid_class *V1D_sub_CoR_grid;
		Remap_grid_class *T1D_sub_CoR_grid;
		int H2D_sub_grid_order;
		int V1D_sub_grid_order;
		int T1D_sub_grid_order;
		int bottom_field_id;
		int bottom_field_variation_type;   // 0: static; 1: dynamic; 2: external

		void generate_remapping_grids();
		
	public:
		Original_grid_info(int, int, const char*, const char*, Remap_grid_class*);
		const char *get_annotation();
		const char *get_grid_name() const { return grid_name; }
		int get_local_grid_id() const { return grid_id; }
		~Original_grid_info() {}
		int get_comp_id() const { return comp_id; }
		int get_grid_id() const { return grid_id; }
		int get_bottom_field_variation_type() const { return bottom_field_variation_type; }
		const char *get_bottom_field_name() const { return bottom_field_name; }
		void set_bottom_field_variation_type(int type) { bottom_field_variation_type = type; }
		void set_unique_bottom_field(int field_id, int type);
		Remap_grid_class *get_original_CoR_grid() const { return original_CoR_grid; }
		Remap_grid_class *get_H2D_sub_CoR_grid() { return H2D_sub_CoR_grid; }
		Remap_grid_class *get_V1D_sub_CoR_grid() { return V1D_sub_CoR_grid; }
		Remap_grid_class *get_T1D_sub_CoR_grid() { return T1D_sub_CoR_grid; }
		bool is_V1D_sub_grid_after_H2D_sub_grid();
		bool is_3D_grid() { return H2D_sub_CoR_grid != NULL && V1D_sub_CoR_grid != NULL && T1D_sub_CoR_grid == NULL; }
		bool is_H2D_grid() { return H2D_sub_CoR_grid != NULL && V1D_sub_CoR_grid == NULL && T1D_sub_CoR_grid == NULL; } 
		void write_grid_into_array(char **, int &, int &);
		int get_bottom_field_id() { return bottom_field_id; }
};


class Original_grid_mgt
{
	private:
		std::vector<Original_grid_info*> original_grids;
		char CoR_script_name[NAME_STR_SIZE];
		Remap_mgt *CoR_grids;

	public:
		Original_grid_mgt();
		~Original_grid_mgt();
		void check_for_grid_definition(int, const char*, const char*);
		int get_CoR_defined_grid(int, const char*, const char*, const char*);
		Original_grid_info *search_grid_info(const char*, int);
		Original_grid_info *search_grid_info(int);
		Remap_grid_class *get_original_CoR_grid(int) const;
		Original_grid_info *get_original_grid(int) const;
		bool is_grid_id_legal(int) const;		
		int get_comp_id_of_grid(int) const;
		const char *get_name_of_grid(int) const;
		int get_grid_size(int, const char*) const;
		int add_original_grid(int, const char*, Remap_grid_class*);
		int get_num_grid_levels(int);
		bool is_V1D_sub_grid_after_H2D_sub_grid(int);
		int register_H2D_grid_via_data(int, const char *, const char *, const char *, const char *, const char *, int, int, int, int, 
											   int, int, int, int, void *, void *, int *, void *, void *, void *, const char *, int);
		int register_H2D_grid_via_file(int, const char *, const char *, const char *);
		int register_H2D_grid_via_comp(int, const char *, const char *);
		int register_V1D_grid_via_data(int, const char *, const char *, const char *, int, double, const double *, const double *, double, const char *);
		int register_md_grid_via_multi_grids(int, const char*, int, int, int, const char*);
		void set_3d_grid_bottom_field(int, int, int, int, int, const char*, const char*);
};




#endif

