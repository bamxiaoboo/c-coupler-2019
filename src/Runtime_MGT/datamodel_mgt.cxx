/***************************************************************
  *  Copyright (c) 2017, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/

#include "global_data.h"
#include <dirent.h>
#include "field_info_mgt.h"
//#include "coupling_generator.h"
#include "quick_sort.h"
#include "datamodel_mgt.h"
//#include "datamodel_io.h"

#define TIME_FORMAT_YYYY                 (0x3C00)
#define TIME_FORMAT_YYYYMM               (0x3F00)
#define TIME_FORMAT_YYYYMMDD             (0x3FC0)
#define TIME_FORMAT_YYYYMMDDHHMMSS       (0x3FFF)
#define TIME_FORMAT_YYYYMMDDSSSSS        (0x3FFE)
#define TIME_FORMAT_MMDDSSSSS            (0x3FE)
#define TIME_FORMAT_MMDDHHMMSS           (0x3FF)
#define TIME_FORMAT_DDSSSSS              (0xFE)
#define TIME_FORMAT_DDHHMMSS             (0xFF)
#define TIME_FORMAT_SSSSS                (0x3E)
#define TIME_FORMAT_HHMMSS               (0x3F)
#define TIME_FORMAT_YYYYMMDDHHMM         (0x3FFC)
#define TIME_FORMAT_MMDDHHMM             (0x3FC)
#define TIME_FORMAT_DDHHMM               (0xFC)
#define TIME_FORMAT_HHMM                 (0x3C)
#define TIME_FORMAT_YYYYMMDDHH           (0x3FF0)
#define TIME_FORMAT_MMDDHH               (0x3F0)
#define TIME_FORMAT_DDHH                 (0xF0)
#define TIME_FORMAT_HH                   (0x30)
#define PRINT_READIN_INFO
#define vname(value) #value


void Datamodel_instance_info::check_is_input_datamodel_needed(int host_comp_id, const char *import_comp_name) {
	int current_proc_local_id;
	char XML_file_name[NAME_STR_SIZE];
	std::vector<char*>needed_datamodel_instance_name;

	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, MPI_Comm_rank(comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id, "in datamodel_instance::check_is_input_datamodel_needed"), &current_proc_local_id) == MPI_SUCCESS);
	int line_number;
	Coupling_connection *dm_coupling_connection;

	if (is_coupling_connection_file_read == true)
		return;

	sprintf(XML_file_name,"%s/all/coupling_connection/%s.coupling_connection.xml", comp_comm_group_mgt_mgr->get_config_root_dir(), import_comp_name);

	std::vector<Inout_interface*> import_interfaces_of_this_component;
	inout_interface_mgr->get_all_import_interfaces_of_a_component(import_interfaces_of_this_component, import_comp_name);

	Component_import_interfaces_configuration *this_comp_import_interfaces_config = new Component_import_interfaces_configuration(host_comp_id, import_comp_name, inout_interface_mgr, false);
	is_coupling_connection_file_read=true;
	for (int j = 0; j < import_interfaces_of_this_component.size(); j++) {
		std::vector<const char*> import_fields_name;
		import_interfaces_of_this_component[j]->get_fields_name(&import_fields_name);
		for (int k = 0; k < import_fields_name.size(); k ++) {
			std::vector<std::pair<const char*, const char*> > interface_export_producer_info;
			this_comp_import_interfaces_config->get_interface_field_import_configuration(import_interfaces_of_this_component[j]->get_interface_name(), import_fields_name[k], interface_export_producer_info);

			char datamodel_name_str[NAME_STR_SIZE], datamodel_interface_str[NAME_STR_SIZE];
			dm_coupling_connection = new Coupling_connection(coupling_generator->apply_connection_id());
			strcpy(dm_coupling_connection->dst_comp_full_name, import_comp_name);
			strcpy(dm_coupling_connection->dst_interface_name, import_interfaces_of_this_component[j]->get_interface_name());
			dm_coupling_connection->fields_name.push_back(strdup(import_fields_name[k]));
			char *report_string = NULL;

			for (int m = 0; m < interface_export_producer_info.size(); m++) {
				char *conn_dm_name = strdup(interface_export_producer_info[m].first);//comp_name, in this case null
				char *conn_dm_interface_name = strdup(interface_export_producer_info[m].second);//datamodel_instance_name
				int l;

				for (l = 0; l < strlen(conn_dm_interface_name); l++) {
					if (conn_dm_interface_name[l] != '_')
						datamodel_interface_str[l] = conn_dm_interface_name[l];
					else break;
				}
				datamodel_interface_str[l] = '\0';
				if (words_are_the_same(datamodel_interface_str,"Datainst")) {
					is_datamodel_needed=true;
					dm_coupling_connection->src_comp_interfaces.push_back(std::make_pair(strdup(conn_dm_name), strdup(conn_dm_interface_name)));
				}
			}

			if (interface_export_producer_info.size() > 0) {
				report_string = new char[NAME_STR_SIZE*dm_coupling_connection->src_comp_interfaces.size()];
				report_string[0] = '\0';
				for (int j = 0; j < dm_coupling_connection->src_comp_interfaces.size(); j++)
					sprintf(report_string, "%s 				%d) datamodel_instance is \"%s\"\n", report_string, j+1, dm_coupling_connection->src_comp_interfaces[j].second);
			}
			EXECUTION_REPORT(REPORT_ERROR, host_comp_id, interface_export_producer_info.size() <= 1, "Error happens when reading datamodel related coupling configuration: Field \"%s\" of the import interface \"%s\" in the component model \"%s\" have more than one datamodel instance sources as follows. Please verify.\n%s\n", import_fields_name[k], import_interfaces_of_this_component[j]->get_interface_name(), import_fields_name, report_string);
			if (interface_export_producer_info.size() != 0) {//如果通过耦合连接配置文件指定，应该不会发生一对多的情况，耦合连接配置文件的结构导致不会发生这种情况
				EXECUTION_REPORT_LOG(REPORT_LOG, -1, true, "Field \"%s\" of the import interface \"%s\" in the component model \"%s\" have one source as follows. %s\n", dm_coupling_connection->fields_name[0], dm_coupling_connection->dst_interface_name, dm_coupling_connection->dst_comp_full_name, report_string);
				datamodel_coupling_connections.push_back(dm_coupling_connection);
				needed_datamodel_instance_name.push_back(strdup(dm_coupling_connection->src_comp_interfaces[0].second));
			}
		}
	}
	if (is_datamodel_needed) {
		load_datamodel_instatnces_configuration(host_comp_id, import_comp_name, XML_file_name,needed_datamodel_instance_name);//过一遍datamodel_instance设置
	}
}

void Datamodel_instance_info::load_datamodel_instatnces_configuration(int host_comp_id, const char *host_comp_full_name, char* XML_file_name, std::vector<char*> needed_datamodel_instance_name) {
	int line_number;

	TiXmlDocument *XML_file = open_XML_file_to_read(host_comp_id, XML_file_name, MPI_COMM_NULL, false);
	EXECUTION_REPORT(REPORT_ERROR, -1, XML_file != NULL, "There is no coupling_connections configuration file (the file name should be \"%s.coupling_connections.xml\") specified for the component \"%s\", the coupling procedures of the import/export interfaces of this component will be generated automatically", host_comp_full_name, host_comp_full_name);

	TiXmlNode *root_XML_element_node = get_XML_first_child_of_unique_root(host_comp_id, XML_file_name, XML_file);
	TiXmlElement *root_XML_element = XML_file->FirstChildElement();
	for (; root_XML_element_node != NULL; root_XML_element_node = root_XML_element_node->NextSibling()) {
		if (root_XML_element_node->Type() != TiXmlNode::TINYXML_ELEMENT)
			continue;
		root_XML_element = root_XML_element_node->ToElement();
		if (words_are_the_same(root_XML_element->Value(), "datamodel_instances"))
			break;
	}
	if (root_XML_element == NULL) {
		delete XML_file;
		return;
	}

	for (int i = 0; i < needed_datamodel_instance_name.size(); i++) {
		bool datamodel_instance_found = false;
		for (TiXmlNode *datamodel_instance_element_node = root_XML_element_node->FirstChild(); datamodel_instance_element_node != NULL; datamodel_instance_element_node = datamodel_instance_element_node->NextSibling()) {
			if (datamodel_instance_element_node->Type() != TiXmlNode::TINYXML_ELEMENT)
				continue;
			TiXmlElement *datamodel_instance_element = datamodel_instance_element_node->ToElement();
			EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(datamodel_instance_element->Value(),"datamodel_instance"), "The XML element for specifying the configuration information of a Datamodel instance in the XML configuration file \"%s\" should be named \"datamodel_instance\". Please verify the XML file around the line number %d.", XML_file_name, datamodel_instance_element->Row());
			const char *datamodel_instance_name = get_XML_attribute(host_comp_id, 80, datamodel_instance_element, "name", XML_file_name, line_number, "the \"name\" of a datamodel instance", "Datamodel instance configuration file", true);
			if (words_are_the_same(datamodel_instance_name,needed_datamodel_instance_name[i])) {
				datamodel_instance_found = true;
				if (!is_XML_setting_on(host_comp_id, datamodel_instance_element, XML_file_name, "the \"status\" of the Datamodel instance for a component model", "Datamodel instance configuration file"))
					continue;
				check_and_verify_name_format_of_string_for_XML(host_comp_id, datamodel_instance_name, "datamodel instance", XML_file_name, line_number);
				read_datamodel_instance_configuration(host_comp_id, host_comp_full_name, XML_file_name, datamodel_instance_name, datamodel_instance_element_node);
			}
			else break;
		}
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, !datamodel_instance_found, "the datamodel instance \"%s\" needed for component model \"%s\" has not been specified in node \"datamodel_instances\" in configuration file \"%s\", Please Verify.",needed_datamodel_instance_name[i],host_comp_full_name,XML_file_name);
	}
}
void Datamodel_instance_info::read_datamodel_instance_configuration(int host_comp_id, const char *host_comp_full_name, char *XML_file_name, const char *datamodel_instance_name, TiXmlNode *datamodel_instance_element_node) {
	//
}