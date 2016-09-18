/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "coupling_generator.h"


Import_direction_setting::Import_direction_setting(Import_interface_configuration *interface_configuration, const char *comp_full_name, const char *interface_name, TiXmlElement *redirection_element, const char *XML_file_name, std::vector<const char*> &interface_fields_name, int *fields_count)
{
	int comp_id = comp_comm_group_mgt_mgr->search_global_node(comp_full_name)->get_comp_id();
	TiXmlElement *fields_element = NULL, *components_element = NULL, *remapping_element = NULL, *merge_element = NULL;
	int i, line_number;

	printf("in Import_direction_setting::Import_direction_setting\n");
	strcpy(this->interface_name, interface_name);
	for (TiXmlNode *detailed_element_node = redirection_element->FirstChild(); detailed_element_node != NULL; detailed_element_node = detailed_element_node->NextSibling()) {
		TiXmlElement *detailed_element = detailed_element_node->ToElement();
		if (words_are_the_same(detailed_element->Value(), "fields")) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_element == NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the attribute of \"fields\" has been set at least twice in a redirection specification. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, detailed_element->Row());
			fields_element = detailed_element;
			const char *default_str = detailed_element->Attribute("default", &line_number);
			EXECUTION_REPORT(REPORT_ERROR, comp_id, default_str != NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please set the value of \"default\" for the attribute of \"fields\" (arround line %d of the XML file).", interface_name, XML_file_name, detailed_element->Row());
			EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(default_str, "off") || words_are_the_same(default_str, "all") || words_are_the_same(default_str, "remain"), "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the value of \"default\" for the attribute of \"fields\" is wrong (legal values are \"off\", \"all\" and \"remain\"). Please verify the XML file arround the line number %d.", interface_name, XML_file_name, line_number);
			if (words_are_the_same(default_str, "off")) {
				fields_default_setting = 0;
				for (TiXmlNode *field_element_node = detailed_element->FirstChild(); field_element_node != NULL; field_element_node = field_element_node->NextSibling()) {
					TiXmlElement *field_element = field_element_node->ToElement();
					EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(field_element->Value(),"field"), "When setting the attribute \"fields\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please use the keyword \"field\" for the name of a field (arround line %d of the XML file)", interface_name, XML_file_name, field_element->Row());
					const char *field_name = field_element->Attribute("name", &line_number);
					EXECUTION_REPORT(REPORT_ERROR, comp_id, field_name != NULL, "When setting the attribute \"fields\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please set the value of \"name\" for a field (arround line %d of the XML file)", interface_name, XML_file_name, field_element->Row());
					char field_name_str[NAME_STR_SIZE];
					strcpy(field_name_str, field_name);	
					check_and_verify_name_format_of_string_for_XML(comp_id, field_name_str, "the field", XML_file_name, line_number);
					EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_info->search_field_info(field_name) != NULL, "When setting the attribute \"fields\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", an illegal field name (\"%s\") is detected. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, field_name, field_element->Row());
					for (i = 0; i < interface_fields_name.size(); i ++)
						if (words_are_the_same(interface_fields_name[i], field_name_str))
							break;
					if (i < interface_fields_name.size())
						fields_name.push_back(strdup(interface_fields_name[i]));
					else EXECUTION_REPORT(REPORT_WARNING, comp_id, true, "When setting the attribute \"fields\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the interface does not contain a field with the name of \"%s\"", interface_name, XML_file_name, field_name);
				}
				EXECUTION_REPORT(REPORT_WARNING, comp_id, fields_name.size() > 0, "When setting a redirection configuration of the import interface \"%s\" in the XML file \"%s\", there are no fields specified. Please note the XML file arround the line number %d.", interface_name, XML_file_name, detailed_element->Row());
			}
			else if (words_are_the_same(default_str, "all")) {
				fields_default_setting = 1;
				printf("use all fields in configuration %d\n", interface_fields_name.size());
				for (i = 0; i < interface_fields_name.size(); i ++) {
					EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_count[i] == 0, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the configuration information of field \"%s\" has been set more than once. This is not allowed. Please note that the default value \"all\" means all fields. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, interface_fields_name[i], detailed_element->Row()); 
					fields_count[i] ++;
					fields_name.push_back(strdup(interface_fields_name[i]));					
				}
			}
			else {
				fields_default_setting = 2;
				for (i = 0; i < interface_fields_name.size(); i ++) {
					if (fields_count[i] != 0)
						continue;
					fields_count[i] ++;
					fields_name.push_back(strdup(interface_fields_name[i]));					
				}
			}
		}
		else if (words_are_the_same(detailed_element->Value(), "components")) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, components_element == NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the attribute of \"components\" has been set at least twice in a redirection specification. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, redirection_element->Row());
			components_element = detailed_element;
			const char *default_str = detailed_element->Attribute("default", &line_number);
			EXECUTION_REPORT(REPORT_ERROR, comp_id, default_str != NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please set the value of \"default\" for the attribute of \"components\" (arround line %d of the XML file)", interface_name, XML_file_name, detailed_element->Row());
			EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(default_str, "off") || words_are_the_same(default_str, "all"), "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the value of \"default\" for the attribute of \"componets\" is wrong (legal values are \"off\" and \"all\"). Please verify the XML file arround the line number %d.", interface_name, XML_file_name, line_number);
			if (words_are_the_same(default_str, "off")) {
				components_default_setting = 0;
				for (TiXmlNode *component_element_node = detailed_element->FirstChild(); component_element_node != NULL; component_element_node = component_element_node->NextSibling()) {
					TiXmlElement *component_element = component_element_node->ToElement();
					EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(component_element->Value(),"component"), "When setting the attribute \"components\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please use the keyword \"component\" for the full name of a component. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, component_element->Row());
					const char *full_name = component_element->Attribute("full_name", &line_number);
					EXECUTION_REPORT(REPORT_ERROR, comp_id, full_name != NULL, "When setting the attribute \"components\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please set the value of \"full_name\" for a component. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, component_element->Row());
					char *full_name_str = strdup(full_name);				
					EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_comm_group_mgt_mgr->search_global_node(full_name_str) != NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the full component name (\"%s\") is wrong. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, full_name_str, line_number);
					for (i = 0; i < components_full_name.size(); i ++)
						if (words_are_the_same(components_full_name[i], full_name_str))
							break;
					if (i == components_full_name.size())
						components_full_name.push_back(full_name_str);				
				}
			}
			else {
				comp_comm_group_mgt_mgr->get_all_components_ids();
				components_default_setting = 1;
				const int *all_components_ids = comp_comm_group_mgt_mgr->get_all_components_ids();
				for (i = 1; i < all_components_ids[0]; i ++) {
					char *full_name_str = strdup(comp_comm_group_mgt_mgr->get_global_node_of_local_comp(all_components_ids[i], "in Import_direction_setting()")->get_comp_full_name());
					if (!words_are_the_same(full_name_str, comp_full_name))
						components_full_name.push_back(full_name_str);
					else delete [] full_name_str;
				}
			}
		}
		else if (words_are_the_same(detailed_element->Value(), "remapping_setting")) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the attribute of \"remapping_setting\" is not supported currently", interface_name, XML_file_name);
		}
		else if (words_are_the_same(detailed_element->Value(), "merge_setting")) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the attribute of \"merge_setting\" is not supported currently", interface_name, XML_file_name);
		}
		else EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", \"%s\" is not a legal attribute. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, detailed_element->Value(), detailed_element->Row());
	}		
	EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_element != NULL, "For a redirection configuration of the import interface \"%s\" in the XML file \"%s\", the information about fields is not set. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, redirection_element->Row());
	EXECUTION_REPORT(REPORT_ERROR, comp_id, components_element != NULL, "For a redirection configuration of the import interface \"%s\" in the XML file \"%s\", the information about components is not set. Please verify the XML file arround the line number %d.", interface_name, XML_file_name, redirection_element->Row());
	printf("qiguai config %d %d\n", fields_name.size(), components_full_name.size());
	for (i = 0; i < fields_name.size(); i ++)
		for (int j = 0; j < components_full_name.size(); j ++)
			interface_configuration->add_field_src_component(comp_id, fields_name[i], components_full_name[j]);
}


Import_direction_setting::~Import_direction_setting()
{
	for (int i = 0; i < components_full_name.size(); i ++)
		delete [] components_full_name[i];
}


Import_interface_configuration::Import_interface_configuration(const char *comp_full_name, const char *interface_name, TiXmlElement *interface_element, const char *XML_file_name, Inout_interface_mgt *all_interfaces_mgr)
{
	int *fields_count, line_number;
	Inout_interface *interface_ptr = all_interfaces_mgr->get_interface(comp_full_name, interface_name);
	std::vector<const char*> components_long_names;
	

	printf("in Import_interface_configuration::Import_interface_configuration\n");
	
	strcpy(this->interface_name, interface_name);
	interface_ptr->get_fields_name(&fields_name);
	printf("whywhy %d\n", fields_name.size());
	fields_count = new int [fields_name.size()];
	for (int i = 0; i < fields_name.size(); i ++)
		fields_count[i] = 0;

	for (int i = 0; i < fields_name.size(); i ++)
		fields_src_components.push_back(components_long_names);

	for (TiXmlNode *redirection_element_node = interface_element->FirstChild(); redirection_element_node != NULL; redirection_element_node = redirection_element_node->NextSibling()) {
		TiXmlElement *redirection_element = redirection_element_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, interface_ptr->get_comp_id(), words_are_the_same(redirection_element->Value(),"import_redirection"), "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the XML element for specifying the redirection configuration should be named \"import_redirection\". Please verify the XML file arround the line number %d.", interface_name, XML_file_name, redirection_element->Row());
		const char *status = redirection_element->Attribute("status", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, interface_ptr->get_comp_id(), status != NULL, "In the XML file \"%s\", the status of some redirection configurations for the import interface \"%s\" has not been set. Please verify the XML file arround the line number %d.", XML_file_name, interface_name, redirection_element->Row());
		EXECUTION_REPORT(REPORT_ERROR, interface_ptr->get_comp_id(), words_are_the_same(status, "on") || words_are_the_same(status, "off"), "In the XML file \"%s\", the value of the status of some redirection configurations for the import interface \"%s\" are wrong (the value must be \"on\" or \"off\"). Please verify the XML file arround the line number %d.", XML_file_name, interface_name, line_number);
		if (words_are_the_same(status, "off"))
			continue;
		import_directions.push_back(new Import_direction_setting(this, comp_full_name, interface_name, redirection_element, XML_file_name, fields_name, fields_count));
	}

	for (int i = 0; i < fields_name.size(); i ++) {
		printf("The src components for field \"%s\" according to the coupling configuration are:\n", fields_name[i]);
		for (int j = 0; j < fields_src_components[i].size(); j ++)
			printf("        \"%s\"\n", fields_src_components[i][j]);
	}
}


void Import_interface_configuration::add_field_src_component(int comp_id, const char *field_name, const char *comp_full_name)
{
	int i;

	
	for (i = 0; i < fields_name.size(); i ++)
		if (words_are_the_same(field_name, fields_name[i]))
			break;

	printf("add field src component %s %s\n", field_name, comp_full_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, i < fields_name.size(), "Software error in Import_interface_configuration::add_field_src_component");
	fields_src_components[i].push_back(comp_full_name);
}


void Import_interface_configuration::get_field_import_configuration(const char *field_name, std::vector<const char*> &components_full_name)
{
	int i;


	for (i = 0; i < fields_name.size(); i ++)
		if (words_are_the_same(fields_name[i], field_name)) {
			for (int j = 0; j < fields_src_components[i].size(); j ++)
				components_full_name.push_back(fields_src_components[i][j]);
			break;
		}

	EXECUTION_REPORT(REPORT_ERROR, -1, i < fields_name.size(), "Software error in Import_interface_configuration::get_field_import_configuration");
}


Component_import_interfaces_configuration::Component_import_interfaces_configuration(int comp_id, Inout_interface_mgt *interface_mgr)
{
	char XML_file_name[NAME_STR_SIZE];
	FILE *tmp_file;
	int line_number;


	strcpy(comp_full_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name());
	sprintf(XML_file_name, "%s/CCPL_configs/%s.import.redirection.xml", comp_comm_group_mgt_mgr->get_root_working_dir(), comp_full_name);
	tmp_file = fopen(XML_file_name, "r");
	if (tmp_file == NULL) {
		EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "As there is no import interface configuration file (the file name should be \"%s.import.redirection.xml\") specified for the component \"%s\", the coupling procedures of the import/export interfaces of this component will be generated automatically", 
			             comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name());
		return;
	}
	fclose(tmp_file);

	TiXmlDocument XML_file(XML_file_name);
	sprintf(XML_file_name, "%s.import.redirection.xml", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name());
	EXECUTION_REPORT(REPORT_ERROR, -1, XML_file.LoadFile(), "Fail to read the XML configuration file \"%s\", because the file is not in a legal XML format. Please check.", XML_file_name);
	TiXmlElement *root_XML_element = XML_file.FirstChildElement();
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(root_XML_element->Value(),"component_import_interfaces_configuration"), "The root XML element of the XML configuration file \"%s\" should be named \"component_import_interfaces_configuration\". Please verify the XML file arround the line number %d.", XML_file_name, root_XML_element->Row());
	for (TiXmlNode *interface_XML_element_node = root_XML_element->FirstChild(); interface_XML_element_node != NULL; interface_XML_element_node = interface_XML_element_node->NextSibling()) {
		TiXmlElement *interface_XML_element = interface_XML_element_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(interface_XML_element->Value(),"import_interface"), "The XML element for specifying the configuration information of an import interface in the XML configuration file \"%s\" should be named \"import_interface\". Please verify the XML file arround the line number %d.", XML_file_name, interface_XML_element->Row());
		const char *interface_name = interface_XML_element->Attribute("name", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, interface_name != NULL, "The format of the XML configuration file \"%s\" is wrong: the \"name\" of some import interface is not specified. Please verify the XML file arround the line number %d.", XML_file_name, interface_XML_element->Row());
		const char *status = interface_XML_element->Attribute("status", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, status != NULL, "The format of the XML configuration file \"%s\" is wrong: the \"status\" of the redirection configurations for import interface \"%s\" is not specified. Please verify the XML file arround the line number %d.", XML_file_name, interface_name, interface_XML_element->Row());
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(status, "on") || words_are_the_same(status, "off"), "In the XML file \"%s\", the value of the status of the redirection configurations for the import interface \"%s\" are wrong (the value must be \"on\" or \"off\"). Please verify the XML file arround the line number %d.", XML_file_name, interface_name, line_number);
		if (words_are_the_same(status, "off"))
			continue;
		interface_name = interface_XML_element->Attribute("name", &line_number);
		char local_interface_name[NAME_STR_SIZE];
		strcpy(local_interface_name, interface_name);
		check_and_verify_name_format_of_string_for_XML(-1, local_interface_name, "the import interface", XML_file_name, line_number);
		Inout_interface *import_interface = interface_mgr->get_interface(comp_full_name, local_interface_name);
		printf("to get import interface %s  %s: %lx\n", comp_full_name, local_interface_name, import_interface);
		if (import_interface == NULL) {
			EXECUTION_REPORT(REPORT_WARNING, -1, false, "The redirection configuration of the import interface named \"%s\" has been specified in the XML configuration file \"%s\", while the component \"%s\" does not register the corresponding import interface. So this redirection configuration information is negleted.\"", 
				             local_interface_name, XML_file_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name());
			continue;
		}
		EXECUTION_REPORT(REPORT_ERROR, -1, import_interface->get_import_or_export() == 0, "The redirection configuration of the import interface named \"%s\" has been specified in the XML configuration file \"%s\", while the component \"%s\" registers \"%s\" as an export interface. Please verify the model code or the XML file",
			             local_interface_name, XML_file_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name(), local_interface_name);
		for (int i = 0; i < import_interfaces_configuration.size(); i ++)
			EXECUTION_REPORT(REPORT_ERROR, -1, !words_are_the_same(import_interfaces_configuration[i]->get_interface_name(), import_interface->get_interface_name()), "The redirection configuration of the import interface named \"%s\" has been set more than once in the XML file \"%s\", which is not allowed (only once for an interface). Please verify.", import_interface->get_interface_name(), XML_file_name);
				
		import_interfaces_configuration.push_back(new Import_interface_configuration(comp_full_name, import_interface->get_interface_name(), interface_XML_element, XML_file_name, interface_mgr));
	}
}


Component_import_interfaces_configuration::~Component_import_interfaces_configuration()
{
}


void Component_import_interfaces_configuration::get_interface_field_import_configuration(const char *interface_name, const char *field_name, std::vector<const char*> &components_full_name)
{
	components_full_name.clear();
	for (int i = 0; i < import_interfaces_configuration.size(); i ++)
		if (words_are_the_same(import_interfaces_configuration[i]->get_interface_name(), interface_name)) 
			import_interfaces_configuration[i]->get_field_import_configuration(field_name, components_full_name);
}


void Coupling_generator::generate_coupling_procedures()
{
	bool define_use_wrong = false;
	char *temp_array_buffer = NULL;
	int current_array_buffer_size, max_array_buffer_size, temp_int;	
	Coupling_connection *coupling_connection;
	std::pair<char[NAME_STR_SIZE],char[NAME_STR_SIZE]> src_comp_interface;
	

	inout_interface_mgr->merge_inout_interface_fields_info(TYPE_COMP_LOCAL_ID_PREFIX);
	if (comp_comm_group_mgt_mgr->get_current_proc_global_id() == 0) {
		Inout_interface_mgt *all_interfaces_mgr = new Inout_interface_mgt(inout_interface_mgr->get_temp_array_buffer(), inout_interface_mgr->get_buffer_content_size());
		all_interfaces_mgr->write_all_interfaces_fields_info();
		generate_interface_fields_source_dst(inout_interface_mgr->get_temp_array_buffer(), inout_interface_mgr->get_buffer_content_size());
		const int *all_components_ids = comp_comm_group_mgt_mgr->get_all_components_ids();
		for (int i = 1; i < all_components_ids[0]; i ++) {
			std::vector<Inout_interface*> import_interfaces_of_a_component;
			all_interfaces_mgr->get_all_import_interfaces_of_a_component(import_interfaces_of_a_component, all_components_ids[i]);
			Component_import_interfaces_configuration *comp_import_interfaces_config = new Component_import_interfaces_configuration(all_components_ids[i], all_interfaces_mgr);
			for (int j = 0; j < import_interfaces_of_a_component.size(); j ++) {
				std::vector<const char*> import_fields_name;
				import_interfaces_of_a_component[j]->get_fields_name(&import_fields_name);
				for (int k = 0; k < import_fields_name.size(); k ++) {
					std::vector<const char*> configuration_export_components_full_name;
					coupling_connection = new Coupling_connection;
					comp_import_interfaces_config->get_interface_field_import_configuration(import_interfaces_of_a_component[j]->get_interface_name(), import_fields_name[k], configuration_export_components_full_name);
					strcpy(coupling_connection->dst_comp_full_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(all_components_ids[i], "in Component_import_interfaces_configuration")->get_full_name());
					strcpy(coupling_connection->dst_interface_name, import_interfaces_of_a_component[j]->get_interface_name());
					strcpy(coupling_connection->field_name, import_fields_name[k]);					
					int field_index = export_field_index_lookup_table->search(import_fields_name[k],false);
					if (field_index != 0) {
						if (configuration_export_components_full_name.size() == 0) {
							for (int l = 0; l < export_fields_dst_components[field_index].size(); l ++) {
								if (words_are_the_same(export_fields_dst_components[field_index][l].first, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(all_components_ids[i], "in Component_import_interfaces_configuration")->get_full_name()))
									continue;
								strcpy(src_comp_interface.first, export_fields_dst_components[field_index][l].first);
								strcpy(src_comp_interface.second, export_fields_dst_components[field_index][l].second);
								coupling_connection->src_comp_interfaces.push_back(src_comp_interface);
							}
						}
						else {
							for (int l = 0; l < configuration_export_components_full_name.size(); l ++) {
								for (int m = 0; m < export_fields_dst_components[field_index].size(); m ++)
									if (words_are_the_same(configuration_export_components_full_name[l], export_fields_dst_components[field_index][m].first)) {
										strcpy(src_comp_interface.first, export_fields_dst_components[field_index][m].first);
										strcpy(src_comp_interface.second, export_fields_dst_components[field_index][m].second);
										coupling_connection->src_comp_interfaces.push_back(src_comp_interface);
									}
							}
						}
					}
					all_coupling_connections.push_back(coupling_connection);
					if (coupling_connection->src_comp_interfaces.size() == 1)
						printf("field \"%s\" of import interface \"%s\" in component \"%s\" have %d source as follows. \n", coupling_connection->field_name, coupling_connection->dst_interface_name, coupling_connection->dst_comp_full_name, coupling_connection->src_comp_interfaces.size());
					else if (coupling_connection->src_comp_interfaces.size() == 0) {
						define_use_wrong = true;
						if (export_fields_dst_components[field_index].size() == 0)
							printf("ERROR: field \"%s\" of import interface \"%s\" in component \"%s\" does not have source: no component exports this field. \n", coupling_connection->field_name, coupling_connection->dst_interface_name, coupling_connection->dst_comp_full_name);
						else printf("ERROR: field \"%s\" of import interface \"%s\" in component \"%s\" does not have source: there are components exporting this field, however, none of which is specified in the corresponding configuration XML file\n", coupling_connection->field_name, coupling_connection->dst_interface_name, coupling_connection->dst_comp_full_name);
					}
					else {
						printf("ERROR: field \"%s\" of import interface \"%s\" in component \"%s\" have more than 1 (%d) sources as follows. Please add or modify the corresponding configuration XML file\n", coupling_connection->field_name, coupling_connection->dst_interface_name, coupling_connection->dst_comp_full_name, coupling_connection->src_comp_interfaces.size());
						define_use_wrong = true;
						
					}
					for (int j = 0; j < coupling_connection->src_comp_interfaces.size(); j ++)
						printf("		component is \"%s\", interface is \"%s\"\n", coupling_connection->src_comp_interfaces[j].first, coupling_connection->src_comp_interfaces[j].second);
				}
			}
			
			delete comp_import_interfaces_config;
		}
		delete all_interfaces_mgr;
		for (int i = all_coupling_connections.size() - 1; i >= 0; i --) {
			for (int j = all_coupling_connections[i]->src_comp_interfaces.size()-1; j >= 0; j --) {
				write_data_into_array_buffer(all_coupling_connections[i]->src_comp_interfaces[j].second, NAME_STR_SIZE, &temp_array_buffer, max_array_buffer_size, current_array_buffer_size);
				write_data_into_array_buffer(all_coupling_connections[i]->src_comp_interfaces[j].first, NAME_STR_SIZE, &temp_array_buffer, max_array_buffer_size, current_array_buffer_size);
			}
			temp_int = all_coupling_connections[i]->src_comp_interfaces.size();
			write_data_into_array_buffer(&temp_int, sizeof(int), &temp_array_buffer, max_array_buffer_size, current_array_buffer_size);
			write_data_into_array_buffer(all_coupling_connections[i]->dst_interface_name, NAME_STR_SIZE, &temp_array_buffer, max_array_buffer_size, current_array_buffer_size);
			write_data_into_array_buffer(all_coupling_connections[i]->dst_comp_full_name, NAME_STR_SIZE, &temp_array_buffer, max_array_buffer_size, current_array_buffer_size);			
			write_data_into_array_buffer(all_coupling_connections[i]->field_name, NAME_STR_SIZE, &temp_array_buffer, max_array_buffer_size, current_array_buffer_size);
		}
		temp_int = all_coupling_connections.size();
		write_data_into_array_buffer(&temp_int, sizeof(int), &temp_array_buffer, max_array_buffer_size, current_array_buffer_size);
	}
	
	EXECUTION_REPORT(REPORT_ERROR, -1, !define_use_wrong, "Errors are reported when automatically generating coupling procedures");

	MPI_Bcast(&current_array_buffer_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if (comp_comm_group_mgt_mgr->get_current_proc_global_id() != 0) 
		temp_array_buffer = new char [current_array_buffer_size];
	MPI_Bcast(temp_array_buffer, current_array_buffer_size, MPI_CHAR, 0, MPI_COMM_WORLD);
	if (comp_comm_group_mgt_mgr->get_current_proc_global_id() != 0) {
		int num_connections, num_sources, buffer_content_iter = current_array_buffer_size;
		read_data_from_array_buffer(&num_connections, sizeof(int), temp_array_buffer, buffer_content_iter);
		for (int i = 0; i < num_connections; i ++) {
			coupling_connection = new Coupling_connection;
			read_data_from_array_buffer(coupling_connection->field_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
			read_data_from_array_buffer(coupling_connection->dst_comp_full_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
			read_data_from_array_buffer(coupling_connection->dst_interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
			read_data_from_array_buffer(&num_sources, sizeof(int), temp_array_buffer, buffer_content_iter);
			for (int j = 0; j < num_sources; j ++) {
				read_data_from_array_buffer(src_comp_interface.first, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
				read_data_from_array_buffer(src_comp_interface.second, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
				coupling_connection->src_comp_interfaces.push_back(src_comp_interface);
			}
			all_coupling_connections.push_back(coupling_connection);
		}
		EXECUTION_REPORT(REPORT_ERROR, -1, buffer_content_iter == 0, "Software error in Coupling_generator::generate_coupling_procedures: %d", buffer_content_iter);
	}

	if (comp_comm_group_mgt_mgr->get_current_proc_global_id() == 2) {
		for (int i = 0; i < all_coupling_connections.size(); i ++) {
			printf("check field \"%s\" of import interface \"%s\" in component \"%s\" have %d source as follows. \n", all_coupling_connections[i]->field_name, all_coupling_connections[i]->dst_interface_name, all_coupling_connections[i]->dst_comp_full_name, all_coupling_connections[i]->src_comp_interfaces.size());
			for (int j = 0; j < all_coupling_connections[i]->src_comp_interfaces.size(); j ++)
				printf("		component is \"%s\", interface is \"%s\"\n", all_coupling_connections[i]->src_comp_interfaces[j].first, all_coupling_connections[i]->src_comp_interfaces[j].second);
		}
	}

	delete [] temp_array_buffer;

	for (int i = 0; i < all_coupling_connections.size(); i ++) {
		generate_coupling_connection(all_coupling_connections[i]);
	}
}


void Coupling_generator::generate_coupling_connection(const Coupling_connection *coupling_connection)
{
	Comp_comm_group_mgt_node *src_comp_node = comp_comm_group_mgt_mgr->search_global_node(coupling_connection->src_comp_interfaces[0].first);
	Comp_comm_group_mgt_node *dst_comp_node =comp_comm_group_mgt_mgr->search_global_node(coupling_connection->dst_comp_full_name);
	int current_proc_id_src_comp = src_comp_node->get_current_proc_local_id();
	int	current_proc_id_dst_comp = dst_comp_node->get_current_proc_local_id();
	int src_comp_root_proc_global_id = src_comp_node->get_root_proc_global_id();
	int dst_comp_root_proc_global_id = dst_comp_node->get_root_proc_global_id();
	int msg_tag;
	MPI_Request send_req, recv_req;
	MPI_Status status;

	
	if (current_proc_id_src_comp == -1 && current_proc_id_dst_comp == -1)
		return;

	if (current_proc_id_src_comp != -1)
		MPI_Barrier(src_comp_node->get_comm_group());
	if (current_proc_id_dst_comp != -1)
		MPI_Barrier(dst_comp_node->get_comm_group());

	if (current_proc_id_src_comp == 0) {		
		MPI_Isend(&msg_tag, 1, MPI_INT, dst_comp_root_proc_global_id, 1000+src_comp_root_proc_global_id, MPI_COMM_WORLD, &send_req);
	}
	if (current_proc_id_dst_comp == 0) {
		MPI_Irecv(&msg_tag, 1, MPI_INT, src_comp_root_proc_global_id, 1000+src_comp_root_proc_global_id, MPI_COMM_WORLD, &recv_req);	 		
	}


	if (current_proc_id_src_comp == 0) {
		MPI_Wait(&send_req, &status);
	}
	
	if (current_proc_id_dst_comp == 0) {
		MPI_Wait(&recv_req, &status);
	}
	printf("generate coupling connection at process %d\n", comp_comm_group_mgt_mgr->get_current_proc_global_id());
}


void Coupling_generator::generate_interface_fields_source_dst(const char *temp_array_buffer, int buffer_content_size)
{
	std::vector<const char*> distinct_import_fields_name;
	std::vector<const char*> distinct_export_fields_name;


	import_field_index_lookup_table = new Dictionary<int>(1024);
	export_field_index_lookup_table = new Dictionary<int>(1024);

	if (comp_comm_group_mgt_mgr->get_current_proc_global_id() == 0) {
		int buffer_content_iter = buffer_content_size;
		int import_or_export, field_id_iter = 100, field_index, num_fields;
		while (buffer_content_iter > 0) {
			char comp_full_name[NAME_STR_SIZE], interface_name[NAME_STR_SIZE], field_name[NAME_STR_SIZE];
			read_data_from_array_buffer(comp_full_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
			read_data_from_array_buffer(interface_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
			read_data_from_array_buffer(&import_or_export, sizeof(int), temp_array_buffer, buffer_content_iter);
			read_data_from_array_buffer(&num_fields, sizeof(int), temp_array_buffer, buffer_content_iter);
			for (int i = 0; i < num_fields; i ++) {
				read_data_from_array_buffer(field_name, NAME_STR_SIZE, temp_array_buffer, buffer_content_iter);
				if (import_or_export == 0) {
					if (import_field_index_lookup_table->search(field_name, false) == 0) {
						import_field_index_lookup_table->insert(field_name, field_id_iter++);
						distinct_import_fields_name.push_back(strdup(field_name));
					}
					field_index = import_field_index_lookup_table->search(field_name, false);
					import_fields_src_components[field_index].push_back(std::pair<const char*,const char*>(strdup(comp_full_name),strdup(interface_name)));
					printf("push producer comp %s for field %s\n", comp_full_name, field_name);
				}
				else {
					if (export_field_index_lookup_table->search(field_name, false) == 0) {
						export_field_index_lookup_table->insert(field_name, field_id_iter++);
						distinct_export_fields_name.push_back(strdup(field_name));
					}
					field_index = export_field_index_lookup_table->search(field_name, false);
					export_fields_dst_components[field_index].push_back(std::pair<const char*,const char*>(strdup(comp_full_name),strdup(interface_name)));
				}
			}
		}
		for (int i = 0; i < distinct_import_fields_name.size(); i ++) {
			int field_index = import_field_index_lookup_table->search(distinct_import_fields_name[i],false);
			printf("The components that uses field %s include: \n", distinct_import_fields_name[i]);
			for(int i = 0; i < import_fields_src_components[field_index].size(); i ++) {
				printf("           \"%s\"    \"%s\"\n", import_fields_src_components[field_index][i].first, import_fields_src_components[field_index][i].second);
			}
		}
		for (int i = 0; i < distinct_export_fields_name.size(); i ++) {
			int field_index = export_field_index_lookup_table->search(distinct_export_fields_name[i],false);
			printf("The components that produces field %s include: \n", distinct_export_fields_name[i]);
			for(int i = 0; i < export_fields_dst_components[field_index].size(); i ++) {
				printf("           \"%s\"    \"%s\"\n", export_fields_dst_components[field_index][i].first, export_fields_dst_components[field_index][i].second);
			}
		}
	}	
}

