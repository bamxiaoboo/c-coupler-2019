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
		std::vector<const char *> parameters_name;
		std::vector<const char *> parameters_value;

	public:
		Remapping_algorithm_specification(const char*, int);
		Remapping_algorithm_specification(int, TiXmlElement*, const char*, int);
		~Remapping_algorithm_specification();
};


class Remapping_setting
{
	private:
		int comp_id;
		int XML_start_line_number;
		Remapping_algorithm_specification *H2D_remapping_algorithm;
		Remapping_algorithm_specification *V1D_remapping_algorithm;
		Remapping_algorithm_specification *T1D_remapping_algorithm;
		int field_specification_manner;    // 0 means all fields of a component; 1 means type; 2 means name
		std::vector<const char *> fields_specification;

	public:
		Remapping_setting(const char*, const char*);
		Remapping_setting(int, TiXmlElement*, const char*);
		~Remapping_setting();
		void detect_conflict(Remapping_setting*, const char*);
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
};


#endif

