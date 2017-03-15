/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "runtime_remapping_weights_mgt.h"
#include "remap_operator_bilinear.h"
#include "remap_operator_linear.h"
#include "remap_operator_distwgt.h"
#include "remap_operator_conserv_2D.h"
#include "remap_operator_spline_1D.h"
#include "global_data.h"


Runtime_remapping_weights::Runtime_remapping_weights(int src_comp_id, int dst_comp_id, Original_grid_info *src_original_grid, Original_grid_info *dst_original_grid, Remapping_setting *remapping_setting, Decomp_info *dst_decomp_info)
{
	Remap_operator_basis *remap_operator_H2D = NULL;
	Remap_operator_basis *remap_operator_V1D = NULL;
	Remap_operator_basis *remap_operator_T1D = NULL;
	Remap_operator_basis *remap_operators[3];
	Remap_grid_class *remap_grids[2];
	Remapping_setting *cloned_remapping_setting = remapping_setting->clone();
	char parameter_name[NAME_STR_SIZE], parameter_value[NAME_STR_SIZE];
	int num_remap_operators = 0;

	
	this->src_comp_id = src_comp_id;
	this->dst_comp_id = dst_comp_id;
	this->src_original_grid = src_original_grid;
	this->dst_original_grid = dst_original_grid;
	this->remapping_setting = cloned_remapping_setting;
	this->dst_decomp_info = dst_decomp_info;
	this->src_decomp_info = NULL;
	this->sequential_remapping_weights = NULL;
	this->parallel_remapping_weights = NULL;

	if (src_original_grid->get_H2D_sub_CoR_grid() != NULL) {
		remap_grids[0] = src_original_grid->get_H2D_sub_CoR_grid();
		remap_grids[1] = dst_original_grid->get_H2D_sub_CoR_grid();
        if (words_are_the_same(cloned_remapping_setting->get_H2D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_BILINEAR))
            remap_operator_H2D = new Remap_operator_bilinear("H2D_algorithm", 2, remap_grids);
        else if (words_are_the_same(cloned_remapping_setting->get_H2D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_CONSERV_2D)) 
            remap_operator_H2D = new Remap_operator_conserv_2D("H2D_algorithm", 2,  remap_grids);
        else if (words_are_the_same(cloned_remapping_setting->get_H2D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_DISTWGT))
            remap_operator_H2D = new Remap_operator_distwgt("H2D_algorithm", 2,  remap_grids);
        else EXECUTION_REPORT(REPORT_ERROR, -1, "Software error in Runtime_remapping_weights::Runtime_remapping_weights: wrong H2D algorithm");
		for (int i = 0; i < cloned_remapping_setting->get_H2D_remapping_algorithm()->get_num_parameters(); i ++) {
			cloned_remapping_setting->get_H2D_remapping_algorithm()->get_parameter(i, parameter_name, parameter_value);
			remap_operator_H2D->set_parameter(parameter_name, parameter_value);
		}
		remap_operators[num_remap_operators++] = remap_operator_H2D;
	}
	if (src_original_grid->get_V1D_sub_CoR_grid() != NULL) {
		remap_grids[0] = src_original_grid->get_V1D_sub_CoR_grid();
		remap_grids[1] = dst_original_grid->get_V1D_sub_CoR_grid();
        if (words_are_the_same(cloned_remapping_setting->get_V1D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_LINEAR))
            remap_operator_V1D = new Remap_operator_linear("V1D_algorithm", 2, remap_grids);
        else if (words_are_the_same(cloned_remapping_setting->get_V1D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_SPLINE_1D))
            remap_operator_V1D = new Remap_operator_spline_1D("V1D_algorithm", 2, remap_grids);
        else EXECUTION_REPORT(REPORT_ERROR, -1, "Software error in Runtime_remapping_weights::Runtime_remapping_weights: wrong V1D algorithm");		
		for (int i = 0; i < cloned_remapping_setting->get_V1D_remapping_algorithm()->get_num_parameters(); i ++) {
			cloned_remapping_setting->get_V1D_remapping_algorithm()->get_parameter(i, parameter_name, parameter_value);
			remap_operator_V1D->set_parameter(parameter_name, parameter_value);
		}
		remap_operators[num_remap_operators++] = remap_operator_V1D;
	}
	if (src_original_grid->get_T1D_sub_CoR_grid() != NULL) {
		remap_grids[0] = src_original_grid->get_T1D_sub_CoR_grid();
		remap_grids[1] = dst_original_grid->get_T1D_sub_CoR_grid();
        if (words_are_the_same(cloned_remapping_setting->get_T1D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_LINEAR))
            remap_operator_T1D = new Remap_operator_linear("T1D_algorithm", 2, remap_grids);
        else if (words_are_the_same(cloned_remapping_setting->get_T1D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_SPLINE_1D))
            remap_operator_T1D = new Remap_operator_spline_1D("T1D_algorithm", 2, remap_grids);
        else EXECUTION_REPORT(REPORT_ERROR, -1, "Software error in Runtime_remapping_weights::Runtime_remapping_weights: wrong T1D algorithm");		
		for (int i = 0; i < cloned_remapping_setting->get_T1D_remapping_algorithm()->get_num_parameters(); i ++) {
			cloned_remapping_setting->get_T1D_remapping_algorithm()->get_parameter(i, parameter_name, parameter_value);
			remap_operator_T1D->set_parameter(parameter_name, parameter_value);
		}
		remap_operators[num_remap_operators++] = remap_operator_T1D;
	}

	execution_phase_number = 1;
	EXECUTION_REPORT(REPORT_ERROR, -1, num_remap_operators > 0, "Software error in Runtime_remapping_weights::Runtime_remapping_weights: no remapping operator");
	remapping_strategy = new Remap_strategy_class("runtime_remapping_strategy", num_remap_operators, remap_operators);
	EXECUTION_REPORT(REPORT_LOG, dst_decomp_info->get_host_comp_id(), true, "before generating sequential_remapping_weights from original grid %s to %s", src_original_grid->get_grid_name(), dst_original_grid->get_grid_name());	
	sequential_remapping_weights = new Remap_weight_of_strategy_class("runtime_remapping_weights", remapping_strategy, src_original_grid->get_original_CoR_grid(), dst_original_grid->get_original_CoR_grid());
	EXECUTION_REPORT(REPORT_LOG, dst_decomp_info->get_host_comp_id(), true, "after generating sequential_remapping_weights from original grid %s to %s", src_original_grid->get_grid_name(), dst_original_grid->get_grid_name());	
	execution_phase_number = 2;

	if (dst_original_grid->get_H2D_sub_CoR_grid() == NULL || dst_decomp_info == NULL) {
		EXECUTION_REPORT(REPORT_ERROR, -1, dst_original_grid->get_H2D_sub_CoR_grid() == NULL && dst_decomp_info == NULL, "Software error in Coupling_connection::generate_interpolation: conflict between grid and decomp");
		parallel_remapping_weights = sequential_remapping_weights;
	}	
	else generate_parallel_remapping_weights();
}


Runtime_remapping_weights::~Runtime_remapping_weights()
{
	delete remapping_setting;
	delete remapping_strategy;
	delete sequential_remapping_weights;
	delete parallel_remapping_weights;
}


bool Runtime_remapping_weights::match_requirements(int src_comp_id, int dst_comp_id, Original_grid_info *src_original_grid, Original_grid_info *dst_original_grid, Remapping_setting *remapping_setting, Decomp_info *dst_decomp_info)
{
	return this->src_comp_id == src_comp_id && this->dst_comp_id == dst_comp_id && 
		   this->src_original_grid == src_original_grid && this->dst_original_grid == dst_original_grid && 
		   this->remapping_setting->is_the_same_as_another(remapping_setting) && this->dst_decomp_info == dst_decomp_info;
}


void Runtime_remapping_weights::generate_parallel_remapping_weights()
{
    Remap_grid_class **remap_related_grids, **remap_related_decomp_grids;
    Remap_grid_class *decomp_original_grids[256];
    int num_remap_related_grids;
    int *global_cells_local_indexes_in_decomps[256];
    int i, j;


    EXECUTION_REPORT(REPORT_ERROR,-1, sequential_remapping_weights != NULL, "C-Coupler software error remap weights is not found\n");
    cpl_check_remap_weights_format(sequential_remapping_weights);
	EXECUTION_REPORT(REPORT_ERROR,-1, src_original_grid->get_H2D_sub_CoR_grid()->is_subset_of_grid(sequential_remapping_weights->get_data_grid_src()) && dst_original_grid->get_H2D_sub_CoR_grid()->is_subset_of_grid(sequential_remapping_weights->get_data_grid_dst()),
	                 "Software error in Runtime_remapping_weights::generate_parallel_remapping_weights: grid inconsistency");

	EXECUTION_REPORT(REPORT_LOG, dst_decomp_info->get_comp_id(), true, "before generating remap_weights_src_decomp");
	src_decomp_info = decomps_info_mgr->generate_remap_weights_src_decomp(dst_decomp_info, src_original_grid, dst_original_grid, sequential_remapping_weights);
	EXECUTION_REPORT(REPORT_LOG, dst_decomp_info->get_comp_id(), true, "after generating remap_weights_src_decomp");
	EXECUTION_REPORT(REPORT_LOG, dst_decomp_info->get_comp_id(), true, "before generating parallel remap weights for runtime_remap_algorithm");

    decomp_original_grids[0] = src_original_grid->get_H2D_sub_CoR_grid();
    decomp_original_grids[1] = dst_original_grid->get_H2D_sub_CoR_grid();
    remap_related_grids = sequential_remapping_weights->get_remap_related_grids(num_remap_related_grids);
    remap_related_decomp_grids = new Remap_grid_class *[num_remap_related_grids];

    for (i = 0; i < num_remap_related_grids; i ++) {
        j = 0;
		remap_related_decomp_grids[i] = remap_related_grids[i];
        if (decomp_original_grids[0]->is_subset_of_grid(remap_related_grids[i])) {
            remap_related_decomp_grids[i] = decomp_grids_mgr->search_decomp_grid_info(src_decomp_info->get_decomp_id(), remap_related_grids[i], false)->get_decomp_grid();
            j ++;
        }
        if (decomp_original_grids[1]->is_subset_of_grid(remap_related_grids[i])) {
			remap_related_decomp_grids[i] = decomp_grids_mgr->search_decomp_grid_info(dst_decomp_info->get_decomp_id(), remap_related_grids[i], false)->get_decomp_grid();
            j ++;
        }
		EXECUTION_REPORT(REPORT_ERROR, -1, j <= 1, "Software error in Runtime_remapping_weights::generate_parallel_remapping_weights: wrong j");
    }

    global_cells_local_indexes_in_decomps[0] = new int [decomp_original_grids[0]->get_grid_size()];
    global_cells_local_indexes_in_decomps[1] = new int [decomp_original_grids[1]->get_grid_size()];
	for (j = 0; j < decomp_original_grids[0]->get_grid_size(); j ++)
		global_cells_local_indexes_in_decomps[0][j] = -1;
	for (j = 0; j < src_decomp_info->get_num_local_cells(); j ++)
		if (src_decomp_info->get_local_cell_global_indx()[j] >= 0)
			global_cells_local_indexes_in_decomps[0][src_decomp_info->get_local_cell_global_indx()[j]] = j;
	for (j = 0; j < decomp_original_grids[1]->get_grid_size(); j ++)
		global_cells_local_indexes_in_decomps[1][j] = -1;
	for (j = 0; j < dst_decomp_info->get_num_local_cells(); j ++)
		if (dst_decomp_info->get_local_cell_global_indx()[j] >= 0)
			global_cells_local_indexes_in_decomps[1][dst_decomp_info->get_local_cell_global_indx()[j]] = j;  
	parallel_remapping_weights = sequential_remapping_weights->generate_parallel_remap_weights(remap_related_decomp_grids, decomp_original_grids, global_cells_local_indexes_in_decomps);
	
	EXECUTION_REPORT(REPORT_LOG, dst_decomp_info->get_comp_id(), true, "after generating parallel remap weights for runtime_remap_algorithm");

	delete [] remap_related_decomp_grids;
	delete [] remap_related_grids;
	delete [] global_cells_local_indexes_in_decomps[0];
	delete [] global_cells_local_indexes_in_decomps[1];
}


Runtime_remapping_weights_mgt::~Runtime_remapping_weights_mgt()
{
	for (int i = 0; i < runtime_remapping_weights.size(); i ++)
		delete runtime_remapping_weights[i];
}


Runtime_remapping_weights *Runtime_remapping_weights_mgt::search_or_generate_runtime_remapping_weights(int src_comp_id, int dst_comp_id, Original_grid_info *src_original_grid, Original_grid_info *dst_original_grid, Remapping_setting *remapping_setting, Decomp_info *dst_decomp_info)
{
	for (int i = 0; i < runtime_remapping_weights.size(); i ++)
		if (runtime_remapping_weights[i]->match_requirements(src_comp_id, dst_comp_id, src_original_grid, dst_original_grid, remapping_setting, dst_decomp_info))
			return runtime_remapping_weights[i];

	runtime_remapping_weights.push_back(new Runtime_remapping_weights(src_comp_id, dst_comp_id, src_original_grid, dst_original_grid, remapping_setting, dst_decomp_info));
	return runtime_remapping_weights[runtime_remapping_weights.size()-1];
}

