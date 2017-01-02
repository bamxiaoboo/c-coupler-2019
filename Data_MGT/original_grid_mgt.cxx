/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "original_grid_mgt.h"


Original_grid_info::Original_grid_info(int comp_id, int grid_id, const char *grid_name, const char *annotation, Remap_grid_class *original_CoR_grid)
{
	this->comp_id = comp_id;
	this->grid_id = grid_id;
	this->original_CoR_grid = original_CoR_grid;
	strcpy(this->grid_name, grid_name);
	annotation_mgr->add_annotation(this->grid_id, "grid_registration", annotation);
	generate_remapping_grids();
}


const char *Original_grid_info::get_annotation()
{
	return annotation_mgr->get_annotation(this->grid_id, "grid_registration");
}


void Original_grid_info::generate_remapping_grids()
{
	Remap_grid_class *lon_sub_grid = NULL, *lat_sub_grid = NULL, *leaf_grids[256];
	int num_leaf_grids;

	
	H2D_sub_CoR_grid = NULL;
	V1D_sub_CoR_grid = NULL;
	T1D_sub_CoR_grid = NULL;
	H2D_sub_grid_order = -1;
	V1D_sub_grid_order = -1;
	T1D_sub_grid_order = -1;
	
	if (original_CoR_grid->get_num_dimensions() == 2)
		EXECUTION_REPORT(REPORT_ERROR, -1, original_CoR_grid->get_is_sphere_grid(), "Software error in Original_grid_info::generate_remapping_grids: not a sphere grid");
	if (original_CoR_grid->get_num_dimensions() == 1)
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(original_CoR_grid->get_coord_label(),COORD_LABEL_LEV) || words_are_the_same(original_CoR_grid->get_coord_label(),COORD_LABEL_TIME), "Software error in Original_grid_info::generate_remapping_grids: not a vertical grid or not a time grid");
	H2D_sub_CoR_grid = original_CoR_grid->get_sphere_sub_grid();
	
	original_CoR_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, original_CoR_grid);
	for (int i = 0; i < num_leaf_grids; i ++) {
		if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LON)) {
			lon_sub_grid = leaf_grids[i];
			H2D_sub_grid_order = i;
		}
		else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LAT))
			lat_sub_grid = leaf_grids[i];
		else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LEV)) {
			V1D_sub_CoR_grid = leaf_grids[i];
			V1D_sub_grid_order = i;
		}
		else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_TIME)) {
			T1D_sub_CoR_grid = leaf_grids[i];
			T1D_sub_grid_order = i;
		}
	}

	if (H2D_sub_CoR_grid == NULL && (lon_sub_grid != NULL || lat_sub_grid != NULL)) {
		EXECUTION_REPORT(REPORT_ERROR, -1, lon_sub_grid != NULL && lat_sub_grid != NULL, "Software error in Original_grid_info::generate_remapping_grids: wrong sub grids for a sphere grid");
		leaf_grids[0] = lon_sub_grid;
		leaf_grids[1] = lat_sub_grid;
		char H2D_grid_name[NAME_STR_SIZE];
		sprintf(H2D_grid_name, "H2D_grid_for_%s", grid_name);
		H2D_sub_CoR_grid = new Remap_grid_class(H2D_grid_name, 2, leaf_grids, 0);
	}	

	EXECUTION_REPORT(REPORT_ERROR, -1, H2D_sub_CoR_grid != NULL || V1D_sub_CoR_grid != NULL || T1D_sub_CoR_grid != NULL, "Software error in Original_grid_info::generate_remapping_grids: empty grid");
}


bool Original_grid_info::is_V1D_sub_grid_after_H2D_sub_grid()
{
	if (H2D_sub_CoR_grid == NULL || V1D_sub_CoR_grid == NULL)
		return true;

	return H2D_sub_grid_order < V1D_sub_grid_order;
}


Original_grid_mgt::Original_grid_mgt()
{
	original_grids.clear();
	sprintf(CoR_script_name, "%s/CCPL_grid.cor", comp_comm_group_mgt_mgr->get_config_root_comp_dir());
	printf("CoR script is %s\n", CoR_script_name);
	FILE *fp = fopen(CoR_script_name, "r");
	if (fp == NULL)
		CoR_script_name[0] = '\0';
	else fclose(fp);
	printf("CoR script is %s\n", CoR_script_name);
	if (strlen(CoR_script_name) != 0)
		CoR_grids = new Remap_mgt(CoR_script_name);
	else CoR_grids = new Remap_mgt(NULL);
}


Original_grid_mgt::~Original_grid_mgt()
{
	delete CoR_grids;
}


Original_grid_info *Original_grid_mgt::search_grid_info(const char *grid_name, int comp_id)
{
	for (int i = 0; i < original_grids.size(); i ++)
		if (words_are_the_same(original_grids[i]->get_grid_name(), grid_name) && original_grids[i]->get_comp_id() == comp_id)
			return original_grids[i];

	return NULL;
}


Original_grid_info *Original_grid_mgt::search_grid_info(int grid_id)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "software error in Original_grid_mgt::search_grid_info based on grid_id");	
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK];
}


void Original_grid_mgt::check_for_grid_definition(int comp_id, const char *grid_name, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "The component id is wrong when defining a grid \"%s\". Please check the model code with the annotation \"%s\"", grid_name, annotation);
	if (search_grid_info(grid_name, comp_id) != NULL)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "grid \"%s\" has been defined in component \"%s\" before. Please check the model code with the annoation \"%s\" and \"%s\"",
		                 grid_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "C-Coupler code in check_for_grid_definition for getting component management node")->get_comp_name(), search_grid_info(grid_name, comp_id)->get_annotation(), annotation);
}


int Original_grid_mgt::get_CoR_defined_grid(int comp_id, const char *grid_name, const char *CoR_grid_name, const char *annotation)
{
	Remap_grid_class *original_CoR_grid;

	
	check_for_grid_definition(comp_id, grid_name, annotation);
	original_CoR_grid = remap_grid_manager->search_remap_grid_with_grid_name(CoR_grid_name);
	if (original_CoR_grid == NULL)
		if (strlen(CoR_script_name) > 0)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "cannot register the grid \"%s\" because the CoR script \"%s\" does not define a grid named \"%s\". Please check the model code with the annotation \"%s\" or the CoR script",
				             grid_name, CoR_script_name, CoR_grid_name, annotation);	
		else EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "cannot register the grid \"%s\" because the CoR script is not specified. Please verify.",
				              grid_name, CoR_script_name, CoR_grid_name, annotation);	
	EXECUTION_REPORT(REPORT_ERROR, comp_id, original_CoR_grid->format_sub_grids(original_CoR_grid), "Please modify the definition of grid \"%s\" in the CoR script \"%s\". We propose to order the dimensions of the grid into the order such as lon, lat, level and time");
	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, annotation, original_CoR_grid));
	return original_grids[original_grids.size()-1]->get_local_grid_id();
}


bool Original_grid_mgt::is_grid_id_legal(int grid_id) const
{
	int true_grid_id = grid_id & TYPE_ID_SUFFIX_MASK;


	if ((grid_id & TYPE_ID_PREFIX_MASK) != TYPE_GRID_LOCAL_ID_PREFIX)
		return false;

	if (true_grid_id < 0 || true_grid_id >= original_grids.size())
		return false;

	return true;
}


int Original_grid_mgt::get_comp_id_of_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_comp_id_of_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_comp_id();
}


const char *Original_grid_mgt::get_name_of_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_name_of_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_grid_name();
}


Remap_grid_class *Original_grid_mgt::get_original_CoR_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_original_CoR_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_original_CoR_grid();
}


Original_grid_info *Original_grid_mgt::get_original_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_original_CoR_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK];
}

int Original_grid_mgt::get_grid_size(int grid_id, const char *annotation) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "The grid id used for getting grid size is wrong. Please verify the model code corresponding to the annotation \"%s\"", annotation);		
	return get_original_CoR_grid(grid_id)->get_grid_size();
}


int Original_grid_mgt::add_original_grid(int comp_id, const char *grid_name, Remap_grid_class *original_CoR_grid)
{
	Original_grid_info *existing_grid = search_grid_info(grid_name, comp_id);
	if (existing_grid != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, original_CoR_grid == existing_grid->get_original_CoR_grid(), "Software error in Original_grid_mgt::add_original_grid");
		return existing_grid->get_local_grid_id();
	}

	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, "Original_grid_mgt::add_original_grid", original_CoR_grid));
	return original_grids[original_grids.size()-1]->get_local_grid_id();
}


int Original_grid_mgt::get_num_grid_levels(int grid_id)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_num_grid_levels: wrong grid id");		
	if (original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_V1D_sub_CoR_grid() == NULL)
		return 1;

	EXECUTION_REPORT(REPORT_ERROR, -1, original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_V1D_sub_CoR_grid()->get_grid_size() > 0, "Software error in Original_grid_mgt::get_num_grid_levels: wrong size of vertical grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_V1D_sub_CoR_grid()->get_grid_size();
}


bool Original_grid_mgt::is_V1D_sub_grid_after_H2D_sub_grid(int grid_id)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Original_grid_info::is_V1D_sub_grid_after_H2D_sub_grid: wrong grid id");		
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->is_V1D_sub_grid_after_H2D_sub_grid();
}

