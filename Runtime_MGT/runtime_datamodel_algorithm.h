/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef _RUNTIME_DATAMODEL_ALGORITHM_H_
#define _RUNTIME_DATAMODEL_ALGORITHM_H_

#include "Runtime_Algorithm_Basis.h"
#include "runtime_remap_algorithm.h"
#include "field_info_mgt.h"
#include "decomp_info_mgt.h"
#include "timer_mgt.h"
#include "cor_global_data.h"
#include "memory_mgt.h"
#include <vector>


class Runtime_datamodel_algorithm;


struct Datamodel_field_info {
	char cfg_info_remain_line[NAME_STR_SIZE];
    char field_name_in_IO_file[NAME_STR_SIZE];
    char field_datatype_IO_file[NAME_STR_SIZE];
    Field_mem_info *field_data_mem;
    double scale_factor;
    double add_offset;
    bool have_scale_factor;
};


struct Time_location_in_files
{
	int date;
	int num_elapsed_day;
	int second_in_day;
	char file_name[NAME_STR_SIZE];
	int time_dim_num;
};


class Datamodel_field_read_handler
{
	struct Field_read_info {
		int last_pos_time_interval_left;
		int last_pos_time_interval_right;
		long last_read_full_time;
		int reference_count;
		Field_mem_info *temp_field_before_time_remap_left;
		Field_mem_info *temp_field_before_time_remap_right;
		Field_mem_info *temp_field_readin;
		Field_mem_info *output_field_instance;
		Runtime_remap_algorithm *remap_algorithm_for_readin_left;
		Runtime_remap_algorithm *remap_algorithm_for_readin_right;
	};
	
	private:
		/* Information for file reading */
		char handler_name[NAME_STR_SIZE];
		int id_time_format_in_file_name;
		char file_dir[NAME_STR_SIZE];
		char file_name_pattern[NAME_STR_SIZE];
		char time_format_type_in_file_name[NAME_STR_SIZE];
		std::vector<Time_location_in_files> time_filename_map; 
		std::vector<Field_read_info> fields_read_info;
		int num_time_field;
		char time_field_name[16][NAME_STR_SIZE];
		char time_field_format[16][NAME_STR_SIZE];
		int time_field_format_id[16];
		int offset_num_elapsed_day;                         // Time offset is a date
		int id_time_format_time_offset;
		int period;                              // 0: acyclic; 1: year; 2: month; 3: day
		int pos_time_interval_left;
		int pos_time_interval_right;
		int default_time_pos;                   // 1: start, 2: middle, 3: end. Default is 2

		void initialize_time_filename_map();
		bool get_time_from_string(const char*, const char*, int, int, bool, int&, int&);
		int check_time_format(const char*, const char*);
		
	public:	
		Datamodel_field_read_handler(const char*);
		long search_current_time_interval(int);
		const char *get_handler_name() { return handler_name; }
		void update_one_field(int);
		int register_read_info_for_a_field(Field_mem_info*, Remap_weight_of_strategy_class*);
};


class Datamodel_field_read_handler_mgt
{
    private:
		std::vector<Datamodel_field_read_handler*> datamodel_field_read_handlers;

	public:
		Datamodel_field_read_handler_mgt() {}
		~Datamodel_field_read_handler_mgt();
		Datamodel_field_read_handler *get_a_handler(const char*);
};


class Runtime_datamodel_algorithm : public Runtime_algorithm_basis
{
    private:
        std::vector<Datamodel_field_info*> datamodel_fields;
		std::vector<int> field_read_info_indexes;
		
        char datamodel_type[NAME_STR_SIZE];
        char IO_file_type[NAME_STR_SIZE];
        char IO_file_name[NAME_STR_SIZE];
		char IO_file_name_keyword[NAME_STR_SIZE];
        char write_type[NAME_STR_SIZE];
		char fields_cfg_file_name[NAME_STR_SIZE];
        IO_netcdf *netcdf_file_object;
        Coupling_timer *io_timer;
        Coupling_timer *change_file_timer;
		bool write_grid_name;
		Datamodel_field_read_handler *field_read_handler;
		long last_read_full_time;
		
        void datamodel_read(void);
        void datamodel_check(void);
        void datamodel_write(void);
		void generate_algorithm_info_from_cfg_file();
		void allocate_one_field(int);

    public:
        Runtime_datamodel_algorithm(const char *);    
        ~Runtime_datamodel_algorithm();
        char *get_datamodel_type() { return datamodel_type; }
        void change_IO_file_name_for_restart(const char *);
        virtual void run(bool);
        void allocate_src_dst_fields(bool);
		const char *get_algorithm_cfg_name() { return algorithm_cfg_name; }
};


#endif

