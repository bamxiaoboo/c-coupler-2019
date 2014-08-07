/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef RESTART_MGT
#define RESTART_MGT


#include "common_utils.h" 
#include "runtime_datamodel_algorithm.h"
#include "timer_mgt.h"
#include "cor_global_data.h"
#include "memory_mgt.h"


class Restart_mgt
{
    private:
        char restart_cfg_file_name[NAME_STR_SIZE];
		char restart_read_file_name[NAME_STR_SIZE];
        IO_netcdf *restart_write_nc_file;
		IO_netcdf *restart_read_nc_file;
        long current_restart_full_time;
        long current_restart_num_time_step;
		long restart_read_num_time_step;
		long restart_begin_full_time; 
        Field_mem_info *scalar_int_field;
		char *restart_read_fields_attr_strings;
		unsigned long num_restart_read_fields_attrs;

        bool check_is_restart_timer_on();
		void create_restart_write_nc_file();
		void log_last_restart_output_info();
		void read_in_restart_read_fields_attrs();

    public:
        Restart_mgt(int, int, const char*);
        ~Restart_mgt();
        bool is_in_restart_write_time_window();
		bool is_in_restart_read_time_window();
		void do_restart_write();
		bool is_restart_run();
		void read_check_restart_basic_info();
		void read_restart_fields_on_restart_date();
		long get_restart_begin_full_time() { return restart_begin_full_time; }
        void write_one_restart_field(Field_mem_info*, int);
		int get_restart_read_field_computing_count(const char*, const char*, const char*, const char*, int);
        void read_one_restart_field(Field_mem_info*);
		void get_field_datatype_for_transfer(const char*, const char*, const char*, const char*, int, char*);
		long get_restart_read_num_time_step() { return restart_read_num_time_step; }
};

#endif
