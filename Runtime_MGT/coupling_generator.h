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


class Import_interface_configuration;


struct Coupling_connection
{
	char field_name[NAME_STR_SIZE];
	std::vector<std::pair<char[NAME_STR_SIZE],char[NAME_STR_SIZE]> > src_comp_interfaces;
	char dst_comp_full_name[NAME_STR_SIZE];
	char dst_interface_name[NAME_STR_SIZE];
};


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
		Import_direction_setting(Import_interface_configuration *, const char*, const char*, TiXmlElement*, const char*, std::vector<const char*>&, int*);
		~Import_direction_setting();
};


class Import_interface_configuration
{
	private:
		char interface_name[NAME_STR_SIZE];
		std::vector<Import_direction_setting*> import_directions;
		std::vector<const char*> fields_name;
		std::vector<std::vector<const char*> > fields_src_components;

	public:
		Import_interface_configuration(const char*, const char*, TiXmlElement*, const char*, Inout_interface_mgt *);
		~Import_interface_configuration();
		const char *get_interface_name() { return interface_name; }
		void add_field_src_component(int comp_id, const char*, const char*);
		void get_field_import_configuration(const char*, std::vector<const char*>&);
};


class Component_import_interfaces_configuration
{
	private:
		char comp_full_name[NAME_STR_SIZE];
		std::vector<Import_interface_configuration*> import_interfaces_configuration;

	public:
		Component_import_interfaces_configuration(int, Inout_interface_mgt*);
		~Component_import_interfaces_configuration();
		void get_interface_field_import_configuration(const char*, const char*, std::vector<const char*>&);
};



class Coupling_generator
{
	private:
		std::map<int,std::vector<std::pair<const char*, const char*> > > import_fields_src_components;
		std::map<int,std::vector<std::pair<const char*, const char*> > > export_fields_dst_components;
		Dictionary<int> *import_field_index_lookup_table;
		Dictionary<int> *export_field_index_lookup_table;
		std::vector<Coupling_connection*> all_coupling_connections;
		
		void generate_interface_fields_source_dst(const char*, int);
		void generate_components_connections();
		void generate_coupling_connection(const Coupling_connection *);

	public:
		Coupling_generator() {};
		~Coupling_generator() {}
		void generate_coupling_procedures();
};


#endif

