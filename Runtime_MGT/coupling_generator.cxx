/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "coupling_generator.h"


Import_direction_setting::Import_direction_setting(const char *comp_full_name, const char *interface_name, TiXmlElement *redirection_element, const char *XML_file_name, std::vector<const char*> &interface_fields_name, int *fields_count)
{
	int comp_id = comp_comm_group_mgt_mgr->search_global_node(comp_full_name)->get_comp_id();
	TiXmlElement *fields_element = NULL, *components_element = NULL, *remapping_element = NULL, *merge_element = NULL;
	int i;

	
	strcpy(this->interface_name, interface_name);
	for (TiXmlNode *detailed_element_node = redirection_element->FirstChild(); detailed_element_node != NULL; detailed_element_node = detailed_element_node->NextSibling()) {
		TiXmlElement *detailed_element = detailed_element_node->ToElement();
		if (words_are_the_same(detailed_element->Value(), "fields")) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_element == NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the attribute of \"fields\" has been set at least twice in a redirection specification. Please verify.", interface_name, XML_file_name);
			fields_element = detailed_element;
			const char *default_str = detailed_element->Attribute("default");
			EXECUTION_REPORT(REPORT_ERROR, comp_id, default_str != NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please set the value of \"default\" for the attribute of \"fields\"", interface_name, XML_file_name);
			EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(default_str, "off") || words_are_the_same(default_str, "all") || words_are_the_same(default_str, "remain"), "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the value of \"default\" for the attribute of \"fields\" is wrong (legal values are \"off\", \"all\" and \"remain\"). Please verify.", interface_name, XML_file_name);
			if (words_are_the_same(default_str, "off")) {
				fields_default_setting = 0;
				for (TiXmlNode *field_element_node = detailed_element->FirstChild(); field_element_node != NULL; field_element_node = field_element_node->NextSibling()) {
					TiXmlElement *field_element = field_element_node->ToElement();
					EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(field_element->Value(),"field"), "When setting the attribute \"fields\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please use the keyword \"field\" for the name of a field", interface_name, XML_file_name);
					const char *field_name = field_element->Attribute("name");
					EXECUTION_REPORT(REPORT_ERROR, comp_id, field_name != NULL, "When setting the attribute \"fields\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please set the value of \"name\" for a field", interface_name, XML_file_name);
					char *field_name_str = new char [NAME_STR_SIZE];
					strcpy(field_name_str, field_name);	
					check_and_verify_name_format_of_string_for_XML(comp_id, field_name_str, "the field", XML_file_name);
					EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_info->search_field_info(field_name) != NULL, "When setting the attribute \"fields\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", an illegal field name (\"%s\") is detected. Please verify", interface_name, XML_file_name, field_name);
					for (i = 0; i < interface_fields_name.size(); i ++)
						if (words_are_the_same(interface_fields_name[i], field_name_str))
							break;
					if (i < interface_fields_name.size())
						fields_name.push_back(interface_fields_name[i]);
					else EXECUTION_REPORT(REPORT_WARNING, comp_id, true, "When setting the attribute \"fields\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the interface does not contain a field with the name of \"%s\"", interface_name, XML_file_name, field_name);
				}				
			}
			else if (words_are_the_same(default_str, "all")) {
				fields_default_setting = 1;
				for (i = 0; i < interface_fields_name.size(); i ++) {
					EXECUTION_REPORT(REPORT_ERROR, comp_id, fields_count[i] == 0, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the configuration information of field \"%s\" has been set more than once. This is not allowed. Please note that the default value \"all\" means all fields. Please verify", interface_name, XML_file_name); 
					fields_count[i] ++;
					fields_name.push_back(interface_fields_name[i]);					
				}
			}
			else {
				fields_default_setting = 2;
				for (i = 0; i < interface_fields_name.size(); i ++) {
					if (fields_count[i] != 0)
						continue;
					fields_count[i] ++;
					fields_name.push_back(interface_fields_name[i]);					
				}
			}
		}
		else if (words_are_the_same(detailed_element->Value(), "components")) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, components_element == NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the attribute of \"components\" has been set at least twice in a redirection specification. Please verify.", interface_name, XML_file_name);
			components_element = detailed_element;
			const char *default_str = detailed_element->Attribute("default");
			EXECUTION_REPORT(REPORT_ERROR, comp_id, default_str != NULL, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please set the value of \"default\" for the attribute of \"components\"", interface_name, XML_file_name);
			EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(default_str, "off") || words_are_the_same(default_str, "all"), "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the value of \"default\" for the attribute of \"componets\" is wrong (legal values are \"off\" and \"all\"). Please verify.", interface_name, XML_file_name);
			if (words_are_the_same(default_str, "off")) {
				components_default_setting = 0;
				for (TiXmlNode *component_element_node = detailed_element->FirstChild(); component_element_node != NULL; component_element_node = component_element_node->NextSibling()) {
					TiXmlElement *component_element = component_element_node->ToElement();
					EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(component_element->Value(),"component"), "When setting the attribute \"components\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please use the keyword \"component\" for the full name of a component", interface_name, XML_file_name);
					const char *full_name = component_element->Attribute("full_name");
					EXECUTION_REPORT(REPORT_ERROR, comp_id, full_name != NULL, "When setting the attribute \"components\" for the redirection configuration of the import interface \"%s\" in the XML file \"%s\", please set the value of \"full_name\" for a component", interface_name, XML_file_name);
					char *full_name_str = new char [NAME_STR_SIZE];
					strcpy(full_name_str, full_name);				
					check_and_verify_name_format_of_string_for_XML(comp_id, full_name_str, "the the component model", XML_file_name);
					components_full_name.push_back(full_name_str);
				}
			}
			else components_default_setting = 1;
		}
		else if (words_are_the_same(detailed_element->Value(), "remapping_setting")) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the attribute of \"remapping_setting\" is not supported currently", interface_name, XML_file_name);
		}
		else if (words_are_the_same(detailed_element->Value(), "merge_setting")) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the attribute of \"merge_setting\" is not supported currently", interface_name, XML_file_name);
		}
		else EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", \"%s\" is not a legal attribute. Please verify.", interface_name, XML_file_name, detailed_element->Value());
	}		
}


Import_direction_setting::~Import_direction_setting()
{
	for (int i = 0; i < components_full_name.size(); i ++)
		delete [] components_full_name[i];
}


Import_interface_configuration::Import_interface_configuration(const char *comp_full_name, const char *interface_name, TiXmlElement *interface_element, const char *XML_file_name, Inout_interface_mgt *all_interfaces_mgr)
{
	std::vector<const char*> fields_name;
	int *fields_count;
	Inout_interface *interface_ptr = all_interfaces_mgr->get_interface(comp_full_name, interface_name);

	
	strcpy(this->interface_name, interface_name);
	interface_ptr->get_fields_name(&fields_name);
	fields_count = new int [fields_name.size()];
	for (int i = 0; i < fields_name.size(); i ++)
		fields_count[i] = 1;

	for (TiXmlNode *redirection_element_node = interface_element->FirstChild(); redirection_element_node != NULL; redirection_element_node = redirection_element_node->NextSibling()) {
		TiXmlElement *redirection_element = redirection_element_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, interface_ptr->get_comp_id(), words_are_the_same(redirection_element->Value(),"import_redirection"), "When setting the redirection configuration of the import interface \"%s\" in the XML file \"%s\", the XML element for specifying the redirection configuration should be named \"import_redirection\". Please verify.", interface_name, XML_file_name);
		const char *status = redirection_element->Attribute("status");
		EXECUTION_REPORT(REPORT_ERROR, interface_ptr->get_comp_id(), status != NULL, "In the XML file \"%s\", the status of some redirection configurations for the import interface \"%s\" has not been set. Please verify.",
			             XML_file_name, interface_name);
		EXECUTION_REPORT(REPORT_ERROR, interface_ptr->get_comp_id(), words_are_the_same(status, "on") || words_are_the_same(status, "off"), "In the XML file \"%s\", the value of the status of some redirection configurations for the import interface \"%s\" are wrong (the value must be \"on\" or \"off\"). Please verify.",
			             XML_file_name, interface_name);
		if (words_are_the_same(status, "off"))
			continue;
		import_directions.push_back(new Import_direction_setting(comp_full_name, interface_name, redirection_element, XML_file_name, fields_name, fields_count));
	}
}


Component_import_interfaces_configuration::Component_import_interfaces_configuration(int comp_id, Inout_interface_mgt *interface_mgr)
{
	char XML_file_name[NAME_STR_SIZE];
	FILE *tmp_file;


	strcpy(comp_full_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name());
	sprintf(XML_file_name, "%s/CCPL_configs/%s.import.redirection.xml", comp_comm_group_mgt_mgr->get_root_working_dir(), comp_full_name);
	tmp_file = fopen(XML_file_name, "r");
	if (tmp_file == NULL) {
		EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "As there is no import interface configuration file (the file name should be \"%s.import.redirection.xml\") specified for the component \"%s\", the coupling procedures of the import/export interfaces of this component will be generated automatically", 
			             comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_comp_name());
		return;
	}
	fclose(tmp_file);

	TiXmlDocument XML_file(XML_file_name);
	sprintf(XML_file_name, "%s.import.redirection.xml", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_full_name());
	EXECUTION_REPORT(REPORT_ERROR, -1, XML_file.LoadFile(), "Fail to read the XML configuration file \"%s\", because the file is not in a legal XML format. Please check.", XML_file_name);
	TiXmlElement *root_XML_element = XML_file.FirstChildElement();
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(root_XML_element->Value(),"component_import_interfaces_configuration"), "The root XML element of the XML configuration file \"%s\" should be named \"component_import_interfaces_configuration\". Please verify.", XML_file_name);
	for (TiXmlNode *interface_XML_element_node = root_XML_element->FirstChild(); interface_XML_element_node != NULL; interface_XML_element_node = interface_XML_element_node->NextSibling()) {
		TiXmlElement *interface_XML_element = interface_XML_element_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(interface_XML_element->Value(),"import_interface"), "The XML element for specifying the configuration information of an import interface in the XML configuration file \"%s\" should be named \"import_interface\". Please verify.", XML_file_name);
		const char *interface_name = interface_XML_element->Attribute("name");
		EXECUTION_REPORT(REPORT_ERROR, -1, interface_name != NULL, "The format of the XML configuration file \"%s\" is wrong: the \"name\" of some import interface is not specified.", XML_file_name);
		char local_interface_name[NAME_STR_SIZE];
		strcpy(local_interface_name, interface_name);
		check_and_verify_name_format_of_string_for_XML(-1, local_interface_name, "the import interface", XML_file_name);
		Inout_interface *import_interface = interface_mgr->get_interface(comp_full_name, local_interface_name);
		printf("to get import interface %s  %s: %lx\n", comp_full_name, local_interface_name, import_interface);
		if (import_interface == NULL) {
			printf("report warning\n");
			EXECUTION_REPORT(REPORT_WARNING, -1, true, "The redirection configuration of the import interface named \"%s\" has been specified in the XML configuration file \"%s\", while the component \"%s\" does not register the corresponding import interface. So this redirection configuration information is negleted.\"", 
				             local_interface_name, XML_file_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_comp_name());
			continue;
		}
		EXECUTION_REPORT(REPORT_ERROR, -1, import_interface->get_import_or_export() == 0, "The redirection configuration of the import interface named \"%s\" has been specified in the XML configuration file \"%s\", while the component \"%s\" registers \"%s\" as an export interface. Please verify the model code or the XML file",
			             local_interface_name, XML_file_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in Component_import_interfaces_configuration")->get_comp_name(), local_interface_name);
		import_interfaces_configuration.push_back(new Import_interface_configuration(comp_full_name, import_interface->get_interface_name(), interface_XML_element, XML_file_name, interface_mgr));
	}
}


Component_import_interfaces_configuration::~Component_import_interfaces_configuration()
{
}


void Coupling_generator::generate_coupling_procedures()
{
	inout_interface_mgr->merge_inout_interface_fields_info(TYPE_COMP_LOCAL_ID_PREFIX);
	if (comp_comm_group_mgt_mgr->get_current_proc_global_id() == 0) {
		Inout_interface_mgt *all_interfaces_mgr = new Inout_interface_mgt(inout_interface_mgr->get_temp_array_buffer(), inout_interface_mgr->get_buffer_content_size());
		all_interfaces_mgr->write_all_interfaces_fields_info();
		generate_interface_fields_source_dst(inout_interface_mgr->get_temp_array_buffer(), inout_interface_mgr->get_buffer_content_size());
		const int *all_components_ids = comp_comm_group_mgt_mgr->get_all_components_ids();
		for (int i = 1; i < all_components_ids[0]; i ++) {
			Component_import_interfaces_configuration *comp_import_interfaces_config = new Component_import_interfaces_configuration(all_components_ids[i], all_interfaces_mgr);
			delete comp_import_interfaces_config;
		}
		delete all_interfaces_mgr;
	}
}


void Coupling_generator::generate_interface_fields_source_dst(const char *temp_array_buffer, int buffer_content_size)
{
	std::vector<const char*> distinct_import_fields_name;
	std::vector<const char*> distinct_export_fields_name;


	import_field_index_lookup_table = new Dictionary<int>(1024);
	export_field_index_lookup_table = new Dictionary<int>(1024);

	if (comp_comm_group_mgt_mgr->get_current_proc_global_id() == 0) {
		int buffer_content_iter = buffer_content_size;
		while (buffer_content_iter > 0) {
			char comp_full_name[NAME_STR_SIZE], interface_name[NAME_STR_SIZE], field_name[NAME_STR_SIZE];
			int import_or_export, field_id_iter = 100, field_index, num_fields;
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
					import_fields_src_components[field_index].push_back(strdup(comp_full_name));
					printf("push producer comp %s for field %s\n", comp_full_name, field_name);
				}
				else {
					if (export_field_index_lookup_table->search(field_name, false) == 0) {
						export_field_index_lookup_table->insert(field_name, field_id_iter++);
						distinct_export_fields_name.push_back(strdup(field_name));
					}
					field_index = export_field_index_lookup_table->search(field_name, false);
					export_fields_dst_components[field_index].push_back(strdup(comp_full_name));
				}
			}
		}
		for (int i = 0; i < distinct_import_fields_name.size(); i ++) {
			int field_index = import_field_index_lookup_table->search(distinct_import_fields_name[i],false);
			printf("The components that uses field %s include: \n", distinct_import_fields_name[i]);
			for(int i = 0; i < import_fields_src_components[field_index].size(); i ++) {
				printf("           %s\n", import_fields_src_components[field_index][i]);
			}
		}
		for (int i = 0; i < distinct_export_fields_name.size(); i ++) {
			int field_index = export_field_index_lookup_table->search(distinct_export_fields_name[i],false);
			printf("The components that produces field %s include: \n", distinct_export_fields_name[i]);
			for(int i = 0; i < export_fields_dst_components[field_index].size(); i ++) {
				printf("           %s\n", export_fields_dst_components[field_index][i]);
			}
		}
	}	
}

