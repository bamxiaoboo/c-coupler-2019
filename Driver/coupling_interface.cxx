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
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "coupling_interface.h"


int coupling_process_control_counter = 0;


extern "C" void register_model_data_(void *model_buf, int *data_size, const char *decomp_name, const char *field_name, const char *data_type, const char *grid_name, int *have_fill_value, void *fill_value, bool *is_restart_field)
{
    EXECUTION_REPORT(REPORT_ERROR, coupling_process_control_counter > 0, "C-Coupler interface coupling_interface_initialize has not been called\n"); 
    if ((*have_fill_value) == 1)
        memory_manager->register_model_data_buf(decomp_name, field_name, data_type, model_buf, grid_name, fill_value, *is_restart_field, *data_size);
    else memory_manager->register_model_data_buf(decomp_name, field_name, data_type, model_buf, grid_name, NULL, *is_restart_field, *data_size);
}


extern "C" void coupling_get_field_size_(void *model_buf, const char *annotation, int *field_size)
{
    EXECUTION_REPORT(REPORT_ERROR, coupling_process_control_counter > 0, 
		             "C-Coupler interface coupling_interface_initialize has not been called before running the code corresponding to annotation \"%s\"\n", annotation); 
	*field_size = memory_manager->get_field_size(model_buf, annotation);
}


extern "C" void export_field_data_(void *model_buf, int *data_size, const char *field_name, const char *decomp_name, const char *grid_name, const char *data_type)
{
    EXECUTION_REPORT(REPORT_ERROR, coupling_process_control_counter > 0, 
                 "C-Coupler interface coupling_interface_initialize has not been called\n"); 
    memory_manager->export_field_data(model_buf, field_name, decomp_name, grid_name, data_type, *data_size);
}


extern "C" void withdraw_model_data_(const char *decomp_name, const char *field_name, const char *grid_name)
{
    EXECUTION_REPORT(REPORT_ERROR, coupling_process_control_counter > 0, 
                 "C-Coupler interface coupling_interface_initialize has not been called\n"); 
    memory_manager->withdraw_model_data_buf(decomp_name, field_name, grid_name);
}


extern "C" void register_sigma_grid_bottom_field_(void *model_buf, const char *grid_name)
{
	Field_mem_info *bottom_field;
	Remap_grid_class *field_grid, *sigma_grid;


	sigma_grid = remap_grid_manager->search_remap_grid_with_grid_name(grid_name);
	EXECUTION_REPORT(REPORT_ERROR, sigma_grid != NULL, "\"%s\" has not been defined in the CoR script", grid_name);
	EXECUTION_REPORT(REPORT_ERROR, sigma_grid->is_sigma_grid(), "grid \"%s\" is not a sigma grid", grid_name);

	bottom_field = memory_manager->search_field_via_data_buf(model_buf, false);
	EXECUTION_REPORT(REPORT_ERROR, bottom_field != NULL && bottom_field->get_is_registered_model_buf(), "the model bottom field for the sigma grid \"%s\" has not been registered to C-Coupler", grid_name);
	EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(bottom_field->get_grid_name(), "NULL"), "scalar model variable \"%s\" cannot be used as the model bottom field for a sigma grid", bottom_field->get_field_name());

	field_grid = remap_grid_manager->search_remap_grid_with_grid_name(bottom_field->get_grid_name());
	EXECUTION_REPORT(REPORT_ERROR, field_grid != NULL, "C-Coupler error in register_sigma_grid_bottom_field");
	EXECUTION_REPORT(REPORT_ERROR, field_grid->get_is_sphere_grid(), "field \"%s\" that will be set as the model bottom field for the sigma grid \"%s\" is not on a sphere grid: \"%s\" is not a sphere grid",
                     bottom_field->get_field_name(), grid_name, field_grid->get_grid_name());
	EXECUTION_REPORT(REPORT_ERROR, field_grid->is_subset_of_grid(sigma_grid), "the grid \"%s\" for the grid bottom field is not a sub grid for the sigma grid \"%s\"", 
					 bottom_field->get_grid_name(), grid_name);
	EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(bottom_field->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT) || words_are_the_same(bottom_field->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE),
					 "the data type of the bottom field for the sigma grid \"%s\" must be real4 or real8", grid_name); 
	// check the whether the parallel decomposition covers all grid points
	EXECUTION_REPORT(REPORT_LOG, true, "Register surface field of grid %s on decomposition %s", grid_name, bottom_field->get_decomp_name());
	decomp_grids_mgr->search_decomp_grid_info(bottom_field->get_decomp_name(), sigma_grid, true)->get_decomp_grid()->set_sigma_grid_dynamic_surface_value_field(bottom_field->get_field_data());
}                  


extern "C" void coupling_add_decomposition_(const char *decomp_name, const char *grid_name,
                                            int *num_cells_in_decomp, int *decomp_cell_indexes)
{
    EXECUTION_REPORT(REPORT_ERROR, coupling_process_control_counter > 0, 
                 "C-Coupler interface coupling_interface_initialize has not been called\n");
    decomps_info_mgr->add_decomp_from_model_interface(decomp_name, grid_name, *num_cells_in_decomp, decomp_cell_indexes);
}


extern "C" void initialize_coupling_managers_(int *restart_date, int *restart_second, const char *restart_read_file)
{
    FILE *root_cfg_fp;
    FILE *tmp_cfg_fp;
    char root_cfg_name[NAME_STR_SIZE];
    char line[NAME_STR_SIZE*16];
	char shared_field_attribute[NAME_STR_SIZE*16], private_field_attribute[NAME_STR_SIZE*16];
    char alg_name[NAME_STR_SIZE];
    char full_name[NAME_STR_SIZE];
    char *line_p;


	global_algorithm_id = 0;
	execution_phase_number = 2;

	performance_timing_mgr = new Performance_timing_mgt();
	external_algorithm_mgr = new External_algorithm_mgt();

    strcpy(root_cfg_name, compset_communicators_info_mgr->get_current_comp_name());
    strcat(root_cfg_name, "_coupling.cfg");
    EXECUTION_REPORT(REPORT_LOG, true, "root runtime configuration file name is %s", root_cfg_name);

    root_cfg_fp = open_config_file(root_cfg_name);

    EXECUTION_REPORT(REPORT_LOG, true, "execute CoR to generate grid and remap operators");
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, root_cfg_fp), "Please specify the configuration file (a CoR script) for grid management and data interpolation in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.", root_cfg_name);
    sprintf(full_name, "%s/\0", C_COUPLER_CONFIG_DIR);
    strcat(full_name, line);
    execution_phase_number = 1;
	if (words_are_the_same(line, "NULL"))
		grid_remap_mgr = NULL;
	else grid_remap_mgr = new Remap_mgt(full_name);
	line_number = -1;
	execution_phase_number = 2;
	
    EXECUTION_REPORT(REPORT_LOG, true, "build fields info");
    /* Generate the data structure for managing field info */
	EXECUTION_REPORT(REPORT_ERROR, get_next_line(shared_field_attribute, root_cfg_fp), "Please specify the configuration file for the field attributes shared by all components in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.", root_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR, get_next_line(private_field_attribute, root_cfg_fp), "Please specify the configuration file for the field attributes privately owned by the component \"%s\" in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.", 
		             compset_communicators_info_mgr->get_current_comp_name(), root_cfg_name);
    fields_info = new Field_info_mgt(shared_field_attribute, private_field_attribute);

    EXECUTION_REPORT(REPORT_LOG, true, "build memory_mgt info");
    /* Generate memory management */
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, root_cfg_fp), "Please specify the configuration file for the field instances that will be registered by the code of component \"%s\" in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.",
		             compset_communicators_info_mgr->get_current_comp_name(), root_cfg_name);    
    memory_manager = new Memory_mgt(line);

    /* Initialize the objects for parallel decomposition */
    decomps_info_mgr = new Decomp_info_mgt();
    decomp_grids_mgr = new Decomp_grid_mgt();
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, root_cfg_fp), "Please specify the configuration file for the default parallel decompositions of component \"%s\" in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.",
					 compset_communicators_info_mgr->get_current_comp_name(), root_cfg_name);
    EXECUTION_REPORT(REPORT_LOG, true, "build decomposition info %s", line);
    decomps_info_mgr->add_decomps_from_cfg_file(line);
    
    EXECUTION_REPORT(REPORT_LOG, true, "build routing info manager");
    routing_info_mgr = new Routing_info_mgt();

	EXECUTION_REPORT(REPORT_LOG, true, "build runtime process manager");    
    runtime_process_mgr = new Runtime_process_mgt();
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, root_cfg_fp), "Please specify the configuration file for the runtime algorithms of component \"%s\" in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.",
					 compset_communicators_info_mgr->get_current_comp_name(), root_cfg_name);
    runtime_process_mgr->add_runtime_algorithms(line);
    EXECUTION_REPORT(REPORT_ERROR, get_next_line(line, root_cfg_fp), "Please specify the configuration file for the runtime procedures of component \"%s\" in the configuration file \"%s\". Please specify \"NULL\" when there is no such configuration file.",
					 compset_communicators_info_mgr->get_current_comp_name(), root_cfg_name);
    runtime_process_mgr->add_runtime_procedures(line);

    fields_gather_scatter_mgr = new Fields_gather_scatter_mgt();

    restart_mgr = new Restart_mgt(*restart_date, *restart_second, restart_read_file);

	ensemble_mgr = new Ensemble_mgt();

	datamodel_field_read_handler_mgr = new Datamodel_field_read_handler_mgt();

    fclose(root_cfg_fp);
    EXECUTION_REPORT(REPORT_LOG, true, "coupling initialization finishes (%s)", root_cfg_name);
}


extern "C" void finalize_coupling_managers_()
{
	performance_timing_mgr->performance_timing_output();
	delete performance_timing_mgr;
    EXECUTION_REPORT(REPORT_LOG, true, "finish deleting performance_timing_mgr");
	if (grid_remap_mgr != NULL)
        delete grid_remap_mgr;
    EXECUTION_REPORT(REPORT_LOG, true, "finish deleting grid managers");
    delete fields_info;
    EXECUTION_REPORT(REPORT_LOG, true, "finish deleting fields info");
    delete memory_manager;
    EXECUTION_REPORT(REPORT_LOG, true, "delete memory manager");
    delete decomps_info_mgr;
    EXECUTION_REPORT(REPORT_LOG, true, "finish deleting decomposition info manager");
    delete routing_info_mgr;
    EXECUTION_REPORT(REPORT_LOG, true, "finish deleting routers info manager");
    delete runtime_process_mgr;
    EXECUTION_REPORT(REPORT_LOG, true, "finish deleting runtime process manager");
    delete restart_mgr;
    EXECUTION_REPORT(REPORT_LOG, true, "finish deleting restart manager");
    delete fields_gather_scatter_mgr;
    EXECUTION_REPORT(REPORT_LOG, true, "before deleting time manager");
	delete timer_mgr;
	if (restart_read_timer_mgr != NULL)
		delete restart_read_timer_mgr;
    EXECUTION_REPORT(REPORT_LOG, true, "before deleting communicator manager");
    delete compset_communicators_info_mgr;
	delete ensemble_mgr;
	delete datamodel_field_read_handler_mgr;
}


extern "C" int comm_initialize_(const char *exp_model, const char *current_comp_name, const char *compset_filename, MPI_Comm *comm, const char *case_name, 
                                const char *case_desc, const char *case_mode, const char *comp_namelist,
                                const char *current_config_time, const char *original_case_name, const char *original_config_time)
{
	strcpy(software_name, "C-Coupler");
	execution_phase_number = 2;
    compset_communicators_info_mgr = new Compset_communicators_info_mgt(exp_model, current_comp_name, compset_filename, case_name, case_desc, case_mode, comp_namelist, current_config_time, original_case_name, original_config_time);
    *comm = compset_communicators_info_mgr->get_current_comp_comm_group();
    EXECUTION_REPORT(REPORT_ERROR, coupling_process_control_counter == 0, "the first coupling interface to run is coupling_interface_init\n");
	EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(case_mode, "initial") || words_are_the_same(case_mode, "restart") || words_are_the_same(case_mode, "hybrid"), "run type must be initial, restart or hybrid\n");
	EXECUTION_REPORT(REPORT_LOG, true, "The %d process of the current component is run on the host %s", compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group(), compset_communicators_info_mgr->get_host_computing_node_name());
    coupling_process_control_counter = 1;
    return 0;
}


extern "C" void coupling_initialize_ensemble_manager_(int *ensemble_id, int *have_random_seed_for_perturbation, int *root_random_seed_for_perturbation, const char *perturbation_type)
{
	EXECUTION_REPORT(REPORT_ERROR, ensemble_mgr != NULL, "C-Coupler software error: ensemble_mgr is not created before the initialization");
	ensemble_mgr->Initialize(*ensemble_id, *have_random_seed_for_perturbation, *root_random_seed_for_perturbation, perturbation_type);
}


extern "C" void coupling_add_field_for_perturbing_roundoff_errors_(void *data_buf)
{
	ensemble_mgr->register_a_field_for_perturbation(data_buf);
}


extern "C" void coupling_execute_procedure_(const char *procedure_name, const char *procedure_stage)
{
    EXECUTION_REPORT(REPORT_ERROR, coupling_process_control_counter > 0, 
                 "C-Coupler interface coupling_interface_initialize has not been called\n");
    runtime_process_mgr->execute_coupling_procedure(procedure_name, procedure_stage);
}


extern "C" void coupling_perturb_roundoff_errors_for_an_array_(void *field_data_buf, const char *data_type, int *field_size)
{
	ensemble_mgr->perturb_a_model_array(field_data_buf, data_type, *field_size);
}


extern "C" void coupling_perturb_roundoff_errors_()
{
	ensemble_mgr->run();
}


extern "C" void coupling_advance_timer_()
{
    timer_mgr->advance_coupling_step();
	if (restart_read_timer_mgr != NULL)
		restart_read_timer_mgr->advance_coupling_step();
	ensemble_mgr->run();
}


extern "C" void coupling_check_coupled_run_finished_(bool *is_coupled_run_finished)
{
    *is_coupled_run_finished = timer_mgr->check_is_coupled_run_finished();
}


extern "C" void coupling_check_coupled_run_restart_time_(bool *is_restart_time)
{
    *is_restart_time = timer_mgr->check_is_coupled_run_restart_time();
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


extern "C" void coupling_get_current_comp_comm_(MPI_Comm *local_comm)
{
    *local_comm = compset_communicators_info_mgr->get_current_comp_comm_group();
}


extern "C" void coupling_get_float_current_calendar_time_(float *cal_time, int *shift_seconds)
{
    *cal_time = timer_mgr->get_float_current_calendar_time(*shift_seconds);
}


extern "C" void initialize_coupler_timer_(const int *start_date, const int *start_second, const int *stop_date, const int *stop_second, const int *reference_date, const bool *leap_year_on, 
	                          const int *cpl_step, const char *rest_freq_unit, const int *rest_freq_count, const int *stop_latency_seconds)
{
	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "restart") || words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "hybrid"))
		restart_read_timer_mgr = new Timer_mgt(*start_date, *start_second, *stop_date, *stop_second, *reference_date, *leap_year_on, *cpl_step, rest_freq_unit, *rest_freq_count, *stop_latency_seconds);
    timer_mgr = new Timer_mgt(*start_date, *start_second, *stop_date, *stop_second, *reference_date, *leap_year_on, *cpl_step, rest_freq_unit, *rest_freq_count, *stop_latency_seconds);
}


extern "C" void coupling_check_grid_values_consistency_(const char *decomp_name, const char *grid_name, const char *label, const char *data_type, const void *grid_values)
{
    bool check_result;


    EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(grid_name) != NULL, "grid %s is not defined when checking the consistency of grid values\n", grid_name);
    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(label, COORD_LABEL_LON) || words_are_the_same(label, COORD_LABEL_LAT) || words_are_the_same(label, COORD_LABEL_LEV) || words_are_the_same(label, GRID_MASK_LABEL), 
                 "the label of grid values for checking consistency must be one of lon, lat, lev and mask\n");
    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(data_type, DATA_TYPE_INT) || words_are_the_same(data_type, DATA_TYPE_FLOAT) || words_are_the_same(data_type, DATA_TYPE_DOUBLE), 
                 "C-Coupler error in check_grid_values_consistency_\n");
    if (words_are_the_same(data_type, DATA_TYPE_INT))
        EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(label, GRID_MASK_LABEL), "when the data type of grid values for checking consistency is integer, the label must be mask\n");

    Decomp_grid_info *decomp_grid = decomp_grids_mgr->search_decomp_grid_info(decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(grid_name), false);
	if (decomp_grid->get_decomp_grid() == NULL)
		return;
    
    if (words_are_the_same(label, COORD_LABEL_LON) || words_are_the_same(label, COORD_LABEL_LAT) || words_are_the_same(label, COORD_LABEL_LEV))
        check_result = decomp_grid->get_decomp_grid()->check_coord_values_consistency(label, data_type, grid_values);
    else if (words_are_the_same(label, GRID_MASK_LABEL))
        check_result = decomp_grid->get_decomp_grid()->check_mask_values_consitency(data_type, grid_values);

    EXECUTION_REPORT(REPORT_ERROR, check_result, "the consistency checking of <%s %s %s> failed\n", decomp_name, grid_name, label);
}


extern "C" void coupling_do_restart_write_()
{
    restart_mgr->do_restart_write();
}


extern "C" void coupling_do_restart_read_()
{
	restart_mgr->read_restart_fields_on_restart_date();
	memory_manager->check_all_restart_fields_have_been_read();
}


extern "C" void coupling_is_first_step_(bool *result)
{
	*result = false;
	if (!words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"))
		return;

	if (timer_mgr->get_current_num_time_step() == 0)
		*result = true;
}


extern "C" void coupling_is_first_restart_step_(bool *result)
{
	*result = false;
	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"))
		return;

	if (restart_read_timer_mgr->get_current_num_time_step() - restart_mgr->get_restart_read_num_time_step() <= 1)
		*result = true;
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
    *step_size = timer_mgr->get_comp_frequency();
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
	EXECUTION_REPORT(REPORT_ERROR, false, error_string);
}


extern "C" void coupling_is_model_data_renewed_in_current_time_step_(void *model_data, int *result)
{
	if (memory_manager->is_model_data_renewed_in_current_time_step(model_data))
		*result = 1;
	else *result = 0;
}


extern "C" void coupling_is_model_data_active_in_coupling_(void *model_data, int *result)
{
	if (memory_manager->is_model_data_active_in_coupling(model_data))
		*result = 1;
	else *result = 0;
}


extern "C" void coupling_log_case_info_in_netcdf_file_(int *ncfile_id, int *not_at_def_mode)
{
	IO_netcdf *model_ncfile = new IO_netcdf(*ncfile_id);
	int rcode;

	if (*not_at_def_mode == 0) {
		rcode = nc_enddef(*ncfile_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for model file when logging case information\n", nc_strerror(rcode));;
	}
	compset_communicators_info_mgr->write_case_info(model_ncfile);
	if (*not_at_def_mode == 0) {
		rcode = nc_redef(*ncfile_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for model file when logging case information\n", nc_strerror(rcode));
	}	
}


extern "C" void coupling_check_sum_for_external_data_(void *external_data, int *size, const char *data_type, const char *hint)
{
	int partial_sum = 0, total_sum, i;
	int data_type_size = get_data_type_size(data_type);
	int int_array_size = ((*size)*data_type_size+3)/4;
	int *new_data_buf = new int [int_array_size];
	

	memset(new_data_buf, 0, sizeof(int)*int_array_size);
	memcpy(new_data_buf, external_data, (*size)*data_type_size);
	for (i = 0; i < int_array_size; i ++)
		partial_sum += new_data_buf[i];
	delete [] new_data_buf;
	
    MPI_Allreduce(&partial_sum, &total_sum, 1, MPI_INT, MPI_SUM, compset_communicators_info_mgr->get_current_comp_comm_group());
    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) 
        EXECUTION_REPORT(REPORT_PROGRESS, true, "check sum of external data of \"%s\" is %lx", hint, total_sum);
}


void C_Coupler_interface_register_model_algorithm(const char *algorithm_name, Model_algorithm model_algorithm)
{
	runtime_process_mgr->register_model_algorithm(algorithm_name, model_algorithm);
}


extern "C" void coupling_check_sum_for_all_fields_()
{
	EXECUTION_REPORT(REPORT_ERROR, memory_manager != NULL, "C-Coupler is not initialized when the component call the interface for checking sum of all fields managed by the C-Coupler");
	memory_manager->check_sum_of_all_fields();
}


extern "C" void coupling_add_field_info_(const char *field_name, const char *field_unit, const char *field_long_name)
{
	EXECUTION_REPORT(REPORT_ERROR, fields_info != NULL, "the C-Coupler manager for the information of fields is not initialized");
	fields_info->add_field_info(field_name, field_long_name, field_unit, "none");
}

