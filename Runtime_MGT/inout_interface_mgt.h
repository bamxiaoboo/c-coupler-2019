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


class Inout_interface
{
	private:
		char interface_name[NAME_STR_SIZE];
		int interface_id;
		int comp_id;
		int import_or_export;     // 0: import; 1: export;
		std::vector<Field_mem_info *> fields_mem;
		std::vector<Coupling_timer*> timers;
		std::vector<const char*> fields_name;

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
		void merge_inout_interface_fields_info(int);
		void write_all_interfaces_fields_info();
		const char *get_temp_array_buffer() { return temp_array_buffer; } 
		int get_buffer_content_size()  { return buffer_content_size; }
};

#endif
