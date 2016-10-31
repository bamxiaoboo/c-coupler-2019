/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef INOUT_INTERFACE_MGT_H
#define INOUT_INTERFACE_MGT_H


#include <vector>
#include "common_utils.h"
#include "memory_mgt.h"
#include "timer_mgt.h"


class Inout_interface;
class Coupling_connection;


struct Connection_field_time_info
{
	int current_year;
	int current_month;
	int current_day;
	int current_second;
	int num_elapsed_days;
	int time_step_in_second;
	Coupling_timer *timer;
};


class Connection_coupling_procedure
{
	private:
		std::vector<Field_mem_info *> fields_mem_registered;
		std::vector<Field_mem_info *> fields_mem_averaged;
		std::vector<Field_mem_info *> fields_mem_remapped;
		std::vector<Field_mem_info *> fields_mem_datatype_transformed;
		std::vector<Field_mem_info *> fields_mem_transfer;
		std::vector<Connection_field_time_info *> fields_time_info_src;
		std::vector<Connection_field_time_info *> fields_time_info_dst;
		std::vector<bool> transfer_process_on;
		Coupling_connection *coupling_connection;
		Inout_interface *inout_interface;
		
	public:
		Connection_coupling_procedure(Inout_interface*, Coupling_connection*);
		void alloc_field_inst_for_datatype_transformation(const char*, const char*);
		void execute(bool);
};


class Inout_interface
{
	private:
		char interface_name[NAME_STR_SIZE];
		int interface_id;
		int comp_id;
		int import_or_export;     // 0: import; 1: export;
		std::vector<Field_mem_info *> fields_mem_registered;
		std::vector<Coupling_timer*> timers;
		std::vector<const char*> fields_name;
		std::vector<Connection_coupling_procedure*> coupling_procedures;
		int execution_checking_status;
		std::vector<Runtime_algorithm_basis *> runtime_algorithms;

	public:
		Inout_interface(const char*, int&);
		Inout_interface(const char*, int, int, int, int*, int*, const char*);
		~Inout_interface() {}
		const char *get_interface_name() { return interface_name; }
		int get_comp_id() { return comp_id; }
		int get_interface_id() { return interface_id; }
		int get_import_or_export() { return import_or_export; }
		void report_common_field_instances(const Inout_interface*);
		void get_fields_name(std::vector<const char*>*);
		const char *get_field_name(int);
		int get_num_fields();
		void transform_interface_into_array(char**, int&, int&);
		Field_mem_info *search_registered_field_instance(const char*);
		Coupling_timer *search_a_timer(const char*);
		void add_coupling_procedure(Connection_coupling_procedure*);
		void execute(bool, const char*);
		void add_runtime_algorithm(Runtime_algorithm_basis * runtime_algorithm) {runtime_algorithms.push_back(runtime_algorithm);}
};


class Inout_interface_mgt
{
	private:
		std::vector<Inout_interface*> interfaces;
		char *temp_array_buffer;
		int buffer_max_size;
		int buffer_content_size;
		int buffer_content_iter;

	public:
		Inout_interface_mgt(const char*, int);
		Inout_interface_mgt();
		~Inout_interface_mgt();
		int register_inout_interface(const char *, int import_or_export, int num_fields, int *field_ids, int *timer_ids, const char*);
		bool is_interface_id_legal(int);
		Inout_interface *get_interface(int);
		Inout_interface *get_interface(const char*, const char*);
		Inout_interface *get_interface(int, const char*);
		void get_all_import_interfaces_of_a_component(std::vector<Inout_interface*>&, int);
		void merge_inout_interface_fields_info(int);
		void write_all_interfaces_fields_info();
		const char *get_temp_array_buffer() { return temp_array_buffer; } 
		int get_buffer_content_size()  { return buffer_content_size; }
		void execute_interface(int, bool, const char*);
		void execute_interface(int, const char*, bool, const char*);
};

#endif
