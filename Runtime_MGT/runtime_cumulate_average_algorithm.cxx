/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "runtime_cumulate_average_algorithm.h"
#include "runtime_config_dir.h"
#include "cor_global_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Runtime_cumulate_average_algorithm::Runtime_cumulate_average_algorithm(const char * cfg)
{
	strcpy(algorithm_cfg_name, cfg);
	fields_allocated = false;
}


template<typename T> void template_cumulate_or_average(T* dst, const T* src, const int length, 
        const int computing_count, const bool do_average)
{
    if (computing_count == 1) {
        for (int i = 0; i < length; i++)
            dst[i] = src[i];
    } 
    else {
        for (int i = 0; i < length; i++)
            dst[i] += src[i];
    }
    if (do_average) {
        /// a trick
        T frac = 1 / ((T)computing_count);
        if (frac == 0) {
            /// not a float number
            for (int i = 0; i < length; i++)
                dst[i] = dst[i] / computing_count;

        } else {
            /// float number
            for (int i = 0; i < length; i++)
                dst[i] = dst[i] * frac;
        }
    }
}


void Runtime_cumulate_average_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
    cumulate_average_field_info *cumulate_average_field;
    FILE * fp_cfg;
    char line[NAME_STR_SIZE * 16];
    char comp_name[NAME_STR_SIZE];
    char field_name[NAME_STR_SIZE];
    char decomp_name[NAME_STR_SIZE];
    char grid_name[NAME_STR_SIZE];
    char * line_p;
    int buf_mark_dst;
    int i;


	if (fields_allocated)
		return;

	fields_allocated = true;
    fp_cfg = open_config_file(algorithm_cfg_name, RUNTIME_AVGHIST_ALG_DIR);

    while (get_next_line(line, fp_cfg)) {
        line_p = line;
        cumulate_average_field = new cumulate_average_field_info;
        get_next_attr(comp_name, &line_p);
        get_next_attr(field_name, &line_p);
        get_next_attr(decomp_name, &line_p);
        get_next_attr(grid_name, &line_p);
        get_next_integer_attr(&line_p, buf_mark_dst);
        cumulate_average_field->timer = new Coupling_timer(&line_p, algorithm_cfg_name);
        cumulate_average_field->mem_info_src = alloc_mem(comp_name, decomp_name, grid_name, field_name, NULL, 0, true, algorithm_cfg_name);
		add_runtime_datatype_transformation(cumulate_average_field->mem_info_src, true, NULL, algorithm_cfg_name);
        cumulate_average_field->mem_info_dst = alloc_mem(comp_name, decomp_name, grid_name, field_name, cumulate_average_field->mem_info_src->get_field_data()->get_grid_data_field()->data_type_in_application, buf_mark_dst, false, algorithm_cfg_name);
		add_runtime_datatype_transformation(cumulate_average_field->mem_info_dst, false, cumulate_average_field->timer, algorithm_cfg_name);
        cumulate_average_field->num_elements_in_field = cumulate_average_field->mem_info_src->get_field_data()->get_grid_data_field()->required_data_size;
        cumulate_average_field->field_data_type = cumulate_average_field->mem_info_src->get_field_data()->get_grid_data_field()->data_type_in_application;
        cumulate_average_field->current_computing_count = 0;
        for (i = 0; i < cumulate_average_fields.size(); i ++)
            EXECUTION_REPORT(REPORT_ERROR, cumulate_average_fields[i]->mem_info_dst != cumulate_average_field->mem_info_dst,
                         "Find repeated field in cumulate average algorithm: (%s)\n", line);
        cumulate_average_fields.push_back(cumulate_average_field);
    }

	read_restart_computing_count();

    fclose(fp_cfg);
}


void Runtime_cumulate_average_algorithm::cumulate_or_average(bool is_algorithm_in_kernel_stage)
{
    bool do_average; 


	EXECUTION_REPORT(REPORT_LOG, true, "before cumulate or average");
	for (int i = 0; i < cumulate_average_fields.size(); i ++) {
		cumulate_average_fields[i]->mem_info_src->check_field_sum();
		cumulate_average_fields[i]->mem_info_dst->check_field_sum();
	}
	
    for (int i = 0; i < cumulate_average_fields.size(); i ++) {
        cumulate_average_fields[i]->current_computing_count ++;
        do_average = !is_algorithm_in_kernel_stage || cumulate_average_fields[i]->timer->is_timer_on();
        if (words_are_the_same(cumulate_average_fields[i]->field_data_type, DATA_TYPE_FLOAT))
            template_cumulate_or_average<float>((float *) (cumulate_average_fields[i]->mem_info_dst->get_data_buf()), 
                                         (float *) (cumulate_average_fields[i]->mem_info_src->get_data_buf()), 
                                         cumulate_average_fields[i]->num_elements_in_field,
                                         cumulate_average_fields[i]->current_computing_count,
                                         do_average);
        else if (words_are_the_same(cumulate_average_fields[i]->field_data_type, DATA_TYPE_DOUBLE))
            template_cumulate_or_average<double>((double *) (cumulate_average_fields[i]->mem_info_dst->get_data_buf()), 
                                         (double *) (cumulate_average_fields[i]->mem_info_src->get_data_buf()), 
                                         cumulate_average_fields[i]->num_elements_in_field,
                                         cumulate_average_fields[i]->current_computing_count,
                                         do_average);
        else EXECUTION_REPORT(REPORT_ERROR, false, "error data type in cumulate_average algorithm\n"); 
        if (do_average) {
			EXECUTION_REPORT(REPORT_LOG, true, "do average at computing count is %d", cumulate_average_fields[i]->current_computing_count);
            cumulate_average_fields[i]->current_computing_count = 0;			
        }
		cumulate_average_fields[i]->mem_info_src->use_field_values(algorithm_cfg_name);
		cumulate_average_fields[i]->mem_info_dst->define_field_values(false);
    }

	EXECUTION_REPORT(REPORT_LOG, true, "after cumulate or average");
	for (int i = 0; i < cumulate_average_fields.size(); i ++) {
		cumulate_average_fields[i]->mem_info_src->check_field_sum();
		cumulate_average_fields[i]->mem_info_dst->check_field_sum();
	}
}


void Runtime_cumulate_average_algorithm::run(bool is_algorithm_in_kernel_stage)
{
    cumulate_or_average(is_algorithm_in_kernel_stage);
}


Field_mem_info *Runtime_cumulate_average_algorithm::add_one_field(Field_mem_info *input_field, Coupling_timer *timer)
{
	cumulate_average_field_info *cumulate_average_field; 

	
	cumulate_average_field = new cumulate_average_field_info;
	cumulate_average_field->timer = timer;
	cumulate_average_field->mem_info_src = input_field;
	cumulate_average_field->mem_info_dst = alloc_mem(input_field->get_comp_name(), input_field->get_decomp_name(), input_field->get_grid_name(), input_field->get_field_name(), 
		                                   cumulate_average_field->mem_info_src->get_field_data()->get_grid_data_field()->data_type_in_application, algorithm_id*10000+cumulate_average_fields.size(), false, algorithm_cfg_name);
	cumulate_average_field->num_elements_in_field = cumulate_average_field->mem_info_src->get_field_data()->get_grid_data_field()->required_data_size;
	cumulate_average_field->field_data_type = cumulate_average_field->mem_info_src->get_field_data()->get_grid_data_field()->data_type_in_application;
	cumulate_average_field->current_computing_count = 0;
	cumulate_average_fields.push_back(cumulate_average_field);

	return cumulate_average_field->mem_info_dst;
}


Runtime_cumulate_average_algorithm::~Runtime_cumulate_average_algorithm()
{
    for (int i = 0; i < cumulate_average_fields.size(); i ++) {
        delete cumulate_average_fields[i]->timer;
        delete cumulate_average_fields[i];
    }
}


void Runtime_cumulate_average_algorithm::write_restart_fields()
{
    char field_mem_full_name[NAME_STR_SIZE];
    int i;


    for (i = 0; i < cumulate_average_fields.size(); i ++)
        if (!cumulate_average_fields[i]->timer->is_timer_on()) {
            cumulate_average_fields[i]->mem_info_dst->get_field_mem_full_name(field_mem_full_name);
            EXECUTION_REPORT(REPORT_LOG, true, "averaging algorithm should do restart write for averaging field %s", field_mem_full_name);
			restart_mgr->write_one_restart_field(cumulate_average_fields[i]->mem_info_dst, cumulate_average_fields[i]->current_computing_count);
        }
}


void Runtime_cumulate_average_algorithm::read_restart_computing_count()
{
	int computing_count;


	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"))
		return;

	for (int i = 0; i < cumulate_average_fields.size(); i ++) {
		computing_count = restart_mgr->get_restart_read_field_computing_count(cumulate_average_fields[i]->mem_info_dst->get_comp_name(), 
		                                                                      cumulate_average_fields[i]->mem_info_dst->get_decomp_name(),
		                                                                      cumulate_average_fields[i]->mem_info_dst->get_grid_name(),
		                                                                      cumulate_average_fields[i]->mem_info_dst->get_field_name(),
		                                                                      cumulate_average_fields[i]->mem_info_dst->get_buf_type());
		if (computing_count >= 0) {
			cumulate_average_fields[i]->current_computing_count = computing_count;
			EXECUTION_REPORT(REPORT_LOG, true, "comulative_averaging algorithm read restart field (%s %s %s %s %d), count is %d ", cumulate_average_fields[i]->mem_info_dst->get_comp_name(), 
		                                                                      cumulate_average_fields[i]->mem_info_dst->get_decomp_name(),
		                                                                      cumulate_average_fields[i]->mem_info_dst->get_grid_name(),
		                                                                      cumulate_average_fields[i]->mem_info_dst->get_field_name(),
		                                                                      cumulate_average_fields[i]->mem_info_dst->get_buf_type(),
		                                                                      computing_count);
		}
	}
}

