/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "decomp_info_mgt.h"
#include "global_data.h"
#include "memory_mgt.h"
#include "cor_global_data.h"
#include "cor_cpl_interface.h"
#include <string.h>
      

Decomp_info::Decomp_info(const char *decomp_name, int decomp_id, int host_comp_id, int grid_id, int num_local_cells, const int *cell_indexes_in_decomp, const char *annotation, bool registered)
{
	Remap_grid_class *CoR_grid;
	int i;
	

	EXECUTION_REPORT(REPORT_ERROR, -1, original_grid_mgr->is_grid_id_legal(grid_id), "the grid_id for registering decomposition \"%s\" is wrong. Please check the model code related to the annotation \"%s\".", decomp_name, annotation);
	
	this->decomp_id = decomp_id;
	this->grid_id = grid_id;
	this->comp_id = original_grid_mgr->get_comp_id_of_grid(grid_id);
	if (host_comp_id == -1)
		host_comp_id = this->comp_id;
	this->host_comp_id = host_comp_id;
    this->num_global_cells = original_grid_mgr->get_original_CoR_grid(grid_id)->get_grid_size();
	this->num_local_cells = num_local_cells;
	this->local_cell_global_indx = NULL;
	strcpy(this->decomp_name, decomp_name);
	annotation_mgr->add_annotation(decomp_id, "register decomposition", annotation);
	is_registered = registered;
	if (registered)
		check_and_verify_name_format_of_string_for_API(host_comp_id, this->decomp_name, API_ID_DECOMP_MGT_REG_DECOMP, "the parallel decomposition", annotation);
	synchronize_comp_processes_for_API(host_comp_id, API_ID_DECOMP_MGT_REG_DECOMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id,"C-Coupler code in Decomp_info::Decomp_info for getting comm group"), "for register a parallel decomposition of a grid", annotation);
	if (registered)
		comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(host_comp_id, API_ID_DECOMP_MGT_REG_DECOMP, annotation);
	check_API_parameter_string(host_comp_id, API_ID_DECOMP_MGT_REG_DECOMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id,"C-Coupler code in Decomp_info::Decomp_info for getting comm group"), "for register a parallel decomposition of a grid", decomp_name, "decomp_name", annotation);
	check_API_parameter_string(host_comp_id, API_ID_DECOMP_MGT_REG_DECOMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id,"C-Coupler code in Decomp_info::Decomp_info for getting comm group"), "for register a parallel decomposition of a grid", original_grid_mgr->get_name_of_grid(grid_id), "grid_id (the corresponding grid name)", annotation);	

	CoR_grid = original_grid_mgr->search_grid_info(grid_id)->get_original_CoR_grid();
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, CoR_grid->get_is_sphere_grid(), "the grid \"%s\" for registering parallel decomposition \"%s\" is not a H2D grid. Please check the model code related to the annoation \"%s\"", CoR_grid->get_grid_name(), decomp_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, num_local_cells >= 0, "the parameter \"num_local_cells\" for registering decomposition \"%s\" cannot be a negative value. Please check the model code related to the annotation \"%s\".", decomp_name, annotation);

    if (num_local_cells == 0)
        local_cell_global_indx = NULL;
    else {
        EXECUTION_REPORT(REPORT_LOG, host_comp_id, true, "num local cells according to input decomp data is %d", num_local_cells);
        local_cell_global_indx = new int [num_local_cells];
        for (i = 0; i < num_local_cells; i ++) {
			if (cell_indexes_in_decomp[i] <= 0)
				local_cell_global_indx[i] = -1;
			else {
	            EXECUTION_REPORT(REPORT_ERROR, host_comp_id, cell_indexes_in_decomp[i] > 0 && cell_indexes_in_decomp[i] <= CoR_grid->get_grid_size(), 
	                             "the cell index specified for registering parallel decompostion of %s is out of the bound of grid size. Please check the model code with the annotation \"%s\"",
	                             decomp_name, annotation);
	            local_cell_global_indx[i] = cell_indexes_in_decomp[i] - 1;  // -1 because fortran array index starts from 1 but c/c++ starts from 0
			}
        }
    }
}


Decomp_info::~Decomp_info()
{
	EXECUTION_REPORT(REPORT_LOG,-1, true, "deleting decomposition %s", decomp_name);
    if (local_cell_global_indx != NULL)
        delete [] local_cell_global_indx;
}


void Decomp_info::check_local_cell_global_indx()
{
    for (int i = 0; i < num_local_cells; i ++)
        EXECUTION_REPORT(REPORT_ERROR,-1, local_cell_global_indx[i] >= -1 && local_cell_global_indx[i] < num_global_cells, "C-Coupler error in check_local_cell_global_indx\n");
}


Decomp_info_mgt::~Decomp_info_mgt()
{
    for (int i = 0; i < decomps_info.size(); i ++)
        delete decomps_info[i];
}


int Decomp_info_mgt::generate_fully_decomp(int original_decomp_id)
{
	char fully_decomp_name[NAME_STR_SIZE];
	Decomp_info *fully_decomp;
	int *local_cells_global_indexes, num_global_cells;


	sprintf(fully_decomp_name, "fully_decomp_for_%s", get_decomp_info(original_decomp_id)->get_decomp_name());
	fully_decomp = search_decomp_info(fully_decomp_name, get_decomp_info(original_decomp_id)->get_comp_id());
	if (fully_decomp != NULL)
		return fully_decomp->get_decomp_id();

	if (comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(get_decomp_info(original_decomp_id)->get_comp_id(), "in Decomp_info_mgt::generate_fully_decomp") == 0) {
		num_global_cells = get_decomp_info(original_decomp_id)->get_num_global_cells();
		local_cells_global_indexes = new int [num_global_cells];
		for (int i = 0; i < num_global_cells; i ++)
			local_cells_global_indexes[i] = i + 1;
		fully_decomp = new Decomp_info(fully_decomp_name, (TYPE_DECOMP_ID_PREFIX|decomps_info.size()), -1, get_decomp_info(original_decomp_id)->get_grid_id(), num_global_cells, local_cells_global_indexes, "fully decomp", false);
		delete [] local_cells_global_indexes;	
	}
	else fully_decomp = new Decomp_info(fully_decomp_name, (TYPE_DECOMP_ID_PREFIX|decomps_info.size()), -1, get_decomp_info(original_decomp_id)->get_grid_id(), 0, NULL, "fully decomp", false);
    decomps_info.push_back(fully_decomp);

    return fully_decomp->get_decomp_id();
}


Decomp_info *Decomp_info_mgt::generate_remap_weights_src_decomp(Decomp_info *dst_decomp_info, Original_grid_info *src_original_grid, Original_grid_info *dst_original_grid, Remap_weight_of_strategy_class *remap_weights)
{
    Decomp_info *decomp_for_remap;    
    long *decomp_map_src, *decomp_map_dst;
    int *local_cell_global_indexes;
    char decomp_name_remap[NAME_STR_SIZE];
	int i, j, num_local_cells = 0;


    sprintf(decomp_name_remap, "src_decomp_for_%s_%s_%s", remap_weights->get_object_name(), dst_decomp_info->get_decomp_name(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(dst_decomp_info->get_comp_id(),"in Decomp_info_mgt::generate_remap_weights_src_decomp")->get_comp_full_name());
	dst_decomp_info->check_local_cell_global_indx();
    decomp_map_src = new long [src_original_grid->get_H2D_sub_CoR_grid()->get_grid_size()];
	decomp_map_dst = new long [dst_original_grid->get_H2D_sub_CoR_grid()->get_grid_size()];
	for (i = 0; i < dst_decomp_info->get_num_global_cells(); i ++)
		decomp_map_dst[i] = 0;
	for (j = 0; j < dst_decomp_info->get_num_local_cells(); j ++)
		if (dst_decomp_info->get_local_cell_global_indx()[j] >= 0)
			decomp_map_dst[dst_decomp_info->get_local_cell_global_indx()[j]] = 1;
	EXECUTION_REPORT(REPORT_LOG,-1, true, "before calculate_src_decomp");
	printf("checkqiguai %d\n", remap_weights->get_num_remap_weights_of_operators());
	remap_weights->calculate_src_decomp(src_original_grid->get_H2D_sub_CoR_grid(), dst_original_grid->get_H2D_sub_CoR_grid(), decomp_map_src, decomp_map_dst);
	EXECUTION_REPORT(REPORT_LOG,-1, true, "after calculate_src_decomp");

    for (long i = 0; i < src_original_grid->get_H2D_sub_CoR_grid()->get_grid_size(); i ++)
        if (decomp_map_src[i] != 0)
            num_local_cells ++;
    local_cell_global_indexes = new int [num_local_cells];
    num_local_cells = 0;
    for (long i = 0; i < src_original_grid->get_H2D_sub_CoR_grid()->get_grid_size(); i ++)
        if (decomp_map_src[i] != 0)
            local_cell_global_indexes[num_local_cells++] = i+1;
	decomp_for_remap = new Decomp_info(decomp_name_remap, (TYPE_DECOMP_ID_PREFIX|decomps_info.size()), dst_original_grid->get_comp_id(), src_original_grid->get_grid_id(), num_local_cells, local_cell_global_indexes, "in Decomp_info_mgt::generate_remap_weights_src_decomp", false);
    decomps_info.push_back(decomp_for_remap);

    delete [] decomp_map_src;
    delete [] local_cell_global_indexes;
    delete [] decomp_map_dst;

	printf("number of local cells in remap_decomp and dst decomp is %d vs %d\n", decomp_for_remap->get_num_local_cells(), dst_decomp_info->get_num_local_cells());

    return decomp_for_remap;
}


Decomp_info *Decomp_info_mgt::search_decomp_info(const char *decomp_name, int comp_id)
{
	printf("search search %s %x\n", decomp_name, comp_id);
	for (int i = 0; i < decomps_info.size(); i ++)
		if (words_are_the_same(decomps_info[i]->get_decomp_name(), decomp_name) && decomps_info[i]->get_comp_id() == comp_id) {
			return decomps_info[i];
		}

	return NULL;
}


int Decomp_info_mgt::register_H2D_parallel_decomposition(const char *decomp_name, int grid_id, int num_local_cells, const int *cell_indexes_in_decomp, const char *annotation)
{
	Decomp_info *new_decomp = new Decomp_info(decomp_name, (TYPE_DECOMP_ID_PREFIX|decomps_info.size()), -1, grid_id, num_local_cells, cell_indexes_in_decomp, annotation, true);

	if (search_decomp_info(decomp_name, original_grid_mgr->get_comp_id_of_grid(grid_id)) != NULL)
		EXECUTION_REPORT(REPORT_ERROR, new_decomp->get_comp_id(), false, "The parallel decomposition \"%s\" corresponding to grid \"%s\" has been registered before. Please check the model code corresponding to annotations \"%s\" and \"%s\"",
					     decomp_name, original_grid_mgr->get_name_of_grid(grid_id), annotation_mgr->get_annotation(search_decomp_info(decomp_name, original_grid_mgr->get_comp_id_of_grid(grid_id))->get_decomp_id(), "register decomposition"), annotation);

	decomps_info.push_back(new_decomp);

	return new_decomp->get_decomp_id();
} 
	
	
bool Decomp_info_mgt::is_decomp_id_legal(int decomp_id) const
{
	int true_decomp_id = decomp_id & TYPE_ID_SUFFIX_MASK;

	
	if ((decomp_id & TYPE_ID_PREFIX_MASK) != TYPE_DECOMP_ID_PREFIX)
		return false;

	if (true_decomp_id < 0 || true_decomp_id >= decomps_info.size())
		return false;

	return true;
}


int Decomp_info_mgt::get_comp_id_of_decomp(int decomp_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_decomp_id_legal(decomp_id), "Software error when calling Decomp_info_mgt::get_comp_id_of_decomp");
	return decomps_info[decomp_id&TYPE_ID_SUFFIX_MASK]->get_comp_id();
}


Remap_grid_class *Decomp_info_mgt::get_CoR_grid_of_decomp(int decomp_id) const
{	
	EXECUTION_REPORT(REPORT_ERROR, -1, is_decomp_id_legal(decomp_id), "Software error when calling Decomp_info_mgt::get_comp_id_of_decomp");
	return original_grid_mgr->get_original_CoR_grid(decomps_info[decomp_id&TYPE_ID_SUFFIX_MASK]->get_grid_id());
}


Decomp_info *Decomp_info_mgt::get_decomp_info(int decomp_id)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_decomp_id_legal(decomp_id), "Software error when calling Decomp_info_mgt::get_decomp_info");
	return decomps_info[decomp_id&TYPE_ID_SUFFIX_MASK];
}

