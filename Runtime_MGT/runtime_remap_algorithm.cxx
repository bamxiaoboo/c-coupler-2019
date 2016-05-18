/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "runtime_transfer_algorithm.h"
#include "global_data.h"
#include "runtime_remap_algorithm.h"
#include "Runtime_Algorithm_Basis.h"
#include "runtime_config_dir.h"
#include "cor_cpl_interface.h"
#include "cor_global_data.h"
#include <stdio.h>


Runtime_remap_algorithm::Runtime_remap_algorithm(const char *cfg_name)
{
    FILE *cfg_fp, *field_fp;
    char remap_weights_name[NAME_STR_SIZE];
    char comp_name[NAME_STR_SIZE];
    char field_name[NAME_STR_SIZE];    
    char line[NAME_STR_SIZE*3];
    char *line_p;
    int algorithm_mode;


	parent = NULL;
	dynamic_surface_field_origin_grid = NULL;
	dynamic_surface_field_origin_mem = NULL;
	dynamic_remap_weight_of_operator_for_vertical_1D_grid = NULL;
	runtime_remap_grid_for_vertical_1D_src = NULL;
	runtime_remap_grid_for_vertical_1D_dst = NULL;
	fields_allocated = false;
	strcpy(algorithm_cfg_name, cfg_name);
	
	EXECUTION_REPORT(REPORT_LOG, true, "in generating Runtime_remap_algorithm");

    cfg_fp = open_config_file(cfg_name, RUNTIME_REMAP_ALG_DIR);

    EXECUTION_REPORT(REPORT_ERROR, get_next_line(remap_weights_name, cfg_fp), "Please specify remapping weights for the runtime remapping algorithm \"%s\"", cfg_name);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(decomp_name_src, cfg_fp), "Please specify the parallel decomposition of source fields for the runtime remapping algorithm \"%s\"", cfg_name);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(decomp_name_dst, cfg_fp), "Please specify the parallel decomposition of target fields for the runtime remapping algorithm \"%s\"", cfg_name);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, cfg_fp), "Please specify the timer for triggering the execution of the runtime remapping algorithm \"%s\"", cfg_name);
	line_p = line;
    timer = new Coupling_timer(&line_p, cfg_name);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, cfg_fp), "Please specify the mode (value of \"1\" means consevative remapping and other values mean non-conservative remapping) of the runtime remapping algorithm \"%s\"", cfg_name);
	line_p = line;
	EXECUTION_REPORT(REPORT_ERROR, get_next_integer_attr(&line_p, algorithm_mode), "The mode of a runtime remapping algorithm must be an integer. Please verify the configuration file \"%s\"", cfg_name);
    sequential_remap_weights = remap_weights_of_strategy_manager->search_remap_weight_of_strategy(remap_weights_name);
	EXECUTION_REPORT(REPORT_ERROR, sequential_remap_weights != NULL, "Remapping weights \"%s\" is not defined according the corresponding CoR script. Please verify the configuration file \"%s\"", remap_weights_name, cfg_name);
	generate_parallel_interpolation_and_decomposition(remap_weights_name);
	
	if (sequential_remap_weights->get_num_operations_for_caculating_sigma_values_of_grid() > 0 && sequential_remap_weights->get_data_grid_src()->is_sigma_grid() && 
		sequential_remap_weights->get_data_grid_src()->has_specified_sigma_grid_surface_value_field()) {
		dynamic_surface_field_origin_grid = decomp_grids_mgr->search_decomp_grid_info(decomp_name_src, sequential_remap_weights->get_data_grid_src(), false)->get_decomp_grid();
	}
	if (sequential_remap_weights->get_num_operations_for_caculating_sigma_values_of_grid() > 0 && sequential_remap_weights->get_data_grid_dst()->is_sigma_grid() && 
		sequential_remap_weights->get_data_grid_dst()->has_specified_sigma_grid_surface_value_field()) {
		EXECUTION_REPORT(REPORT_ERROR, dynamic_surface_field_origin_grid == NULL, 
						 "The surface value fields (for 3D sigma grid) in source and target grids of remapping weights %s are both specified by users. Only one surface value field can be specified by users.", sequential_remap_weights->get_object_name());
		dynamic_surface_field_origin_grid = decomp_grids_mgr->search_decomp_grid_info(decomp_name_dst, sequential_remap_weights->get_data_grid_dst(), false)->get_decomp_grid();
	}
	if (dynamic_surface_field_origin_grid != NULL) {
		for (int i = 0; i < parallel_remap_weights->get_num_remap_weights_of_operators(); i ++) {
			if (!(parallel_remap_weights->get_remap_weights_of_operator(i)->get_original_remap_operator()->get_src_grid()->has_grid_coord_label(COORD_LABEL_LEV))) 
				continue;
			EXECUTION_REPORT(REPORT_ERROR, parallel_remap_weights->get_remap_weights_of_operator(i)->get_original_remap_operator()->get_src_grid()->get_num_dimensions() == 1, 
							 "C-Coupler error1 in Runtime_remap_algorithm constructor");
			EXECUTION_REPORT(REPORT_ERROR, dynamic_remap_weight_of_operator_for_vertical_1D_grid == NULL, "C-Coupler error2 in Runtime_remap_algorithm constructor");
			dynamic_remap_weight_of_operator_for_vertical_1D_grid = parallel_remap_weights->get_remap_weights_of_operator(i);
			runtime_remap_grid_for_vertical_1D_src = dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_field_data_grid_src()->generate_remap_operator_runtime_grid(dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_original_remap_operator()->get_src_grid(),
																																											dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_original_remap_operator(), NULL);
			runtime_remap_grid_for_vertical_1D_dst = dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_field_data_grid_dst()->generate_remap_operator_runtime_grid(dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_original_remap_operator()->get_dst_grid(),
																																											dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_original_remap_operator(), NULL);
		}
		EXECUTION_REPORT(REPORT_ERROR, dynamic_remap_weight_of_operator_for_vertical_1D_grid != NULL, "C-Coupler error3 in Runtime_remap_algorithm constructor");
		EXECUTION_REPORT(REPORT_ERROR, !dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_original_remap_operator()->get_src_grid()->has_specified_sigma_grid_surface_value_field() &&
									   !dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_original_remap_operator()->get_dst_grid()->has_specified_sigma_grid_surface_value_field(),
						 "There is requirement of dynamic 3D interpolation between grid %s and %s. C-Coupler currently cannot handle the corresponding 3D interpolation because there are 3-D mask values",
						 dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_original_remap_operator()->get_src_grid()->get_grid_name(), dynamic_remap_weight_of_operator_for_vertical_1D_grid->get_original_remap_operator()->get_dst_grid()->get_grid_name());
	}

    if (algorithm_mode == 1) {
        EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, cfg_fp), "Please specify the source fraction field for the conservative dynamic remapping algorithm %s", algorithm_cfg_name);
        line_p = line;
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(comp_name, &line_p), "Please specify the component name of the source fraction field for the conservative dynamic remapping algorithm %s", algorithm_cfg_name);
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(field_name, &line_p), "Please specify the field name of the source fraction field for the conservative dynamic remapping algorithm %s", algorithm_cfg_name);
        src_frac_field_before_rearrange = alloc_mem(comp_name, decomp_name_src, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, DATA_TYPE_DOUBLE, 0, true, algorithm_cfg_name); 
        src_frac_field_after_rearrange = alloc_mem(comp_name, decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, DATA_TYPE_DOUBLE, 0, false, algorithm_cfg_name); 
		src_area_field_after_rearrange = alloc_mem(decomps_info_mgr->search_decomp_info(decomp_name_remap)->get_model_name(), decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), AREA_GF, DATA_TYPE_DOUBLE, 0, false, algorithm_cfg_name);
		temp_src_field = alloc_mem(comp_name, decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, DATA_TYPE_DOUBLE, 10, false, algorithm_cfg_name);
        EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, cfg_fp), "Please specify the destination fraction field for the conservative dynamic remapping algorithm %s", algorithm_cfg_name);
        line_p = line;
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(comp_name, &line_p), "Please specify the component name of the destination fraction field for the conservative dynamic remapping algorithm %s", algorithm_cfg_name);
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(field_name, &line_p), "Please specify the field name of the destionation fraction field for the conservative dynamic remapping algorithm %s", algorithm_cfg_name);
        dst_frac_field = alloc_mem(comp_name, decomp_name_dst, sequential_remap_weights->get_data_grid_dst()->get_grid_name(), field_name, DATA_TYPE_DOUBLE, 0, true, algorithm_cfg_name);
		dst_area_field = alloc_mem(decomps_info_mgr->search_decomp_info(decomp_name_dst)->get_model_name(), decomp_name_dst, sequential_remap_weights->get_data_grid_dst()->get_grid_name(), AREA_GF, DATA_TYPE_DOUBLE, 0, true, algorithm_cfg_name);
    }
    else {
        src_frac_field_before_rearrange = NULL;
        src_frac_field_after_rearrange = NULL;
		src_area_field_after_rearrange = NULL;
		dst_area_field = NULL;
        dst_frac_field = NULL;
    }

    EXECUTION_REPORT(REPORT_ERROR, get_next_line(cfg_file_name_src_fields, cfg_fp), "Please specify the configuration file of source fields for the runtime remapping algorithm \"%s\"", cfg_name);
	EXECUTION_REPORT(REPORT_ERROR, get_next_line(cfg_file_name_dst_fields, cfg_fp), "Please specify the configuration file of target fields for the runtime remapping algorithm \"%s\"", cfg_name);
    fclose(cfg_fp);
}


Runtime_remap_algorithm::Runtime_remap_algorithm(Runtime_remap_algorithm *parent, Field_mem_info *surface_field_mem_src, 
													   Field_mem_info *surface_field_mem_dst, Remap_weight_of_strategy_class *sequential_remap_weights)
{
    Field_mem_info *transfered_fields[2];
	

	this->parent = parent;
	dynamic_surface_field_origin_grid = NULL;
	fields_allocated = true;
	strcpy(algorithm_cfg_name, parent->algorithm_cfg_name);
	timer = new Coupling_timer(parent->timer);
	src_frac_field_before_rearrange = NULL;
	src_frac_field_after_rearrange = NULL;
	src_area_field_after_rearrange = NULL;
	dst_area_field = NULL;
	dst_frac_field = NULL;
	strcpy(decomp_name_src, surface_field_mem_src->get_decomp_name());
	strcpy(decomp_name_dst, surface_field_mem_dst->get_decomp_name());
	this->sequential_remap_weights = sequential_remap_weights;
	generate_parallel_interpolation_and_decomposition(sequential_remap_weights->get_object_name());
	src_double_remap_fields_before_rearrange.push_back(surface_field_mem_src);
	src_double_remap_fields_after_rearrange.push_back(alloc_mem(surface_field_mem_src->get_comp_name(), decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), surface_field_mem_src->get_field_name(), DATA_TYPE_DOUBLE, 0, false, "  C-Coupler error  "));
	dst_double_remap_fields.push_back(surface_field_mem_dst);

	transfered_fields[0] = src_double_remap_fields_before_rearrange[0];
	transfered_fields[1] = src_double_remap_fields_after_rearrange[0];
	
	EXECUTION_REPORT(REPORT_LOG, true, "after generating rearrange rearrange algorithm for runtime_remap_algorithm");
	EXECUTION_REPORT(REPORT_LOG, true, "before generating raarrange router for runtime_remap_algorithm");
	rearrange_src_router = routing_info_mgr->search_or_add_router(compset_communicators_info_mgr->get_current_comp_name(), decomp_name_src, decomp_name_remap);
	EXECUTION_REPORT(REPORT_LOG, true, "after generating raarrange router for runtime_remap_algorithm %s", algorithm_cfg_name);	
	runtime_rearrange_algorithm = new Runtime_transfer_algorithm(2, transfered_fields, rearrange_src_router, timer);
}


Runtime_remap_algorithm::~Runtime_remap_algorithm()
{
    delete timer;
	delete runtime_rearrange_algorithm;

	if (parallel_remap_weights != NULL)
	    delete parallel_remap_weights;

	for (int i = 0; i < operations_for_dynamic_sigma_grid.size(); i ++) {
		if (operations_for_dynamic_sigma_grid[i]->runtime_remap_algorithm != NULL)
			delete operations_for_dynamic_sigma_grid[i]->runtime_remap_algorithm;
		delete operations_for_dynamic_sigma_grid[i];
	}
}


void Runtime_remap_algorithm::generate_parallel_interpolation_and_decomposition(const char *remap_weights_name)
{
	MPI_Status mpi_status;
    Decomp_info *local_remap_decomp_src, *local_decomp_src, *local_decomp_dst;
    Remap_grid_class **remap_related_grids, **remap_related_decomp_grids;
    Remap_grid_class *decomp_original_grids[256];
    int num_remap_related_grids;
	long array_size, current_array_size, offset;
    int *global_cells_local_indexes_in_decomps[256], *local_cell_global_indx_src, *local_cell_global_indx_dst;
	int num_proc_computing_node_comp_group, current_proc_id_computing_node_comp_group, num_local_cells_src, num_local_cells_dst;
    int i, j;
	char *remap_weight_array;
	int num_iter;
	

    cpl_check_remap_weights_format(sequential_remap_weights);
    EXECUTION_REPORT(REPORT_ERROR, sequential_remap_weights != NULL, "C-Coupler software error remap weights is not found\n");

	local_decomp_src = decomps_info_mgr->search_decomp_info(decomp_name_src);
    local_decomp_dst = decomps_info_mgr->search_decomp_info(decomp_name_dst);
	EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(local_decomp_src->get_grid_name())->is_subset_of_grid(sequential_remap_weights->get_data_grid_src()),
	                 "for runtime configuration file %s, the source grid of remapping weights %s does not match the source parallel decompostion %s", 
	                 algorithm_cfg_name, remap_weights_name, decomp_name_src);
	EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(local_decomp_dst->get_grid_name())->is_subset_of_grid(sequential_remap_weights->get_data_grid_dst()),
	                 "for runtime configuration file %s, the destination grid of remapping weights %s does not match the destination parallel decompostion %s", 
	                 algorithm_cfg_name, remap_weights_name, decomp_name_dst);

	EXECUTION_REPORT(REPORT_LOG, true, "before generating remap_weights_src_decomp");
	local_remap_decomp_src = decomps_info_mgr->generate_remap_weights_src_decomp(decomp_name_dst, decomp_name_src, remap_weights_name);
	EXECUTION_REPORT(REPORT_LOG, true, "after generating remap_weights_src_decomp");

	EXECUTION_REPORT(REPORT_LOG, true, "before generating parallel remap weights for runtime_remap_algorithm");

    strcpy(decomp_name_remap, local_remap_decomp_src->get_decomp_name());
    decomp_original_grids[0] = remap_grid_manager->search_remap_grid_with_grid_name(local_remap_decomp_src->get_grid_name());
    decomp_original_grids[1] = remap_grid_manager->search_remap_grid_with_grid_name(local_decomp_dst->get_grid_name());

    remap_related_grids = sequential_remap_weights->get_remap_related_grids(num_remap_related_grids);
    remap_related_decomp_grids = new Remap_grid_class *[num_remap_related_grids];

    for (i = 0; i < num_remap_related_grids; i ++) {
        j = 0;
		remap_related_decomp_grids[i] = remap_related_grids[i];
        if (decomp_original_grids[0]->is_subset_of_grid(remap_related_grids[i])) {
            remap_related_decomp_grids[i] = decomp_grids_mgr->search_decomp_grid_info(decomp_name_remap, remap_related_grids[i], false)->get_decomp_grid();
            j ++;
        }
        if (decomp_original_grids[1]->is_subset_of_grid(remap_related_grids[i])) {
			remap_related_decomp_grids[i] = decomp_grids_mgr->search_decomp_grid_info(decomp_name_dst, remap_related_grids[i], false)->get_decomp_grid();
            j ++;
        }
        EXECUTION_REPORT(REPORT_ERROR, j <= 1, "C-Coupler error2 in Runtime_remap_algorithm\n");
    }

    global_cells_local_indexes_in_decomps[0] = new int [decomp_original_grids[0]->get_grid_size()];
    global_cells_local_indexes_in_decomps[1] = new int [decomp_original_grids[1]->get_grid_size()];
	local_cell_global_indx_src = new int [decomp_original_grids[0]->get_grid_size()];
	local_cell_global_indx_dst = new int [decomp_original_grids[1]->get_grid_size()];

	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_size(compset_communicators_info_mgr->get_computing_node_comp_group(), &num_proc_computing_node_comp_group) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_rank(compset_communicators_info_mgr->get_computing_node_comp_group(), &current_proc_id_computing_node_comp_group) == MPI_SUCCESS);

	if (current_proc_id_computing_node_comp_group > 0) {
		num_local_cells_src = local_remap_decomp_src->get_num_local_cells();
		EXECUTION_REPORT(REPORT_ERROR, MPI_Send(&num_local_cells_src, 1, MPI_INT, 0, 100, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR, MPI_Send((int*)(local_remap_decomp_src->get_local_cell_global_indx()), num_local_cells_src, MPI_INT, 0, 101, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
		num_local_cells_dst = local_decomp_dst->get_num_local_cells();
		EXECUTION_REPORT(REPORT_ERROR, MPI_Send(&num_local_cells_dst, 1, MPI_INT, 0, 102, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR, MPI_Send((int*)(local_decomp_dst->get_local_cell_global_indx()), num_local_cells_dst, MPI_INT, 0, 103, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR, MPI_Recv(&array_size, 1, MPI_LONG, 0, 104, compset_communicators_info_mgr->get_computing_node_comp_group(), &mpi_status) == MPI_SUCCESS);
		remap_weight_array = new char [array_size];
		num_iter = (array_size-1+(long)0x3FFFFFFF) / ((long)0x3FFFFFFF);
		for (j = 0; j < num_iter; j ++) {
			current_array_size = (long)0x3FFFFFFF;
			offset = j*(long)0x3FFFFFFF;
			if (j == num_iter-1) {
				current_array_size = array_size % ((long)0x3FFFFFFF);				
				EXECUTION_REPORT(REPORT_ERROR, offset+current_array_size == array_size, "C-Coupler error3 in Runtime_remap_algorithm\n");
			}
			EXECUTION_REPORT(REPORT_ERROR, MPI_Recv(remap_weight_array+offset, current_array_size, MPI_CHAR, 0, 105+j, compset_communicators_info_mgr->get_computing_node_comp_group(), &mpi_status) == MPI_SUCCESS);			
		}
		parallel_remap_weights = new Remap_weight_of_strategy_class();
		parallel_remap_weights->set_basic_fields(sequential_remap_weights->get_object_name(), sequential_remap_weights->get_remap_strategy(), remap_related_decomp_grids[0], remap_related_decomp_grids[1]);
		parallel_remap_weights->read_remap_weights_from_array(remap_weight_array, NULL, array_size, false, remap_related_decomp_grids, true);
		delete [] remap_weight_array;
	}
	else {
		for (i = num_proc_computing_node_comp_group-1; i >= 0; i --) {
			EXECUTION_REPORT(REPORT_LOG, true, "master process computes parallel remap weights for process %d", i);
			if (i > 0) {
				EXECUTION_REPORT(REPORT_ERROR, MPI_Recv(&num_local_cells_src, 1, MPI_INT, i, 100, compset_communicators_info_mgr->get_computing_node_comp_group(), &mpi_status) == MPI_SUCCESS);
				EXECUTION_REPORT(REPORT_LOG, true, "master process computes parallel remap weights for process %d: after communication 1", i);
				EXECUTION_REPORT(REPORT_ERROR, MPI_Recv(local_cell_global_indx_src, num_local_cells_src, MPI_INT, i, 101, compset_communicators_info_mgr->get_computing_node_comp_group(), &mpi_status) == MPI_SUCCESS);
				EXECUTION_REPORT(REPORT_LOG, true, "master process computes parallel remap weights for process %d: after communication 2", i);
				EXECUTION_REPORT(REPORT_ERROR, MPI_Recv(&num_local_cells_dst, 1, MPI_INT, i, 102, compset_communicators_info_mgr->get_computing_node_comp_group(), &mpi_status) == MPI_SUCCESS);
				EXECUTION_REPORT(REPORT_LOG, true, "master process computes parallel remap weights for process %d: after communication 3", i);
				EXECUTION_REPORT(REPORT_ERROR, MPI_Recv(local_cell_global_indx_dst, num_local_cells_dst, MPI_INT, i, 103, compset_communicators_info_mgr->get_computing_node_comp_group(), &mpi_status) == MPI_SUCCESS);			
				EXECUTION_REPORT(REPORT_LOG, true, "master process computes parallel remap weights for process %d: after communication 4", i);
			}
			else {
				num_local_cells_src = local_remap_decomp_src->get_num_local_cells();
				num_local_cells_dst = local_decomp_dst->get_num_local_cells();
				for (j = 0; j < num_local_cells_src; j ++)
					local_cell_global_indx_src[j] = local_remap_decomp_src->get_local_cell_global_indx()[j];
				for (j = 0; j < num_local_cells_dst; j ++)
					local_cell_global_indx_dst[j] = local_decomp_dst->get_local_cell_global_indx()[j];
			}
			for (j = 0; j < decomp_original_grids[0]->get_grid_size(); j ++)
				global_cells_local_indexes_in_decomps[0][j] = -1;
			for (j = 0; j < num_local_cells_src; j ++)
				if (local_cell_global_indx_src[j] >= 0)
					global_cells_local_indexes_in_decomps[0][local_cell_global_indx_src[j]] = j;
			for (j = 0; j < decomp_original_grids[1]->get_grid_size(); j ++)
				global_cells_local_indexes_in_decomps[1][j] = -1;
			for (j = 0; j < num_local_cells_dst; j ++)
				if (local_cell_global_indx_dst[j] >= 0)
					global_cells_local_indexes_in_decomps[1][local_cell_global_indx_dst[j]] = j;  
			parallel_remap_weights = sequential_remap_weights->generate_parallel_remap_weights(remap_related_decomp_grids, decomp_original_grids, global_cells_local_indexes_in_decomps);
			if (i > 0) {
				parallel_remap_weights->write_remap_weights_into_array(&remap_weight_array, array_size, false);
				delete parallel_remap_weights;
				EXECUTION_REPORT(REPORT_LOG, true, "master process computes parallel remap weights for process %d: before communication 5: size is %ld", i, array_size);
				EXECUTION_REPORT(REPORT_ERROR, MPI_Send(&array_size, 1, MPI_LONG, i, 104, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
				EXECUTION_REPORT(REPORT_LOG, true, "master process computes parallel remap weights for process %d: after communication 5", i);
				num_iter = (array_size-1+(long)0x3FFFFFFF) / ((long)0x3FFFFFFF);
				for (j = 0; j < num_iter; j ++) {
					current_array_size = (long)0x3FFFFFFF;
					offset = j*(long)0x3FFFFFFF;
					if (j == num_iter-1) {
						current_array_size = array_size % ((long)0x3FFFFFFF);				
						EXECUTION_REPORT(REPORT_ERROR, offset+current_array_size == array_size, "C-Coupler error3 in Runtime_remap_algorithm\n");
					}
					EXECUTION_REPORT(REPORT_ERROR, MPI_Send(remap_weight_array+offset, current_array_size, MPI_CHAR, i, 105+j, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
				}
				EXECUTION_REPORT(REPORT_LOG, true, "master process computes parallel remap weights for process %d: after communication 6", i);
				delete [] remap_weight_array;
			}
		}
	}
	parallel_remap_weights->add_remap_weight_of_operators_to_manager(true);
	
	EXECUTION_REPORT(REPORT_LOG, true, "after generating parallel remap weights for runtime_remap_algorithm");
	MPI_Barrier(compset_communicators_info_mgr->get_computing_node_comp_group());

	delete [] remap_related_decomp_grids;
	delete [] remap_related_grids;
	delete [] global_cells_local_indexes_in_decomps[0];
	delete [] global_cells_local_indexes_in_decomps[1];
	delete [] local_cell_global_indx_src;
	delete [] local_cell_global_indx_dst;
}


void Runtime_remap_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
    FILE *field_fp;
    char comp_name[NAME_STR_SIZE];
    char field_name[NAME_STR_SIZE];    
    char line[NAME_STR_SIZE*3];
	char attr_str[NAME_STR_SIZE];
    char *line_p, *line_p2;
    int num_transfered_fields;
    Field_mem_info **transfered_fields;
	Field_mem_info *last_define_mem;
    int i, j;
	bool has_integer;
	int buf_mark;
	

	if (is_algorithm_in_kernel_stage && !timer->is_timer_on())
		return;
		
	if (fields_allocated)
		return;
	
	fields_allocated = true;

    /* set the source variables */
	i = 0;
    field_fp = open_config_file(cfg_file_name_src_fields, RUNTIME_REMAP_ALG_DIR);
    while(get_next_line(line, field_fp)) {
		i ++;
        line_p = line;
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(comp_name, &line_p), "Please specify the component name for the %dth field in the configuration file %s", i, cfg_file_name_src_fields);
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(field_name, &line_p), "Please specify the field name for the %dth field in the configuration file %s", i, cfg_file_name_src_fields);
		line_p2 = line_p;
		if (get_next_attr(attr_str, &line_p))
			EXECUTION_REPORT(REPORT_ERROR, get_next_integer_attr(&line_p2, buf_mark), "The buffer mark of field should be an integer. Please verify the %dth field in the configuration file %s", i, cfg_file_name_src_fields);
		else buf_mark = 0; 
        src_double_remap_fields_before_rearrange.push_back(alloc_mem(comp_name, decomp_name_src, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, DATA_TYPE_DOUBLE, buf_mark, true, cfg_file_name_src_fields));
		last_define_mem = memory_manager->search_last_define_field(comp_name, decomp_name_src, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, buf_mark, true, cfg_file_name_src_fields);
        EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(last_define_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT) || words_are_the_same(last_define_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE),
                     "src field %s can not be used to remap because its data type is not real4 or real8", field_name);
		add_runtime_datatype_transformation(src_double_remap_fields_before_rearrange[src_double_remap_fields_before_rearrange.size()-1], true, timer, cfg_file_name_src_fields);
        src_double_remap_fields_after_rearrange.push_back(alloc_mem(comp_name, decomp_name_remap, sequential_remap_weights->get_data_grid_src()->get_grid_name(), field_name, DATA_TYPE_DOUBLE, buf_mark, false, cfg_file_name_src_fields));
    }
    fclose(field_fp);

    /* set the dst variables */
    field_fp = open_config_file(cfg_file_name_dst_fields, RUNTIME_REMAP_ALG_DIR);
    while(get_next_line(line, field_fp)) {
        line_p = line;
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(comp_name, &line_p), "Please specify the component name for the %dth field in the configuration file %s", i, cfg_file_name_dst_fields);
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(field_name, &line_p), "Please specify the field name for the %dth field in the configuration file %s", i, cfg_file_name_dst_fields);
		line_p2 = line_p;
		if (get_next_attr(attr_str, &line_p))
			EXECUTION_REPORT(REPORT_ERROR, get_next_integer_attr(&line_p2, buf_mark), "The buffer mark of field should be an integer. Please verify the %dth field in the configuration file %s", i, cfg_file_name_dst_fields);
		else buf_mark = 0; 
        dst_double_remap_fields.push_back(alloc_mem(comp_name, decomp_name_dst, sequential_remap_weights->get_data_grid_dst()->get_grid_name(), field_name, DATA_TYPE_DOUBLE, buf_mark, false, cfg_file_name_dst_fields));
		add_runtime_datatype_transformation(dst_double_remap_fields[dst_double_remap_fields.size()-1], false, timer, cfg_file_name_dst_fields);
    }
    fclose(field_fp);

	EXECUTION_REPORT(REPORT_ERROR, src_double_remap_fields_after_rearrange.size() == dst_double_remap_fields.size(), "the numbers of source fields and target fields are not the same for runtime remapping algorithm %s", algorithm_cfg_name);
	for (i = 0; i < src_double_remap_fields_after_rearrange.size(); i ++)
		EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(src_double_remap_fields_after_rearrange[i]->get_field_name(), dst_double_remap_fields[i]->get_field_name()),
		                 "for runtime remapping algorithm %s, the field name does not match (%s and %s) at %d line", algorithm_cfg_name,
						 src_double_remap_fields_after_rearrange[i]->get_field_name(), dst_double_remap_fields[i]->get_field_name(), i+1);

	if (dynamic_surface_field_origin_grid == decomp_grids_mgr->search_decomp_grid_info(decomp_name_src, sequential_remap_weights->get_data_grid_src(), false)->get_decomp_grid()) 
        dynamic_surface_field_origin_mem = alloc_mem(compset_communicators_info_mgr->get_current_comp_name(), decomp_name_src, dynamic_surface_field_origin_grid->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name(), SURFACE_FIELD_GF, DATA_TYPE_DOUBLE, 0, false, "  C-Coupler error  ");
	if (dynamic_surface_field_origin_grid == decomp_grids_mgr->search_decomp_grid_info(decomp_name_dst, sequential_remap_weights->get_data_grid_dst(), false)->get_decomp_grid()) 
        dynamic_surface_field_origin_mem = alloc_mem(compset_communicators_info_mgr->get_current_comp_name(), decomp_name_dst, dynamic_surface_field_origin_grid->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name(), SURFACE_FIELD_GF, DATA_TYPE_DOUBLE, 0, false, "  C-Coupler error  ");

    num_transfered_fields = src_double_remap_fields_before_rearrange.size()*2;
    if (src_frac_field_before_rearrange != NULL)
        num_transfered_fields += 2;
    transfered_fields = new Field_mem_info* [num_transfered_fields+2];
    for (i = 0, j = 0; i < src_double_remap_fields_before_rearrange.size(); i ++)
        transfered_fields[j++] = src_double_remap_fields_before_rearrange[i];
    if (src_frac_field_before_rearrange != NULL)
        transfered_fields[j++] = src_frac_field_before_rearrange;
	if (dynamic_surface_field_origin_grid == decomp_grids_mgr->search_decomp_grid_info(decomp_name_src, sequential_remap_weights->get_data_grid_src(), false)->get_decomp_grid())	{
		if (dynamic_surface_field_origin_grid->get_sigma_grid_dynamic_surface_value_field() != NULL) {
			EXECUTION_REPORT(REPORT_LOG, true, "Surface field of 3D grid %s will be rearraged for dynamic 3D interpolation.", dynamic_surface_field_origin_grid->get_grid_name());
			transfered_fields[j++] = dynamic_surface_field_origin_mem;
			dynamic_surface_field_origin_mem->define_field_values(false);
		}
	}
    for (i = 0; i < src_double_remap_fields_after_rearrange.size(); i ++)
        transfered_fields[j++] = src_double_remap_fields_after_rearrange[i];
    if (src_frac_field_before_rearrange != NULL)
        transfered_fields[j++] = src_frac_field_after_rearrange;
	if (dynamic_surface_field_origin_grid == decomp_grids_mgr->search_decomp_grid_info(decomp_name_src, sequential_remap_weights->get_data_grid_src(), false)->get_decomp_grid())	{
		if (dynamic_surface_field_origin_grid->get_sigma_grid_dynamic_surface_value_field() != NULL)
			transfered_fields[j++] = alloc_mem(compset_communicators_info_mgr->get_current_comp_name(), decomp_name_remap, dynamic_surface_field_origin_grid->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_name(), SURFACE_FIELD_GF, DATA_TYPE_DOUBLE, 0, false, "  C-Coupler error  "); 
	}	
	num_transfered_fields = j;

	EXECUTION_REPORT(REPORT_LOG, true, "after generating rearrange rearrange algorithm for runtime_remap_algorithm");
	EXECUTION_REPORT(REPORT_LOG, true, "before generating raarrange router for runtime_remap_algorithm");
	rearrange_src_router = routing_info_mgr->search_or_add_router(compset_communicators_info_mgr->get_current_comp_name(), decomp_name_src, decomp_name_remap);
	EXECUTION_REPORT(REPORT_LOG, true, "after generating raarrange router for runtime_remap_algorithm");	
	runtime_rearrange_algorithm = new Runtime_transfer_algorithm(num_transfered_fields, transfered_fields, rearrange_src_router, timer);

	delete [] transfered_fields;
}


void Runtime_remap_algorithm::do_remap(bool is_algorithm_in_kernel_stage)
{
    long i, j, field_size_src_before_rearrange, field_size_src_after_rearrange;
	long field_size_dst, decomp_size_src_before_rearrange, decomp_size_src_after_rearrange, num_levels;
    double *src_frac_values, *dst_frac_values, *src_field_values, *dst_field_values, frac_1x;
    double *temp_src_values;
	Remap_grid_class *decomp_grid, *original_grid;
	Decomp_info *decomp;
	bool grid_dynamic_surface_field_updated;


	if (parallel_remap_weights == NULL)
		return;

	if (this->parent == NULL)
		performance_timing_mgr->performance_timing_start(TIMING_TYPE_COMPUTATION, 0, compset_communicators_info_mgr->get_current_comp_id(), algorithm_cfg_name);

	for (i = 0; i < src_double_remap_fields_before_rearrange.size(); i ++)
		src_double_remap_fields_before_rearrange[i]->check_field_sum();

    /* Change the data type and then rearrange src data for parallel remapping */
    field_size_src_before_rearrange = src_double_remap_fields_before_rearrange[0]->get_field_data()->get_grid_data_field()->required_data_size;
    field_size_src_after_rearrange = src_double_remap_fields_after_rearrange[0]->get_field_data()->get_grid_data_field()->required_data_size;
    for (i = 0; i < src_double_remap_fields_before_rearrange.size(); i ++)
        src_double_remap_fields_before_rearrange[i]->use_field_values(cfg_file_name_src_fields);
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
			decomp_grid = decomp_grids_mgr->search_decomp_grid_info(src_double_remap_fields_before_rearrange[0]->get_decomp_name(), original_grid, false)->get_decomp_grid();
			src_double_remap_fields_before_rearrange[i]->get_field_data()->interchange_grid_data(decomp_grid);
			original_grid = remap_grid_manager->search_remap_grid_with_grid_name(decomps_info_mgr->search_decomp_info(src_double_remap_fields_after_rearrange[0]->get_decomp_name())->get_grid_name());
			decomp_grid = decomp_grids_mgr->search_decomp_grid_info(src_double_remap_fields_after_rearrange[0]->get_decomp_name(), original_grid, false)->get_decomp_grid();
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

	grid_dynamic_surface_field_updated = false;
	if (dynamic_surface_field_origin_grid != NULL) {
		decomp_grids_mgr->check_unique_registered_decomp_for_dynamic_sigma_grid(dynamic_surface_field_origin_grid->get_original_grid(), "which is not allowed for setting bottom field of the sigma grid");
		if (dynamic_surface_field_origin_grid->get_sigma_grid_dynamic_surface_value_field() != NULL && dynamic_surface_field_origin_grid->is_sigma_grid_surface_value_field_updated(dynamic_surface_field_origin_mem->get_field_data()))
			grid_dynamic_surface_field_updated = true;
	}

	runtime_rearrange_algorithm->allocate_src_dst_fields(is_algorithm_in_kernel_stage);
    runtime_rearrange_algorithm->run(is_algorithm_in_kernel_stage);
	for (i = 0; i < src_double_remap_fields_after_rearrange.size(); i ++)
		src_double_remap_fields_after_rearrange[i]->check_field_sum();

	if (grid_dynamic_surface_field_updated)
		update_vertical_remap_weights_for_dynamic_sigma_grid(is_algorithm_in_kernel_stage);

    /* Do remap */
    field_size_src_before_rearrange = src_double_remap_fields_after_rearrange[0]->get_field_data()->get_grid_data_field()->required_data_size;
    field_size_dst = dst_double_remap_fields[0]->get_field_data()->get_grid_data_field()->required_data_size;
    for (i = 0; i < src_double_remap_fields_after_rearrange.size(); i ++) {
        src_field_values = (double*) src_double_remap_fields_after_rearrange[i]->get_data_buf();
        dst_field_values = (double*) dst_double_remap_fields[i]->get_data_buf();
        if (src_frac_field_before_rearrange != NULL) {
			src_frac_field_before_rearrange->use_field_values(algorithm_cfg_name);
            temp_src_values = (double*) temp_src_field->get_data_buf();
            src_frac_values = (double*) src_frac_field_after_rearrange->get_data_buf();
            for (j = 0; j < field_size_src_before_rearrange; j ++)
                temp_src_values[j] = src_field_values[j] * src_frac_values[j];
            for (j = 0; j < field_size_src_before_rearrange; j ++)
                printf("frac is %d: %lf %lf\n", j, src_field_values[j], src_frac_values[j]);
			//temp_src_field->calculate_field_conservative_sum(src_area_field_after_rearrange);
            parallel_remap_weights->do_remap(temp_src_field->get_field_data(), dst_double_remap_fields[i]->get_field_data());
			dst_double_remap_fields[i]->calculate_field_conservative_sum(dst_area_field);
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
		dst_frac_field->use_field_values(algorithm_cfg_name);
    }    
    for (i = 0; i < dst_double_remap_fields.size(); i ++) {
		dst_double_remap_fields[i]->define_field_values(false);
		dst_double_remap_fields[i]->check_field_sum();
	}

    if (words_are_the_same(sequential_remap_weights->get_object_name(), "bccam_to_mom_bilinear_wgts")) {
        printf("remap dst\n");
        for (int k = 0; k < dst_double_remap_fields[0]->get_size_of_field(); k ++)
            printf("dst value of %d is %lf\n", k, ((double*)dst_double_remap_fields[0]->get_data_buf())[k]);
    }
    if (words_are_the_same(sequential_remap_weights->get_object_name(), "bccam_to_mom_bilinear_wgts")) {
        printf("remap src\n");
        for (int k = 0; k < src_double_remap_fields_after_rearrange[0]->get_size_of_field(); k ++)
            printf("src value of %d is %lf   %lx\n", k, ((double*)src_double_remap_fields_after_rearrange[0]->get_data_buf())[k], src_double_remap_fields_after_rearrange[0]->get_data_buf());
    }


	if (this->parent == NULL)
		performance_timing_mgr->performance_timing_stop(TIMING_TYPE_COMPUTATION, 0, compset_communicators_info_mgr->get_current_comp_id(), algorithm_cfg_name);
	EXECUTION_REPORT(REPORT_LOG, true, "finish parallel interpolation of %s", algorithm_cfg_name);
}


void Runtime_remap_algorithm::run(bool is_algorithm_in_kernel_stage)
{
    if (!is_algorithm_in_kernel_stage || timer->is_timer_on())
        do_remap(is_algorithm_in_kernel_stage);
}


void Runtime_remap_algorithm::update_vertical_remap_weights_for_dynamic_sigma_grid(bool is_algorithm_in_kernel_stage)
{
	Remap_grid_class *data_grid_src, *data_grid_dst;


	EXECUTION_REPORT(REPORT_ERROR, dynamic_surface_field_origin_grid->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_grid_size() == dynamic_surface_field_origin_mem->get_size_of_field(), "C-Coupler error1 in update_vertical_remap_weights_for_dynamic_sigma_grid");
	EXECUTION_REPORT(REPORT_LOG, true, "Need to update vertical remap weights %s due to dynamic sigma grid", parallel_remap_weights->get_object_name());

	build_operations_for_dynamic_sigma_grid();
	renew_sigma_values(is_algorithm_in_kernel_stage);
	dynamic_remap_weight_of_operator_for_vertical_1D_grid->renew_vertical_remap_weights(runtime_remap_grid_for_vertical_1D_src, runtime_remap_grid_for_vertical_1D_dst);
}


void Runtime_remap_algorithm::build_operations_for_dynamic_sigma_grid()
{
	Remap_weight_of_strategy_class *sequential_weights_of_strategy_for_interpolating_surface_fields;
	Remap_operator_basis *original_remap_operators[256], *operator_for_interpolating_surface_fields;
	Remap_grid_class *data_grid_src, *data_grid_dst;
	Remap_grid_class *operator_field_data_grids[256], *sized_grids_src[256], *sized_grids_dst[256];
	Operation_for_dynamic_sigma_grid *operation_for_dynamic_sigma_grid;
	int i, j, k, num_operator_field_data_grids, num_sized_grids_src, num_sized_grids_dst;

	
	if (operations_for_dynamic_sigma_grid.size() > 0)
		return;

	EXECUTION_REPORT(REPORT_LOG, true, "Need to update vertical remap weights %s due to dynamic sigma grid", parallel_remap_weights->get_object_name());

	data_grid_src = parallel_remap_weights->get_data_grid_src();
	data_grid_dst = parallel_remap_weights->get_data_grid_dst();

	if (data_grid_src->has_specified_sigma_grid_surface_value_field()) {
		operator_field_data_grids[0] = data_grid_src;
		num_operator_field_data_grids = 1;
		for (i = 0, j = 0; i < parallel_remap_weights->get_remap_strategy()->get_num_remap_operator(); i ++) {
			operator_field_data_grids[num_operator_field_data_grids++] = parallel_remap_weights->get_remap_weights_of_operator(i)->get_field_data_grid_src();
			operator_field_data_grids[num_operator_field_data_grids++] = parallel_remap_weights->get_remap_weights_of_operator(i)->get_field_data_grid_dst();
			original_remap_operators[j++] = parallel_remap_weights->get_remap_weights_of_operator(i)->get_original_remap_operator();
		}
		operator_field_data_grids[num_operator_field_data_grids++] = data_grid_dst;
	}
	else {
		operator_field_data_grids[0] = data_grid_dst;
		num_operator_field_data_grids = 1;
		for (i = parallel_remap_weights->get_remap_strategy()->get_num_remap_operator()-1, j = 0; i >= 0 ; i --) {
			operator_field_data_grids[num_operator_field_data_grids++] = parallel_remap_weights->get_remap_weights_of_operator(i)->get_field_data_grid_dst();
			operator_field_data_grids[num_operator_field_data_grids++] = parallel_remap_weights->get_remap_weights_of_operator(i)->get_field_data_grid_src();
			original_remap_operators[j++] = parallel_remap_weights->get_remap_weights_of_operator(i)->get_original_remap_operator();
		}
		operator_field_data_grids[num_operator_field_data_grids++] = data_grid_src;
	}

	EXECUTION_REPORT(REPORT_LOG, true, "Find %d grids needing to be checked for updating vertical remap weights of dynamic sigma grid", num_operator_field_data_grids);

	for (i = 1, k = 0; i < num_operator_field_data_grids; i ++) {
		if (!operator_field_data_grids[i]->is_sigma_grid())
			continue;
		if (operator_field_data_grids[i-1] == operator_field_data_grids[i])
			continue;
		EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i-1]->is_sigma_grid(), "C-Coupler error3 in update_vertical_remap_weights_of_dynamic_sigma_grid");			
		EXECUTION_REPORT(REPORT_ERROR, !operator_field_data_grids[i]->has_specified_sigma_grid_surface_value_field(), "C-Coupler error6 in update_vertical_remap_weights_of_dynamic_sigma_grid");
		EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i-1]->is_sigma_grid(), "C-Coupler error7 in update_vertical_remap_weights_of_dynamic_sigma_grid");
		EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field() != NULL, "C-Coupler error8 in update_vertical_remap_weights_of_dynamic_sigma_grid");
		operator_field_data_grids[i]->allocate_sigma_grid_specific_fields(NULL, NULL, NULL, 0, 0);
		operation_for_dynamic_sigma_grid = new Operation_for_dynamic_sigma_grid;
		operation_for_dynamic_sigma_grid->current_3D_sigma_grid_dst = operator_field_data_grids[i];
		operation_for_dynamic_sigma_grid->current_3D_sigma_grid_src = operator_field_data_grids[i-1];
		operation_for_dynamic_sigma_grid->surface_field_of_sigma_grid_src = alloc_mem(compset_communicators_info_mgr->get_current_comp_name(), operator_field_data_grids[i-1]->get_decomp_name(), operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_original_grid()->get_grid_name(), SURFACE_FIELD_GF, DATA_TYPE_DOUBLE, 0, false, "  C-Coupler error  ");
		operation_for_dynamic_sigma_grid->surface_field_of_sigma_grid_dst = alloc_mem(compset_communicators_info_mgr->get_current_comp_name(), operator_field_data_grids[i]->get_decomp_name(), operator_field_data_grids[i]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_original_grid()->get_grid_name(), SURFACE_FIELD_GF, DATA_TYPE_DOUBLE, 0, false, "  C-Coupler error  ");
		EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i-1]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_original_grid() == sequential_remap_weights->get_operation_for_caculating_sigma_values_of_grid(k)->current_3D_sigma_grid_src->get_sigma_grid_surface_value_field()->get_coord_value_grid(), "C-Coupler error9 in update_vertical_remap_weights_of_dynamic_sigma_grid");
		EXECUTION_REPORT(REPORT_ERROR, operator_field_data_grids[i]->get_sigma_grid_surface_value_field()->get_coord_value_grid()->get_original_grid() == sequential_remap_weights->get_operation_for_caculating_sigma_values_of_grid(k)->current_3D_sigma_grid_dst->get_sigma_grid_surface_value_field()->get_coord_value_grid(), "C-Coupler error10 in update_vertical_remap_weights_of_dynamic_sigma_grid");
		operation_for_dynamic_sigma_grid->runtime_remap_algorithm = NULL;
		operations_for_dynamic_sigma_grid.push_back(operation_for_dynamic_sigma_grid);			
		if (sequential_remap_weights->get_operation_for_caculating_sigma_values_of_grid(k)->remap_weights == NULL) {
			k ++;
			continue;
		}
		operator_field_data_grids[i-1]->get_sized_sub_grids(&num_sized_grids_src, sized_grids_src);
		operator_field_data_grids[i]->get_sized_sub_grids(&num_sized_grids_dst, sized_grids_dst);
		for (j = 0; j < num_sized_grids_src; j ++) {
			if (sized_grids_dst[j]->has_grid_coord_label(COORD_LABEL_LEV))
				continue;
			EXECUTION_REPORT(REPORT_ERROR, sized_grids_src[j]->get_original_grid() == sequential_remap_weights->get_operation_for_caculating_sigma_values_of_grid(k)->current_3D_sigma_grid_src->get_sigma_grid_surface_value_field()->get_coord_value_grid(), "C-Coupler error11 in update_vertical_remap_weights_of_dynamic_sigma_grid");
			EXECUTION_REPORT(REPORT_ERROR, sized_grids_dst[j]->get_original_grid() == sequential_remap_weights->get_operation_for_caculating_sigma_values_of_grid(k)->current_3D_sigma_grid_dst->get_sigma_grid_surface_value_field()->get_coord_value_grid(), "C-Coupler error12 in update_vertical_remap_weights_of_dynamic_sigma_grid");
			EXECUTION_REPORT(REPORT_ERROR, !sized_grids_src[j]->is_similar_grid_with(sized_grids_dst[j]), "C-Coupler error13 in update_vertical_remap_weights_of_dynamic_sigma_grid");
			EXECUTION_REPORT(REPORT_ERROR, (i%2) == 0, "C-Coupler error12 in update_vertical_remap_weights_of_dynamic_sigma_grid");
			EXECUTION_REPORT(REPORT_ERROR, original_remap_operators[(i-1)/2]->get_src_grid()->get_is_sphere_grid(), "C-Coupler error12 in update_vertical_remap_weights_of_dynamic_sigma_grid");
			EXECUTION_REPORT(REPORT_LOG, true, "C-Coupler will use sequential remapping weights for interpolating surface field values between grid %s and %s", 
							 sized_grids_src[j]->get_original_grid()->get_grid_name(), sized_grids_dst[j]->get_original_grid()->get_grid_name());
			EXECUTION_REPORT(REPORT_LOG, true, "C-Coupler will generate surface field values for sigma grid through interpolation between grid %s and %s", 
							 sized_grids_src[j]->get_grid_name(), sized_grids_dst[j]->get_grid_name());
			sequential_weights_of_strategy_for_interpolating_surface_fields = sequential_remap_weights->get_operation_for_caculating_sigma_values_of_grid(k)->remap_weights;
			operation_for_dynamic_sigma_grid->runtime_remap_algorithm = new Runtime_remap_algorithm(this, operation_for_dynamic_sigma_grid->surface_field_of_sigma_grid_src, operation_for_dynamic_sigma_grid->surface_field_of_sigma_grid_dst, sequential_weights_of_strategy_for_interpolating_surface_fields);
		}
		k ++;
	}
}


void Runtime_remap_algorithm::renew_sigma_values(bool is_algorithm_in_kernel_stage)
{
	long i, j, field_size;


	EXECUTION_REPORT(REPORT_ERROR, operations_for_dynamic_sigma_grid.size() > 0, "C-Coupler error1 in renew_sigma_values of Runtime_remap_algorithm");

	dynamic_surface_field_origin_mem->define_field_values(false);

	operations_for_dynamic_sigma_grid[0]->current_3D_sigma_grid_src->copy_sigma_grid_surface_value_field(operations_for_dynamic_sigma_grid[0]->surface_field_of_sigma_grid_src->get_field_data());	
	operations_for_dynamic_sigma_grid[0]->current_3D_sigma_grid_src->calculate_lev_sigma_values();
	operations_for_dynamic_sigma_grid[0]->current_3D_sigma_grid_src->set_coord_vertex_values_in_default();

	for (i = 0; i < operations_for_dynamic_sigma_grid.size(); i ++) {
		if (operations_for_dynamic_sigma_grid[i]->runtime_remap_algorithm == NULL) {
			EXECUTION_REPORT(REPORT_LOG, true, "C-Coupler copy surface field values for sigma grid from grid %s to %s", 
							operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_src->get_grid_name(), operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_dst->get_grid_name());
			EXECUTION_REPORT(REPORT_ERROR, operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_src->get_sigma_grid_surface_value_field()->get_grid_data_field()->read_data_size == operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_dst->get_sigma_grid_surface_value_field()->get_grid_data_field()->read_data_size,
							 "C-Coupler error2 in renew_sigma_values of Runtime_remap_algorithm");
			operations_for_dynamic_sigma_grid[i]->surface_field_of_sigma_grid_src->use_field_values("  C-Coupler error  ");
			memcpy(operations_for_dynamic_sigma_grid[i]->surface_field_of_sigma_grid_dst->get_data_buf(), operations_for_dynamic_sigma_grid[i]->surface_field_of_sigma_grid_src->get_data_buf(), operations_for_dynamic_sigma_grid[i]->surface_field_of_sigma_grid_dst->get_size_of_field()*sizeof(double));
			operations_for_dynamic_sigma_grid[i]->surface_field_of_sigma_grid_dst->define_field_values(false);
		}
		else {
			EXECUTION_REPORT(REPORT_ERROR, operations_for_dynamic_sigma_grid[i]->runtime_remap_algorithm->src_double_remap_fields_before_rearrange[0]->get_size_of_field() == operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_src->get_sigma_grid_surface_value_field()->get_grid_data_field()->read_data_size,
							 "C-Coupler error4 in renew_sigma_values of Runtime_remap_algorithm");
			EXECUTION_REPORT(REPORT_ERROR, operations_for_dynamic_sigma_grid[i]->runtime_remap_algorithm->dst_double_remap_fields[0]->get_size_of_field() == operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_dst->get_sigma_grid_surface_value_field()->get_grid_data_field()->read_data_size,
							 "C-Coupler error5 in renew_sigma_values of Runtime_remap_algorithm");
			EXECUTION_REPORT(REPORT_LOG, true, "C-Coupler interpolate surface field values for sigma grid from grid %s to %s", 
							operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_src->get_grid_name(), operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_dst->get_grid_name());
			operations_for_dynamic_sigma_grid[i]->runtime_remap_algorithm->run(is_algorithm_in_kernel_stage);
		}
		operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_dst->copy_sigma_grid_surface_value_field(operations_for_dynamic_sigma_grid[i]->surface_field_of_sigma_grid_dst->get_field_data());
		operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_dst->calculate_lev_sigma_values();
		operations_for_dynamic_sigma_grid[i]->current_3D_sigma_grid_dst->set_coord_vertex_values_in_default();
	}
}

