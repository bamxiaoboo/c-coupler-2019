/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef COUPLING_GENERATOR_H
#define COUPLING_GENERATOR_H


#include <vector>
#include <map>
#include "inout_interface_mgt.h"
#include "dictionary.h"


class Import_direction_setting
{
	private:
		char interface_name[NAME_STR_SIZE];
		int status;                      // 0 means off; 1 means on
		int fields_default_setting;       // 0 means off; 1 means all; 2 means remain"
		int components_default_setting;  // 0 means off; 1 means all;
		std::vector<const char*> fields_name;
		std::vector<char*> components_full_name;
		
		// ... remapping and merge setting;
	
	public:
		Import_direction_setting(const char*, const char*, TiXmlElement*, const char*, std::vector<const char*>&, int*);
		~Import_direction_setting();
};


class Import_interface_configuration
{
	private:
		char interface_name[NAME_STR_SIZE];
		std::vector<Import_direction_setting*> import_directions;

	public:
		Import_interface_configuration(const char*, const char*, TiXmlElement*, const char*, Inout_interface_mgt *);
		~Import_interface_configuration();
};


class Component_import_interfaces_configuration
{
	private:
		char comp_full_name[NAME_STR_SIZE];
		std::vector<Import_interface_configuration*> import_interfaces_configuration;

	public:
		Component_import_interfaces_configuration(int, Inout_interface_mgt*);
		~Component_import_interfaces_configuration();
};



class Coupling_generator
{
	private:
		std::map<int,std::vector<char*> > import_fields_src_components;
		std::map<int,std::vector<char*> > export_fields_dst_components;
		Dictionary<int> *import_field_index_lookup_table;
		Dictionary<int> *export_field_index_lookup_table;
		
		void generate_interface_fields_source_dst(const char*, int);
		void generate_components_connections();

	public:
		Coupling_generator() {};
		~Coupling_generator() {}
		void generate_coupling_procedures();
};


#endif

