/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "cor_global_data.h"
#include "remap_mgt.h"
#include "quick_sort.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "coupling_interface.h"
#include <mpi.h>


int coupling_process_control_counter = 0;


extern "C" void coupling_get_field_size_(void *model_buf, const char *annotation, int *field_size)
{
    EXECUTION_REPORT(REPORT_ERROR,-1, coupling_process_control_counter > 0, 
		             "C-Coupler interface coupling_interface_initialize has not been called before running the code corresponding to annotation \"%s\"\n", annotation); 
	*field_size = memory_manager->get_field_size(model_buf, annotation);
}


extern "C" void register_sigma_grid_bottom_field_(void *model_buf, const char *grid_name)
{
	Field_mem_info *bottom_field;
	Remap_grid_class *field_grid, *sigma_grid;

/*
	sigma_grid = remap_grid_manager->search_remap_grid_with_grid_name(grid_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, sigma_grid != NULL, "\"%s\" has not been defined in the CoR script", grid_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, sigma_grid->is_sigma_grid(), "grid \"%s\" is not a sigma grid", grid_name);

	bottom_field = memory_manager->search_field_via_data_buf(model_buf, false);
	EXECUTION_REPORT(REPORT_ERROR,-1, bottom_field != NULL && bottom_field->get_is_registered_model_buf(), "the model bottom field for the sigma grid \"%s\" has not been registered to C-Coupler", grid_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, !words_are_the_same(bottom_field->get_grid_name(), "NULL"), "scalar model variable \"%s\" cannot be used as the model bottom field for a sigma grid", bottom_field->get_field_name());

	field_grid = remap_grid_manager->search_remap_grid_with_grid_name(bottom_field->get_grid_name());
	EXECUTION_REPORT(REPORT_ERROR,-1, field_grid != NULL, "C-Coupler error in register_sigma_grid_bottom_field");
	EXECUTION_REPORT(REPORT_ERROR,-1, field_grid->get_is_sphere_grid(), "field \"%s\" that will be set as the model bottom field for the sigma grid \"%s\" is not on a sphere grid: \"%s\" is not a sphere grid",
                     bottom_field->get_field_name(), grid_name, field_grid->get_grid_name());
	EXECUTION_REPORT(REPORT_ERROR,-1, field_grid->is_subset_of_grid(sigma_grid), "the grid \"%s\" for the grid bottom field is not a sub grid for the sigma grid \"%s\"", 
					 bottom_field->get_grid_name(), grid_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(bottom_field->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT) || words_are_the_same(bottom_field->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE),
					 "the data type of the bottom field for the sigma grid \"%s\" must be real4 or real8", grid_name); 
	// check the whether the parallel decomposition covers all grid points
	EXECUTION_REPORT(REPORT_LOG,-1, true, "Register surface field of grid %s on decomposition %s", grid_name, bottom_field->get_decomp_name());
	decomp_grids_mgr->search_decomp_grid_info(bottom_field->get_decomp_name(), sigma_grid, true)->get_decomp_grid()->set_sigma_grid_dynamic_surface_value_field(bottom_field->get_field_data());
*/
}                  



extern "C" void coupling_initialize_ensemble_manager_(int *ensemble_id, int *have_random_seed_for_perturbation, int *root_random_seed_for_perturbation, const char *perturbation_type)
{
	EXECUTION_REPORT(REPORT_ERROR,-1, ensemble_mgr != NULL, "C-Coupler software error: ensemble_mgr is not created before the initialization");
	ensemble_mgr->Initialize(*ensemble_id, *have_random_seed_for_perturbation, *root_random_seed_for_perturbation, perturbation_type);
}


extern "C" void coupling_add_field_for_perturbing_roundoff_errors_(void *data_buf)
{
	ensemble_mgr->register_a_field_for_perturbation(data_buf);
}


extern "C" void coupling_perturb_roundoff_errors_for_an_array_(void *field_data_buf, const char *data_type, int *field_size)
{
	ensemble_mgr->perturb_a_model_array(field_data_buf, data_type, *field_size);
}


extern "C" void coupling_perturb_roundoff_errors_()
{
	ensemble_mgr->run();
}



extern "C" void coupling_get_double_current_calendar_time_(double *cal_time, int *shift_seconds)
{
    *cal_time = timer_mgr->get_double_current_calendar_time(*shift_seconds);
}


extern "C" void coupling_get_current_date_(int *date)
{
    *date = timer_mgr->get_current_date();
}


extern "C" void coupling_get_current_second_(int *date)
{
    *date = timer_mgr->get_current_second();
}


extern "C" void coupling_get_float_current_calendar_time_(float *cal_time, int *shift_seconds)
{
    *cal_time = timer_mgr->get_float_current_calendar_time(*shift_seconds);
}


extern "C" void coupling_is_first_step_(bool *result)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, false, "coupling_is_first_step has not been implemented");
}


extern "C" void coupling_is_first_restart_step_(bool *result)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, false, "coupling_is_first_restart_step has not been implemented");
	
}


extern "C" void coupling_reset_timer_()
{
	timer_mgr->reset_timer();
}


extern "C" void coupling_get_current_nstep_(int *nstep)
{
	*nstep = timer_mgr->get_current_num_time_step();
}


extern "C" void coupling_get_num_total_step_(int *nstep)
{
	*nstep = (int) timer_mgr->get_num_total_step();
}


extern "C" void coupling_get_step_size_(int *step_size)
{
    *step_size = timer_mgr->get_time_step_in_second();
}


extern "C" void coupling_get_start_time_(int *year, int *month, int *day, int *seconds)
{
	*year = timer_mgr->get_start_full_time() / 1000000000;
	*month = (timer_mgr->get_start_full_time() / 10000000)%100;
	*day = (timer_mgr->get_start_full_time() / 100000)%100;
	*seconds = timer_mgr->get_start_full_time() % 100000;
}


extern "C" void coupling_get_stop_time_(int *year, int *month, int *day, int *second)
{
	*year = timer_mgr->get_stop_year();
	*month = timer_mgr->get_stop_month();
	*day = timer_mgr->get_stop_day();
	*second = timer_mgr->get_stop_second();
}


extern "C" void coupling_get_previous_time_(int *year, int *month, int *day, int *seconds)
{
	*year = timer_mgr->get_previous_full_time() / 1000000000;
	*month = (timer_mgr->get_previous_full_time() / 10000000)%100;
	*day = (timer_mgr->get_previous_full_time() / 100000)%100;
	*seconds = timer_mgr->get_previous_full_time() % 100000;
}


extern "C" void coupling_get_current_time_(int *year, int *month, int *day, int *second, int *shift_second)
{
	timer_mgr->get_current_time(*year, *month, *day, *second, *shift_second);
}


extern "C" void coupling_get_current_num_days_in_year_(int *days)
{
	*days = timer_mgr->get_current_num_days_in_year();
}


extern "C" void coupling_get_current_year_(int *year)
{
	*year = timer_mgr->get_current_year();
}


extern "C" void coupling_get_elapsed_days_from_start_date_(int *days, int *seconds)
{
	timer_mgr->get_elapsed_days_from_start_date(days, seconds);
}


extern "C" void coupling_get_elapsed_days_from_reference_date_(int *days, int *seconds)
{
	timer_mgr->get_elapsed_days_from_reference_date(days, seconds);
}


extern "C" void coupling_abort_(const char *error_string)
{
	EXECUTION_REPORT(REPORT_ERROR,-1, false, error_string);
}


extern "C" void coupling_check_sum_for_all_fields_()
{
	EXECUTION_REPORT(REPORT_ERROR,-1, memory_manager != NULL, "C-Coupler is not initialized when the component call the interface for checking sum of all fields managed by the C-Coupler");
	memory_manager->check_sum_of_all_fields();
}


extern "C" void coupling_add_field_info_(const char *field_name, const char *field_unit, const char *field_long_name)
{
	EXECUTION_REPORT(REPORT_ERROR,-1, fields_info != NULL, "the C-Coupler manager for the information of fields is not initialized");
	fields_info->add_field_info(field_name, field_long_name, field_unit, "none", "state");
}


extern "C" void initialize_CCPL_mgrs(const char *executable_name)
{
    char root_cfg_name[NAME_STR_SIZE], line[NAME_STR_SIZE];
	FILE *root_cfg_fp;
	int i;


	for (i = strlen(executable_name)-1; i >= 0; i --)
		if (executable_name[i] == '/')
			break;
    sprintf(root_cfg_name, "%s_coupling.cfg", executable_name+i+1);
    EXECUTION_REPORT(REPORT_LOG,-1, true, "root runtime configuration file name is %s", root_cfg_name);
	root_cfg_fp = open_config_file(root_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, root_cfg_fp), "Please specify the configuration file (a CoR script) for grid management and data interpolation in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.", root_cfg_name);
	sprintf(root_cfg_name, "%s/%s", C_COUPLER_CONFIG_DIR, line);
	execution_phase_number = 1;
	annotation_mgr = new Annotation_mgt();
	original_grid_mgr = new Original_grid_mgt(root_cfg_name);
	decomps_info_mgr = new Decomp_info_mgt();
	decomp_grids_mgr = new Decomp_grid_mgt();
	memory_manager = new Memory_mgt();
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, root_cfg_fp), "Please specify the field information table in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.", root_cfg_name);
	fields_info = new Field_info_mgt(line, "NULL");
	components_time_mgrs = new Components_time_mgt();
	timer_mgr2 = new Timer_mgt();
	execution_phase_number = 2;
	inout_interface_mgr = new Inout_interface_mgt();
	IO_fields_mgr = new IO_field_mgt();
	components_IO_output_procedures_mgr = new Components_IO_output_procedures_mgt();
	fields_gather_scatter_mgr = new Fields_gather_scatter_mgt();
	remapping_configuration_mgr = new Remapping_configuration_mgt();
	routing_info_mgr = new Routing_info_mgt();
}


extern "C" void register_root_component_(MPI_Comm *comm, const char *comp_name, const char *annotation, int *comp_id, 
										const char *executable_name, const char *exp_model, const char *case_name, const char *case_desc, const char *case_mode, const char *comp_namelist,
                                		const char *current_config_time, const char *original_case_name, const char *original_config_time)
{
	int flag;
	MPI_Comm local_comm = -1;
	int root_comp_id;
	int current_proc_global_id;
	char file_name[NAME_STR_SIZE], local_comp_name[NAME_STR_SIZE];


	initialize_CCPL_mgrs(executable_name);

	strcpy(local_comp_name, comp_name);

	check_and_verify_name_format_of_string_for_API(-1, local_comp_name, API_ID_COMP_MGT_REG_ROOT_COMP, "the root component", annotation);

	if (comp_comm_group_mgt_mgr != NULL) 
		EXECUTION_REPORT(REPORT_ERROR,-1, comp_comm_group_mgt_mgr == NULL, "The root component has been initialized before %s", comp_comm_group_mgt_mgr->get_annotation_start());  // add debug information
	MPI_Initialized(&flag);
	if (flag == 0) {
		EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "Initialize MPI when registerring the root component \"%s\"", local_comp_name);
		MPI_Init(NULL, NULL);
	}

	synchronize_comp_processes_for_API(-1, API_ID_COMP_MGT_REG_ROOT_COMP, MPI_COMM_WORLD, "registering root component", annotation);

	comp_comm_group_mgt_mgr = new Comp_comm_group_mgt_mgr(executable_name, exp_model, case_name, case_desc, case_mode, comp_namelist,
                                		current_config_time, original_case_name, original_config_time);

	if (*comm != -1) {
		EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "Before MPI_barrier at root component \"%s\" for synchronizing the processes of the component (the corresponding model code annotation is \"%s\").", local_comp_name, annotation);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Barrier(*comm) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "After MPI_barrier at root component \"%s\" for synchronizing the processes of the component (the corresponding model code annotation is \"%s\").", local_comp_name, annotation);
		
	}

	root_comp_id = comp_comm_group_mgt_mgr->register_component(local_comp_name, local_comm, -1, annotation);

	if (*comm != -1) {
		int input_comm_size, new_comm_size;
		int *input_comm_process_ids, *new_comm_process_ids, *temp_array;
		int current_proc_global_id, current_proc_local_id;
		MPI_Comm new_comm;
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(*comm, &input_comm_size) == MPI_SUCCESS);
		new_comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(root_comp_id, "C-Coupler code in register_root_component for getting component management node");
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(new_comm, &new_comm_size) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, input_comm_size == new_comm_size);  // add debug information
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(MPI_COMM_WORLD, &current_proc_global_id) == MPI_SUCCESS);
		input_comm_process_ids = new int [input_comm_size];
		new_comm_process_ids = new int [new_comm_size];
		temp_array = new int [new_comm_size];
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(&current_proc_global_id, 1, MPI_INT, input_comm_process_ids, 1, MPI_INT, *comm) == MPI_SUCCESS);
		EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(&current_proc_global_id, 1, MPI_INT, new_comm_process_ids, 1, MPI_INT, new_comm) == MPI_SUCCESS);
		do_quick_sort(input_comm_process_ids, temp_array, 0, input_comm_size-1);
		do_quick_sort(new_comm_process_ids, temp_array, 0, new_comm_size-1);
		for (int i = 0; i < input_comm_size; i ++)
			EXECUTION_REPORT(REPORT_ERROR,-1, input_comm_process_ids[i] == new_comm_process_ids[i], 
			                 "The communicator of root component \"%s\" does not match the communicator generated (processes of the two communicators are not the same). Please check the model code with the annotation \"%s\"",
			                 local_comp_name, annotation);
		delete [] input_comm_process_ids;
		delete [] new_comm_process_ids;
		delete [] temp_array;
	}
	else *comm = local_comm;

	*comp_id = root_comp_id;

	sprintf(file_name, "%s/CCPL_configs/env_run.xml", comp_comm_group_mgt_mgr->get_root_working_dir());
	components_time_mgrs->define_root_comp_time_mgr(*comp_id, file_name);
	remapping_configuration_mgr->add_remapping_configuration(comp_comm_group_mgt_mgr->get_global_node_root()->get_comp_id());
	remapping_configuration_mgr->add_remapping_configuration(*comp_id);
}


extern "C" void register_component_(int *parent_comp_id, const char *comp_name, MPI_Comm *comm, const char *annotation, int *comp_id)
{
	char local_comp_name[NAME_STR_SIZE];


	strcpy(local_comp_name, comp_name);
	check_and_verify_name_format_of_string_for_API(-1, local_comp_name, API_ID_COMP_MGT_REG_COMP, "the new component", annotation);
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(*parent_comp_id), 
					 "For the registration of component (name=\"%s\", type=\"%s\"), the input parameter of the ID of the parent component is wrong (the corresponding annotation of model code is \"%s\").",
					 local_comp_name, annotation);
	
	if (strlen(annotation) != 0)
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr != NULL, "Please call interface CCPL_register_root_component before calling interface CCPL_register_component (corresponding to annotation \"%s\")", annotation);
	else EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr != NULL, "Please call interface CCPL_register_root_component before calling interface CCPL_register_component");

	if (*comm !=-1) {
		synchronize_comp_processes_for_API(*parent_comp_id, API_ID_COMP_MGT_REG_COMP, *comm, "registering a component based on the parent component", annotation);
		check_API_parameter_string(*parent_comp_id, API_ID_COMP_MGT_REG_COMP, *comm, "registering a component based on an available communicator", local_comp_name, "comp_name", annotation);
	}
	else synchronize_comp_processes_for_API(*parent_comp_id, API_ID_COMP_MGT_REG_COMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(*parent_comp_id, "C-Coupler code for get comm group in register_component interface"), "registering component based on the parent component", annotation);

	*comp_id = comp_comm_group_mgt_mgr->register_component(local_comp_name, *comm, *parent_comp_id, annotation);
	components_time_mgrs->clone_parent_comp_time_mgr(*comp_id, *parent_comp_id, annotation);
	remapping_configuration_mgr->add_remapping_configuration(*comp_id);
}


extern "C" void get_id_of_component_(const char *comp_name, const char *annotation, int *comp_id)
{
	char local_comp_name[NAME_STR_SIZE];


	strcpy(local_comp_name, comp_name);
	check_and_verify_name_format_of_string_for_API(-1, local_comp_name, API_ID_COMP_MGT_GET_COMP_ID, "the component", annotation);

	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr != NULL, "No component has been registered. Please call interface CCPL_register_root_component before calling interface CCPL_get_id_of_component. Please check the model code related to the annotation \"%s\".", annotation);

	Comp_comm_group_mgt_node *node = comp_comm_group_mgt_mgr->search_global_node(local_comp_name);

	if (node == NULL) {
		EXECUTION_REPORT(REPORT_ERROR, -1, false, "no component named \"%s\" has been registerred. Please check the model code related to the annotation \"%s\"", local_comp_name, annotation);
		*comp_id = -1;
	}
	else *comp_id = node->get_local_node_id();
	
}


extern "C" void get_current_proc_id_in_comp_(int *comp_id, int *proc_id, const char * annotation)
{
	*proc_id = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(*comp_id, annotation);
}


extern "C" void get_num_proc_in_comp_(int *comp_id, int *num_proc, const char * annotation)
{
	*num_proc = comp_comm_group_mgt_mgr->get_num_proc_in_comp(*comp_id, annotation);
}


extern "C" void end_registration_(int *comp_id, const char * annotation)
{
	if (strlen(annotation) != 0)
		EXECUTION_REPORT(REPORT_ERROR,-1, comp_comm_group_mgt_mgr != NULL, "Please call interface CCPL_register_root_component before calling interface CCPL_end_comp_registration (corresponding to annotation \"%s\")", annotation);
	else EXECUTION_REPORT(REPORT_ERROR,-1, comp_comm_group_mgt_mgr != NULL, "Please call interface CCPL_register_root_component before calling interface CCPL_end_comp_registration");

	synchronize_comp_processes_for_API(*comp_id, API_ID_COMP_MGT_END_COMP_REG, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(*comp_id, "C-Coupler code in register_component for getting component management node"), "first synchorization for ending the registration of a component", annotation);	
	comp_comm_group_mgt_mgr->merge_comp_comm_info(*comp_id, annotation);
	inout_interface_mgr->merge_inout_interface_fields_info(*comp_id);
	if (((*comp_id) & TYPE_ID_SUFFIX_MASK) == 1) {
		Coupling_generator *coupling_generator = new Coupling_generator();
		coupling_generator->generate_coupling_procedures();
		coupling_generator->generate_IO_procedures();
		delete coupling_generator;
	}

	synchronize_comp_processes_for_API(*comp_id, API_ID_COMP_MGT_END_COMP_REG, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(*comp_id, "C-Coupler code in register_component for getting component management node"), "second synchorization for ending the registration of a component", annotation);
}


extern "C" void register_cor_defined_grid_(int *comp_id, const char *CCPL_grid_name, const char *CoR_grid_name, const char *annotation, int *grid_id)
{
	char local_CCPL_grid_name[NAME_STR_SIZE], local_CoR_grid_name[NAME_STR_SIZE];


	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr != NULL, "No component has been registered. Please call interface CCPL_register_root_component before calling interface CCPL_get_id_of_component. Please check the model code related to the annotation \"%s\".", annotation);
	synchronize_comp_processes_for_API(*comp_id, API_ID_GRID_MGT_REG_GRID_VIA_COR, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(*comp_id, "C-Coupler code in register_cor_defined_grid for getting component management node"), "registering a grid based on a CoR grid", annotation);
	strcpy(local_CCPL_grid_name, CCPL_grid_name);
	strcpy(local_CoR_grid_name, CoR_grid_name);
	check_and_verify_name_format_of_string_for_API(*comp_id, local_CCPL_grid_name, API_ID_GRID_MGT_REG_GRID_VIA_COR, "the C-Coupler grid", annotation);
	check_and_verify_name_format_of_string_for_API(*comp_id, local_CoR_grid_name, API_ID_GRID_MGT_REG_GRID_VIA_COR, "the CoR grid", annotation);
	comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(*comp_id, API_ID_GRID_MGT_REG_GRID_VIA_COR, annotation);
	check_API_parameter_string(*comp_id, API_ID_GRID_MGT_REG_GRID_VIA_COR, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(*comp_id, "C-Coupler code in register_cor_defined_grid for getting component management node"), "registering grid", local_CCPL_grid_name, "CCPL_grid_name", annotation);
	check_API_parameter_string(*comp_id, API_ID_GRID_MGT_REG_GRID_VIA_COR, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(*comp_id, "C-Coupler code in register_cor_defined_grid for getting component management node"), "registering grid", local_CoR_grid_name, "CoR_grid_name", annotation);
	*grid_id = original_grid_mgr->get_CoR_defined_grid(*comp_id, local_CCPL_grid_name, local_CoR_grid_name, annotation);
	printf("grid id is %d %lx\n", *grid_id, *grid_id);
}


extern "C" void get_grid_size_(int *grid_id, int *grid_size, const char *annotation)
{
	*grid_size = original_grid_mgr->get_grid_size(*grid_id, annotation);
}


extern "C" void register_parallel_decomposition_(int *decomp_id, int *grid_id, int *num_local_cells, int *array_size, const int *local_cells_global_indx, const char *decomp_name, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, *num_local_cells >= 0, "Parallel decomposition \"%s\" cannot be registered because the number of local cells is smaller than 0. Please check the model code related to \"%s\"",
		             decomp_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, -1, *num_local_cells <= *array_size, "Parallel decomposition \"%s\" cannot be registered because the number of local cells is larger than the size of the array of local cells' global indexes. Please check the model code related to \"%s\"",
		             decomp_name, annotation);
	EXECUTION_REPORT(REPORT_WARNING, -1, *num_local_cells == *array_size, "The number of local cells is different from the size of the array of local cells' global indexes when registering parallel decomposition \"%s\". Please check the model code related to \"%s\"",
		             decomp_name, annotation);
	*decomp_id = decomps_info_mgr->register_H2D_parallel_decomposition(decomp_name, *grid_id, *num_local_cells, local_cells_global_indx, annotation);
}


extern "C" void register_external_field_instance_(int *field_instance_id, const char *field_name, void *data_buffer, int *field_size, int *decomp_id, int *comp_or_grid_id, 
	                                             int *buf_mark, const char *unit, const char *data_type, const char *annotation)
{
	*field_instance_id = memory_manager->register_external_field_instance(field_name, data_buffer, *field_size, *decomp_id, *comp_or_grid_id, *buf_mark, unit, data_type, annotation);
}


extern "C" void register_an_io_field_from_field_instance_(int *field_inst_id, const char *field_IO_name, const char *annotation)
{
	IO_fields_mgr->register_IO_field(*field_inst_id, field_IO_name, annotation);
}


extern "C" void register_a_new_io_field_(int *comp_or_grid_id, int *decomp_id, int *field_size, void *data_buffer, const char *field_IO_name, 
	                                    const char *long_name, const char *unit, const char *data_type, const char * annotation)
{
	IO_fields_mgr->register_IO_field(*comp_or_grid_id, *decomp_id, *field_size, data_buffer, field_IO_name, long_name, unit, data_type, annotation);
}


extern "C" void define_single_timer_(int *comp_id, int *timer_id, const char *freq_unit, int *freq_count, int *del_count, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(*comp_id), "The component id is wrong when defining a timer. Please check the model code with the annotation \"%s\"", annotation);
	comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(*comp_id, API_ID_TIME_MGT_DEFINE_SINGLE_TIMER, annotation);
	EXECUTION_REPORT(REPORT_ERROR, *comp_id, components_time_mgrs->get_time_mgr(*comp_id)->get_time_step_in_second() > 0, "The time step of the component \%s\" has not been set yet. Please specify the time step before defining a timer at the model code with the annotation \"%s\"", 
		             comp_comm_group_mgt_mgr->get_global_node_of_local_comp(*comp_id, annotation)->get_comp_name(), annotation);
	*timer_id = timer_mgr2->define_timer(*comp_id, freq_unit, *freq_count, *del_count, annotation);
	printf("timer comp id is %x  %x at %lx\n", *timer_id, timer_mgr2->get_timer(*timer_id)->get_comp_id(), timer_mgr2->get_timer(*timer_id));
}


extern "C" void define_complex_timer_(int *comp_id, int *timer_id, int *children_timers_id, int *num_children_timers, int *or_or_and, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(*comp_id), "The component id is wrong when defining a timer. Please check the model code with the annotation \"%s\"", annotation);
	comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(*comp_id, API_ID_TIME_MGT_DEFINE_COMPLEX_TIMER, annotation);
	EXECUTION_REPORT(REPORT_ERROR, *comp_id, components_time_mgrs->get_time_mgr(*comp_id)->get_time_step_in_second() > 0, "The time step of the component \%s\" has not been set yet. Please specify the time step before defining a timer at the model code with the annotation \"%s\"", 
		             comp_comm_group_mgt_mgr->get_global_node_of_local_comp(*comp_id, annotation)->get_comp_name(), annotation);
	*timer_id = timer_mgr2->define_timer(*comp_id, children_timers_id, *num_children_timers, *or_or_and, annotation);
}


extern "C" void set_component_time_step_(int *comp_id, int *time_step_in_second, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(*comp_id), "The component id is wrong when setting the step of a component. Please check the model code with the annotation \"%s\"", annotation);
	comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(*comp_id, API_ID_TIME_MGT_SET_TIME_STEP, annotation);
	synchronize_comp_processes_for_API(*comp_id, API_ID_TIME_MGT_SET_TIME_STEP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(*comp_id, "C-Coupler code in set_component_time_step_"), "setting the time step of a component", annotation);
	check_API_parameter_int(*comp_id, API_ID_TIME_MGT_SET_TIME_STEP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(*comp_id,"C-Coupler code in set_component_time_step_"), "setting the time step of a component", *time_step_in_second, "the value of the time step (the unit is seconds)", annotation);
	components_time_mgrs->set_component_time_step(*comp_id, *time_step_in_second, annotation);
}


extern "C" void advance_component_time_(int *comp_id, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(*comp_id), "The component id is wrong when advance the time step of a component. Please check the model code with the annotation \"%s\"", annotation);	
	printf("before IO output\n");
	components_IO_output_procedures_mgr->get_component_IO_output_procedures(*comp_id)->execute();
	printf("after IO output\n");
	components_time_mgrs->advance_component_time(*comp_id, annotation);
}


extern "C" void check_ccpl_component_current_time_(int *comp_id, int *date, int *second, const char *annotation)
{
	components_time_mgrs->check_component_current_time(*comp_id, *date, *second, annotation);
}


extern "C" void is_ccpl_timer_on_(int *timer_id, int *is_on, const char *annotation)
{
	if (timer_mgr2->is_timer_on(*timer_id, annotation))
		*is_on = 1;
	else *is_on = 0;
}


extern "C" void check_is_ccpl_model_run_ended_(int *comp_id, int *is_ended, const char *annotation)
{
	if (components_time_mgrs->is_model_run_ended(*comp_id, annotation))
		*is_ended = 1;
	else *is_ended = 0;
}


extern "C" void register_inout_interface_(const char *interface_name, int *interface_id, int *import_or_export, int *num_fields, int *field_ids, int *timer_ids, int *inst_or_aver, const char *annotation, int *array_size1, int *array_size2, int *array_size3)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, *array_size1 >= *num_fields, "When registering an import/export interface named \"%s\", the size of the array for the IDs of field instances is smaller than the parameter \"num_field_instances\". Please verify the model code with the annotation \"%s\"",
		             interface_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, -1, *array_size2 == 1 || *array_size2 >= *num_fields, "When registering an import/export interface named \"%s\", the size of the array for the IDs of timers is smaller than the parameter \"num_field_instances\". Please verify the model code with the annotation \"%s\"",
		             interface_name, annotation);
	if (*import_or_export == 0)
		EXECUTION_REPORT(REPORT_ERROR, -1, *array_size3 == 1 || *array_size3 >= *num_fields, "When registering an import/export interface named \"%s\", the size of the array for specifying instantaneous or average value is smaller than the parameter \"num_field_instances\". Please verify the model code with the annotation \"%s\"",
			             interface_name, annotation);
	if (*import_or_export == 0)
		*interface_id = inout_interface_mgr->register_inout_interface(interface_name, *import_or_export, *num_fields, field_ids, timer_ids, inst_or_aver, annotation, *array_size2, *array_size3, INTERFACE_TYPE_REGISTER);
	else *interface_id = inout_interface_mgr->register_inout_interface(interface_name, *import_or_export, *num_fields, field_ids, timer_ids, NULL, annotation, *array_size2, *array_size3, INTERFACE_TYPE_REGISTER);
}


extern "C" void execute_inout_interface_with_id_(int *interface_id, int *bypass_timer, const char *annotation)
{
	inout_interface_mgr->execute_interface(*interface_id, *bypass_timer == 1, annotation);
}


extern "C" void execute_inout_interface_with_name_(int *comp_id, const char *interface_name, int *bypass_timer, const char *annotation)
{
	inout_interface_mgr->execute_interface(*comp_id, interface_name, *bypass_timer == 1, annotation);
}

