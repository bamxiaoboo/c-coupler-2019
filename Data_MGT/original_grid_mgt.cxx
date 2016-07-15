/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "original_grid_mgt.h"


Original_grid_info::Original_grid_info(int comp_id, int local_grid_id, const char *grid_name, const char *annotation, Remap_grid_class *CoR_grid)
{
	this->local_comp_id = comp_id;
	this->local_grid_id = local_grid_id;
	this->global_grid_id = -1;
	this->CoR_grid = CoR_grid;
	strcpy(this->grid_name, grid_name);
	strcpy(this->annotation, annotation);
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


Original_grid_info *Original_grid_mgt::search_grid_info(const char *grid_name)
{
	for (int i = 0; i < original_grids.size(); i ++)
		if (words_are_the_same(original_grids[i]->get_grid_name(), grid_name))
			return original_grids[i];

	return NULL;
}


void Original_grid_mgt::check_for_grid_definition(int comp_id, const char *grid_name, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "The component id is wrong when defining a grid \"%s\". Please check the model code with the annotation \"%s\"", grid_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, !comp_comm_group_mgt_mgr->is_local_comp_definition_finalized(comp_id), "grid \"%s\" cannot be registered (the corresponding annotation is \"%s\") because the coupling registration component \"%s\" has been ended (the corresponding annotation is \"%s\"). Please check.",
					 grid_name, annotation, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id)->get_comp_name(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id)->get_annotation_end());
	if (search_grid_info(grid_name) != NULL)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "grid \"%s\" has been defined in component \"%s\" before. Please check the model code with the annoation \"%s\" and \"%s\"",
		                 grid_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id)->get_comp_name(), search_grid_info(grid_name)->get_annotation(), annotation);
}


int Original_grid_mgt::get_CoR_defined_grid(int comp_id, const char *grid_name, const char *CoR_grid_name, const char *annotation)
{
	Remap_grid_class *CoR_grid;

	
	check_for_grid_definition(comp_id, grid_name, annotation);
	synchronize_comp_processes_for_API(comp_id, API_ID_GRID_MGT_REG_GRID_VIA_COR, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id), "registering a grid based on a CoR grid", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, CoR_grids != NULL, "cannot register the grid \"%s\" based on a CoR grid (the corresponding annotation is \"%s\") because there is no CoR script specified.", grid_name, annotation);
	CoR_grid = remap_grid_manager->search_remap_grid_with_grid_name(CoR_grid_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, CoR_grid != NULL, "cannot register the grid \"%s\" because the CoR script \"%s\" does not define a grid named \"%s\". Please check the model code with the annotation \"%s\" or the CoR script",
		             grid_name, CoR_script_name, CoR_grid_name, annotation);	
	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, annotation, CoR_grid));
	return original_grids[original_grids.size()-1]->get_local_grid_id();
}


