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

class Datamodel_instance_info
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
};

bool varname_or_value(const char*);
int set_unit(const char*);
int check_time_format(const char*, const char*);

#endif