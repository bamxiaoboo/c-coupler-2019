/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef IO_FIELD_MGT
#define IO_FIELD_MGT


#include <vector>
#include "common_utils.h"
#include "timer_mgt.h"


class IO_field
{
	private:
		int IO_field_id;
		int field_instance_id;
		int comp_id;
		char field_IO_name[NAME_STR_SIZE];
		char field_long_name[NAME_STR_SIZE];
		char field_unit[NAME_STR_SIZE];
		Coupling_timer *timer;

	public:
		IO_field(int, int, int, const char *, const char *);
		IO_field(int, int, int, int, int, void *, const char *, const char *, const char *, const char *, const char *);
		~IO_field();
		int get_comp_id() { return comp_id; }
		int get_IO_field_id() { return IO_field_id; }
		int get_field_instance_id() { return field_instance_id; }
		const char *get_field_IO_name() { return field_IO_name; }
};


class IO_field_mgt
{
	private:
		std::vector<IO_field*> IO_fields;

	public:
		IO_field_mgt() {}
		~IO_field_mgt();
		int register_IO_field(int, int, const char *, const char *);
		int register_IO_field(int, int, int, int, void *, const char *, const char *, const char *, const char *, const char *);
		IO_field *search_IO_field(int, const char*);
		void check_for_registering_IO_field(IO_field *, const char *);
};

#endif

