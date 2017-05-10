/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef REMAPPING_CONFIGURATION_MGT
#define REMAPPING_CONFIGURATION_MGT


#define REMAP_ALGORITHM_TYPE_H2D     1
#define REMAP_ALGORITHM_TYPE_V1D     2
#define REMAP_ALGORITHM_TYPE_T1D     3


#include <vector>
#include "tinyxml.h"
#include "common_utils.h"


class Remapping_algorithm_specification
{
	private:
		int comp_id;		
		int type_id;
		char algorithm_name[NAME_STR_SIZE];
		std::vector<char *> parameters_name;
		std::vector<char *> parameters_value;

	public:
		Remapping_algorithm_specification(const Remapping_algorithm_specification*);
		Remapping_algorithm_specification(const char*, int);
		Remapping_algorithm_specification(const char *, int*);
		Remapping_algorithm_specification(int, TiXmlElement*, const char*, int);
		Remapping_algorithm_specification() {}
		~Remapping_algorithm_specification();
		void print();
		void write_remapping_algorithm_specification_into_array(char **, int &, int &);
		const char *get_algorithm_name() { return algorithm_name; }
		int get_num_parameters() { return parameters_name.size(); }
		void get_parameter(int, char *, char *);
		Remapping_algorithm_specification *clone();
		bool is_the_same_as_another(Remapping_algorithm_specification*);
};


class Remapping_setting
{
	private:
		int comp_id;
		int XML_start_line_number;
		Remapping_algorithm_specification *H2D_remapping_algorithm;
		Remapping_algorithm_specification *V1D_remapping_algorithm;
		Remapping_algorithm_specification *T1D_remapping_algorithm;
		int field_specification_manner;    // 0 means default of a component; 1 means type; 2 means name
		std::vector<const char *> fields_specification;

	public:
		Remapping_setting();
		Remapping_setting(const char*, const char*);
		Remapping_setting(int, TiXmlElement*, const char*);
		~Remapping_setting();
		void detect_conflict(Remapping_setting*, const char*);
		void reset_remapping_setting();
		int get_field_specification_manner() { return field_specification_manner; }
		void get_field_remapping_setting(Remapping_setting&, const char*);
		Remapping_algorithm_specification *get_H2D_remapping_algorithm() { return H2D_remapping_algorithm; }
		Remapping_algorithm_specification *get_V1D_remapping_algorithm() { return V1D_remapping_algorithm; }
		Remapping_algorithm_specification *get_T1D_remapping_algorithm() { return T1D_remapping_algorithm; }
		void write_remapping_setting_into_array(char **, int &, int &);
		void read_remapping_setting_from_array(const char *, int &);
		void print();
		Remapping_setting *clone();
		bool is_the_same_as_another(Remapping_setting*);
};


class Remapping_configuration
{
	private:
		int comp_id;
		std::vector<Remapping_setting*> remapping_settings;

	public:
		Remapping_configuration();
		Remapping_configuration(int, const char*);
		~Remapping_configuration();
		int get_comp_id() { return comp_id; }
		bool get_field_remapping_setting(Remapping_setting&, const char*);
};


class Remapping_configuration_mgt
{
	private:
		std::vector<Remapping_configuration*> remapping_configurations;

	public:
		Remapping_configuration_mgt() {}
		~Remapping_configuration_mgt();
		void add_remapping_configuration(int);
		Remapping_configuration *search_remapping_configuration(int);
		void get_field_remapping_setting(Remapping_setting &, int, const char*);
};


#endif

