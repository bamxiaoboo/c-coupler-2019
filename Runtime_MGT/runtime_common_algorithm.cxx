/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "runtime_common_algorithm.h"
#include "runtime_config_dir.h"
#include "cor_global_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_DECOMP_NUM	128


Runtime_common_algorithm::Runtime_common_algorithm(const char * cfg)
{
    FILE * fp_cfg;
    char line[NAME_STR_SIZE * 16];
    char alg_name[NAME_STR_SIZE];


	fields_allocated = false;
	strcpy(cfg_file_name, cfg);

    fp_cfg = open_config_file(cfg, RUNTIME_COMMON_ALG_DIR);
    get_next_line(alg_name, fp_cfg);
    c_coupler_algorithm = external_algorithm_mgr->search_c_coupler_algorithm_pointer(alg_name);
	model_algorithm = external_algorithm_mgr->search_model_algorithm_pointer(alg_name);
	EXECUTION_REPORT(REPORT_ERROR, c_coupler_algorithm != NULL || model_algorithm != NULL, 
					 "external algorithm %s has not been registerred before using it", alg_name);

    get_next_line(line, fp_cfg);
	timer = new Coupling_timer(line);

	if (model_algorithm != NULL) {
    	src_fields_data_buffers = NULL;
		dst_fields_data_buffers = NULL;
		num_elements_in_field_buffers = NULL;
		return;
	}
    fclose(fp_cfg);
}


void Runtime_common_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
    Decomp_info *decomp;
	Field_mem_info *field_mem;
    FILE * fp_cfg;
    char line[NAME_STR_SIZE * 16];
    char alg_name[NAME_STR_SIZE];
    char input_field_file_name[NAME_STR_SIZE];
    char output_field_file_name[NAME_STR_SIZE];
    char comp_name[NAME_STR_SIZE];
    char field_name[NAME_STR_SIZE];
    char decomp_name[NAME_STR_SIZE];
	char grid_name[NAME_STR_SIZE];
	char data_type[NAME_STR_SIZE];
    char buf_type_str[NAME_STR_SIZE];
    char *line_p;
    char exist_decomps[MAX_DECOMP_NUM][NAME_STR_SIZE];
    int i, j;
    int num_distinct_decomp_infos_of_fields = 0;
    int buf_type;


	if (is_algorithm_in_kernel_stage && !timer->is_timer_on())
		return;

	if (fields_allocated)
		return;
	fields_allocated = true;

    fp_cfg = open_config_file(cfg_file_name, RUNTIME_COMMON_ALG_DIR);
    get_next_line(alg_name, fp_cfg);
    get_next_line(line, fp_cfg);
    get_next_line(input_field_file_name, fp_cfg);
    get_next_line(output_field_file_name, fp_cfg);
    fclose(fp_cfg);

    num_src_fields = get_num_fields_in_config_file(input_field_file_name, RUNTIME_COMMON_ALG_DIR);
    num_dst_fields = get_num_fields_in_config_file(output_field_file_name, RUNTIME_COMMON_ALG_DIR);

    runtime_algorithm_common_initialize(num_src_fields, num_dst_fields, 0);

	if (num_src_fields > 0) {
	    fp_cfg = open_config_file(input_field_file_name, RUNTIME_COMMON_ALG_DIR);
	    for(i = 0; i < num_src_fields; i ++) {
	        get_next_line(line, fp_cfg);
	        line_p = line;
	        get_next_attr(comp_name, &line_p);
	        get_next_attr(field_name, &line_p);
	        get_next_attr(decomp_name, &line_p);
			get_next_attr(grid_name, &line_p);
			get_next_attr(data_type, &line_p);
	        buf_type = 0;
	        if (get_next_attr(buf_type_str, &line_p))
	            buf_type = atoi(buf_type_str);
			field_mem = alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, true);
	        src_fields_data_buffers[i] = field_mem->get_data_buf();
			add_runtime_datatype_transformation(field_mem, true, timer);
	        if(words_are_the_same(decomp_name, "NULL")) 
				continue;
	        for (j = 0; j < num_distinct_decomp_infos_of_fields; j ++) {
	            if (words_are_the_same(exist_decomps[j], decomp_name))
	               break;
	        }
	        if (j == num_distinct_decomp_infos_of_fields) 
	            strcpy(exist_decomps[num_distinct_decomp_infos_of_fields++], decomp_name);
	    }
	    fclose(fp_cfg);
	}

	if (num_dst_fields > 0) {
	    fp_cfg = open_config_file(output_field_file_name, RUNTIME_COMMON_ALG_DIR);
	    for(i = 0; i < num_dst_fields; i ++) {
	        get_next_line(line, fp_cfg);
	        line_p = line;
	        get_next_attr(comp_name, &line_p);
	        get_next_attr(field_name, &line_p);
	        get_next_attr(decomp_name, &line_p);
			get_next_attr(grid_name, &line_p);
			get_next_attr(data_type, &line_p);
	        buf_type = 0;
	        if (get_next_attr(buf_type_str, &line_p))
	            buf_type = atoi(buf_type_str);
			field_mem = alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, false);
	        dst_fields_data_buffers[i] = field_mem->get_data_buf();
			add_runtime_datatype_transformation(field_mem, false, timer);
	        if(words_are_the_same(decomp_name, "NULL")) 
				continue;
	        for(j = 0; j < num_distinct_decomp_infos_of_fields; j ++)
	            if(words_are_the_same(exist_decomps[j], decomp_name))
	                break;
	        if(j == num_distinct_decomp_infos_of_fields)
	            strcpy(exist_decomps[num_distinct_decomp_infos_of_fields++], decomp_name);
	    }
	    fclose(fp_cfg);
	}
	
    num_elements_in_field_buffers = new int[num_distinct_decomp_infos_of_fields+2];
    ///Search the num_elements_in_field_buffers of used grids.
    for(i = 0; i < num_distinct_decomp_infos_of_fields; i++){
        decomp = decomps_info_mgr->search_decomp_info(exist_decomps[i]);
        num_elements_in_field_buffers[i] = decomp->get_num_local_cells();
    }
    num_elements_in_field_buffers[i] = num_src_fields;
    num_elements_in_field_buffers[i+1] = num_dst_fields;
}


Runtime_common_algorithm::~Runtime_common_algorithm()
{
	delete timer;
}


void Runtime_common_algorithm::run(bool is_algorithm_in_kernel_stage)
{
    if (!is_algorithm_in_kernel_stage || timer->is_timer_on()) {
        for (int i = 0; i < num_src_fields; i ++) {
            memory_manager->search_field_via_data_buf(src_fields_data_buffers[i])->check_field_sum();
			memory_manager->search_field_via_data_buf(src_fields_data_buffers[i])->use_field_values();
        }
		if (c_coupler_algorithm != NULL)
	        c_coupler_algorithm(src_fields_data_buffers, dst_fields_data_buffers, num_elements_in_field_buffers);
		else model_algorithm();
        for (int i = 0; i < num_dst_fields; i ++) {
            memory_manager->search_field_via_data_buf(dst_fields_data_buffers[i])->check_field_sum();
			memory_manager->search_field_via_data_buf(dst_fields_data_buffers[i])->define_field_values(false);
        }
    }
}

