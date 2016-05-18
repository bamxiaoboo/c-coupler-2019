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
	char *line_p;
    char alg_name[NAME_STR_SIZE];


	fields_allocated = false;
	strcpy(algorithm_cfg_name, cfg);
    src_fields_data_buffers = NULL;
    dst_fields_data_buffers = NULL;
    num_elements_in_field_buffers = NULL;

    fp_cfg = open_config_file(cfg, RUNTIME_COMMON_ALG_DIR);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(alg_name, fp_cfg), "Please specify the name of the runtime common algorithm in the configuration file \"%s\".", cfg);
    c_coupler_algorithm = external_algorithm_mgr->search_c_coupler_algorithm_pointer(alg_name);
	model_algorithm = external_algorithm_mgr->search_model_algorithm_pointer(alg_name);
	EXECUTION_REPORT(REPORT_ERROR, c_coupler_algorithm != NULL || model_algorithm != NULL, 
					 "external algorithm %s has not been registerred before using it", alg_name);

    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, fp_cfg), "Please specify the timer of the runtime common algorithm in the configuration file \"%s\".", cfg);
	line_p = line;
	timer = new Coupling_timer(&line_p, cfg);
    fclose(fp_cfg);
}


void Runtime_common_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
    Decomp_info *decomp;
	Field_mem_info *field_mem;
    FILE * fp_cfg;
    char line[NAME_STR_SIZE * 16];
    char alg_name[NAME_STR_SIZE];
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

    fp_cfg = open_config_file(algorithm_cfg_name, RUNTIME_COMMON_ALG_DIR);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(alg_name, fp_cfg), "Please specify the name of the runtime common algorithm in the configuration file \"%s\".", algorithm_cfg_name);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, fp_cfg), "Please specify the timer of the runtime common algorithm in the configuration file \"%s\".", algorithm_cfg_name);
	if (!get_next_line(input_field_file_name, fp_cfg))
		strcpy(input_field_file_name, "NULL");
	if (!get_next_line(output_field_file_name, fp_cfg))
		strcpy(output_field_file_name, "NULL");
    fclose(fp_cfg);

    num_src_fields = get_num_fields_in_config_file(input_field_file_name, RUNTIME_COMMON_ALG_DIR);
    num_dst_fields = get_num_fields_in_config_file(output_field_file_name, RUNTIME_COMMON_ALG_DIR);


	if (num_src_fields > 0)
	    src_fields_data_buffers = new void *[num_src_fields];
	if (num_dst_fields > 0)
	    dst_fields_data_buffers = new void *[num_dst_fields];

    runtime_algorithm_common_initialize(num_src_fields, num_dst_fields);

	if (num_src_fields > 0) {
	    fp_cfg = open_config_file(input_field_file_name, RUNTIME_COMMON_ALG_DIR);
	    for(i = 0; i < num_src_fields; i ++) {
	        get_next_line(line, fp_cfg);
	        line_p = line;
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(comp_name, &line_p), "Please specify the component name for the %dth field instance in the configuration file \"%s\"", i, input_field_file_name);
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(field_name, &line_p), "Please specify the field name for the %dth field instance in the configuration file \"%s\"", i, input_field_file_name); 
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(decomp_name, &line_p), "Please specify the parallel decomposition name for the %dth field instance in the configuration file \"%s\"", i, input_field_file_name);  
			EXECUTION_REPORT(REPORT_ERROR, get_next_attr(grid_name, &line_p), "Please specify the grid name for the %dth field instance in the configuration file \"%s\"", i, input_field_file_name);  
			EXECUTION_REPORT(REPORT_ERROR, get_next_attr(data_type, &line_p), "Please specify the data type for the %dth field instance in the configuration file \"%s\"", i, input_field_file_name);  
	        buf_type = 0;
	        if (get_next_attr(buf_type_str, &line_p)) {
				line_p = buf_type_str;
				EXECUTION_REPORT(REPORT_ERROR, get_next_integer_attr(&line_p, buf_type), "Please specify the buffer mark for the %dth field instance in the configuration file \"%s\"", i, input_field_file_name);
	        }
			field_mem = alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, true, input_field_file_name);
	        src_fields_data_buffers[i] = field_mem->get_data_buf();
			add_runtime_datatype_transformation(field_mem, true, timer, input_field_file_name);
	        if(words_are_the_same(decomp_name, "NULL")) 
				continue;
	        for (j = 0; j < num_distinct_decomp_infos_of_fields; j ++) {
	            if (words_are_the_same(exist_decomps[j], decomp_name) ||
					decomps_info_mgr->search_decomp_info(exist_decomps[j])->get_num_local_cells() == decomps_info_mgr->search_decomp_info(decomp_name)->get_num_local_cells())
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
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(comp_name, &line_p), "Please specify the component name for the %dth field instance in the configuration file \"%s\"", i, output_field_file_name);
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(field_name, &line_p), "Please specify the field name for the %dth field instance in the configuration file \"%s\"", i, output_field_file_name); 
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(decomp_name, &line_p), "Please specify the parallel decomposition name for the %dth field instance in the configuration file \"%s\"", i, output_field_file_name);  
			EXECUTION_REPORT(REPORT_ERROR, get_next_attr(grid_name, &line_p), "Please specify the grid name for the %dth field instance in the configuration file \"%s\"", i, output_field_file_name);  
			EXECUTION_REPORT(REPORT_ERROR, get_next_attr(data_type, &line_p), "Please specify the data type for the %dth field instance in the configuration file \"%s\"", i, output_field_file_name);  
	        buf_type = 0;
	        if (get_next_attr(buf_type_str, &line_p)) {
				line_p = buf_type_str;
				EXECUTION_REPORT(REPORT_ERROR, get_next_integer_attr(&line_p, buf_type), "Please specify the buffer mark for the %dth field instance in the configuration file \"%s\"", i, input_field_file_name);
	        }
			field_mem = alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, false, output_field_file_name);
	        dst_fields_data_buffers[i] = field_mem->get_data_buf();
			add_runtime_datatype_transformation(field_mem, false, timer, output_field_file_name);
	        if(words_are_the_same(decomp_name, "NULL")) 
				continue;
	        for(j = 0; j < num_distinct_decomp_infos_of_fields; j ++)
	            if(words_are_the_same(exist_decomps[j], decomp_name) 
				    || decomps_info_mgr->search_decomp_info(exist_decomps[j])->get_num_local_cells() == decomps_info_mgr->search_decomp_info(decomp_name)->get_num_local_cells())
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

    if (src_fields_data_buffers != NULL)
        delete [] src_fields_data_buffers;
    if (dst_fields_data_buffers != NULL)
        delete [] dst_fields_data_buffers;
    if (num_elements_in_field_buffers != NULL)
        delete [] num_elements_in_field_buffers;
}


void Runtime_common_algorithm::run(bool is_algorithm_in_kernel_stage)
{
    if (!is_algorithm_in_kernel_stage || timer->is_timer_on()) {
        for (int i = 0; i < num_src_fields; i ++) {
            memory_manager->search_field_via_data_buf(src_fields_data_buffers[i], true)->check_field_sum();
			memory_manager->search_field_via_data_buf(src_fields_data_buffers[i], true)->use_field_values(input_field_file_name);
        }		
		performance_timing_mgr->performance_timing_start(TIMING_TYPE_COMPUTATION, 0, compset_communicators_info_mgr->get_current_comp_id(), algorithm_cfg_name);
		if (c_coupler_algorithm != NULL)
	        c_coupler_algorithm(src_fields_data_buffers, dst_fields_data_buffers, num_elements_in_field_buffers);
		else model_algorithm();
		performance_timing_mgr->performance_timing_stop(TIMING_TYPE_COMPUTATION, 0, compset_communicators_info_mgr->get_current_comp_id(), algorithm_cfg_name);
        for (int i = 0; i < num_dst_fields; i ++) {
            memory_manager->search_field_via_data_buf(dst_fields_data_buffers[i], true)->check_field_sum();
			memory_manager->search_field_via_data_buf(dst_fields_data_buffers[i], true)->define_field_values(false);
        }
    }
}

