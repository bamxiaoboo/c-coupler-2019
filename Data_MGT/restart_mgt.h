/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef RESTART_MGT_H
#define RESTART_MGT_H


#define RESTART_BUF_TYPE_TIME            "time_restart"
#define RESTART_BUF_TYPE_AVERAGE         "average_restart"


#include "common_utils.h"
#include "timer_mgt.h"
#include <vector>


class Restart_buffer_container
{
	private:
		char comp_full_name[NAME_STR_SIZE];
		char buf_type[NAME_STR_SIZE];
		char keyword[NAME_STR_SIZE];
		const char *buffer_content;
		long buffer_content_iter;

	public:
		Restart_buffer_container(const char*, const char *, const char *, const char *, long);
		Restart_buffer_container(const char *, long &);
		~Restart_buffer_container() { delete [] buffer_content; }		
		void dump_string(const char*, long, char **, long &, long &);
		char *load_string(char *, long &, const char *, long &);
		void dump(char **, long &, long &);
		
};


class Restart_mgt
{
	private:
		long last_timer_on_full_time;
		std::vector<Restart_buffer_container*> restart_buffer_containers;
		int comp_id;
		Time_mgt *time_mgr;

	public:
		Restart_mgt(int);
		~Restart_mgt();
		void clean();
		void do_restart_write(const char *);
		void write_into_file();
		void do_restart_read(const char *, const char *);
		void reset_comp_id(int comp_id) { this->comp_id = comp_id; }
};


#endif
