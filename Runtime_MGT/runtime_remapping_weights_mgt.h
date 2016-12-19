/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/



#ifndef RUNTIME_REMAPPING_WEIGHTS_MGT_H
#define RUNTIME_REMAPPING_WEIGHTS_MGT_H


#include "remapping_configuration_mgt.h"
#include "remap_strategy_class.h"
#include "original_grid_mgt.h"
#include "decomp_info_mgt.h"
#include "remap_weight_of_strategy_class.h"


class Runtime_remapping_weights
{
	private:
		int src_comp_id;
		int dst_comp_id;
		Original_grid_info *src_original_grid;
		Original_grid_info *dst_original_grid;
		Decomp_info *src_decomp_info;
		Decomp_info *dst_decomp_info;
		Remapping_setting *remapping_setting;
		Remap_strategy_class *remapping_strategy;
		Remap_weight_of_strategy_class *sequential_remapping_weights;
		Remap_weight_of_strategy_class *parallel_remapping_weights;

		void generate_parallel_remapping_weights();
		
	public:
		Runtime_remapping_weights(int, int, Original_grid_info *, Original_grid_info *, Remapping_setting *, Decomp_info*);
		Remap_weight_of_strategy_class *get_sequential_remapping_weights() { return sequential_remapping_weights; }
};


#endif

