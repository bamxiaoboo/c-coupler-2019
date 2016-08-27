/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "original_grid_mgt.h"


Original_grid_info::Original_grid_info(int comp_id, int grid_id, const char *grid_name, const char *annotation, Remap_grid_class *CoR_grid)
{
	this->comp_id = comp_id;
	this->grid_id = grid_id;
	this->CoR_grid = CoR_grid;
	strcpy(this->grid_name, grid_name);
	annotation_mgr->add_annotation(this->grid_id, "grid_registration", annotation);
}


const char *Original_grid_info::get_annotation()
{
	return annotation_mgr->get_annotation(this->grid_id, "grid_registration");
}


Original_grid_mgt::Original_grid_mgt(const char *script)
{
	original_grids.clear();
	strcpy(CoR_script_name, script);
	if (strlen(CoR_script_name) != 0)
		CoR_grids = new Remap_mgt(script);
	else CoR_grids = NULL;
}


Original_grid_mgt::~Original_grid_mgt()
{
	if (CoR_grids != NULL)
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
	Remap_grid_class *CoR_grid;

	
	check_for_grid_definition(comp_id, grid_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, CoR_grids != NULL, "cannot register the grid \"%s\" based on a CoR grid (the corresponding annotation is \"%s\") because there is no CoR script specified.", grid_name, annotation);
	CoR_grid = remap_grid_manager->search_remap_grid_with_grid_name(CoR_grid_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, CoR_grid != NULL, "cannot register the grid \"%s\" because the CoR script \"%s\" does not define a grid named \"%s\". Please check the model code with the annotation \"%s\" or the CoR script",
		             grid_name, CoR_script_name, CoR_grid_name, annotation);	
	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, annotation, CoR_grid));
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


Remap_grid_class *Original_grid_mgt::get_CoR_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_CoR_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_CoR_grid();
}


int Original_grid_mgt::get_grid_size(int grid_id, const char *annotation) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "The grid id used for getting grid size is wrong. Please verify the model code corresponding to the annotation \"%s\"", annotation);		
	return get_CoR_grid(grid_id)->get_grid_size();
}

