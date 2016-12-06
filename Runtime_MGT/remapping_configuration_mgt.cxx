/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "remapping_configuration_mgt.h"
#include "CCPL_api_mgt.h"


Remapping_algorithm_specification::Remapping_algorithm_specification(const char *algorithm_name, int algorithm_type)
{
	// check the algorithm exists and its type is the same as the given
	strcpy(this->algorithm_name, algorithm_name);
	this->type_id = algorithm_type;
}


Remapping_algorithm_specification::Remapping_algorithm_specification(int comp_id, TiXmlElement *XML_element, const char *XML_file_name, int algorithm_type)
{
	int line_number;

	
	this->type_id = algorithm_type;
	this->comp_id = comp_id;
	const char *algorithm_name = get_XML_attribute(comp_id, XML_element, "name", XML_file_name, line_number, "the name of a remapping algorithm", "remapping configuration");
	// check the algorithm exists and its type is the same as the given
	strcpy(this->algorithm_name, algorithm_name);
	for (TiXmlNode *detailed_element_node = XML_element->FirstChild(); detailed_element_node != NULL; detailed_element_node = detailed_element_node->NextSibling()) {
		TiXmlElement *detailed_element = detailed_element_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(detailed_element->Value(), "parameter"), "When setting the remapping configuration in the XML file \"%s\", \"%s\" is not a legal attribute. Please verify the XML file arround the line number %d.", XML_file_name, detailed_element->Value(), detailed_element->Row());
		const char *parameter_name = get_XML_attribute(comp_id, detailed_element, "name", XML_file_name, line_number, "the name of a parameter of the corresponding remapping algorithm",  "remapping configuration");
		const char *parameter_value = get_XML_attribute(comp_id, detailed_element, "value", XML_file_name, line_number, "the value of a parameter of the corresponding remapping algorithm",  "remapping configuration");
		// check whether the paramter name and value are legal
		parameters_name.push_back(strdup(parameter_name));
		parameters_value.push_back(strdup(parameter_value));
	}
}


Remapping_algorithm_specification::~Remapping_algorithm_specification()
{
	for (int i = 0; i < parameters_name.size(); i ++) {
		delete [] parameters_name[i];
		delete [] parameters_value[i];
	}
}


Remapping_setting::Remapping_setting(const char *H2D_remapping_algorithm_name, const char *field_type)
{
	this->comp_id = comp_comm_group_mgt_mgr->get_global_node_root()->get_comp_id();
	H2D_remapping_algorithm = new Remapping_algorithm_specification(H2D_remapping_algorithm_name, REMAP_ALGORITHM_TYPE_H2D);
	V1D_remapping_algorithm = new Remapping_algorithm_specification(REMAP_OPERATOR_NAME_LINEAR, REMAP_ALGORITHM_TYPE_V1D);
	T1D_remapping_algorithm = new Remapping_algorithm_specification(REMAP_OPERATOR_NAME_LINEAR, REMAP_ALGORITHM_TYPE_T1D);
	field_specification_manner = 1;
	fields_specification.push_back(strdup(field_type));
}


Remapping_setting::Remapping_setting(int comp_id, TiXmlElement *XML_element, const char *XML_file_name)
{
	int i, line_number;
	int num_remapping_algorithm_section = 0, num_fields_section = 0;


	this->comp_id = comp_id;
	this->XML_start_line_number = XML_element->Row();
	H2D_remapping_algorithm = NULL;
	V1D_remapping_algorithm = NULL;
	T1D_remapping_algorithm = NULL;
	
	for (TiXmlNode *detailed_element_node = XML_element->FirstChild(); detailed_element_node != NULL; detailed_element_node = detailed_element_node->NextSibling()) {
		TiXmlElement *detailed_element = detailed_element_node->ToElement();
		if (words_are_the_same(detailed_element->Value(), "remapping_algorithms")) {		
			if (!is_XML_setting_on(comp_id, detailed_element, XML_file_name, "the status of the configuration of a section about remapping algorithms", "remapping configuration"))
				continue;
			EXECUTION_REPORT(REPORT_ERROR, comp_id, num_remapping_algorithm_section == 0, "When setting the remapping configuration in the XML file \"%s\", there are more than one section for specifying remapping algorithms. That is not allowed. Please verify the XML file arround the line number %d", XML_file_name, detailed_element->Row());		
			for (TiXmlNode *algorithm_element_node = detailed_element_node->FirstChild(); algorithm_element_node != NULL; algorithm_element_node = algorithm_element_node->NextSibling()) {
				TiXmlElement *algorithm_element = algorithm_element_node->ToElement();
				if (words_are_the_same(algorithm_element->Value(), "H2D_algorithm")) {
					EXECUTION_REPORT(REPORT_ERROR, comp_id, H2D_remapping_algorithm == NULL, "When setting the remapping configuration in the XML file \"%s\", H2D_algorithm has been set more than once. Please verify the XML file arround the line number %d", XML_file_name, detailed_element->Row());
					H2D_remapping_algorithm = new Remapping_algorithm_specification(comp_id, algorithm_element, XML_file_name, REMAP_ALGORITHM_TYPE_H2D);
				}
				else if (words_are_the_same(algorithm_element->Value(), "V1D_algorithm")) {
					EXECUTION_REPORT(REPORT_ERROR, comp_id, V1D_remapping_algorithm == NULL, "When setting the remapping configuration in the XML file \"%s\", V1D_algorithm has been set more than once. Please verify the XML file arround the line number %d", XML_file_name, detailed_element->Row());
					V1D_remapping_algorithm = new Remapping_algorithm_specification(comp_id, algorithm_element, XML_file_name, REMAP_ALGORITHM_TYPE_V1D);
				}
				else EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "When setting the remapping configuration in the XML file \"%s\", \"%s\" is not a legal attribute of remapping algorithm. Please verify the XML file arround the line number %d", XML_file_name, algorithm_element->Value(), algorithm_element->Row());
			}
			num_remapping_algorithm_section ++;
		}
		else if (words_are_the_same(detailed_element->Value(),"fields")) {		
			if (!is_XML_setting_on(comp_id, detailed_element, XML_file_name, "the status of the configuration of a section about fields", "remapping configuration"))
				continue;
			EXECUTION_REPORT(REPORT_ERROR, comp_id, num_fields_section == 0, "When setting the remapping configuration in the XML file \"%s\", there are more than one section for specifying fields. That is not allowed. Please verify the XML file arround the line number %d", XML_file_name, detailed_element->Row());		
			const char *specification_type = get_XML_attribute(comp_id, detailed_element, "specification", XML_file_name, line_number, "how to specify the fields corresponding to a remapping setting", "remapping configuration");
			EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(specification_type, "type") || words_are_the_same(specification_type, "all fields") || words_are_the_same(specification_type, "name"), "In the XML file \"%s\", the manner for how to specify fields must be \"type\", \"all fields\" or \"name\". Please verify the XML file arround the line number %d.", XML_file_name, line_number);
			if (words_are_the_same(specification_type, "type")) {
				field_specification_manner = 1;
				for (TiXmlNode *type_element_node = detailed_element_node->FirstChild(); type_element_node != NULL; type_element_node = type_element_node->NextSibling()) {
					TiXmlElement *type_element = type_element_node->ToElement();
					const char *field_type = get_XML_attribute(comp_id, type_element, "value", XML_file_name, line_number, "the field type corresponding to a remapping setting", "remapping configuration");
					printf("field type is \"%s\"\n", field_type);
					if (words_are_the_same(field_type, "state") || words_are_the_same(field_type, "flux"))
						printf("feichangqiguai\n");
					EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(field_type, "state") || words_are_the_same(field_type, "flux"), "In the XML file \"%s\" for remapping configuration, the field type \"%s\" is wrong. C-Coupler only supports field types \"state\" and \"flux\" at this time. Please verify the XML file arround the line number %d.", XML_file_name, field_type, line_number);
					EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_specification.size() == 0, "In the XML file \"%s\" for remapping configuration, there are more than one field type specified while only one field type can be set for a remapping setting. Please verify the XML file arround the line number %d.", XML_file_name, type_element->Row());
					fields_specification.push_back(strdup(field_type));
				}
				EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_specification.size() > 0, "In the XML file \"%s\" for remapping configuration, no field type has been specified for a remapping setting. Please verify the XML file arround the line number %d.", XML_file_name, detailed_element->Row());
			}
			else if (words_are_the_same(specification_type, "all fields"))
				field_specification_manner = 0;
			else {
				field_specification_manner = 2;
				for (TiXmlNode *field_element_node = detailed_element_node->FirstChild(); field_element_node != NULL; field_element_node = field_element_node->NextSibling()) {
					TiXmlElement *field_element = field_element_node->ToElement();
					const char *field_name = get_XML_attribute(comp_id, field_element, "value", XML_file_name, line_number, "the field name corresponding to a remapping setting", "remapping configuration");
					for (i = 0; i < fields_specification.size(); i ++)
						if (words_are_the_same(fields_specification[i], field_name))
							break;
					if (i == fields_specification.size())
						fields_specification.push_back(strdup(field_name));
				}
				EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_specification.size() > 0, "In the XML file \"%s\" for remapping configuration, no field name has been specified for a remapping setting. Please verify the XML file arround the line number %d.", XML_file_name, detailed_element->Row());
			}
			num_fields_section ++;
		}
		else EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "When setting the remapping configuration in the XML file \"%s\", \"%s\" is not a legal attribute. Please verify the XML file arround the line number %d.", XML_file_name, detailed_element->Value(), detailed_element->Row());
	}	
	EXECUTION_REPORT(REPORT_ERROR, comp_id, num_remapping_algorithm_section == 1 && (H2D_remapping_algorithm != NULL || V1D_remapping_algorithm != NULL), "For the XML file \"%s\" that is for remapping configuration, no remapping algorithms is specified for the remapping setting starting from the line number %d. Please verify.", XML_file_name, XML_element->Row());
	EXECUTION_REPORT(REPORT_ERROR, comp_id, num_fields_section == 1, "For the XML file \"%s\" that is for remapping configuration, no fields is specified for the remapping setting starting from the line number %d. Please verify.", XML_file_name, XML_element->Row());
}


Remapping_setting::~Remapping_setting()
{
	for (int i = 0; i < fields_specification.size(); i ++)
		delete [] fields_specification[i];
}


void Remapping_setting::detect_conflict(Remapping_setting *another_setting, const char *XML_file_name)
{
	if (this->field_specification_manner == 0 && another_setting->field_specification_manner == 0) 
		EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "In the XML file \%s\" that is for remapping configuration, there is conflict between the remapping settings starting from line %d and %d respectively: both settings specify all fields. Please verify. ", XML_file_name, another_setting->XML_start_line_number, this->XML_start_line_number);

	if (this->field_specification_manner == 1 && another_setting->field_specification_manner == 1)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, !words_are_the_same(this->fields_specification[0], another_setting->fields_specification[0]), "In the XML file \%s\" that is for remapping configuration, there is conflict (the same type of fields: \"%s\") between the remapping settings starting from line %d and %d respectively. Please verify. ", XML_file_name, this->fields_specification[0], another_setting->XML_start_line_number, this->XML_start_line_number);

	if (this->field_specification_manner == 2 && another_setting->field_specification_manner == 2) {
		for (int i = 0; i < this->fields_specification.size(); i ++)
			for (int j = 0; j < another_setting->fields_specification.size(); j ++)
				EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(this->fields_specification[i], another_setting->fields_specification[j]), "In the XML file \%s\" that is for remapping configuration, there is conflict (the same field name: \"%s\") between the remapping settings starting from line %d and %d respectively. Please verify. ", XML_file_name, this->fields_specification[i], another_setting->XML_start_line_number, this->XML_start_line_number); 
	}
}


Remapping_configuration::Remapping_configuration()
{
	comp_id = comp_comm_group_mgt_mgr->get_global_node_root()->get_comp_id();
	remapping_settings.push_back(new Remapping_setting(REMAP_OPERATOR_NAME_BILINEAR, "state"));
	remapping_settings.push_back(new Remapping_setting(REMAP_OPERATOR_NAME_CONSERV_2D, "flux"));
}


Remapping_configuration::Remapping_configuration(int comp_id, const char *XML_file_name)
{
	this->comp_id = comp_id;
	TiXmlDocument XML_file(XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, XML_file.LoadFile(), "Fail to read the XML configuration file \"%s\", because the file is not in a legal XML format. Please check.", XML_file_name);
	for (TiXmlNode *XML_element_node = XML_file.FirstChildElement(); XML_element_node != NULL; XML_element_node = XML_element_node->NextSibling()) {
		TiXmlElement *XML_element = XML_element_node->ToElement();
		if (!is_XML_setting_on(comp_id, XML_element, XML_file_name, "the status of a remapping setting", "remapping configuration"))
			continue;
		remapping_settings.push_back(new Remapping_setting(comp_id, XML_element, XML_file_name));
	}

	for (int i = 0; i < remapping_settings.size(); i ++)
		for (int j = i+1; j < remapping_settings.size(); j ++)
			remapping_settings[i]->detect_conflict(remapping_settings[j], XML_file_name);
}


Remapping_configuration::~Remapping_configuration()
{
	for (int i = 0; i < remapping_settings.size(); i ++)
		delete remapping_settings[i];
}


Remapping_configuration_mgt::~Remapping_configuration_mgt()
{
	for (int i = 0; i < remapping_configurations.size(); i ++)
		delete remapping_configurations[i];
}


void Remapping_configuration_mgt::add_remapping_configuration(int comp_id)
{
	if (comp_id == comp_comm_group_mgt_mgr->get_global_node_root()->get_comp_id()) {
		printf("add default remapping configuration\n");
		remapping_configurations.push_back(new Remapping_configuration());
	}

	char XML_file_name[NAME_STR_SIZE];
	sprintf(XML_file_name, "%s/CCPL_configs/remapping_configuration.xml", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Remapping_configuration_mgt::add_remapping_configuration")->get_working_dir());
	FILE *fp = fopen(XML_file_name, "r");
	if (fp == NULL) {
		EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "The remapping configuration file \"%s\" for the current component does not exist.", XML_file_name);
		return;
	}
	remapping_configurations.push_back(new Remapping_configuration(comp_id, XML_file_name));
}


Remapping_configuration *Remapping_configuration_mgt::search_remapping_configuration(int comp_id)
{
	for (int i = 0; i < remapping_configurations.size(); i ++)
		if (remapping_configurations[i]->get_comp_id() == comp_id)
			return remapping_configurations[i];

	return NULL;
}

