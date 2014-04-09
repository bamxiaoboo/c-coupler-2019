/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "runtime_transfer_algorithm.h"
#include "global_data.h"
#include "runtime_remap_algorithm.h"
#include "Runtime_Algorithm_Basis.h"
#include "runtime_config_dir.h"
#include "cor_cpl_interface.h"
#include "cor_global_data.h"
#include <stdio.h>
#include <mpi.h>


Runtime_remap_algorithm::Runtime_remap_algorithm(const char *cfg_name)
{
    FILE *cfg_fp, *field_fp;
    char remap_weights_name[NAME_STR_SIZE];
    char decomp_name_src[NAME_STR_SIZE];
    char decomp_name_dst[NAME_STR_SIZE];
    char delay_count_str[NAME_STR_SIZE];
    char comp_name[NAME_STR_SIZE];
    char field_name[NAME_STR_SIZE];    
    char line[NAME_STR_SIZE*3];
    const char *decomp_name_remap;
    char *line_p;
    int algorithm_mode;
    Decomp_info *local_remap_decomp_src, *local_decomp_src, *local_decomp_dst;
    int num_transfered_fields;
    Field_mem_info **transfered_fields;
    int num_remap_related_grids;
    Remap_grid_class **remap_related_grids, **remap_related_decomp_grids;
    Remap_grid_class *decomp_original_grids[256], *decomp_grids[256];
    int *global_cells_local_indexes_in_decomps[256];
    int i, j;
	bool has_integer;
	int buf_mark;
	int num_proc_computing_node_comp_group, current_proc_id_computing_node_comp_group, temp_value;
	MPI_Status mpi_status;
	
	
	EXECUTION_REPORT(REPORT_LOG, true, "in generating Runtime_remap_algorithm");

    cfg_fp = open_config_file(cfg_name, RUNTIME_REMAP_ALG_DIR);

    EXECUTION_REPORT(REPORT_ERROR, get_next_line(remap_weights_name, cfg_fp));
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(decomp_name_src, cfg_fp));
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(decomp_name_dst, cfg_fp));
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, cfg_fp));
    timer = new Coupling_timer(line);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, cfg_fp));
    sscanf(line, "%d", &algorithm_mode);
    sequential_remap_weights = remap_weights_manager->search_remap_weight_of_strategy(remap_weights_name, false);

    cpl_check_remap_weights_format(sequential_remap_weights);
    EXECUTION_REPORT(REPORT_ERROR, sequential_remap_weights != NULL, "C-Coupler software error remap weights is not found\n");

	local_decomp_src = decomps_info_mgr->search_decomp_info(decomp_name_src);
    local_decomp_dst = decomps_info_mgr->search_decomp_info(decomp_name_dst);
	EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(local_decomp_src->get_grid_name())->is_subset_of_grid(sequential_remap_weights->get_data_grid_src()),
	                 "for runtime configuration file %s, the source grid of remapping weights %s does not match the source parallel decompostion %s", 
	                 cfg_name, remap_weights_name, decomp_name_src);
	EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(local_decomp_dst->get_grid_name())->is_subset_of_grid(sequential_remap_weights->get_data_grid_dst()),
	                 "for runtime configuration file %s, the destination grid of remapping weights %s does not match the destination parallel decompostion %s", 
	                 cfg_name, remap_weights_name, decomp_name_dst);

	EXECUTION_REPORT(REPORT_LOG, true, "before generating remap_weights_src_decomp");
	local_remap_decomp_src = decomps_info_mgr->generate_remap_weights_src_decomp(decomp_name_dst, decomp_name_src, remap_weights_name);
	EXECUTION_REPORT(REPORT_LOG, true, "after generating remap_weights_src_decomp");

    decomp_name_remap = local_remap_decomp_src->get_decomp_name();

    if (algorithm_mode == 1) {
        get_next_line(line, cfg_fp);
        line_p = line;
        get_next_attr(comp_name, &line_p);
        get_next_attr(field_name, &line_p);
        src_frac_field_before_rearrange = alloc_mem(comp_name, decomp_name_src, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, 0); 
        src_frac_field_after_rearrange = alloc_mem(comp_name, decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, 0); 
        temp_src_field = src_frac_field_after_rearrange->get_field_data()->duplicate_grid_data_field(src_frac_field_after_rearrange->get_field_data()->get_coord_value_grid(), 1, false, true);
        get_next_line(line, cfg_fp);
        line_p = line;
        get_next_attr(comp_name, &line_p);
        get_next_attr(field_name, &line_p);
        dst_frac_field = alloc_mem(comp_name, decomp_name_dst, sequential_remap_weights->get_data_grid_dst()->get_grid_name(), field_name, 0);
    }
    else {
        src_frac_field_before_rearrange = NULL;
        src_frac_field_after_rearrange = NULL;
        dst_frac_field = NULL;
    }

    /* set the source variables */
    get_next_line(line, cfg_fp);
    field_fp = open_config_file(line, RUNTIME_REMAP_ALG_DIR);
    while(get_next_line(line, field_fp)) {
        line_p = line;
        get_next_attr(comp_name, &line_p);
        get_next_attr(field_name, &line_p);
		buf_mark = get_next_integer_attr(&line_p, has_integer);
		if (!has_integer)
			buf_mark = 0;
        EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(fields_info->get_field_data_type(field_name), DATA_TYPE_FLOAT) || words_are_the_same(fields_info->get_field_data_type(field_name), DATA_TYPE_DOUBLE),
                     "src field %s can not be used to remap because its data type is not real4 or real8", field_name);
        if (words_are_the_same(fields_info->get_field_data_type(field_name), DATA_TYPE_FLOAT)) {
            src_float_remap_fields_before_rearrange.push_back(alloc_mem(comp_name, decomp_name_src, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, buf_mark));
            src_double_remap_fields_before_rearrange.push_back(alloc_mem_double(comp_name, decomp_name_src, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, buf_mark));
            src_float_remap_fields_after_rearrange.push_back(alloc_mem(comp_name, decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, buf_mark));
            src_double_remap_fields_after_rearrange.push_back(alloc_mem_double(comp_name, decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, buf_mark));
        }
        else {
            src_float_remap_fields_before_rearrange.push_back(NULL);
            src_double_remap_fields_before_rearrange.push_back(alloc_mem(comp_name, decomp_name_src, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, buf_mark));            
            src_float_remap_fields_after_rearrange.push_back(NULL);
            src_double_remap_fields_after_rearrange.push_back(alloc_mem(comp_name, decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, buf_mark));            
        }    
    }
    fclose(field_fp);

    /* set the dst variables */
    get_next_line(line, cfg_fp);
    field_fp = open_config_file(line, RUNTIME_REMAP_ALG_DIR);
    while(get_next_line(line, field_fp)) {
        line_p = line;
        get_next_attr(comp_name, &line_p);
        get_next_attr(field_name, &line_p);
		buf_mark = get_next_integer_attr(&line_p, has_integer);
		if (!has_integer)
			buf_mark = 0;
        EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(fields_info->get_field_data_type(field_name), DATA_TYPE_FLOAT) || words_are_the_same(fields_info->get_field_data_type(field_name), DATA_TYPE_DOUBLE),
                     "dst field %s can not be used to remap because its data type is not real4 or real8", field_name);
        if (words_are_the_same(fields_info->get_field_data_type(field_name), DATA_TYPE_FLOAT)) {
            dst_float_remap_fields.push_back(alloc_mem(comp_name, decomp_name_dst, sequential_remap_weights->get_data_grid_dst()->get_grid_name(), field_name, buf_mark));
            dst_double_remap_fields.push_back(alloc_mem_double(comp_name, decomp_name_dst, sequential_remap_weights->get_data_grid_dst()->get_grid_name(), field_name, buf_mark));            
        }
        else {
            dst_float_remap_fields.push_back(NULL);
            dst_double_remap_fields.push_back(alloc_mem(comp_name, decomp_name_dst, sequential_remap_weights->get_data_grid_dst()->get_grid_name(), field_name, buf_mark));
        }
    }
    fclose(field_fp);

    fclose(cfg_fp);

    num_transfered_fields = src_double_remap_fields_before_rearrange.size()*2;
    if (src_frac_field_before_rearrange != NULL)
        num_transfered_fields += 2;
    transfered_fields = new Field_mem_info* [num_transfered_fields];
    for (i = 0, j = 0; i < src_double_remap_fields_before_rearrange.size(); i ++)
        transfered_fields[j++] = src_double_remap_fields_before_rearrange[i];
    if (src_frac_field_before_rearrange != NULL)
        transfered_fields[j++] = src_frac_field_before_rearrange;
    for (i = 0; i < src_double_remap_fields_after_rearrange.size(); i ++)
        transfered_fields[j++] = src_double_remap_fields_after_rearrange[i];
    if (src_frac_field_before_rearrange != NULL)
        transfered_fields[j++] = src_frac_field_after_rearrange;


	EXECUTION_REPORT(REPORT_LOG, true, "after generating rearrange rearrange algorithm for runtime_remap_algorithm");

    decomp_original_grids[0] = remap_grid_manager->search_remap_grid_with_grid_name(local_remap_decomp_src->get_grid_name());
    decomp_original_grids[1] = remap_grid_manager->search_remap_grid_with_grid_name(local_decomp_dst->get_grid_name());

	if (decomp_grids_mgr->search_decomp_grid_info(decomp_name_dst, decomp_original_grids[1])->get_decomp_grid() == NULL) {
		parallel_remap_weights = NULL;
		if (current_proc_id_computing_node_comp_group < num_proc_computing_node_comp_group - 1)
			EXECUTION_REPORT(REPORT_ERROR, MPI_Send(&temp_value, 1, MPI_INT, current_proc_id_computing_node_comp_group+1, 100, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
		return;
	}

    decomp_grids[0] = decomp_grids_mgr->search_decomp_grid_info(decomp_name_remap, decomp_original_grids[0])->get_decomp_grid();
    decomp_grids[1] = decomp_grids_mgr->search_decomp_grid_info(decomp_name_dst, decomp_original_grids[1])->get_decomp_grid();
    global_cells_local_indexes_in_decomps[0] = new int [decomp_original_grids[0]->get_grid_size()];
    global_cells_local_indexes_in_decomps[1] = new int [decomp_original_grids[1]->get_grid_size()];
    for (i = 0; i < decomp_original_grids[0]->get_grid_size(); i ++)
        global_cells_local_indexes_in_decomps[0][i] = -1;
    for (i = 0; i < local_remap_decomp_src->get_num_local_cells(); i ++)
		if (local_remap_decomp_src->get_local_cell_global_indx()[i] >= 0)
	        global_cells_local_indexes_in_decomps[0][local_remap_decomp_src->get_local_cell_global_indx()[i]] = i;
    for (i = 0; i < decomp_original_grids[1]->get_grid_size(); i ++)
        global_cells_local_indexes_in_decomps[1][i] = -1;
    for (i = 0; i < local_decomp_dst->get_num_local_cells(); i ++)
		if (local_decomp_dst->get_local_cell_global_indx()[i] >= 0)
	        global_cells_local_indexes_in_decomps[1][local_decomp_dst->get_local_cell_global_indx()[i]] = i;    

	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_size(compset_communicators_info_mgr->get_computing_node_comp_group(), &num_proc_computing_node_comp_group) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(compset_communicators_info_mgr->get_computing_node_comp_group(), &current_proc_id_computing_node_comp_group) == MPI_SUCCESS);
	if (current_proc_id_computing_node_comp_group > 0)
		EXECUTION_REPORT(REPORT_ERROR, MPI_Recv(&temp_value, 1, MPI_INT, current_proc_id_computing_node_comp_group-1, 100, compset_communicators_info_mgr->get_computing_node_comp_group(), &mpi_status) == MPI_SUCCESS);

	EXECUTION_REPORT(REPORT_LOG, true, "before generating parallel remap weights for runtime_remap_algorithm");

    sequential_remap_weights = remap_weights_manager->search_remap_weight_of_strategy(remap_weights_name, true);
    remap_related_grids = sequential_remap_weights->get_remap_related_grids(num_remap_related_grids);
    remap_related_decomp_grids = new Remap_grid_class *[num_remap_related_grids];
    for (i = 0; i < num_remap_related_grids; i ++) {
        j = 0;
		remap_related_decomp_grids[i] = remap_related_grids[i];
        if (decomp_original_grids[0]->is_subset_of_grid(remap_related_grids[i])) {
            remap_related_decomp_grids[i] = decomp_grids_mgr->search_decomp_grid_info(decomp_name_remap, remap_related_grids[i])->get_decomp_grid();
            j ++;
        }
        if (decomp_original_grids[1]->is_subset_of_grid(remap_related_grids[i])) {
			remap_related_decomp_grids[i] = decomp_grids_mgr->search_decomp_grid_info(decomp_name_dst, remap_related_grids[i])->get_decomp_grid();
            j ++;
        }
        EXECUTION_REPORT(REPORT_ERROR, j <= 1, "C-Coupler error2 in Runtime_remap_algorithm\n");
    }
	parallel_remap_weights = sequential_remap_weights->generate_parallel_remap_weights(remap_related_decomp_grids, decomp_original_grids, decomp_grids, global_cells_local_indexes_in_decomps);
	sequential_remap_weights->temporarily_cleanup_memory_space();


	EXECUTION_REPORT(REPORT_LOG, true, "after generating parallel remap weights for runtime_remap_algorithm");

	if (current_proc_id_computing_node_comp_group < num_proc_computing_node_comp_group - 1)
		EXECUTION_REPORT(REPORT_ERROR, MPI_Send(&temp_value, 1, MPI_INT, current_proc_id_computing_node_comp_group+1, 100, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);

	EXECUTION_REPORT(REPORT_LOG, true, "before generating raarrange router for runtime_remap_algorithm");
	rearrange_src_router = routing_info_mgr->search_or_add_router(compset_communicators_info_mgr->get_current_comp_name(), decomp_name_src, decomp_name_remap);
	EXECUTION_REPORT(REPORT_LOG, true, "after generating raarrange router for runtime_remap_algorithm");	
	runtime_rearrange_algorithm = new Runtime_transfer_algorithm(num_transfered_fields, transfered_fields, rearrange_src_router, timer);

	delete [] transfered_fields;
	delete [] remap_related_grids;
	delete [] remap_related_decomp_grids;
	delete [] global_cells_local_indexes_in_decomps[0];
	delete [] global_cells_local_indexes_in_decomps[1];

}


Runtime_remap_algorithm::~Runtime_remap_algorithm()
{
    delete timer;

	if (parallel_remap_weights != NULL)
	    delete parallel_remap_weights;
}


void Runtime_remap_algorithm::do_remap(bool is_alglrithm_in_kernel_stage)
{
    long i, j, field_size_src_before_rearrange, field_size_src_after_rearrange;
	long field_size_dst, decomp_size_src_before_rearrange, decomp_size_src_after_rearrange, num_levels;
    double *src_frac_values, *dst_frac_values, *src_field_values, *dst_field_values, frac_1x;
    double *temp_src_values;
	Remap_grid_class *decomp_grid, *original_grid;
	Decomp_info *decomp;


	if (parallel_remap_weights == NULL)
		return;

	for (i = 0; i < src_float_remap_fields_after_rearrange.size(); i ++)
		if (src_float_remap_fields_before_rearrange[i] != NULL) 
			src_float_remap_fields_before_rearrange[i]->check_field_sum();
		else src_double_remap_fields_before_rearrange[i]->check_field_sum();

    /* Change the data type and then rearrange src data for parallel remapping */
    field_size_src_before_rearrange = src_double_remap_fields_before_rearrange[0]->get_field_data()->get_grid_data_field()->required_data_size;
    field_size_src_after_rearrange = src_double_remap_fields_after_rearrange[0]->get_field_data()->get_grid_data_field()->required_data_size;
    for (i = 0; i < src_double_remap_fields_before_rearrange.size(); i ++)
        if (src_float_remap_fields_before_rearrange[i] != NULL) {
			src_float_remap_fields_before_rearrange[i]->use_field_values();
            for (j = 0; j < field_size_src_before_rearrange; j ++)
                ((double*) src_double_remap_fields_before_rearrange[i]->get_data_buf())[j] = ((float*) src_float_remap_fields_before_rearrange[i]->get_data_buf())[j];
        }
		else src_double_remap_fields_before_rearrange[i]->use_field_values();
	decomp_size_src_before_rearrange = decomps_info_mgr->search_decomp_info(src_double_remap_fields_before_rearrange[0]->get_decomp_name())->get_num_local_cells();
	decomp_size_src_after_rearrange = decomps_info_mgr->search_decomp_info(src_double_remap_fields_after_rearrange[0]->get_decomp_name())->get_num_local_cells();

	if (field_size_src_before_rearrange == 0 || decomp_size_src_before_rearrange == 0)
		EXECUTION_REPORT(REPORT_ERROR, field_size_src_before_rearrange == 0 && decomp_size_src_before_rearrange == 0,
				         "C-Coupler error in Runtime_remap_algorithm::do_remap\n");		
	else EXECUTION_REPORT(REPORT_ERROR, field_size_src_before_rearrange % decomp_size_src_before_rearrange == 0,
		                  "C-Coupler error in Runtime_remap_algorithm::do_remap\n");
	if (field_size_src_after_rearrange == 0 || decomp_size_src_after_rearrange == 0)
		EXECUTION_REPORT(REPORT_ERROR, field_size_src_after_rearrange == 0 && decomp_size_src_after_rearrange == 0,
				         "C-Coupler error in Runtime_remap_algorithm::do_remap\n");
	else EXECUTION_REPORT(REPORT_ERROR, field_size_src_after_rearrange % decomp_size_src_after_rearrange == 0,
				          "C-Coupler error in Runtime_remap_algorithm::do_remap\n");
	if (decomp_size_src_before_rearrange > 0) {
		num_levels = field_size_src_before_rearrange / decomp_size_src_before_rearrange;
		for (i = 0; i < src_double_remap_fields_before_rearrange.size(); i ++) {
			original_grid = remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(src_double_remap_fields_before_rearrange[0]->get_decomp_name())->get_grid_name());
			decomp_grid = decomp_grids_mgr->search_decomp_grid_info(src_double_remap_fields_before_rearrange[0]->get_decomp_name(), original_grid)->get_decomp_grid();
			src_double_remap_fields_before_rearrange[i]->get_field_data()->interchange_grid_data(decomp_grid);
			original_grid = remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(src_double_remap_fields_after_rearrange[0]->get_decomp_name())->get_grid_name());
			decomp_grid = decomp_grids_mgr->search_decomp_grid_info(src_double_remap_fields_after_rearrange[0]->get_decomp_name(), original_grid)->get_decomp_grid();
			src_double_remap_fields_after_rearrange[i]->get_field_data()->interchange_grid_data(decomp_grid);
		}
	    for (i = 0; i < src_double_remap_fields_before_rearrange.size(); i ++)
			for (j = 0; j < num_levels; j ++)
		        memcpy(((double*)src_double_remap_fields_after_rearrange[i]->get_data_buf())+j*decomp_size_src_after_rearrange, 
		                ((double*)src_double_remap_fields_before_rearrange[i]->get_data_buf())+j*decomp_size_src_before_rearrange, 
		                decomp_size_src_before_rearrange*sizeof(double));
	    if (src_frac_field_before_rearrange != NULL)
	        memcpy(src_frac_field_after_rearrange->get_data_buf(), src_frac_field_before_rearrange->get_data_buf(), decomp_size_src_before_rearrange*sizeof(double));
	}
    runtime_rearrange_algorithm->run(is_alglrithm_in_kernel_stage);

    /* Do remap */
    field_size_src_before_rearrange = src_double_remap_fields_after_rearrange[0]->get_field_data()->get_grid_data_field()->required_data_size;
    field_size_dst = dst_double_remap_fields[0]->get_field_data()->get_grid_data_field()->required_data_size;
    for (i = 0; i < dst_double_remap_fields.size(); i ++)
        if (dst_float_remap_fields[i] != NULL) {
            for (j = 0; j < field_size_dst; j ++)
                ((double*) dst_double_remap_fields[i]->get_data_buf())[j] = ((float*) dst_float_remap_fields[i]->get_data_buf())[j];
        }
    for (i = 0; i < src_double_remap_fields_after_rearrange.size(); i ++) {
        src_field_values = (double*) src_double_remap_fields_after_rearrange[i]->get_data_buf();
        dst_field_values = (double*) dst_double_remap_fields[i]->get_data_buf();
        if (src_frac_field_before_rearrange != NULL) {
			src_frac_field_before_rearrange->use_field_values();
            temp_src_values = (double*) temp_src_field->get_grid_data_field()->data_buf;
            src_frac_values = (double*) src_frac_field_after_rearrange->get_data_buf();
            for (j = 0; j < field_size_src_before_rearrange; j ++)
                temp_src_values[j] = src_field_values[j] * src_frac_values[j];
            parallel_remap_weights->do_remap(temp_src_field, dst_double_remap_fields[i]->get_field_data());
        }
        else parallel_remap_weights->do_remap(src_double_remap_fields_after_rearrange[i]->get_field_data(), dst_double_remap_fields[i]->get_field_data());;
    }

    /* Adjust the data according to fraction value and then change the data type */
    if (dst_frac_field != NULL) {
        dst_frac_values = (double*) dst_frac_field->get_data_buf();
        for (j = 0; j < field_size_dst; j ++) {
            if (dst_frac_values[j] != 0) {
                frac_1x = 1.0/dst_frac_values[j];
                for (i = 0; i < dst_double_remap_fields.size(); i ++) {
                    dst_field_values = (double*) dst_double_remap_fields[i]->get_data_buf();
                    dst_field_values[j] = dst_field_values[j] * frac_1x;
                }
            }
        }
		dst_frac_field->use_field_values();
    }    
    for (i = 0; i < dst_double_remap_fields.size(); i ++)
        if (dst_float_remap_fields[i] != NULL) {
			dst_float_remap_fields[i]->define_field_values();
            for (j = 0; j < field_size_dst; j ++)
                ((float*) dst_float_remap_fields[i]->get_data_buf())[j] = ((double*) dst_double_remap_fields[i]->get_data_buf())[j];
			dst_float_remap_fields[i]->check_field_sum();
        }
		else {
			dst_double_remap_fields[i]->define_field_values();
			dst_double_remap_fields[i]->check_field_sum();
		}
}


void Runtime_remap_algorithm::run(bool is_alglrithm_in_kernel_stage)
{
    if (!is_alglrithm_in_kernel_stage || timer->is_timer_on())
        do_remap(is_alglrithm_in_kernel_stage);
}

