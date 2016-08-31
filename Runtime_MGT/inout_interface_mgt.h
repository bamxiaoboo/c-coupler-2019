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

	public:
		Inout_interface(const char*, int, int, int, int*, int*, const char*);
		~Inout_interface() {}
		const char *get_interface_name() { return interface_name; }
		int get_comp_id() { return comp_id; }
		int get_interface_id() { return interface_id; }
		void report_common_field_instances(const Inout_interface*);
};


class Inout_interface_mgt
{
	private:
		std::vector<Inout_interface*> interfaces;

	public:
		Inout_interface_mgt() {}
		~Inout_interface_mgt();
		int register_inout_interface(const char *, int import_or_export, int num_fields, int *field_ids, int *timer_ids, const char*);
};

#endif
