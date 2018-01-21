/***************************************************************
  *  Copyright (c) 2017, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef RESTART_MGT_H
#define RESTART_MGT_H


#define RESTART_BUF_TYPE_TIME            "time_restart"
#define RESTART_BUF_TYPE_INTERFACE       "interface"


#include "common_utils.h"
#include "timer_mgt.h"
#include <vector>


class Restart_buffer_container
{
	private:
		char comp_full_name[NAME_STR_SIZE];
		char buf_type[NAME_STR_SIZE];
		char keyword[NAME_STR_SIZE];
		char *buffer_content;
		long buffer_content_iter;
		long buffer_max_size;

	public:
		Restart_buffer_container(const char *, const char *, const char *);
		Restart_buffer_container(const char*, const char *, const char *, char *, long);
		Restart_buffer_container(const char *, long &, const char *);
		~Restart_buffer_container() { delete [] buffer_content; }		
		const char *get_buffer_content() { return buffer_content; }
		long get_buffer_content_iter() { return buffer_content_iter; }
		void dump_out(char **, long &, long &);
		bool match(const char *, const char *);
		void dump_in_string(const char *, long);
		void dump_in_data(const void *, long);
		char **get_buffer_content_ptr() { return &buffer_content; }
		long *get_buffer_content_iter_ptr() { return &buffer_content_iter; }
		long *get_buffer_max_size_ptr() { return &buffer_max_size; }
};


class Restart_mgt
{
	private:
		long last_timer_on_full_time;
		std::vector<Restart_buffer_container*> restart_write_buffer_containers;
		std::vector<Restart_buffer_container*> restart_read_buffer_containers;
		int comp_id;
		Time_mgt *time_mgr;
		char *input_restart_mgt_info_file;
		char *restart_read_annotation;

	public:
		Restart_mgt(int);
		~Restart_mgt();
		void clean(bool);
		void do_restart_write(const char *, bool);
		void write_into_file();
		void read_restart_mgt_info(bool, const char *, const char *);
		void read_restart_mgt_info(const char *, const char *);
		void reset_comp_id(int comp_id) { this->comp_id = comp_id; }
		void bcast_buffer_container(Restart_buffer_container **, int);
		Restart_buffer_container *search_then_bcast_buffer_container(const char *, const char *);
		Restart_buffer_container *search_restart_buffer(const char *, const char*);
		int get_comp_id() { return comp_id; }
		void get_file_name_in_rpointer_file(char *);
		const char *get_input_restart_mgt_info_file();
		const char *get_restart_read_annotation();
		Restart_buffer_container *apply_restart_buffer(const char *, const char *, const char *, bool);
};



#endif
