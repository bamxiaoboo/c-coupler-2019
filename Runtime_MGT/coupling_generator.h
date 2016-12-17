/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu and then
  *  modified by Dr. Cheng Zhang and Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn or
  *  Dr. Cheng Zhang via zhangc-cess@tsinghua.edu.cn
  ***************************************************************/



#ifndef COUPLING_GENERATOR_H
#define COUPLING_GENERATOR_H


#include <vector>
#include <map>
#include "inout_interface_mgt.h"
#include "dictionary.h"
#include "remapping_configuration_mgt.h"


#define USING_INSTANTANEOUS_VALUE            0
#define USING_AVERAGE_VALUE                  1


class Import_interface_configuration;
class Coupling_generator;
class IO_output_procedure;


struct Interface_field_info
{
	char grid_name[NAME_STR_SIZE];
	char data_type[NAME_STR_SIZE];
        char decomp_name[NAME_STR_SIZE];
	char unit[NAME_STR_SIZE];
	Coupling_timer *timer;
	int time_step_in_second;
        int inst_or_aver;
};


class Coupling_connection
{
	private:
		friend class Coupling_generator;
		friend class Connection_coupling_procedure;
		friend class IO_output_procedure;
		int connection_id;
		std::vector<const char*> fields_name;
		std::vector<std::pair<char[NAME_STR_SIZE],char[NAME_STR_SIZE]> > src_comp_interfaces;
		char dst_comp_full_name[NAME_STR_SIZE];
		char dst_interface_name[NAME_STR_SIZE];
		std::vector<Interface_field_info*> src_fields_info;
		std::vector<Interface_field_info*> dst_fields_info;
		Inout_interface *import_interface;
		Inout_interface *export_interface;
		Connection_coupling_procedure *import_procedure;
		Connection_coupling_procedure *export_procedure;
		Comp_comm_group_mgt_node *src_comp_node;
		Comp_comm_group_mgt_node *dst_comp_node;
		int current_proc_id_src_comp;
		int	current_proc_id_dst_comp;
		int src_comp_root_proc_global_id;
		int dst_comp_root_proc_global_id;
        MPI_Comm union_comm;
        int * src_proc_ranks_in_union_comm;
        int * dst_proc_ranks_in_union_comm;

		void write_connection_fields_info_into_array(Inout_interface *inout_interface, char **array, int &buffer_max_size,int &buffer_content_size);	
		void read_connection_fields_info_from_array(std::vector<Interface_field_info*>&, const char *, int, int);
		void exchange_connection_fields_info();
		void generate_interpolation();
		void exchange_grid(const char*, const char*, bool);
		void exchange_remapping_setting(int, Remapping_setting &);

	public:
		Coupling_connection(int);
		void generate_a_coupling_procedure();
        void create_union_comm();
		void generate_data_transfer();
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
                std::vector<Coupling_connection*> all_IO_connections;
		
		void generate_interface_fields_source_dst(const char*, int);
		void generate_components_connections();

	public:
		Coupling_generator() {};
		~Coupling_generator() {}
		void generate_coupling_procedures();
                void generate_IO_procedures();
};


#endif

