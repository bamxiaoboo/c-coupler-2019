/***************************************************************
  *  Copyright (c) 2017, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/

#ifndef DATAMODEL_MGT_H
#define DATAMODEL_MGT_H

#include "common_utils.h"
#include <netcdf.h>
#include "coupling_generator.h"
#include "timer_mgt.h"
#include "memory_mgt.h"
#include "tinyxml.h"
#include <string>
#include <vector>

#define INPUT_DATAMODEL            ((int)0)
#define OUTPUT_DATAMODEL           ((int)1)

/*class Datamodel_instance_info
{
public:
	char datamodel_instance_name[NAME_STR_SIZE];//Datainst_ not included
	char datamodel_name[NAME_STR_SIZE];//Datamodel_ not included
	char period_start_time[NAME_STR_SIZE];
	char period_time_format[NAME_STR_SIZE];
	int id_period_time_format;
	int period_unit;
	int period_count;
	int offset_unit;
	int offset_count;
	std::vector<char*> name_in_active_model;
	std::vector<char*> name_in_datamodel;

	int host_comp_id;
	bool is_coupling_connection_file_read;
	bool is_datamodel_needed;
	std::vector<Coupling_connection*> datamodel_coupling_connections;//store all the coupling connection between a model and datamodel_instances, should set another datamodel_coupling_connections in datamodel class

	Datamodel_instance_info() ;
	~Datamodel_instance_info();
	void check_is_input_datamodel_needed(int, const char*);
	void load_datamodel_instatnces_configuration(int, const char*, char*, std::vector<char*>);
	void clear_datamodel_instance_settings();
	void read_datamodel_instance_configuration(int, const char*, char*, const char*, TiXmlNode*);
};*/

struct Field_config_info
{
	bool datamodel_type;//input: 0, output: 1
	char *name_in_model;
	char *name_in_file;
	char *grid_name;
	char *float_datatype;
	char *integer_datatype;
	char *operation;
	char *unit;
	double add_offset;
	double scale_factor;
};

class Inout_datamodel {
private:
	friend class Output_handler;
	bool datamodel_type;//input: 0, output: 1
	int implicit_or_explicit;//implicit: 0, explicit: 1
	char datamodel_config_dir[NAME_STR_SIZE];//config dir
	char datamodel_data_dir[NAME_STR_SIZE];//data dir
	char XML_file_name[NAME_STR_SIZE];//dir+filename
	char datamodel_file_name[NAME_STR_SIZE];//dir not included
	char *file_mid_name;
	char *annotation;
	char *datamodel_name;
	char *datamodel_files_dir_name;
	char *file_dir;
	char *file_name_prefix;
	char *file_name_suffix;
	int id_time_format_in_file_names;
	char *file_type;
	int host_comp_id;
	std::vector<int> output_timer_ids;
	std::vector<int> file_timer_ids;
	std::vector<int> time_points;
	std::vector<int> id_time_point_formats;
	std::vector<int> id_time_point_units;
	std::vector<char*> time_point_units;
	std::vector<int> output_freq_counts;
	std::vector<int> id_time_format_in_data_files;
	std::vector<char*> field_specifications;
	std::vector<char*> output_freq_units;
	std::vector<int> id_output_freq_units;
	std::vector<char*> default_operations;
	std::vector<char*> default_float_types;
	std::vector<char*> default_integer_types;
	std::vector<char*> default_h2d_grids;
	std::vector<char*> default_v3d_grids;
	std::vector<char*> file_freq_units;
	std::vector<int> file_freq_counts;
	std::vector<int> h2d_grid_ids;
	std::vector<int> v1d_grid_ids;
	std::vector<int> v3d_grid_ids;
	std::vector<int> all_grid_ids;
	std::vector<char*> h2d_grid_names;
	std::vector<char*> md_grid_names;
	std::vector<char*> surface_field_names;
	std::vector<std::vector<Field_config_info*> > fields_config_info;
public:
	Inout_datamodel(int, const char*, bool, const char*);
	~Inout_datamodel();
	const char *get_datamodel_name() { return datamodel_name; }
	int get_host_comp_id() { return host_comp_id; }
	char *get_datamodel_data_dir() { return datamodel_data_dir; }
	char *get_XML_file_name() { return XML_file_name; }
	void config_data_files_for_datamodel(TiXmlNode*);
	void config_horizontal_grids_for_datamodel(TiXmlNode*);
	void config_vertical_grids_for_datamodel(TiXmlNode*);
	void config_v3d_grids_for_datamodel(TiXmlNode*);
	void config_horizontal_grid_via_CCPL_grid_file(TiXmlNode*, const char*, const char*);
	void config_horizontal_grid_via_grid_data_file_field(TiXmlNode*, const char*, const char*);
	void config_horizontal_grid_via_uniform_lonlat_grid(TiXmlNode*, const char*, const char*);
	void config_vertical_z_grid(MPI_Comm, TiXmlNode*, IO_netcdf*, char*, void **coord_values, int &, char*, bool);
	void config_vertical_sigma_grid(MPI_Comm, TiXmlNode*, IO_netcdf*, char*, char*, void**, int&, char*, bool);
	void config_vertical_hybrid_grid(MPI_Comm, TiXmlNode*, const char*, IO_netcdf*, char*, char*, void**, void**, int&, char*, bool);
	void config_field_output_settings_for_datamodel(TiXmlNode*);
	void config_field_info(TiXmlNode*, int);
	void visit_time_slots_node(TiXmlNode*);
	void visit_time_points_node(TiXmlNode*);
	bool expected_segment_exists(TiXmlNode*&, const char*);
	void get_all_sub_segment_time_slots(TiXmlNode*, std::vector<TiXmlNode*>);
	void config_output_frequency(TiXmlNode*, int);
	void config_fields_output_setting_attributes(TiXmlNode*);
	void register_common_h2d_grid_for_datamodel(const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char *, const char*, const char*, const char*);
};

class Output_handler {
private:
	Inout_datamodel *output_datamodel;
	char *handler_name;
	int host_comp_id;
	int handler_id;
	int num_fields;
	int implicit_or_explicit;
	//std::vector<int> handler_fields_id;
	char *datamodel_name;
	int sampling_timer_id;
	char *annotation;
public:
	Output_handler(const char*, const char*, int, int, int*, int, int, int, const char *);
	~Output_handler() {}
	int get_handler_id() { return handler_id; }
	int get_host_comp_id() { return host_comp_id; }
	const char *get_handler_name() { return handler_name; }
	//int search_field_id(char *);
	void execute_handler(int, int, const char*);
};

class Datamodel_mgt {
private:
	friend class Output_handler;
	Time_mgt *time_mgr;
	std::vector<Output_handler*> output_handlers;
	std::vector<Inout_datamodel*> output_datamodels;
	std::vector<Inout_datamodel*> input_datamodels;
public:
	Datamodel_mgt() {}
	~Datamodel_mgt();
	int register_datamodel_output_handler(const char*, int, int *, const char*, int, int, int, const char*);
	int get_next_handler_id() { return TYPE_OUTPUT_HANDLER_ID_PREFIX|output_handlers.size(); }
	void common_checking_for_datamodel_handler_registration(int, int*, int, int, int, const char*);
	void handle_normal_output(int, int, int, const char*);
	bool is_legal_handler_id(int);
	Output_handler *get_handler(int);
	Inout_datamodel *search_output_datamodel(char*);
};

bool varname_or_value(const char*);//false: varname, true: value
int set_unit(const char*, const char*);
int check_time_format(const char*, const char*);
char *tolower(const char*);
bool words_are_similar(const char*, const char*);
bool at_most_one_node_of(const char*, TiXmlNode*, TiXmlNode*&, int&);
//char *transform_time_format_to_unit(const char*, const char*);
int outer_time_unit_to_inner(const char*, int, const char*, const char*);

#endif