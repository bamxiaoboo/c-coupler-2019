/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef MEM_MGT
#define MEM_MGT

#include <vector>
#include "common_utils.h"
#include "remap_grid_data_class.h"


struct Registered_field_info
{
    char field_name[NAME_STR_SIZE];
    char decomp_name[NAME_STR_SIZE];
    char grid_name[NAME_STR_SIZE];
};


class Field_mem_info
{
    private:
        char comp_name[NAME_STR_SIZE];
        char decomp_name[NAME_STR_SIZE];
        char grid_name[NAME_STR_SIZE];
        char field_name[NAME_STR_SIZE];
        int buf_type;
        bool is_registered_model_buf;
		bool is_restart_field;
		long last_define_time;
		bool is_field_active;
        Remap_grid_data_class *grided_field_data;

    public:
        Field_mem_info(const char *, const char *, const char *, const char*, const int, bool);
        bool match_field_mem(const char *, const char *, const char *, const char *, const int);
		bool match_field_mem(void*);
        bool get_is_restart_field() { return is_restart_field; }
		bool get_is_registered_model_buf() { return is_registered_model_buf; }
		bool check_is_field_active() { return is_field_active; }
        void *get_data_buf() { return grided_field_data->get_grid_data_field()->data_buf; }
        Remap_grid_data_class *get_field_data() { return grided_field_data; }
        void reset_mem_buf(void *buf, bool);
        void get_field_mem_full_name(char*);
        const char *get_decomp_name() { return decomp_name; }
        const char *get_comp_name() { return comp_name; }
        const char *get_grid_name() { return grid_name; }
        const char *get_field_name() { return field_name; }
		int get_buf_type() { return buf_type; }
        void reset_field_name(const char*);
		void change_datatype_to_double();
		void calculate_field_conservative_sum(Field_mem_info*);
        void check_field_sum();
		void define_field_values(bool);
		void use_field_values();
		bool field_has_been_defined();
		long get_last_define_time() { return last_define_time; }
        ~Field_mem_info();
};


class Memory_mgt
{
    private:
        std::vector<Field_mem_info *> fields_mem;
        std::vector<Registered_field_info *> registered_fields_info;

		void add_registered_field_info(const char*, const char*, const char*);
        
    public: 
        Memory_mgt(const char *);
        Field_mem_info *alloc_mem(const char *, const char *, const char *, const char*, const int, bool);
        void register_model_data_buf(const char*, const char*, const char*, void*, const char*, void*, bool);
		void withdraw_model_data_buf(const char*, const char*, const char*);
        Field_mem_info *alloc_mem_double(const char *, const char *, const char *, const char*, const int);
        Field_mem_info *search_field_via_data_buf(const void*);
		void write_restart_fields();
		void check_all_restart_fields_have_been_read();
		bool is_model_data_renewed_in_current_time_step(void*);
		bool is_model_data_active_in_coupling(void*);
		void check_sum_of_all_fields();
        ~Memory_mgt();
};


extern void *alloc_buf(const char *, const char *, const char *, const char *, const int);
extern Remap_grid_data_class *alloc_field(const char *, const char *, const char*, const char *, const int);
extern Field_mem_info *alloc_mem(const char *, const char *, const char *, const char *, const int);
extern Field_mem_info *alloc_full_grid_mem(const char *, const char *, const char *, const char *, const int);
extern Field_mem_info *alloc_mem_double(const char *, const char *, const char *, const char *, const int);

#endif
