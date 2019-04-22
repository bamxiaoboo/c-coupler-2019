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

class Inout_datamodel {
private:
	bool datamodel_type;//input: 0, output: 1
	bool implicit_or_explicit;//implicit: 0, explicit: 1
	char datamodel_config_dir[NAME_STR_SIZE];//config dir
	char datamodel_data_dir[NAME_STR_SIZE];//data dir
	char XML_file_name[NAME_STR_SIZE];//dir+filename
	char datamodel_file_name[NAME_STR_SIZE];//dir not included
	char datamodel_name[NAME_STR_SIZE];
	char datamodel_files_dir_name[NAME_STR_SIZE];
	char file_dir[NAME_STR_SIZE];
	char file_name_prefix[NAME_STR_SIZE];
	char file_name_suffix[NAME_STR_SIZE];
	char time_format_in_file_names[NAME_STR_SIZE];
	int file_type;//1: netcdf
	int host_comp_id;
	std::vector<int> h2d_grid_ids;
	std::vector<int> v1d_grid_ids;
	std::vector<int> v3d_grid_ids;
	std::vector<int> mid_3d_grid_ids;
	std::vector<int> mid_1d_grid_ids;
	std::vector<char*> surface_field_names;
public:
	Inout_datamodel(int, const char*, bool);
	Inout_datamodel(Inout_datamodel*);
	~Inout_datamodel();
	const char *get_datamodel_name() {return datamodel_name;}
	void config_data_files_for_datamodel(TiXmlNode*);
	void config_horizontal_grids_for_datamodel(TiXmlNode*);
	void config_vertical_grids_for_datamodel(TiXmlNode*);
	void config_v3d_grids_for_datamodel(TiXmlNode*);
	void config_horizontal_grid_via_CCPL_grid_file(TiXmlNode*, const char*);
	void config_horizontal_grid_via_grid_data_file_field(TiXmlNode*, const char*);
	void config_horizontal_grid_via_uniform_lonlat_grid(TiXmlNode*, const char*);
	void config_vertical_z_grid(MPI_Comm, TiXmlNode*, IO_netcdf*, char*, void **coord_values, int &, char*, bool);
	void config_vertical_sigma_grid(MPI_Comm, TiXmlNode*, IO_netcdf*, char*, char*, void**, int&, char*, bool);
	void config_vertical_hybrid_grid(MPI_Comm, TiXmlNode*, const char*, IO_netcdf*, char*, char*, void**, void**, int&, char*, bool);
	void config_field_output_settings_for_datamodel(TiXmlNode*);
	void config_field_info(TiXmlNode*);
	void visit_time_slots_node(TiXmlNode*);
	void visit_time_points_node(TiXmlNode*);
	bool is_expected_segment(TiXmlNode*, const char*);
	void get_all_sub_segment_time_slots(TiXmlNode*, std::vector<TiXmlNode*>);
	void config_output_frequency(TiXmlNode*);
	void config_default_settings(TiXmlNode*);
	void register_common_h2d_grid_for_datamodel(const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char *, const char*, const char*);
};

class Output_handler {
private:
	int host_comp_id;
	int handler_id;
	int num_fields;
	std::vector<int> handler_fields_id;
	Inout_datamodel *output_datamodel;
	int sampling_timer_id;
	char annotation[NAME_STR_SIZE];
public:
	Output_handler(const char*, int, int, int*, int,const char*);
	~Output_handler();
	int get_handler_id() {return handler_id;}
};

class Datamodel_mgt {
private:
	friend class Output_handler;
	std::vector<Output_handler*> output_handlers;
	std::vector<Inout_datamodel*> output_datamodels;
public:
	Datamodel_mgt() {}
	~Datamodel_mgt() {}
	int register_datamodel_output_handler(int, int *, const char*, bool, int, const char*);
	int get_next_handler_id() {return TYPE_OUTPUT_HANDLER_ID_PREFIX|output_handlers.size();}
};

bool varname_or_value(const char*);//false: varname, true: value
int set_unit(const char*);
int check_time_format(const char*, const char*);
void tolower(char*);

#endif