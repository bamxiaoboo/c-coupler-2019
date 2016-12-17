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


Runtime_remapping_weights::Runtime_remapping_weights(int src_comp_id, int dst_comp_id, Original_grid_info *src_original_grid, Original_grid_info *dst_original_grid, Remapping_setting *remapping_setting)
{
	Remap_operator_basis *remap_operator_H2D = NULL;
	Remap_operator_basis *remap_operator_V1D = NULL;
	Remap_operator_basis *remap_operator_T1D = NULL;
	Remap_operator_basis *remap_operators[3];
	Remap_grid_class *remap_grids[2];
	char parameter_name[NAME_STR_SIZE], parameter_value[NAME_STR_SIZE];
	int num_remap_operators = 0;

	
	this->src_comp_id = src_comp_id;
	this->dst_comp_id = dst_comp_id;
	this->src_original_grid = src_original_grid;
	this->dst_original_grid = dst_original_grid;
	this->remapping_setting = remapping_setting;

	if (src_original_grid->get_H2D_sub_CoR_grid() != NULL) {
		remap_grids[0] = src_original_grid->get_H2D_sub_CoR_grid();
		remap_grids[1] = dst_original_grid->get_H2D_sub_CoR_grid();
        if (words_are_the_same(remapping_setting->get_H2D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_BILINEAR))
            remap_operator_H2D = new Remap_operator_bilinear("H2D_algorithm", 2, remap_grids);
        else if (words_are_the_same(remapping_setting->get_H2D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_CONSERV_2D)) 
            remap_operator_H2D = new Remap_operator_conserv_2D("H2D_algorithm", 2,  remap_grids);
        else if (words_are_the_same(remapping_setting->get_H2D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_DISTWGT))
            remap_operator_H2D = new Remap_operator_distwgt("H2D_algorithm", 2,  remap_grids);
        else EXECUTION_REPORT(REPORT_ERROR, -1, "Software error in Runtime_remapping_weights::Runtime_remapping_weights: wrong H2D algorithm");
		for (int i = 0; i < remapping_setting->get_H2D_remapping_algorithm()->get_num_parameters(); i ++) {
			remapping_setting->get_H2D_remapping_algorithm()->get_parameter(i, parameter_name, parameter_value);
			remap_operator_H2D->set_parameter(parameter_name, parameter_value);
		}
		remap_operators[num_remap_operators++] = remap_operator_H2D;
	}
	if (src_original_grid->get_V1D_sub_CoR_grid() != NULL) {
		remap_grids[0] = src_original_grid->get_V1D_sub_CoR_grid();
		remap_grids[1] = dst_original_grid->get_V1D_sub_CoR_grid();
        if (words_are_the_same(remapping_setting->get_V1D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_LINEAR))
            remap_operator_V1D = new Remap_operator_linear("V1D_algorithm", 2, remap_grids);
        else if (words_are_the_same(remapping_setting->get_V1D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_SPLINE_1D))
            remap_operator_V1D = new Remap_operator_spline_1D("V1D_algorithm", 2, remap_grids);
        else EXECUTION_REPORT(REPORT_ERROR, -1, "Software error in Runtime_remapping_weights::Runtime_remapping_weights: wrong V1D algorithm");		
		for (int i = 0; i < remapping_setting->get_V1D_remapping_algorithm()->get_num_parameters(); i ++) {
			remapping_setting->get_V1D_remapping_algorithm()->get_parameter(i, parameter_name, parameter_value);
			remap_operator_V1D->set_parameter(parameter_name, parameter_value);
		}
		remap_operators[num_remap_operators++] = remap_operator_V1D;
	}
	if (src_original_grid->get_T1D_sub_CoR_grid() != NULL) {
		remap_grids[0] = src_original_grid->get_T1D_sub_CoR_grid();
		remap_grids[1] = dst_original_grid->get_T1D_sub_CoR_grid();
        if (words_are_the_same(remapping_setting->get_T1D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_LINEAR))
            remap_operator_T1D = new Remap_operator_linear("T1D_algorithm", 2, remap_grids);
        else if (words_are_the_same(remapping_setting->get_T1D_remapping_algorithm()->get_algorithm_name(), REMAP_OPERATOR_NAME_SPLINE_1D))
            remap_operator_T1D = new Remap_operator_spline_1D("T1D_algorithm", 2, remap_grids);
        else EXECUTION_REPORT(REPORT_ERROR, -1, "Software error in Runtime_remapping_weights::Runtime_remapping_weights: wrong T1D algorithm");		
		for (int i = 0; i < remapping_setting->get_T1D_remapping_algorithm()->get_num_parameters(); i ++) {
			remapping_setting->get_T1D_remapping_algorithm()->get_parameter(i, parameter_name, parameter_value);
			remap_operator_T1D->set_parameter(parameter_name, parameter_value);
		}
		remap_operators[num_remap_operators++] = remap_operator_T1D;
	}

	EXECUTION_REPORT(REPORT_ERROR, -1, num_remap_operators > 0, "Software error in Runtime_remapping_weights::Runtime_remapping_weights: no remapping operator");
	printf("number of remap operators is %d\n", num_remap_operators);
	Remap_strategy_class *remap_strategy = new Remap_strategy_class("runtime_remapping_strategy", num_remap_operators, remap_operators);
	sequential_remapping_weights = new Remap_weight_of_strategy_class("runtime_remapping_weights", remap_strategy, src_original_grid->get_original_CoR_grid(), dst_original_grid->get_original_CoR_grid());
}

