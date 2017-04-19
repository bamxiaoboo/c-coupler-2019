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


Remapping_algorithm_specification::Remapping_algorithm_specification(const Remapping_algorithm_specification *src_specification)
{
	this->type_id = src_specification->type_id;
	strcpy(this->algorithm_name, src_specification->algorithm_name);
	for (int i = 0; i < src_specification->parameters_name.size(); i ++) {
		this->parameters_name.push_back(new char [NAME_STR_SIZE]);
		strcpy(this->parameters_name[this->parameters_name.size()-1], src_specification->parameters_name[i]);
		this->parameters_value.push_back(new char [NAME_STR_SIZE]);
		strcpy(this->parameters_value[this->parameters_value.size()-1], src_specification->parameters_value[i]);
	}
}


Remapping_algorithm_specification::Remapping_algorithm_specification(const char *algorithm_name, int algorithm_type)
{
	strcpy(this->algorithm_name, algorithm_name);
	this->type_id = algorithm_type;
}


Remapping_algorithm_specification::Remapping_algorithm_specification(int comp_id, TiXmlElement *XML_element, const char *XML_file_name, int algorithm_type)
{
	int line_number;

	
	this->type_id = algorithm_type;
	this->comp_id = comp_id;
	const char *algorithm_name = get_XML_attribute(comp_id, XML_element, "name", XML_file_name, line_number, "the name of a remapping algorithm", "remapping configuration");
	if (algorithm_type == REMAP_ALGORITHM_TYPE_H2D)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, remap_operator_manager->get_remap_operator_num_dim(algorithm_name) == 2, "\"%s\" is not a legal remapping operator or not a 2D remapping operator. Please verify the XML file \"%s\" around the line number %d", algorithm_name, XML_file_name, line_number);
	else if (algorithm_type == REMAP_ALGORITHM_TYPE_V1D || algorithm_type == REMAP_ALGORITHM_TYPE_T1D)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, remap_operator_manager->get_remap_operator_num_dim(algorithm_name) == 1, "\"%s\" is not a legal remapping operator or not a 1D remapping operator. Please verify the XML file \"%s\" around the line number %d", algorithm_name, XML_file_name, line_number);
	else EXECUTION_REPORT(REPORT_ERROR, -1, "Software error in Remapping_algorithm_specification::Remapping_algorithm_specification");

	strcpy(this->algorithm_name, algorithm_name);
	for (TiXmlNode *detailed_element_node = XML_element->FirstChild(); detailed_element_node != NULL; detailed_element_node = detailed_element_node->NextSibling()) {
		TiXmlElement *detailed_element = detailed_element_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(detailed_element->Value(), "parameter"), "When setting the remapping configuration in the XML file \"%s\", \"%s\" is not a legal attribute. Please verify the XML file arround the line number %d.", XML_file_name, detailed_element->Value(), detailed_element->Row());
		const char *parameter_name = get_XML_attribute(comp_id, detailed_element, "name", XML_file_name, line_number, "the name of a parameter of the corresponding remapping algorithm",  "remapping configuration");
		const char *parameter_value = get_XML_attribute(comp_id, detailed_element, "value", XML_file_name, line_number, "the value of a parameter of the corresponding remapping algorithm",  "remapping configuration");
		char error_string[NAME_STR_SIZE];
		int parameter_check_result = remap_operator_manager->check_operator_parameter(algorithm_name, parameter_name, parameter_value, error_string);
		if (parameter_check_result == 0)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "The remapping algorithm \"%s\" does not have a parameter named \"%s\". Please verify the XML file \"%s\" around the line number %d", algorithm_name, parameter_name, XML_file_name, line_number);
		else if (parameter_check_result == 1)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "The value of the parameter \"%s\" of the remapping algorithm \"%s\" is wrong. %s. Please verify the XML file \"%s\" around the line number %d", parameter_name, algorithm_name, error_string, XML_file_name, line_number);
		parameters_name.push_back(strdup(parameter_name));
		parameters_value.push_back(strdup(parameter_value));
	}
}


Remapping_algorithm_specification::Remapping_algorithm_specification(const char *array, int *buffer_content_iter)
{
	read_data_from_array_buffer(&type_id, sizeof(int), array, *buffer_content_iter);
	read_data_from_array_buffer(algorithm_name, NAME_STR_SIZE, array, *buffer_content_iter);
	int temp_int;
	read_data_from_array_buffer(&temp_int, sizeof(int), array, *buffer_content_iter);
	for (int i = 0; i < temp_int; i ++) {
		this->parameters_name.push_back(new char [NAME_STR_SIZE]);
		read_data_from_array_buffer(this->parameters_name[this->parameters_name.size()-1], NAME_STR_SIZE, array, *buffer_content_iter);
		this->parameters_value.push_back(new char [NAME_STR_SIZE]);
		read_data_from_array_buffer(this->parameters_value[this->parameters_value.size()-1], NAME_STR_SIZE, array, *buffer_content_iter);		
	}
}


Remapping_algorithm_specification::~Remapping_algorithm_specification()
{
	for (int i = 0; i < parameters_name.size(); i ++) {
		delete [] parameters_name[i];
		delete [] parameters_value[i];
	}
}


void Remapping_algorithm_specification::print()
{
	if (type_id == REMAP_ALGORITHM_TYPE_H2D)
		printf("   H2D remapping algorithm \"%s\" ", algorithm_name);
	if (type_id == REMAP_ALGORITHM_TYPE_V1D)
		printf("   V1D remapping algorithm \"%s\" ", algorithm_name);
	if (type_id == REMAP_ALGORITHM_TYPE_T1D)
		printf("   T1D remapping algorithm \"%s\" ", algorithm_name);
	for (int i = 0; i < parameters_name.size(); i ++)
		printf(": \"%s\"(\"%s\") ", parameters_name[i], parameters_value[i]);
	printf("\n");
}


void Remapping_algorithm_specification::write_remapping_algorithm_specification_into_array(char **array, int &buffer_max_size, int &buffer_content_size)
{
	for (int i = parameters_name.size()-1; i >= 0; i --) {
		write_data_into_array_buffer(parameters_value[i], NAME_STR_SIZE, array, buffer_max_size, buffer_content_size);
		write_data_into_array_buffer(parameters_name[i], NAME_STR_SIZE, array, buffer_max_size, buffer_content_size);
	}
	int temp_int = parameters_name.size();
	write_data_into_array_buffer(&temp_int, sizeof(int), array, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(algorithm_name, NAME_STR_SIZE, array, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&type_id, sizeof(int), array, buffer_max_size, buffer_content_size);
}


void Remapping_algorithm_specification::get_parameter(int i, char *parameter_name, char *parameter_value)
{
	EXECUTION_REPORT(REPORT_ERROR, comp_id, i >= 0 && i < parameters_name.size(), "Software error in Remapping_algorithm_specification::get_parameter");
	strcpy(parameter_name, parameters_name[i]);
	strcpy(parameter_value, parameters_value[i]);
}


Remapping_algorithm_specification *Remapping_algorithm_specification::clone()
{
	Remapping_algorithm_specification *cloned_specification = new Remapping_algorithm_specification;
	cloned_specification->comp_id = this->comp_id;
	cloned_specification->type_id = this->type_id;
	strcpy(cloned_specification->algorithm_name, this->algorithm_name);
	for (int i = 0; i < this->parameters_name.size(); i ++) {
		cloned_specification->parameters_name.push_back(strdup(this->parameters_name[i]));
		cloned_specification->parameters_value.push_back(strdup(this->parameters_value[i]));
	}

	return cloned_specification;
}


bool Remapping_algorithm_specification::is_the_same_as_another(Remapping_algorithm_specification *another)
{
	if (another->type_id != this->type_id)
		return false;
	if (!words_are_the_same(another->algorithm_name,this->algorithm_name))
		return false;
	if (another->parameters_name.size() != this->parameters_name.size())
		return false;
	for (int i = 0; i < this->parameters_name.size(); i ++) {
		if (!words_are_the_same(another->parameters_name[i],this->parameters_name[i]) || !words_are_the_same(another->parameters_value[i],this->parameters_value[i]))
			return false;
	}

	return true;	
}


Remapping_setting::Remapping_setting()
{
	H2D_remapping_algorithm = NULL;
	V1D_remapping_algorithm = NULL;
	T1D_remapping_algorithm = NULL;
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

	if (H2D_remapping_algorithm != NULL)
		delete H2D_remapping_algorithm;
	if (V1D_remapping_algorithm != NULL)
		delete V1D_remapping_algorithm;
	if (T1D_remapping_algorithm != NULL)
		delete T1D_remapping_algorithm;
}


void Remapping_setting::reset_remapping_setting()
{
	for (int i = 0; i < fields_specification.size(); i ++)
		delete [] fields_specification[i];
	fields_specification.clear();
	field_specification_manner = -1;
	H2D_remapping_algorithm = NULL;
	V1D_remapping_algorithm = NULL;
	T1D_remapping_algorithm = NULL;
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


void Remapping_setting::get_field_remapping_setting(Remapping_setting &field_remapping_configuration, const char *field_name)
{
	bool transfer_remapping_algorithms = false;

		
	if (!(field_remapping_configuration.H2D_remapping_algorithm == NULL && this->H2D_remapping_algorithm != NULL ||
		  field_remapping_configuration.V1D_remapping_algorithm == NULL && this->V1D_remapping_algorithm != NULL ||
		  field_remapping_configuration.T1D_remapping_algorithm == NULL && this->T1D_remapping_algorithm != NULL))
		return;

	if (field_specification_manner == 0)
		transfer_remapping_algorithms = true;
	else if (field_specification_manner == 1) {
		if (fields_info->search_field_info(field_name) == NULL)
			transfer_remapping_algorithms = true;
		else if (words_are_the_same(fields_info->search_field_info(field_name)->field_type, fields_specification[0]))
			transfer_remapping_algorithms = true;
	}
	else {
		for (int i = 0; i < fields_specification.size(); i ++)
			if (words_are_the_same(fields_specification[i], field_name)) {
				transfer_remapping_algorithms = true;
				break;
			}
	}

	if (transfer_remapping_algorithms) {
		if (field_remapping_configuration.H2D_remapping_algorithm == NULL && this->H2D_remapping_algorithm != NULL)
			field_remapping_configuration.H2D_remapping_algorithm = new Remapping_algorithm_specification(this->H2D_remapping_algorithm);
		if (field_remapping_configuration.V1D_remapping_algorithm == NULL && this->V1D_remapping_algorithm != NULL)
			field_remapping_configuration.V1D_remapping_algorithm = new Remapping_algorithm_specification(this->V1D_remapping_algorithm);
		if (field_remapping_configuration.T1D_remapping_algorithm == NULL && this->T1D_remapping_algorithm != NULL)
			field_remapping_configuration.T1D_remapping_algorithm = new Remapping_algorithm_specification(this->T1D_remapping_algorithm);
	}
}


void Remapping_setting::print()
{
	printf("\n\nprint a remapping setting:\n");
	if (H2D_remapping_algorithm != NULL)
		H2D_remapping_algorithm->print();
	if (V1D_remapping_algorithm != NULL)
		V1D_remapping_algorithm->print();
	if (T1D_remapping_algorithm != NULL)
		T1D_remapping_algorithm->print();
	printf("\n\n");
}


void Remapping_setting::write_remapping_setting_into_array(char **array, int &buffer_max_size, int &buffer_content_size)
{
	printf("%lx %lx %lx\n", H2D_remapping_algorithm, V1D_remapping_algorithm, T1D_remapping_algorithm);
	EXECUTION_REPORT(REPORT_ERROR, -1, H2D_remapping_algorithm != NULL && V1D_remapping_algorithm != NULL && T1D_remapping_algorithm != NULL, "software error in Remapping_setting::write_remapping_setting_into_array");
	T1D_remapping_algorithm->write_remapping_algorithm_specification_into_array(array, buffer_max_size, buffer_content_size);
	V1D_remapping_algorithm->write_remapping_algorithm_specification_into_array(array, buffer_max_size, buffer_content_size);
	H2D_remapping_algorithm->write_remapping_algorithm_specification_into_array(array, buffer_max_size, buffer_content_size);		
}


void Remapping_setting::read_remapping_setting_from_array(const char *array, int &buffer_content_iter)
{	
	H2D_remapping_algorithm = new Remapping_algorithm_specification(array, &buffer_content_iter);
	V1D_remapping_algorithm = new Remapping_algorithm_specification(array, &buffer_content_iter);
	T1D_remapping_algorithm = new Remapping_algorithm_specification(array, &buffer_content_iter);
	EXECUTION_REPORT(REPORT_ERROR, -1, buffer_content_iter == 0, "Software error in Remapping_setting::read_remapping_setting_from_array");
}


Remapping_setting *Remapping_setting::clone()
{
	Remapping_setting *cloned_setting = new Remapping_setting;
	cloned_setting->comp_id = this->comp_id;
	cloned_setting->XML_start_line_number = this->XML_start_line_number;
	cloned_setting->field_specification_manner = this->field_specification_manner;
	if (this->H2D_remapping_algorithm != NULL)
		cloned_setting->H2D_remapping_algorithm = this->H2D_remapping_algorithm->clone();
	if (this->V1D_remapping_algorithm != NULL)
		cloned_setting->V1D_remapping_algorithm = this->V1D_remapping_algorithm->clone();
	if (this->T1D_remapping_algorithm != NULL)
		cloned_setting->T1D_remapping_algorithm = this->T1D_remapping_algorithm->clone();
	for(int i = 0; i < this->fields_specification.size(); i ++)
		cloned_setting->fields_specification.push_back(strdup(this->fields_specification[i]));

	return cloned_setting;
}


bool Remapping_setting::is_the_same_as_another(Remapping_setting *another)
{
	if (another->field_specification_manner != this->field_specification_manner)
		return false;
	if (another->H2D_remapping_algorithm == NULL && this->H2D_remapping_algorithm != NULL || another->H2D_remapping_algorithm != NULL && this->H2D_remapping_algorithm == NULL)
		return false;
	if (another->V1D_remapping_algorithm == NULL && this->V1D_remapping_algorithm != NULL || another->V1D_remapping_algorithm != NULL && this->V1D_remapping_algorithm == NULL)
		return false;
	if (another->T1D_remapping_algorithm == NULL && this->T1D_remapping_algorithm != NULL || another->T1D_remapping_algorithm != NULL && this->T1D_remapping_algorithm == NULL)
		return false;

	if (another->H2D_remapping_algorithm != NULL && !another->H2D_remapping_algorithm->is_the_same_as_another(this->H2D_remapping_algorithm))
		return false;
	if (another->V1D_remapping_algorithm != NULL && !another->V1D_remapping_algorithm->is_the_same_as_another(this->V1D_remapping_algorithm))
		return false;
	if (another->T1D_remapping_algorithm != NULL && !another->T1D_remapping_algorithm->is_the_same_as_another(this->T1D_remapping_algorithm))
		return false;

	if (another->fields_specification.size() != this->fields_specification.size())
		return false;
	for (int i = 0; i < this->fields_specification.size(); i ++)
		if (!words_are_the_same(another->fields_specification[i], this->fields_specification[i]))
			return false;

	return true;
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
	EXECUTION_REPORT(REPORT_ERROR, comp_id, XML_file.LoadFile(comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in Remapping_configuration::Remapping_configuration")), "Fail to read the XML configuration file \"%s\", because the file is not in a legal XML format. Please check.", XML_file_name);
	for (TiXmlNode *XML_element_node = XML_file.FirstChildElement(); XML_element_node != NULL; XML_element_node = XML_element_node->NextSibling()) {
		TiXmlElement *XML_element = XML_element_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(XML_element->Value(), "remapping_setting"), "\"%s\" is not a legal attribute (the legal is \"remapping_setting\") for defining a remapping setting. Please verify the XML file arround the line number %d.", XML_element->Value(), XML_element->Row());
		if (!is_XML_setting_on(comp_id, XML_element, XML_file_name, "the status of a remapping setting", "remapping configuration"))
			continue;
		remapping_settings.push_back(new Remapping_setting(comp_id, XML_element, XML_file_name));
	}

	for (int i = 0; i < remapping_settings.size(); i ++)
		for (int j = i+1; j < remapping_settings.size(); j ++)
			remapping_settings[i]->detect_conflict(remapping_settings[j], XML_file_name);

	std::vector<Remapping_setting*> temp_remapping_settings;
	for (int i = 0; i < remapping_settings.size(); i ++)
		if (remapping_settings[i]->get_field_specification_manner() == 2)
			temp_remapping_settings.push_back(remapping_settings[i]);
	for (int i = 0; i < remapping_settings.size(); i ++)
		if (remapping_settings[i]->get_field_specification_manner() == 1)
			temp_remapping_settings.push_back(remapping_settings[i]);
	for (int i = 0; i < remapping_settings.size(); i ++)
		if (remapping_settings[i]->get_field_specification_manner() == 0)
			temp_remapping_settings.push_back(remapping_settings[i]);

	EXECUTION_REPORT(REPORT_ERROR, -1, temp_remapping_settings.size() == remapping_settings.size(), "Software error in Remapping_configuration::Remapping_configuration");

	remapping_settings.clear();
	for (int i = 0; i < temp_remapping_settings.size(); i ++)
		remapping_settings.push_back(temp_remapping_settings[i]);
}


Remapping_configuration::~Remapping_configuration()
{
	for (int i = 0; i < remapping_settings.size(); i ++)
		delete remapping_settings[i];
}


bool Remapping_configuration::get_field_remapping_setting(Remapping_setting &field_remapping_configuration, const char *field_name)
{
	printf("get field remapping_setting at comp %s\n", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"")->get_comp_name());

	for (int i = 0; i < remapping_settings.size(); i ++) {
		remapping_settings[i]->get_field_remapping_setting(field_remapping_configuration, field_name);
		if (field_remapping_configuration.get_H2D_remapping_algorithm() != NULL && field_remapping_configuration.get_V1D_remapping_algorithm() != NULL && field_remapping_configuration.get_T1D_remapping_algorithm() != NULL)
			return true;
	}
	return false;
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
	Comp_comm_group_mgt_node *current_comp_node = comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Remapping_configuration_mgt::add_remapping_configuration");
	if (comp_id == comp_comm_group_mgt_mgr->get_global_node_root()->get_comp_id())
		sprintf(XML_file_name, "%s/all/overall_remapping_configuration.xml", comp_comm_group_mgt_mgr->get_config_root_dir());
	else sprintf(XML_file_name, "%s/remapping_configs/%s.remapping_configuration.xml", comp_comm_group_mgt_mgr->get_config_root_comp_dir(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Remapping_configuration_mgt::add_remapping_configuration")->get_full_name());
	FILE *fp = fopen(XML_file_name, "r");
	if (fp == NULL) {
		EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "The remapping configuration file \"%s\" for the current component does not exist.", XML_file_name);
		return;
	}
	fclose(fp);
	remapping_configurations.push_back(new Remapping_configuration(comp_id, XML_file_name));
}


Remapping_configuration *Remapping_configuration_mgt::search_remapping_configuration(int comp_id)
{
	for (int i = 0; i < remapping_configurations.size(); i ++)
		if (remapping_configurations[i]->get_comp_id() == comp_id)
			return remapping_configurations[i];

	return NULL;
}


void Remapping_configuration_mgt::get_field_remapping_setting(Remapping_setting &field_remapping_setting, int comp_id, const char *field_name)
{
	field_remapping_setting.reset_remapping_setting();
	Comp_comm_group_mgt_node *current_comp_node = comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Remapping_configuration_mgt::get_current_remapping_setting");
	EXECUTION_REPORT(REPORT_ERROR, -1, current_comp_node != NULL, "Software error in Remapping_configuration_mgt::get_field_remapping_setting1");
	for (; current_comp_node != NULL; current_comp_node = current_comp_node->get_parent()) {
		Remapping_configuration *current_remapping_configuration = search_remapping_configuration(current_comp_node->get_comp_id());
		if (current_remapping_configuration != NULL)
			if (current_remapping_configuration->get_field_remapping_setting(field_remapping_setting, field_name)) {
				field_remapping_setting.print();
				return;
			}
	}
	EXECUTION_REPORT(REPORT_ERROR, -1, false, "Software error in Remapping_configuration_mgt::get_field_remapping_setting");
}

