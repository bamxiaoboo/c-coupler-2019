/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef RUNTIME_REMAP
#define RUNTIME_REMAP

#include "Runtime_Algorithm_Basis.h"
#include "remap_statement_operand.h"
#include "memory_mgt.h"
#include "remap_weight_of_strategy_class.h"
#include "timer_mgt.h"
#include "routing_info_mgt.h"
#include <vector>


class Runtime_transfer_algorithm;
class Runtime_remap_algorithm;


struct Operation_for_dynamic_sigma_grid
{
	Field_mem_info *surface_field_of_sigma_grid_src;
	Field_mem_info *surface_field_of_sigma_grid_dst;
	Remap_grid_class *current_3D_sigma_grid_src;
	Remap_grid_class *current_3D_sigma_grid_dst;
	Runtime_remap_algorithm *runtime_remap_algorithm;
};


class Runtime_remap_algorithm: public Runtime_algorithm_basis
{
    private:
        std::vector<Field_mem_info*> src_double_remap_fields_after_rearrange;
        std::vector<Field_mem_info*> dst_double_remap_fields;
        std::vector<Field_mem_info*> src_double_remap_fields_before_rearrange;        
		std::vector<Operation_for_dynamic_sigma_grid*> operations_for_dynamic_sigma_grid;
		char cfg_file_name_src_fields[1024];
		char cfg_file_name_dst_fields[1024];
		char decomp_name_src[1024];
		char decomp_name_dst[1024];
		char decomp_name_remap[1024];
        Field_mem_info *src_frac_field_before_rearrange;
        Field_mem_info *src_frac_field_after_rearrange;
		Field_mem_info *src_area_field_after_rearrange;
		Field_mem_info *dst_area_field;
        Field_mem_info *dst_frac_field;
		Field_mem_info *temp_src_field;
        Remap_weight_of_strategy_class *parallel_remap_weights;
        Remap_weight_of_strategy_class *sequential_remap_weights;
        Coupling_timer *timer;

        Routing_info *rearrange_src_router;
        Runtime_transfer_algorithm *runtime_rearrange_algorithm;

		Runtime_remap_algorithm *parent;

		Remap_grid_class *dynamic_surface_field_origin_grid;
		Field_mem_info *dynamic_surface_field_origin_mem;
		Remap_weight_of_operator_class *dynamic_remap_weight_of_operator_for_vertical_1D_grid;
		Remap_grid_class *runtime_remap_grid_for_vertical_1D_src;
		Remap_grid_class *runtime_remap_grid_for_vertical_1D_dst;

        void do_remap(bool);
		void update_vertical_remap_weights_for_dynamic_sigma_grid(bool);
		void generate_parallel_interpolation_and_decomposition(const char*);
		void renew_sigma_values(bool);
		void build_operations_for_dynamic_sigma_grid();

    public:
        Runtime_remap_algorithm(const char *);
		Runtime_remap_algorithm(Runtime_remap_algorithm*, Field_mem_info*, Field_mem_info*, Remap_weight_of_strategy_class*);
        void run(bool);
		void allocate_src_dst_fields(bool);
        ~Runtime_remap_algorithm();
};

#endif
