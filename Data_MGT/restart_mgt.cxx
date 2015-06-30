/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "common_utils.h"
#include "restart_mgt.h"
#include "runtime_config_dir.h"
#include "global_data.h"
#include "cor_global_data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


bool Restart_mgt::is_in_restart_write_time_window()
{
    int diff_time_step;


    diff_time_step = timer_mgr->get_current_num_time_step() - current_restart_num_time_step;
    EXECUTION_REPORT(REPORT_LOG, true, "time step diff is %d", diff_time_step);
    EXECUTION_REPORT(REPORT_ERROR, diff_time_step >= 0, "C-Coupler error in is_in_restart_write_time_window\n");
    if (diff_time_step*timer_mgr->get_comp_frequency() > timer_mgr->get_comp_stop_latency_seconds())
        return false;

    EXECUTION_REPORT(REPORT_LOG, true, "is in restart window %ld", timer_mgr->get_current_full_time());
    return true;
}


bool Restart_mgt::is_in_restart_read_time_window()
{
	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"))
		return false;
	
	EXECUTION_REPORT(REPORT_ERROR, restart_read_timer_mgr->get_current_num_time_step() >= restart_read_num_time_step && restart_read_num_time_step >= 0, 
	             "C-Coupler software error in is_in_restart_read_time_window\n");
	return (restart_read_timer_mgr->get_current_num_time_step()-restart_read_num_time_step)*restart_read_timer_mgr->get_comp_frequency() <= restart_read_timer_mgr->get_comp_stop_latency_seconds();
}



bool Restart_mgt::check_is_restart_timer_on()
{
	EXECUTION_REPORT(REPORT_LOG, true, "check is restart timer on");

    if (timer_mgr->check_is_coupled_run_restart_time()) {
        current_restart_full_time = timer_mgr->get_current_full_time();
        current_restart_num_time_step = timer_mgr->get_current_num_time_step(); 
		if (restart_write_nc_file != NULL) {
            delete restart_write_nc_file;
            restart_write_nc_file = NULL;
        }
		return true;
    }

	EXECUTION_REPORT(REPORT_LOG, true, "restart timer is not on");
	return false;
} 


void Restart_mgt::do_restart_write()
{
    if (!check_is_restart_timer_on())
		return;

    runtime_process_mgr->write_restart_fields();
	memory_manager->write_restart_fields();
}


Restart_mgt::Restart_mgt(int restart_date, int restart_second, const char *restart_read_file)
{
    char line[NAME_STR_SIZE],  comp_name[NAME_STR_SIZE], decomp_name[NAME_STR_SIZE], grid_name[NAME_STR_SIZE], field_name[NAME_STR_SIZE];
    char *line_p;
    int buf_mark;
    Field_mem_info *restart_field_mem;
    FILE *fp_cfg;
	bool restart_file_exist = false;
	FILE *fp_tmp;


	EXECUTION_REPORT(REPORT_ERROR, restart_second >=0 && restart_second <= 86400, "restart_second of simulation run must between 0 and 86400");
	EXECUTION_REPORT(REPORT_ERROR, restart_second%timer_mgr->get_comp_frequency()==0 && restart_second%timer_mgr->get_comp_frequency()==0, "restart_second of simulation run must integer multiple of the time step");

	restart_read_fields_attr_strings = NULL;
	num_restart_read_fields_attrs = -999;
    restart_write_nc_file = NULL; 
	restart_read_nc_file = NULL;
	current_restart_num_time_step = ((int)0xffffffff);
	current_restart_full_time = -1;
	restart_read_num_time_step = ((int)0xffffffff);
	restart_begin_full_time = ((long)restart_date)*100000+restart_second;
    scalar_int_field = alloc_mem(compset_communicators_info_mgr->get_current_comp_name(), "NULL", "NULL", "scalar_int", DATA_TYPE_INT, 0, false, "  C-Coupler error  ");

	EXECUTION_REPORT(REPORT_LOG, true, "default current_restart_num_time_step is %ld", current_restart_num_time_step);
	
	strcpy(restart_read_file_name, restart_read_file);
	check_is_restart_timer_on();

	fp_tmp = fopen(restart_read_file_name, "r");
	if (fp_tmp != NULL) {
		restart_file_exist = true;
		fclose(fp_tmp);
	}
	
	if ((words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "restart")||
		  words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "hybrid")) && restart_file_exist) {
		read_check_restart_basic_info();
 		restart_read_num_time_step = restart_read_timer_mgr->get_current_num_time_step();
	}
}


Restart_mgt::~Restart_mgt()
{
	if (restart_read_fields_attr_strings != NULL)
		delete [] restart_read_fields_attr_strings;
}


int Restart_mgt::get_restart_read_field_computing_count(const char *comp_name, const char *decomp_name, const char *grid_name, 
                                                        const char *field_name, int buf_type)
{
	int i, local_buf_type, computing_count;
	long tmp_long_value, field_restart_time;

	
	for (i = 0; i < num_restart_read_fields_attrs/7; i ++) {
		sscanf(restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+5), "%ld", &tmp_long_value);
		local_buf_type = (tmp_long_value & 0xffffffff);
		if (words_are_the_same(comp_name, restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+0)) &&
			words_are_the_same(decomp_name, restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+1)) &&
			words_are_the_same(grid_name, restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+2)) &&
			words_are_the_same(field_name, restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+3)) &&
			local_buf_type == buf_type) {
			computing_count = (tmp_long_value >> 32);
			sscanf(restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+6), "%ld", &field_restart_time);			
			EXECUTION_REPORT(REPORT_ERROR, field_restart_time+restart_read_timer_mgr->get_comp_frequency() >= restart_read_timer_mgr->get_current_full_time(), "C-Coupler error1 in get_restart_read_field_computing_count\n");
			return computing_count;
		}
	}

	return -1;
}


void Restart_mgt::log_last_restart_output_info()
{
	FILE *fp_log;

	
	EXECUTION_REPORT(REPORT_ERROR, restart_write_nc_file == NULL, "C-Coupler error in log_last_restart_output_info\n");

	if (restart_write_nc_file == NULL && compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
		fp_log = fopen("last_restart_output_info.log", "w+");
		fprintf(fp_log, "    original_case_name=%s\n", compset_communicators_info_mgr->get_current_case_name());
		fprintf(fp_log, "    original_config_time=%s\n", compset_communicators_info_mgr->get_current_config_time());
		fprintf(fp_log, "    run_restart_date=%04d-%02d-%02d\n", timer_mgr->get_current_year(), timer_mgr->get_current_month(), timer_mgr->get_current_day());
		fprintf(fp_log, "    run_restart_second=%05d\n", timer_mgr->get_current_second()); 
		fclose(fp_log);
	}
}


void Restart_mgt::create_restart_write_nc_file()
{
    char full_file_name[256];
	int ncfile_id, rcode, dim_ids[2], restart_vars_attrs_id;


    EXECUTION_REPORT(REPORT_ERROR, restart_write_nc_file == NULL, "C-Coupler error in create_restart_write_nc_file\n");
    if (restart_write_nc_file == NULL && compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
        sprintf(full_file_name, "%s.%s.%s.%04d%04d%05d.nc", compset_communicators_info_mgr->get_current_case_name(), compset_communicators_info_mgr->get_current_comp_name(), 
			    "restart", current_restart_full_time/((long)1000000000), current_restart_full_time%((long)1000000000)/100000,
			    current_restart_full_time%100000);
        EXECUTION_REPORT(REPORT_LOG, true, "create restart nc file %s", full_file_name);
        restart_write_nc_file = new IO_netcdf(full_file_name, full_file_name, "w", false);
        compset_communicators_info_mgr->write_case_info(restart_write_nc_file);
		rcode = nc_open(full_file_name, NC_WRITE, &ncfile_id);
    	EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), full_file_name);
        rcode = nc_redef(ncfile_id);
        EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), full_file_name);
        rcode = nc_def_dim(ncfile_id, "num_restart_var_attrs", NC_UNLIMITED, &dim_ids[0]);
        EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), full_file_name);
		rcode = nc_def_dim(ncfile_id, "size_attr_string", NAME_STR_SIZE, &dim_ids[1]);
        EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), full_file_name);
        rcode = nc_def_var(ncfile_id, "restart_vars_attrs", NC_CHAR, 2, dim_ids, &restart_vars_attrs_id);
        EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), full_file_name);
        rcode = nc_enddef(ncfile_id);	
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), full_file_name);
		rcode = nc_close(ncfile_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), full_file_name);
	}
    strcpy(scalar_int_field->get_field_data()->get_grid_data_field()->field_name_in_IO_file, "restart_date");
    ((int*)scalar_int_field->get_data_buf())[0] = current_restart_full_time/100000;
    fields_gather_scatter_mgr->gather_write_field(restart_write_nc_file, scalar_int_field, false, -1, -1, true);
    strcpy(scalar_int_field->get_field_data()->get_grid_data_field()->field_name_in_IO_file, "start_date");
    ((int*)scalar_int_field->get_data_buf())[0] = timer_mgr->get_start_full_time()/100000;
    fields_gather_scatter_mgr->gather_write_field(restart_write_nc_file, scalar_int_field, false, -1, -1, true);
}


void Restart_mgt::write_one_restart_field(Field_mem_info *restart_field_mem, int computing_count)
{
    char full_field_name[256], attr_string[NAME_STR_SIZE];
	int ncfile_id, rcode, restart_vars_attrs_id, num_restart_var_attrs_id;
	unsigned long num_restart_var_attrs, starts[2], counts[2];
	long tmp_long_value;


	if (restart_write_nc_file == NULL) {
		log_last_restart_output_info();
		create_restart_write_nc_file();
	}

    restart_field_mem->get_field_mem_full_name(full_field_name);
    sprintf(restart_field_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, "%s_%ld", full_field_name, timer_mgr->get_current_full_time());
    EXECUTION_REPORT(REPORT_LOG, true, "write restart field %s: %ld vs %ld", restart_field_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, current_restart_full_time, timer_mgr->get_current_full_time());
	restart_field_mem->check_field_sum();
    fields_gather_scatter_mgr->gather_write_field(restart_write_nc_file, restart_field_mem, true, -1, -1, true);    

	if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
		rcode = nc_open(restart_write_nc_file->get_file_name(), NC_WRITE, &ncfile_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		rcode = nc_inq_dimid(ncfile_id, "num_restart_var_attrs", &num_restart_var_attrs_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		rcode = nc_inq_dimlen(ncfile_id, num_restart_var_attrs_id, &num_restart_var_attrs);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
        rcode = nc_inq_varid(ncfile_id, "restart_vars_attrs", &restart_vars_attrs_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		starts[1] = 0;
		counts[0] = 1;
		counts[1] = NAME_STR_SIZE;
		starts[0] = num_restart_var_attrs+0;
		strcpy(attr_string, restart_field_mem->get_comp_name());
		rcode = nc_put_vara_text(ncfile_id, restart_vars_attrs_id, starts, counts, attr_string);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		starts[0] = num_restart_var_attrs+1;
		strcpy(attr_string, restart_field_mem->get_decomp_name());
		rcode = nc_put_vara_text(ncfile_id, restart_vars_attrs_id, starts, counts, attr_string);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		starts[0] = num_restart_var_attrs+2;
		strcpy(attr_string, restart_field_mem->get_grid_name());
		rcode = nc_put_vara_text(ncfile_id, restart_vars_attrs_id, starts, counts, attr_string);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		starts[0] = num_restart_var_attrs+3;
		strcpy(attr_string, restart_field_mem->get_field_name());
		rcode = nc_put_vara_text(ncfile_id, restart_vars_attrs_id, starts, counts, attr_string);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		starts[0] = num_restart_var_attrs+4;
		strcpy(attr_string, restart_field_mem->get_field_data()->get_grid_data_field()->data_type_in_application);
		rcode = nc_put_vara_text(ncfile_id, restart_vars_attrs_id, starts, counts, attr_string);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		starts[0] = num_restart_var_attrs+5;
		tmp_long_value = computing_count;
		tmp_long_value = (tmp_long_value << 32);
		tmp_long_value = (tmp_long_value | restart_field_mem->get_buf_type());
		sprintf(attr_string, "%ld\0", tmp_long_value);
		rcode = nc_put_vara_text(ncfile_id, restart_vars_attrs_id, starts, counts, attr_string);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
		starts[0] = num_restart_var_attrs+6;
		sprintf(attr_string, "%ld\0", timer_mgr->get_current_full_time());
		rcode = nc_put_vara_text(ncfile_id, restart_vars_attrs_id, starts, counts, attr_string);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
    	rcode = nc_close(ncfile_id);
    	EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_write_nc_file->get_file_name());
	}
}


void Restart_mgt::read_one_restart_field(Field_mem_info *restart_field_mem)
{
    char full_field_name[256];

 	
    restart_field_mem->get_field_mem_full_name(full_field_name);
    sprintf(restart_field_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, "%s_%ld", full_field_name, restart_read_timer_mgr->get_current_full_time());
    EXECUTION_REPORT(REPORT_LOG, true, "read restart field %s", restart_field_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file);
    fields_gather_scatter_mgr->read_scatter_field(restart_read_nc_file, restart_field_mem, -1);    
	restart_field_mem->define_field_values(true);
	restart_field_mem->check_field_sum();
}


void Restart_mgt::get_field_datatype_for_transfer(const char *comp_name, const char *decomp_name, const char *grid_name, const char *field_name, int buf_mark, char *out_datatype)
{
	char *restart_comp_name, *restart_decomp_name, *restart_grid_name, *restart_field_name, *data_type;
	int i, restart_buf_mark;
	long tmp_long_value, field_restart_time;
	Field_mem_info *field = NULL;


	EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"), "C-Coupler software error in get_field_datatype_for_transfer");

	read_in_restart_read_fields_attrs();

	for (i = 0; i < num_restart_read_fields_attrs/7; i ++) {
		restart_comp_name = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+0);
		restart_decomp_name = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+1);
		restart_grid_name = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+2);
		restart_field_name = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+3);
		data_type = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+4);
		sscanf(restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+5), "%ld", &tmp_long_value);
		restart_buf_mark = (tmp_long_value & 0xffffffff);
		sscanf(restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+6), "%ld", &field_restart_time);
		if (field_restart_time == restart_read_timer_mgr->get_current_full_time() && buf_mark == restart_buf_mark && words_are_the_same(comp_name, restart_comp_name) &&
			words_are_the_same(decomp_name, restart_decomp_name) && words_are_the_same(grid_name, restart_grid_name) && words_are_the_same(field_name, restart_field_name)) {
			strcpy(out_datatype, data_type);
			EXECUTION_REPORT(REPORT_LOG, true, "the data type of transfer field (%s, %s, %s) is %s", comp_name, decomp_name, field_name, data_type);
			return;
		}
	}

	EXECUTION_REPORT(REPORT_ERROR, false, "Cannot find transfer field (%s, %s, %s) from restart file in restart window for the transfer algorithm", comp_name, decomp_name, field_name);
}


bool Restart_mgt::is_restart_run()
{
    return words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "restart");
}


void Restart_mgt::read_check_restart_basic_info()
{
	int restart_date;
	int start_date;
	long start_full_time, restart_full_time;
	char original_case_name[NAME_STR_SIZE], original_config_time[NAME_STR_SIZE];


    EXECUTION_REPORT(REPORT_LOG, true, "read and check restart basic info from %s", restart_read_file_name);
	restart_read_nc_file = new IO_netcdf("restart_read_file", restart_read_file_name, "r", false);

    restart_read_nc_file->get_global_text("current case name", original_case_name, NAME_STR_SIZE);
	restart_read_nc_file->get_global_text("configuration time", original_config_time, NAME_STR_SIZE);
	EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(compset_communicators_info_mgr->get_original_case_name(), original_case_name), 
		             "the original case name \"%s\" set by users are different from the name \"%s\" in the restart data file for reading",
		             compset_communicators_info_mgr->get_original_case_name(), original_case_name);
	EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(compset_communicators_info_mgr->get_original_config_time(), original_config_time), "the configuration time set by users (%s) are different from that (%s) in the restart data file for reading\n", 
	             compset_communicators_info_mgr->get_original_config_time(), original_config_time);
	
    strcpy(scalar_int_field->get_field_data()->get_grid_data_field()->field_name_in_IO_file, "restart_date");
    fields_gather_scatter_mgr->read_scatter_field(restart_read_nc_file, scalar_int_field, -1);
    restart_date = ((int*)scalar_int_field->get_data_buf())[0];
    strcpy(scalar_int_field->get_field_data()->get_grid_data_field()->field_name_in_IO_file, "start_date");
    fields_gather_scatter_mgr->read_scatter_field(restart_read_nc_file, scalar_int_field, -1);
    start_date = ((int*)scalar_int_field->get_data_buf())[0];

	start_full_time = ((long)start_date)*100000;
	restart_full_time = ((long)restart_date)*100000;
	EXECUTION_REPORT(REPORT_ERROR, restart_begin_full_time == restart_full_time, "the restart date set by users is different from that in the restart data file for reading\n");
	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "restart"))
		timer_mgr->set_restart_time(start_full_time, restart_full_time);
	restart_read_timer_mgr->set_restart_time(start_full_time, restart_full_time);
	check_is_restart_timer_on();
}


void Restart_mgt::read_in_restart_read_fields_attrs()
{
	int ncfile_id, rcode, restart_vars_attrs_id, num_restart_var_attrs_id, i, j;
	unsigned long starts[2], counts[2];
	FILE *fp_tmp;

	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"))
		return;

	if (num_restart_read_fields_attrs != -999)
		return;

	fp_tmp = fopen(restart_read_file_name, "r");
	EXECUTION_REPORT(REPORT_ERROR, fp_tmp != NULL, "the restart read data file %s does not exist\n");	
	fclose(fp_tmp);

	if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
		rcode = nc_open(restart_read_nc_file->get_file_name(), NC_WRITE, &ncfile_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_read_nc_file->get_file_name());
		rcode = nc_inq_dimid(ncfile_id, "num_restart_var_attrs", &num_restart_var_attrs_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_read_nc_file->get_file_name());
		rcode = nc_inq_dimlen(ncfile_id, num_restart_var_attrs_id, &num_restart_read_fields_attrs);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_read_nc_file->get_file_name());
        rcode = nc_inq_varid(ncfile_id, "restart_vars_attrs", &restart_vars_attrs_id);
		EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_read_nc_file->get_file_name());
		restart_read_fields_attr_strings = new char [num_restart_read_fields_attrs*NAME_STR_SIZE];
		starts[1] = 0;
		counts[0] = 1;
		counts[1] = NAME_STR_SIZE;
		for (i = 0; i < num_restart_read_fields_attrs; i ++) {
			starts[0] = i;			
			rcode = nc_get_vara_text(ncfile_id, restart_vars_attrs_id, starts, counts, restart_read_fields_attr_strings+i*NAME_STR_SIZE);
			EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_read_nc_file->get_file_name());
		}
    	rcode = nc_close(ncfile_id);
    	EXECUTION_REPORT(REPORT_ERROR, rcode == NC_NOERR, "Netcdf error: %s for file %s\n", nc_strerror(rcode), restart_read_nc_file->get_file_name());
	}

	MPI_Bcast(&num_restart_read_fields_attrs, 1, MPI_LONG, 0, compset_communicators_info_mgr->get_current_comp_comm_group());

	if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() != 0) {
		EXECUTION_REPORT(REPORT_ERROR, restart_read_fields_attr_strings == NULL, "C-Coupler error in read_restart_fields_on_restart_date\n");
		restart_read_fields_attr_strings = new char [num_restart_read_fields_attrs*NAME_STR_SIZE];
	}
	
	MPI_Bcast(restart_read_fields_attr_strings, num_restart_read_fields_attrs*NAME_STR_SIZE, MPI_CHAR, 0, compset_communicators_info_mgr->get_current_comp_comm_group());

}

void Restart_mgt::read_restart_fields_on_restart_date()
{
	int i, j;
	char *comp_name, *decomp_name, *grid_name, *field_name, *data_type;
	int buf_type;
	long field_restart_time, tmp_long_value;
	Field_mem_info *field;


	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"))
		return;

	read_in_restart_read_fields_attrs();

	EXECUTION_REPORT(REPORT_LOG, true, "begin reading restart fields on restart date");
	EXECUTION_REPORT(REPORT_ERROR, restart_begin_full_time == restart_read_timer_mgr->get_current_full_time(), "the time of reading restart fields are different from the restart date setting by users\n");

	for (i = 0; i < num_restart_read_fields_attrs/7; i ++) {
		comp_name = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+0);
		decomp_name = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+1);
		grid_name = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+2);
		field_name = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+3);
		data_type = restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+4);
		sscanf(restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+5), "%ld", &tmp_long_value);
		buf_type = (tmp_long_value & 0xffffffff);
		sscanf(restart_read_fields_attr_strings+NAME_STR_SIZE*(i*7+6), "%ld", &field_restart_time);
		if (field_restart_time == restart_begin_full_time) {
			field = alloc_mem(comp_name, decomp_name, grid_name, field_name, data_type, buf_type, false, restart_read_nc_file->get_file_name());
			EXECUTION_REPORT(REPORT_LOG, true, "read field %s %s %s %d from restart data file", comp_name, decomp_name, field_name, buf_type);
			read_one_restart_field(field);
		}
	}

	EXECUTION_REPORT(REPORT_LOG, true, "finish reading restart fields on restart date");
}


