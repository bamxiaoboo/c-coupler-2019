/**********************************************M*****************
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
#include <netcdf.h>
#include "quick_sort.h"
#include "datamodel_mgt.h"

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

bool varname_or_value(const char* string) {
    for (int i=0;i<strlen(string);i++) {
        if (!(((string[i]>='0') && (string[i]<='9')) || (string[i]=='.') || string[0] == '-')) 
            return false;//not decimal value
    }
    return true;//decimal value
}

char *tolower(const char *string) {
	if (string == NULL) return NULL;
	char *new_str = strdup(string);
	for (int i = 0; i < strlen(new_str); i++) {
		if (new_str[i]<='Z' || new_str[i]>='A')
			new_str[i]+=32;
	}
	return new_str;
}

bool words_are_similar(const char *str1, const char *str2) {
	char *word1 = strdup(tolower(str1));
	char *word2 = strdup(tolower(str2));
	if (words_are_the_same(word1, word2))
		return true;
	else return false;
}

int set_unit(const char* input_unit, const char *report_hint) {
    int output_unit;
    if (words_are_the_same(input_unit,"year") || words_are_the_same(input_unit,"years") || words_are_the_same(input_unit,"nyear") || words_are_the_same(input_unit,"nyears"))
        output_unit=1;
    else if (words_are_the_same(input_unit,"month") || words_are_the_same(input_unit,"months") || words_are_the_same(input_unit,"nmonth") || words_are_the_same(input_unit,"nmonths"))
        output_unit=2;
    else if (words_are_the_same(input_unit,"day") || words_are_the_same(input_unit,"days") || words_are_the_same(input_unit,"nday") || words_are_the_same(input_unit,"ndays"))
        output_unit=3;
    else if (words_are_the_same(input_unit,"second") || words_are_the_same(input_unit,"seconds") || words_are_the_same(input_unit,"nsecond") || words_are_the_same(input_unit,"nseconds"))
        output_unit=4;
    else EXECUTION_REPORT(REPORT_ERROR, -1, false, "the unit %s for %s is incorrect, Please check.", input_unit, report_hint);
    return output_unit;
}

int outer_time_unit_to_inner(const char *src_time_unit, int src_time_count, const char *dst_time_unit, const char *report_hint) {//only larger unit to smaller unit
	int id_src_time_unit = set_unit(src_time_unit, report_hint);
	int id_dst_time_unit = set_unit(dst_time_unit, report_hint);
	int dst_time_count;
	if (src_time_unit == NULL)
		dst_time_count = src_time_count;
	if ((src_time_unit == NULL) || (id_src_time_unit == 1 && id_dst_time_unit == 1) || (id_src_time_unit == 2 && id_dst_time_unit == 2) || (id_src_time_unit == 3 && id_dst_time_unit == 3) || (id_src_time_unit == 4 && id_dst_time_unit == 4))
		dst_time_count = src_time_count;
	else if (id_src_time_unit == 1 && id_dst_time_unit == 2)
		dst_time_count = src_time_count * 12;
	else if (id_src_time_unit == 1 && id_dst_time_unit == 3)
		dst_time_count = src_time_count * 365;//no leap year
	else if (id_src_time_unit == 1 && id_dst_time_unit == 4)
		dst_time_count = src_time_count * 365 * 24 * 3600;
	else if (id_src_time_unit == 2 && id_dst_time_unit == 3)
		dst_time_count = src_time_count * 30;
	else if (id_src_time_unit == 2 && id_dst_time_unit == 4)
		dst_time_count = src_time_count * 24 * 3600;
	else if (id_src_time_unit == 3 && id_dst_time_unit == 4)
		dst_time_count = src_time_count * 30 * 24 * 3600;
	else EXECUTION_REPORT(REPORT_ERROR, -1, "Error happens when transforming time units for %s, the units can only be one of \"(n)year(s)\", \"(n)month(s)\", \"(n)day(s)\" or \"(n)sedond(s)\", Please verify.", report_hint);
	return dst_time_count;
}

int check_time_format(const char* time_format, const char *report_hint) {
    int id_time_format;

    if (words_are_the_same(time_format, "YYYY"))
        id_time_format = TIME_FORMAT_YYYY;
    else if (words_are_the_same(time_format, "YYYYMM") || words_are_the_same(time_format, "YYYY-MM"))
        id_time_format = TIME_FORMAT_YYYYMM;
    else if (words_are_the_same(time_format, "YYYYMMDD") || words_are_the_same(time_format, "YYYY-MM-DD"))
        id_time_format = TIME_FORMAT_YYYYMMDD;
    else if (words_are_the_same(time_format, "YYYYMMDD.SSSSS") || words_are_the_same(time_format, "YYYY-MM-DD.SSSSS") || 
             words_are_the_same(time_format, "YYYYMMDD-SSSSS") || words_are_the_same(time_format, "YYYY-MM-DD-SSSSS") ||
             words_are_the_same(time_format, "YYYYMMDDSSSSS"))
        id_time_format = TIME_FORMAT_YYYYMMDDSSSSS;
    else if (words_are_the_same(time_format, "YYYYMMDD.HHMMSS") || words_are_the_same(time_format, "YYYY-MM-DD.HH-MM-SS") ||
             words_are_the_same(time_format, "YYYYMMDD.HH-MM-SS") || words_are_the_same(time_format, "YYYY-MM-DD.HHMMSS") ||
             words_are_the_same(time_format, "YYYY-MM-DD-HH-MM-SS") || words_are_the_same(time_format, "YYYYMMDDHHMMSS"))
        id_time_format = TIME_FORMAT_YYYYMMDDHHMMSS;
    else if (words_are_the_same(time_format, "SSSSS"))
        id_time_format = TIME_FORMAT_SSSSS;
    else if (words_are_the_same(time_format, "HHMMSS") || words_are_the_same(time_format, "HH-MM-SS") || words_are_the_same(time_format, "HH:MM:SS"))
        id_time_format = TIME_FORMAT_HHMMSS;
    else if (words_are_the_same(time_format, "MMDD.HHMMSS") || words_are_the_same(time_format, "MMDD-HHMMSS") || words_are_the_same(time_format, "MM-DD.HH-MM-SS") ||
             words_are_the_same(time_format, "MM-DD-HH-MM-SS") || words_are_the_same(time_format, "MMDDHHMMSS"))
        id_time_format = TIME_FORMAT_MMDDHHMMSS;
    else if (words_are_the_same(time_format, "MMDD.SSSSS") || words_are_the_same(time_format, "MMDD-SSSSS") || words_are_the_same(time_format, "MM-DD.SSSSS") ||
             words_are_the_same(time_format, "MM-DD-SSSSS") || words_are_the_same(time_format, "MMDDSSSSS"))
        id_time_format = TIME_FORMAT_MMDDSSSSS;
    else if (words_are_the_same(time_format, "DD.HHMMSS") || words_are_the_same(time_format, "DD-HHMMSS") || words_are_the_same(time_format, "DD.HH-MM-SS") ||
             words_are_the_same(time_format, "DD-HH-MM-SS") || words_are_the_same(time_format, "DDHHMMSS"))
        id_time_format = TIME_FORMAT_DDHHMMSS;
    else if (words_are_the_same(time_format, "DD.SSSSS") || words_are_the_same(time_format, "DD-SSSSS") || words_are_the_same(time_format, "DDSSSSS"))
        id_time_format = TIME_FORMAT_DDSSSSS;
    else if (words_are_the_same(time_format, "YYYYMMDD.HHMM") || words_are_the_same(time_format, "YYYY-MM-DD.HH-MM") ||
             words_are_the_same(time_format, "YYYYMMDD.HH-MM") || words_are_the_same(time_format, "YYYY-MM-DD.HHMM") ||
             words_are_the_same(time_format, "YYYY-MM-DD-HH-MM") || words_are_the_same(time_format, "YYYYMMDDHHMM"))
        id_time_format = TIME_FORMAT_YYYYMMDDHHMM;
    else if (words_are_the_same(time_format, "HHMM") || words_are_the_same(time_format, "HH-MM") || words_are_the_same(time_format, "HH:MM"))
        id_time_format = TIME_FORMAT_HHMM;
    else if (words_are_the_same(time_format, "MMDD.HHMM") || words_are_the_same(time_format, "MMDD-HHMM") || words_are_the_same(time_format, "MM-DD.HH-MM") ||
             words_are_the_same(time_format, "MM-DD-HH-MM") || words_are_the_same(time_format, "MMDDHHMM"))
        id_time_format = TIME_FORMAT_MMDDHHMM;
    else if (words_are_the_same(time_format, "DD.HHMM") || words_are_the_same(time_format, "DD-HHMM") || words_are_the_same(time_format, "DD.HH-MM") ||
             words_are_the_same(time_format, "DD-HH-MM") || words_are_the_same(time_format, "DDHHMM"))
        id_time_format = TIME_FORMAT_DDHHMM;
    else if (words_are_the_same(time_format, "YYYYMMDD.HH") || words_are_the_same(time_format, "YYYY-MM-DD.HH") ||
             words_are_the_same(time_format, "YYYYMMDD.HH") || words_are_the_same(time_format, "YYYY-MM-DD.HH") ||
             words_are_the_same(time_format, "YYYY-MM-DD-HH") || words_are_the_same(time_format, "YYYYMMDDHH"))
        id_time_format = TIME_FORMAT_YYYYMMDDHH;
    else if (words_are_the_same(time_format, "HH"))
        id_time_format = TIME_FORMAT_HH;
    else if (words_are_the_same(time_format, "MMDD.HH") || words_are_the_same(time_format, "MMDD-HH") || words_are_the_same(time_format, "MM-DD.HH") ||
             words_are_the_same(time_format, "MM-DD-HH") || words_are_the_same(time_format, "MMDDHH"))
        id_time_format = TIME_FORMAT_MMDDHH;
    else if (words_are_the_same(time_format, "DD.HH") || words_are_the_same(time_format, "DD-HH") || words_are_the_same(time_format, "DDHH"))
        id_time_format = TIME_FORMAT_DDHH;
    else EXECUTION_REPORT(REPORT_ERROR, -1, false, "The time format \"%s\" for \"%s\" is incorrect", time_format, report_hint);//测一下

    return id_time_format;
}

/*Datamodel_instance_info::Datamodel_instance_info() {
	//initialization
	is_coupling_connection_file_read = false;
	is_datamodel_needed = false;
	host_comp_id = -1;
	id_period_time_format = -1;
	period_unit = -1;
	period_count = -1;
	offset_unit = -1;
	offset_count = -1;
}

void Datamodel_instance_info::check_is_input_datamodel_needed(int host_comp_id, const char *import_comp_name) {
	int current_proc_local_id;
	char XML_file_name[NAME_STR_SIZE];
	std::vector<char*> needed_datamodel_instance_name;

	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, MPI_Comm_rank(comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id, "in datamodel_instance::check_is_input_datamodel_needed"), &current_proc_local_id) == MPI_SUCCESS);
	int line_number;
	Coupling_connection *dm_coupling_connection;

	if (is_coupling_connection_file_read == true)
		return;

	sprintf(XML_file_name,"%s/all/coupling_connections/%s.coupling_connections.xml", comp_comm_group_mgt_mgr->get_config_root_dir(), import_comp_name);

	std::vector<Inout_interface*> import_interfaces_of_this_component;
	inout_interface_mgr->get_all_import_interfaces_of_a_component(import_interfaces_of_this_component, import_comp_name);

	Component_import_interfaces_configuration *this_comp_import_interfaces_config = new Component_import_interfaces_configuration(host_comp_id, import_comp_name, inout_interface_mgr, false);
	is_coupling_connection_file_read=true;
	std::vector<char*> vect1;
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
				datamodel_interface_str[l]='\0';
				strncpy(this->datamodel_instance_name,conn_dm_interface_name+9,strlen(conn_dm_interface_name)-9);
				this->datamodel_instance_name[strlen(conn_dm_interface_name)-9] = '\0';
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
				vect1.push_back(strdup(dm_coupling_connection->src_comp_interfaces[0].second));
			}
		}
	}

	for (int m = 0; m < vect1.size(); m++) {//unique
		if (m == 0) { needed_datamodel_instance_name.push_back(vect1[m]); continue;}
		bool found_the_same=false;
		for (int n = 0; n < needed_datamodel_instance_name.size(); n++) {
			if (words_are_the_same(vect1[m],needed_datamodel_instance_name[n])) {
				found_the_same = true;
			}
		}
		if (!found_the_same) needed_datamodel_instance_name.push_back(vect1[m]);
	}

	if (needed_datamodel_instance_name.size() != 0) {
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
			const char *datamodel_instance_name_str = get_XML_attribute(host_comp_id, 80, datamodel_instance_element, "name", XML_file_name, line_number, "the \"name\" of a datamodel instance", "Datamodel instance configuration file", true);

			if (words_are_the_same(datamodel_instance_name_str,needed_datamodel_instance_name[i]) && is_XML_setting_on(host_comp_id, datamodel_instance_element, XML_file_name, "the \"status\" of the Datamodel instance for a component model", "Datamodel instance configuration file")) {
				datamodel_instance_found = true;
				check_and_verify_name_format_of_string_for_XML(host_comp_id, datamodel_instance_name_str, "datamodel instance", XML_file_name, line_number);
				read_datamodel_instance_configuration(host_comp_id, host_comp_full_name, XML_file_name, datamodel_instance_name_str, datamodel_instance_element_node);
				break;
			}
		}
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, datamodel_instance_found, "the datamodel instance \"%s\" needed for component model \"%s\" has not been specified in node \"datamodel_instances\" in configuration file \"%s\", Please Verify.",needed_datamodel_instance_name[i],host_comp_full_name,XML_file_name);
	}
}

void Datamodel_instance_info::read_datamodel_instance_configuration(int host_comp_id, const char *host_comp_full_name, char *XML_file_name, const char *datamodel_instance_name, TiXmlNode *datamodel_instance_element_node) {
	this->host_comp_id = host_comp_id;
	int line_number;
	TiXmlNode *datamodel_name_element_node = datamodel_instance_element_node->FirstChild();
	TiXmlElement *datamodel_name_element = datamodel_name_element_node->ToElement();
	strcpy(this->datamodel_name,get_XML_attribute(host_comp_id, 80, datamodel_name_element, "name", XML_file_name, line_number, "the datamodel corresponding to this datamodel instance", "datamodel instance configuration file", true));
	TiXmlNode *time_mappings_element_node = datamodel_name_element_node->NextSibling();
	TiXmlElement *time_mappings_element = time_mappings_element_node->ToElement();
	for (TiXmlNode *time_mapping_element_node = time_mappings_element_node->FirstChild(); time_mapping_element_node!=NULL;time_mapping_element_node=time_mapping_element_node->NextSibling()) {
		if (time_mapping_element_node->Type() != TiXmlNode::TINYXML_ELEMENT)
			continue;
		TiXmlElement *time_mapping_element = time_mapping_element_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(time_mapping_element->Value(), "time_mapping_configuration"), "When setting the datamodel instance configuration of the component \"%s\" in the XML file \"%s\", the XML element for specifying the datamodel instance configuration should be named \"time_mapping_configuration\". Pleas verify the XML file arround the line number %d.", host_comp_full_name, XML_file_name, time_mapping_element->Row());
		if (!is_XML_setting_on(host_comp_id, time_mapping_element, XML_file_name,"the status of some time mapping configurations for a datamodel_instance", "coupling_connection file"))
			continue;
		const char *specification = get_XML_attribute(host_comp_id, 80, time_mapping_element, "specification", XML_file_name, line_number, "the way of \"specification\" of a datamodel instance", "datamodel instance configuration file", true);
		bool is_period_set = false;
		bool is_offset_set = false;
		if (words_are_the_same(specification,"period")) {
			for (TiXmlNode *period_setting_element_node = time_mapping_element->FirstChild(); period_setting_element_node != NULL; period_setting_element_node = period_setting_element_node->NextSibling()) {
				TiXmlElement *period_setting_element = period_setting_element_node->ToElement();
				if (!is_XML_setting_on(host_comp_id, period_setting_element, XML_file_name, "the status of period setting configuration for a datamodel_instance","datamodel_instance configuration file"))
					continue;
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,!is_period_set,"the XML element for specifying the period_setting information of a Datamodel instance in file %s has already been set,Please verify the wanted period_setting version around line number %d",XML_file_name,period_setting_element->Row());
				is_period_set = true;
				const char *period_unit_config = get_XML_attribute(host_comp_id, 80, period_setting_element, "period_unit", XML_file_name, line_number, "period unit of a datamodel instance", "datamodel instance configuration file", true);
				const char *period_start_time_config = get_XML_attribute(host_comp_id, 80, period_setting_element, "period_start_time", XML_file_name, line_number, "period start time of a datamodel instance", "datamodel instance configuration file", true);
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,varname_or_value(period_start_time_config),"Error happens when reading the coupling_connection configuration file, the \"period_start_time\" of datamodel_instance %s should be an integer value, Please verify file %s arround line %d.", datamodel_instance_name, XML_file_name, period_setting_element->Row());
				const char *period_time_format = get_XML_attribute(host_comp_id, 80, period_setting_element, "period_time_format", XML_file_name, line_number, "period format of a datamodel instance", "datamodel instance configuration file", true);
				printf("period_time_format:%s\n",period_time_format);
				const char *period_count_config = get_XML_attribute(host_comp_id, 80, period_setting_element, "period_count", XML_file_name, line_number, "period count of a datamodel instance", "datamodel instance configuration file", true);
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,varname_or_value(period_count_config),"Error happens when reading the coupling connection configuration file, the \"period_count\" of datamodel_instance %s should be an integer value, Please verify file %s arround line %d.", datamodel_instance_name, XML_file_name, period_setting_element->Row());
				this->period_unit = set_unit(period_unit_config);
				strcpy(this->period_start_time,period_start_time_config);
				this->id_period_time_format=check_time_format(period_time_format,"period_time_format");
				strcpy(this->period_time_format,period_time_format);
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,check_time_format(period_time_format,"check_period_time_format"),"The period_time_format for datamodel_instance \"%s\" is not legal, Please check the xml configuration file %s arround line number %d.", datamodel_instance_name, XML_file_name, period_setting_element->Row());
				const char *report_hint = "period_time_format";
				this->period_count = atoi(period_count_config);
				//check if period_unit and period_time_format match with each other
                if (this->period_unit == 1) {
                    EXECUTION_REPORT(REPORT_ERROR,host_comp_id, (this->id_period_time_format==TIME_FORMAT_YYYY) || (this->id_period_time_format==TIME_FORMAT_YYYYMM) || (this->id_period_time_format==TIME_FORMAT_YYYYMMDD) || (this->id_period_time_format==TIME_FORMAT_YYYYMMDDHHMM) || (this->id_period_time_format==TIME_FORMAT_YYYYMMDDHHMMSS) || (this->id_period_time_format==TIME_FORMAT_YYYYMMDDSSSSS) || (this->id_period_time_format==TIME_FORMAT_YYYYMMDDHH),"The \"period_start_time/format\" do not match the \"period_unit/count\" specified for datamodel_instance \"%s\", Please check the xml configuration file %s arround line number %d", datamodel_instance_name, XML_file_name, period_setting_element->Row());
                }
                else if (this->period_unit == 2)
                    EXECUTION_REPORT(REPORT_ERROR,host_comp_id,(this->id_period_time_format==TIME_FORMAT_YYYYMM) || (this->id_period_time_format==TIME_FORMAT_MMDDSSSSS) || (this->id_period_time_format==TIME_FORMAT_MMDDHHMMSS) || (this->id_period_time_format==TIME_FORMAT_MMDDHHMM) || (this->id_period_time_format==TIME_FORMAT_YYYYMMDD) || (this->id_period_time_format==TIME_FORMAT_MMDDHH),"The \"period_start_time/format\" do not match the \"period_unit/count\" specified for datamodel_instance \"%s\", Please check the xml configuration file %s arround line number %d", datamodel_instance_name, XML_file_name, period_setting_element->Row());
                else if (this->period_unit == 3)
                    EXECUTION_REPORT(REPORT_ERROR,host_comp_id,(this->id_period_time_format==TIME_FORMAT_YYYYMMDDHHMM) || (this->id_period_time_format==TIME_FORMAT_YYYYMMDDHHMMSS) ||(this->id_period_time_format==TIME_FORMAT_YYYYMMDDSSSSS) || (this->id_period_time_format==TIME_FORMAT_MMDDSSSSS) || (this->id_period_time_format==TIME_FORMAT_MMDDHHMMSS) || (this->id_period_time_format==TIME_FORMAT_DDSSSSS) || (this->id_period_time_format==TIME_FORMAT_DDHHMMSS) || (this->id_period_time_format==TIME_FORMAT_MMDDHHMM) || (this->id_period_time_format==TIME_FORMAT_DDHHMM) || (this->id_period_time_format==TIME_FORMAT_YYYYMMDD) || (this->id_period_time_format==TIME_FORMAT_MMDDHH) || (this->id_period_time_format==TIME_FORMAT_DDHH),"The \"period_start_time/format\" do not match the \"period_unit/count\" specified for datamodel_instance \"%s\", Please check the xml configuration file %s arround line number %d", datamodel_instance_name, XML_file_name, period_setting_element->Row());
                else if ((this->id_period_time_format==TIME_FORMAT_YYYYMMDDHHMM) || (this->id_period_time_format==TIME_FORMAT_MMDDHHMM) || (this->id_period_time_format==TIME_FORMAT_DDHHMM) || (this->id_period_time_format==TIME_FORMAT_HHMM))
                    EXECUTION_REPORT(REPORT_ERROR,host_comp_id,this->period_count%60 == 0,"The \"period_start_time/format\" do not match the \"period_unit/count\" specified for datamodel_instance \"%s\", Please check the xml configuration file %s arround line number %d", datamodel_instance_name, XML_file_name, period_setting_element->Row());
                else if ((this->id_period_time_format==TIME_FORMAT_YYYYMMDDHH) || (this->id_period_time_format==TIME_FORMAT_MMDDHH) || (this->id_period_time_format==TIME_FORMAT_DDHH) || (this->id_period_time_format==TIME_FORMAT_HH))
                    EXECUTION_REPORT(REPORT_ERROR,host_comp_id,this->period_count%3600 == 0,"The \"period_start_time/format\" do not match the \"period_unit/count\" specified for datamodel_instance \"%s\", Please check the xml configuration file %s arround line number %d", datamodel_instance_name, XML_file_name, period_setting_element->Row());
			}
		}
		else if (words_are_the_same(specification,"offset")) {
			for (TiXmlNode *offset_setting_element_node = time_mapping_element->FirstChild(); offset_setting_element_node != NULL; offset_setting_element_node = offset_setting_element_node->NextSibling()) {
				TiXmlElement *offset_setting_element = offset_setting_element_node->ToElement();
				if (!is_XML_setting_on(host_comp_id, offset_setting_element, XML_file_name, "the status of offset setting configuration for a datamodel instance", "datamodel instance configuration file"))
					continue;
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,!is_offset_set,"the XML element for specifying the offset_setting information of a Datamodel_instance in file %s has already been set,Please verify around line number %d",XML_file_name,offset_setting_element->Row());
				is_offset_set = true;
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id, words_are_the_same(offset_setting_element->Value(),"offset_setting"),"The XML element for specifying the offset_setting information of a Datamodel instance in the XML configuration file \"%s\" should be named \"offset_setting\". Please verify datamodel instance configuration file around the line number %d.", XML_file_name, offset_setting_element->Row());
				const char *offset_unit_config = get_XML_attribute(host_comp_id, 80, offset_setting_element, "offset_unit", XML_file_name, line_number, "offset unit of a datamodel instance", "datamodel instance configuration file", true);
				this->offset_unit=set_unit(offset_unit_config);
				const char *offset_count_config = get_XML_attribute(host_comp_id, 80, offset_setting_element, "offset_count", XML_file_name, line_number, "offset count of a datamodel instance", "offset instance configuration file", true);
				this->offset_count=atoi(offset_count_config);
			}
		}
		else if (!words_are_the_same(specification, "default"))
			EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false,"The specification for time mapping configuration of component \"%s\" in the XML file \"%s\" should only be \"default\", \"period\" or \"offset\". Please verify the XML file arround the line number %d.", host_comp_full_name, XML_file_name, time_mapping_element->Row());
	}
}*/

int Datamodel_mgt::register_datamodel_output_handler(const char *handler_name, int num_fields, int *field_ids, const char *output_datamodel_name, int implicit_or_explicit, int sampling_timer_id, int field_instance_ids_size, const char *annotation) {
	int API_id = API_ID_HANDLER_DATAMODEL_OUTPUT, i;
	Inout_datamodel *output_datamodel;
	common_checking_for_datamodel_handler_registration(num_fields, field_ids, implicit_or_explicit, sampling_timer_id, field_instance_ids_size, annotation);
	int host_comp_id = memory_manager->get_field_instance(field_ids[0])->get_comp_id();
	for (i=0; i < output_datamodels.size(); i++) {
		if (words_are_the_same(output_datamodel_name, output_datamodels[i]->get_datamodel_name()) && host_comp_id == output_datamodels[i]->get_host_comp_id()) {
			output_datamodel = output_datamodels[i];
			EXECUTION_REPORT_LOG(REPORT_LOG, host_comp_id, true, "The output_datamodel \"%s\" of which output data dir is %s, needed for output_datamodel_hander has already been configured, and will be used as already configured.", output_datamodel_name, output_datamodel->get_datamodel_data_dir());
			break;
		}
	}
	if (i == datamodel_mgr->output_datamodels.size()) 
		output_datamodel = new Inout_datamodel(host_comp_id, output_datamodel_name, OUTPUT_DATAMODEL, annotation);	
	output_datamodels.push_back(output_datamodel);
	Output_handler *new_output_handler = new Output_handler(handler_name, output_datamodel_name, get_next_handler_id(), num_fields, field_ids, implicit_or_explicit, sampling_timer_id, field_instance_ids_size, annotation);
	output_handlers.push_back(new_output_handler);
	return new_output_handler->get_handler_id();
}

Output_handler::Output_handler(const char *output_handler_name, const char *datamodel_name_str, int handler_id, int num_fields, int *export_field_ids,int implicit_or_explicit, int sampling_timer_id, int field_instance_ids_size, const char *annotation) {
	this->handler_name = strdup(output_handler_name);
	this->sampling_timer_id = sampling_timer_id;
	this->handler_id = handler_id;
	this->num_fields = num_fields;
	this->sampling_timer_id = sampling_timer_id;
	this->datamodel_name =strdup(datamodel_name_str);
	this->implicit_or_explicit = implicit_or_explicit;
	this->annotation = strdup(annotation);

	this->host_comp_id = memory_manager->get_field_instance(export_field_ids[0])->get_comp_id();
	this->output_datamodel = datamodel_mgr->search_output_datamodel(this->datamodel_name);
	//register field instances
	//*field_instance_id = memory_manager->register_external_field_instance(field_name, (void*)(*data_buffer_ptr), *field_size, *decomp_id, *comp_or_grid_id, *buf_mark, *usage_tag, unit, data_type, annotation);
	std::vector<int> import_field_instance_ids;
	for (int i = 0; i < output_datamodel->fields_config_info.size(); i++) {
		if (words_are_the_same(output_datamodel->field_specifications[i], "fields")) {
			for (int j = 0; j < output_datamodel->fields_config_info[i].size(); j++) {
				void *data_buffer_ptr;
				Field_config_info *dst_field_config_info = output_datamodel->fields_config_info[i][j];
				Field_mem_info *source_field_instance = find_handler_field_instance_id(dst_field_config_info->name_in_model);
				if (dst_field_config_info->grid_name != NULL) {
					original_grid_mgr->search_grid_info(grid_name, host_comp_id)->
				}
				if (source_field_instance->get_data_type() == DATA_TYPE_DOUBLE || source_field_instance->get_data_type() == DATA_TYPE_FLOAT) {
					if (words_are_the_same(dst_field_config_info->float_datatype, "double")) {
						double data_buffer = new double [];
					}
				}
				int field_instance_id = memory_manager->register_external_field_instance(dst_field_config_info->name_in_file, , , -1, );
			}
		}
	}
}

Field_mem_info *Output_handler::find_handler_field_instance_id(char *field_name, int *export_field_ids, int num_fields) {
	for (int i = 0; i < num_fields; i++) {
		Field_mem_info *handler_filed_info = memory_manager->get_field_instance(export_field_ids[i]);
		if (words_are_the_same(handler_filed_info->get_field_name(), field_name)) {
			return handler_filed_info;
		}
	}
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, !report_error, "Error happens when registering Output_handler \"%s\", field_instance named \"%s\" (for datamodel \"%s\") has not been registered, Please verify xml configuration file %s.", handler_name, field_name, datamodel_name, output_datamodel->get_XML_file_name());
	return NULL;
}

Inout_datamodel *Datamodel_mgt::search_output_datamodel(char *datamodel_name) {
	for (int i = 0; i < output_datamodel.size(); i++) {
		if (words_are_the_same(output_datamodel[i], datamodel_name))
			return output_datamodel[i];
		break;
	}
}

void Datamodel_mgt::common_checking_for_datamodel_handler_registration(int num_fields, int *field_ids, int implicit_or_explicit, int sampling_timer_id, int field_instance_ids_size, const char *annotation) {
	int comp_id = -1;
	char str[NAME_STR_SIZE], API_label[256];

	get_API_hint(-1, API_ID_HANDLER_DATAMODEL_OUTPUT, API_label);
	EXECUTION_REPORT(REPORT_ERROR, -1, num_fields > 0, "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\", the parameter \"num_field_instances\" (currently is %d) cannot be smaller than 1. Please verify the model code.", API_label, annotation, num_fields);
	EXECUTION_REPORT(REPORT_ERROR, -1, num_fields <= field_instance_ids_size, "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the array size (currently is %d) of parameter \"%s\" cannot be smaller than the parameter \"num_field_instances\" (currently is %d). Please verify the corresponding model code.", API_label, annotation, field_instance_ids_size, "field_instance_ids", num_fields);
	for (int i = 0; i < num_fields; i ++) {
		EXECUTION_REPORT(REPORT_ERROR, -1, memory_manager->check_is_legal_field_instance_id(field_ids[i]) && memory_manager->get_field_instance(field_ids[i])->get_is_registered_model_buf(), "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the parameter \"%s\" contains wrong field instance id (the %dth element of the array is wrong). Please verify the corresponding model code.", API_label, annotation, "field_instance_ids", i+1);
		if (i == 0)
			comp_id = memory_manager->get_field_instance(field_ids[i])->get_comp_id();
		EXECUTION_REPORT_ERROR_OPTIONALLY(REPORT_ERROR, comp_id, comp_id == memory_manager->get_field_instance(field_ids[i])->get_comp_id(), "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the field instances specified via the parameter \"%s\" should but not correspond to the same component model crrently: the first field instance corresponds to the component model \"%s\" while the %dth field instance corresponds to the component model \"%s\". Please verify the model code with the annotation \"%s\".", API_label, annotation, "field_instance_ids", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, false, "")->get_comp_full_name(), i+1, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(memory_manager->get_field_instance(field_ids[i])->get_comp_id(), false, "")->get_comp_full_name());
	}
	sprintf(str, "registering a datamodel handler with the annotation \"%s\"", annotation);
	synchronize_comp_processes_for_API(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), str, annotation);
	int temp_int = (sampling_timer_id == -1)? 0 : 1;
	check_API_parameter_int(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), NULL, temp_int, "sampling_timer_id", annotation);
	if (sampling_timer_id != -1) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, timer_mgr->check_is_legal_timer_id(sampling_timer_id), "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the parameter \"sampling_timer_id\" (currently is 0x%x) is not the legal id of a timer. Please verify the corresponding model code.", API_label, annotation, sampling_timer_id);
		check_API_parameter_timer(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), "registering a datamodel handler", sampling_timer_id, "sampling_timer_id (the information of the timer)", annotation);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_id == timer_mgr->get_timer(sampling_timer_id)->get_comp_id(), "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the parameter \"sampling_timer_id\" and the parameter \"%s\" do not correspond to the same component model (the parameter \"sampling_timer_id\" corresponds to the component model \"%s\" while \"%s\" corresponds to the component model \"%s\"). Please verify.", API_label, annotation, "field_instance_ids", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(timer_mgr->get_timer(sampling_timer_id)->get_comp_id(), false, "")->get_comp_full_name(), "field_instance_ids", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, false, "")->get_comp_full_name());//timer和field_instance必须对应相同的comp_id
	}
	check_API_parameter_int(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), NULL, num_fields, "num_field_instances", annotation);
	sprintf(str, "\"%s\" (the information of the field instances)", "field_instance_ids");
	for (int i = 0; i < num_fields; i++)
		check_API_parameter_field_instance(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), "registering a datamodel handler", field_ids[i], str, annotation);
	check_API_parameter_int(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), "registering a datamodel handler", implicit_or_explicit, "implicit_or_explicit (the tag of executing the handler implicitly or explicitly)", annotation);
}

Inout_datamodel::Inout_datamodel(int host_comp_id, const char *output_datamodel_name, bool datamodel_type_label, const char *annotation) {
	int line_number, num_data_files_node = 0, num_hg_grids_node = 0, num_vg_grids_node = 0, num_vd_grids_node = 0, num_out_freq_node = 0, num_nodes = 0;
	TiXmlNode *sub_node = NULL;
	this->host_comp_id = host_comp_id;
	this->datamodel_type = datamodel_type_label;
	this->datamodel_name = strdup(output_datamodel_name);
	this->annotation = strdup(annotation);
	sprintf(this->datamodel_config_dir, "%s/CCPL_dir/datamodel/config",comp_comm_group_mgt_mgr->get_root_working_dir());
	sprintf(this->datamodel_file_name,"output_datamodel_%s.xml",output_datamodel_name);
	sprintf(this->XML_file_name,"%s/%s", datamodel_config_dir, datamodel_file_name);
	sprintf(this->datamodel_data_dir, "%s/CCPL_dir/datamodel/data",comp_comm_group_mgt_mgr->get_root_working_dir());
	MPI_Comm comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id, "in register_datamodel_output_handler H2D_grid");

	TiXmlDocument *XML_file = open_XML_file_to_read(host_comp_id, XML_file_name, comm, false);
	if (XML_file == NULL) {
		EXECUTION_REPORT(REPORT_ERROR, -1, false, "Can't find the Datamodel configuration file named %s, Please check directory %s.", datamodel_file_name, datamodel_config_dir);
		return;
	}

	TiXmlNode *root_node = get_XML_first_child_of_unique_root(host_comp_id, XML_file_name, XML_file);
	TiXmlElement *root_element = XML_file->FirstChildElement();
	for (; root_node != NULL; root_node = root_node->NextSibling()) {
		if (root_node->Type() != TiXmlNode::TINYXML_ELEMENT)
			continue;
		root_element = root_node->ToElement();
		if (words_are_the_same(root_element->Value(), "output_datamodel"))
			break;
	}

	TiXmlNode *output_datamodel_node = root_node;
	TiXmlElement *output_datamodel_element = root_element;
	//check output_datamodel status
	const char *name_str = get_XML_attribute(host_comp_id, 80, output_datamodel_element, "name", XML_file_name, line_number, "The \"name\" of the output_datamodel","output datamodel xml file",true);
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(name_str, output_datamodel_name),"The name of the Output_datamodel should be %s, Please verify.",output_datamodel_name);
	if (!at_most_one_node_of("data_files", output_datamodel_node, sub_node, num_nodes))
		EXECUTION_REPORT(REPORT_ERROR, -1, num_nodes == 1, "Error happens when reading the xml configuration file \"%s\" for datamodel \"%s\", there should be one and only one node named \"data_files\" under node \"output_datamodel\", %d are detected, Please check.", XML_file_name, datamodel_name, num_nodes);
	if (sub_node != NULL) config_data_files_for_datamodel(sub_node);
	EXECUTION_REPORT(REPORT_ERROR, -1, at_most_one_node_of("horizontal_grids", output_datamodel_node, sub_node, num_nodes), "Error happens when reading the xml configuration file \"%s\" for datamodel \"%s\", %d nodes named \"horizontal_grids\" are found under node \"output_datamodel\", Please check.", XML_file_name, datamodel_name, num_nodes);
	if (sub_node != NULL) config_horizontal_grids_for_datamodel(sub_node);
	EXECUTION_REPORT(REPORT_ERROR, -1, at_most_one_node_of("vertical_grids", output_datamodel_node, sub_node, num_nodes), "Error happens when reading the xml configuration file \"%s\" for datamodel \"%s\", %d nodes named \"vertical_grids\" are found under node \"output_datamodel\", Please check.", XML_file_name, datamodel_name, num_nodes);
	if (sub_node != NULL) config_vertical_grids_for_datamodel(sub_node);
	EXECUTION_REPORT(REPORT_ERROR, -1, at_most_one_node_of("V3D_grids", output_datamodel_node, sub_node, num_nodes), "Error happens when reading the xml configuration file \"%s\" for datamodel \"%s\", %d nodes named \"V3D_grids\" are found under node \"output_datamodel\", Please check.", XML_file_name, datamodel_name, num_nodes);
	if (sub_node != NULL) config_v3d_grids_for_datamodel(sub_node);
	EXECUTION_REPORT(REPORT_ERROR, -1, at_most_one_node_of("fields_output_settings", output_datamodel_node, sub_node, num_nodes), "Error happens when reading the xml configuration file \"%s\" for datamodel \"%s\", %d nodes named \"fields_output_settings\" are found under node \"output_datamodel\", Please check.", XML_file_name, datamodel_name, num_nodes);
	if (sub_node != NULL) config_field_output_settings_for_datamodel(sub_node);
}

bool at_most_one_node_of(const char *node_name, TiXmlNode *outer_node, TiXmlNode *&inner_node, int &num_nodes) {
	int num_node = 0;
	bool return_val;
	TiXmlNode *sub_node = NULL;
	for (sub_node = outer_node->FirstChild(); sub_node != NULL; sub_node = sub_node->NextSibling()) {
		if (words_are_the_same(sub_node->ToElement()->Value(), node_name)) {
			inner_node = sub_node;
			num_node ++;
		}
	}
	num_nodes = num_node;
	return_val = (num_node == 1 || num_node == 0)? true: false;
	return return_val;
}

void Inout_datamodel::config_data_files_for_datamodel(TiXmlNode *data_files_node) {
	TiXmlNode *data_file_node;
	int line_number,pos_last_star,i, num_stars=0;
	bool is_data_file_configured = false;

	for (data_file_node = data_files_node->FirstChild(); data_file_node != NULL; data_file_node = data_file_node->NextSibling()) {
		TiXmlElement *data_file_element = data_file_node->ToElement();
		if (is_XML_setting_on(host_comp_id, data_file_element, XML_file_name,"the status of a data_file node of an output_datamodel", "output_datamodel xml file"))
			EXECUTION_REPORT(REPORT_ERROR, host_comp_id, !is_data_file_configured, "Error happens in output_datamodel configuration file\"%s\" for datamodel \"%s\", only one \"status\" of the \"data_file\" node can be set as \"on\", Please Verify the xml file arround line_number %d.", XML_file_name, datamodel_name, data_file_element->Row());
		else continue;
		is_data_file_configured = true;
		const char *file_names_str = get_XML_attribute(host_comp_id, 80, data_file_element, "file_names", XML_file_name, line_number, "the \"file_names\" of an output_datamodel", "output_datamodel configuration file", true);
		if (file_names_str[0] != '/') {
			datamodel_files_dir_name = new char [strlen(datamodel_data_dir) + strlen(file_names_str) + 1];
			sprintf(datamodel_files_dir_name, "%s/%s", datamodel_data_dir, file_names_str);
		}
		else
			datamodel_files_dir_name = strdup(file_names_str);
		for (i = strlen(datamodel_files_dir_name)-1; i != 0; i--) {
			if (datamodel_files_dir_name[i] == '/')
				break;
		}
		file_dir = new char [i+1];
		strncpy(file_dir, datamodel_files_dir_name, i+1);
		file_dir[i+1]='\0';
		char *file_name = strdup(datamodel_files_dir_name+i+1);
		EXECUTION_REPORT(REPORT_ERROR, -1, opendir(file_dir) != NULL, "Error happens when reading the xml configuration file of datamodel \"%s\", the data file directory specified as \"%s\" cannot be found, Please check the xml configuration file \"%s\" arround line number %d", datamodel_name, file_dir, XML_file_name, data_file_element->Row());
		for (i = 0; i < strlen(file_name); i++) {
			if (file_name[i] == '*') {
				num_stars ++;
				pos_last_star = i;
			}
		}
		file_name_prefix = new char [pos_last_star];
		strncpy(file_name_prefix, file_name, pos_last_star);
		file_name_prefix[pos_last_star] = '\0';
		file_name_suffix =strdup(file_name+pos_last_star+1);
        const char *time_format_in_file_names = get_XML_attribute(host_comp_id, 80, data_file_element, "time_format_in_filename", XML_file_name, line_number, "the \"time_format\" in the file name of a datamodel", "datamodel configuration file", true);
        this->id_time_format_in_file_names = check_time_format(time_format_in_file_names, "time_format_in_file_name");
        EXECUTION_REPORT(REPORT_ERROR,host_comp_id,num_stars == 1,"Error happens when reading the XML configuration file %s arround line number %d, only one * can be specified in data_file names, there are %d detected, Pleas Verify.", XML_file_name, data_file_element->Row(), num_stars);//if there are no * s in filename, do not read time_format_in_filename
        char *file_type_str = strdup(get_XML_attribute(host_comp_id, 80, data_file_element, "file_type", XML_file_name, line_number, "the \"file_type\" of an output_datamodel", "datamodel configuration file", true));
        if (words_are_similar(file_type_str, "netcdf"))
        	this->file_type = strdup("netcdf");
	}
}

void Inout_datamodel::config_horizontal_grids_for_datamodel(TiXmlNode *hgs_node) {
	int line_number;
	for (TiXmlNode *hg_node = hgs_node->FirstChild(); hg_node != NULL; hg_node = hg_node->NextSibling()) {
		TiXmlElement *hg_element = hg_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, hg_element, XML_file_name, "the status of a \"horizontal_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, hg_element, "grid_name", XML_file_name, line_number, "the \"grid_name\" of an output_datamodel horizontal_grid","datamodel configuration file",true);
		char *grid_name_str2 = new char [strlen(grid_name_str) + 16];
		sprintf(grid_name_str2, "datamodel_%s", grid_name_str);
		const char *specification_str = get_XML_attribute(host_comp_id, 80, hg_element, "specification", XML_file_name, line_number, "the \"specification\" of an output_datamodel horizontal_grid","datamodel configuration file",true);
		TiXmlNode *entry_node = hg_node->FirstChild();
		TiXmlElement *entry_element = entry_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(entry_element->Value(), "entry"), "Error happens when reading the xml configuration file for datamodel \"%s\", the first child node of a \"horizontal_grid\" node should be named \"entry\", Please check file %s.", datamodel_name, XML_file_name);
		if (words_are_the_same(specification_str, "CCPL_grid_file"))
			config_horizontal_grid_via_CCPL_grid_file(entry_node, grid_name_str, grid_name_str2);
		else if (words_are_the_same(specification_str, "grid_data_file_field"))
			config_horizontal_grid_via_grid_data_file_field(entry_node, grid_name_str, grid_name_str2);
		else if (words_are_the_same(specification_str, "uniform_lonlat_grid"))
			config_horizontal_grid_via_uniform_lonlat_grid(entry_node, grid_name_str, grid_name_str2);
		else EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "The \"specification\" of a horizontal_grid in a datamodel configuration file (as specified as \"%s\") can only be one of \"CCPL_grid_file\", \"grid_data_file_field\", or \"uniform_lonlat_grid\", Please Verify the xml configuration file \"%s\".", specification_str, XML_file_name);
		delete [] grid_name_str2;
	}
}

void Inout_datamodel::config_horizontal_grid_via_CCPL_grid_file(TiXmlNode *grid_file_entry_node, const char *ori_grid_name, const char *grid_name) {
	int line_number, grid_id;
	char string_annotation[NAME_STR_SIZE];
	const char *CCPL_grid_file_name_str = get_XML_attribute(host_comp_id, 80, grid_file_entry_node->ToElement(), "file_name", XML_file_name, line_number, "the \"file_name\" of an CCPL_grid_file","datamodel configuration file", true);
	char file_name[strlen(datamodel_data_dir) + strlen(CCPL_grid_file_name_str) +1];
	sprintf(file_name, "%s/%s", datamodel_data_dir, CCPL_grid_file_name_str);
	sprintf(string_annotation, "register H2D grid \"%s\" for datamodel \"%s\" via CCPL_grid_file at line %d in xml configuration file \"%s\"", ori_grid_name, datamodel_name, grid_file_entry_node->ToElement()->Row(), XML_file_name);
	grid_id = original_grid_mgr->register_H2D_grid_via_file(host_comp_id, grid_name, file_name, string_annotation);
	h2d_grid_ids.push_back(grid_id);
	all_grid_ids.push_back(grid_id);
	h2d_grid_names.push_back(strdup(grid_name));
}

void Inout_datamodel::config_horizontal_grid_via_grid_data_file_field(TiXmlNode *file_field_entry_node, const char *ori_grid_name, const char *grid_name) {
	int line_number;
	char file_name_str2[NAME_STR_SIZE], string_annotation[NAME_STR_SIZE];
	TiXmlElement *file_field_entry_element = file_field_entry_node->ToElement();
	const char *file_name_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "file_name", XML_file_name, line_number, "the \"file_name\" of a grid_data_file_field", "datamodel configuration file",true);//true for output
	const char *edge_type_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "edge_type", XML_file_name, line_number, "the \"edge_type\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *coord_unit_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *cyclic_or_acyclic_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "cyclic_or_acyclic", XML_file_name, line_number, "the \"cyclic_or_acyclic\" of a grid_data_file_field", "datamodel configuration file",true);
	const char *dim_size1_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "dim_size1", XML_file_name, line_number, "the \"dim_size1\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *dim_size2_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "dim_size2", XML_file_name, line_number, "the \"dim_size2\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *min_lon_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "min_lon", XML_file_name, line_number, "the \"min_lon\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *min_lat_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "min_lat", XML_file_name, line_number, "the \"min_lat\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *max_lon_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "max_lon", XML_file_name, line_number, "the \"max_lon\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *max_lat_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "max_lat", XML_file_name, line_number, "the \"max_lat\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *center_lon_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "center_lon", XML_file_name, line_number, "the \"center_lon\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *center_lat_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "center_lat", XML_file_name, line_number, "the \"center_lat\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *mask_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "mask", XML_file_name, line_number, "the \"mask\" of a grid_data_file_field", "datamodel configuration file", false);
	const char *area_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "area", XML_file_name, line_number, "the \"area\" of a grid_data_file_field", "datamodel configuration file", false);
	const char *vertex_lon_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "vertex_lon", XML_file_name, line_number, "the \"vertex_lon\" of a grid_data_file_field", "datamodel configuration file", false);
	const char *vertex_lat_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "vertex_lat", XML_file_name, line_number, "the \"vertex_lat\" of a grid_data_file_field", "datamodel configuration file", false);
	if (file_name_str[0] == '/')
		strcpy(file_name_str2, strdup(file_name_str));
	else sprintf(file_name_str2, "%s/%s", datamodel_data_dir, file_name_str);
	sprintf(string_annotation, "register H2D grid \"%s\" for datamodel \"%s\" via CCPL_grid_file at line %d in xml configuration file \"%s\"", ori_grid_name, datamodel_name, file_field_entry_element->Row(), XML_file_name);
	register_common_h2d_grid_for_datamodel(grid_name, file_name_str2, edge_type_str, coord_unit_str, cyclic_or_acyclic_str, dim_size1_str, dim_size2_str, min_lon_str, min_lat_str, max_lon_str, max_lat_str, center_lon_str, center_lat_str, mask_str, area_str, vertex_lon_str, vertex_lat_str, string_annotation);
}

void Inout_datamodel::register_common_h2d_grid_for_datamodel(const char *grid_name_str, const char *file_name_str, const char *edge_type_str, const char *coord_unit_str, const char *cyclic_or_acyclic_str, const char *dim_size1_str, const char *dim_size2_str, const char *min_lon_str, const char *min_lat_str, const char *max_lon_str, const char *max_lat_str, const char *center_lon_str, const char *center_lat_str, const char *mask_str, const char *area_str, const char *vertex_lon_str, const char *vertex_lat_str, const char *annotation) {
	int rcode, ncfile_id, grid_id;
	int size_center_lon=-1, size_center_lat=-1, size_mask=-1, size_area=-1, size_vertex_lon=-1, size_vertex_lat=-1;
	int *mask;
	long dim_lon_size, dim_lat_size, dim_H2D_size, dim_size1, dim_size2;
	char *center_lon, *center_lat, *vertex_lon, *vertex_lat, *area;
	//char min_lon[get_data_type_size(DATA_TYPE_DOUBLE)], min_lat[get_data_type_size(DATA_TYPE_DOUBLE)], max_lon[get_data_type_size(DATA_TYPE_DOUBLE)], max_lat[get_data_type_size(DATA_TYPE_DOUBLE)];
	char min_lon[NAME_STR_SIZE], max_lon[NAME_STR_SIZE], min_lat[NAME_STR_SIZE], max_lat[NAME_STR_SIZE];
	char data_type_for_center_lat[16], data_type_for_center_lon[16], data_type_for_vertex_lon[16], data_type_for_vertex_lat[16], data_type_for_mask[16], data_type_for_area[16];
	char data_type_temp[16];

	MPI_Comm comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id, "in register_datamodel_output_handler H2D_grid");
	bool is_root_proc = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(host_comp_id, "in register_datamodel_output_handler H2D_grid") == 0;
	check_API_parameter_string(host_comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comm, "registering an H2D grid for output_datamodel handler", file_name_str, "file_name", "registering an H2D grid for output_datamodel handler");

	char *edge_type = strdup(edge_type_str);
	char *cyclic_or_acyclic = strdup(cyclic_or_acyclic_str);
	char *coord_unit = strdup(coord_unit_str);
	IO_netcdf *netcdf_file_object = new IO_netcdf("datamodel_H2D_grid_data", file_name_str, "r", false);

	if (!varname_or_value(dim_size1_str))
		dim_size1 = netcdf_file_object->get_dimension_size(dim_size1_str, comm, is_root_proc);
	if (!varname_or_value(dim_size2_str))
		dim_size2 = netcdf_file_object->get_dimension_size(dim_size2_str, comm, is_root_proc);

	netcdf_file_object->read_file_field(center_lon_str, (void**)(&center_lon), &size_center_lon, data_type_for_center_lon, comm, is_root_proc);
	netcdf_file_object->read_file_field(center_lat_str, (void**)(&center_lat), &size_center_lat, data_type_for_center_lat, comm, is_root_proc);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, center_lon != NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the longitude value for the center of each grid point (variable \"%s\" in the file) is not specified in the grid file \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, center_lon_str, file_name_str, XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, center_lat != NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the latitude value for the center of each grid point (variable \"%s\" in the file) is not specified in the grid file \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, center_lat_str, file_name_str, XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, data_type_for_center_lat), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\", in the data file \"%s\", the data type of variables \"%s\" and \"%s\" are not the same, Please check the xml file %s.", grid_name_str, datamodel_name, file_name_str, center_lon_str, center_lat_str, XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, DATA_TYPE_FLOAT) || words_are_the_same(data_type_for_center_lon, DATA_TYPE_DOUBLE), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\", in the data file \"%s\", the data type of variables \"%s\" is not floating-point", grid_name_str, datamodel_name, file_name_str, center_lon_str, XML_file_name);

	if (vertex_lon_str != NULL)
		netcdf_file_object->read_file_field(vertex_lon_str, (void**)(&vertex_lon), &size_vertex_lon, data_type_for_vertex_lon, comm, is_root_proc);	
	else vertex_lon = NULL;
	if (vertex_lat_str != NULL)
		netcdf_file_object->read_file_field(vertex_lat_str, (void**)(&vertex_lat), &size_vertex_lat, data_type_for_vertex_lat, comm, is_root_proc);
	else vertex_lat = NULL;
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, (vertex_lon_str != NULL && vertex_lon != NULL) || vertex_lon_str == NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the variable \"%s\" for parameter \"vertex_lon\" cannot be found in grid file \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, vertex_lon_str, file_name_str, XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, (vertex_lat_str != NULL && vertex_lat != NULL) || vertex_lat_str == NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the variable \"%s\" for parameter \"vertex_lat\" cannot be found in grid file \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, vertex_lat_str, file_name_str, XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, (vertex_lon != NULL && vertex_lat != NULL) || (vertex_lon == NULL && vertex_lat == NULL), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the longitude and latitude values for each vertex (variables \"%s\" and \"%s\" in the file) of each grid point must be specified/unspecified at the same time, Please check the xml file %s.", grid_name_str, datamodel_name, file_name_str, vertex_lon_str, vertex_lat_str, XML_file_name);
	if (vertex_lon_str != NULL && vertex_lon != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, data_type_for_vertex_lon), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the data type of varaible \"%s\" is different from the data type of variable \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, file_name_str, center_lon_str, vertex_lon_str, XML_file_name);
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, data_type_for_vertex_lat),"Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the data type of varaible \"%s\" is different from the data type of variable \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, file_name_str, center_lon_str, vertex_lat_str, XML_file_name);
	}

	if (area_str != NULL)
		netcdf_file_object->read_file_field(area_str, (void**)(&area), &size_area, data_type_for_area, comm, is_root_proc);
	else area = NULL;
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, (area_str != NULL && area != NULL) || area_str == NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the variable \"%s\" for parameter \"area\" cannot be found in grid file \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, area_str, file_name_str, XML_file_name);
	if (area_str != NULL && area != NULL)
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, data_type_for_area), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the data type of varaible \"%s\" is different from the data type of variable \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, file_name_str, center_lon_str, vertex_lat_str, XML_file_name);

	if (mask_str != NULL)
		netcdf_file_object->read_file_field(mask_str, (void**)(&mask), &size_mask, data_type_for_mask, comm, is_root_proc);
	else mask = NULL;
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, (mask_str != NULL && mask != NULL) || mask_str == NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the variable \"%s\" for parameter \"mask\" cannot be found in grid file \"%s\", Please check the xml file %s.", grid_name_str, datamodel_name, mask_str, file_name_str, XML_file_name);
	if (mask_str != NULL && mask != NULL)
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_mask, DATA_TYPE_INT), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the data type of variable \"%s\" is not \"integer\", Please check the xml file %s.", grid_name_str, datamodel_name, file_name_str, mask_str, XML_file_name);

	double min_lon_value = atof(min_lon_str);
	double max_lon_value = atof(max_lon_str);
	double min_lat_value = atof(min_lat_str);
	double max_lat_value = atof(max_lat_str);
	memcpy(min_lon, &min_lon_value, sizeof(double));
	memcpy(max_lon, &max_lon_value, sizeof(double));
	memcpy(min_lat, &min_lat_value, sizeof(double));
	memcpy(max_lat, &max_lat_value, sizeof(double));
	transform_datatype_of_arrays(min_lon, min_lon, DATA_TYPE_DOUBLE, data_type_for_center_lon, 1);
	transform_datatype_of_arrays(min_lat, min_lat, DATA_TYPE_DOUBLE, data_type_for_center_lon, 1);
	transform_datatype_of_arrays(max_lon, max_lon, DATA_TYPE_DOUBLE, data_type_for_center_lon, 1);
	transform_datatype_of_arrays(max_lat, max_lat, DATA_TYPE_DOUBLE, data_type_for_center_lon, 1);
	EXECUTION_REPORT_LOG(REPORT_LOG, host_comp_id, true, "Starting to register a H2D grid \"%s\" for datamodel \"%s\".", grid_name_str, datamodel_name);
	grid_id = original_grid_mgr->register_H2D_grid_via_global_data(host_comp_id, grid_name_str, edge_type, coord_unit_str, cyclic_or_acyclic, data_type_for_center_lon, dim_size1, dim_size2, size_center_lon, size_center_lat, size_mask, size_area, size_vertex_lon, size_vertex_lat, min_lon, max_lon, min_lat, max_lat, center_lon, center_lat, mask, area, vertex_lon, vertex_lat, annotation, API_ID_HANDLER_DATAMODEL_OUTPUT);
	delete [] center_lon;
	delete [] center_lat;
	if (vertex_lon_str != NULL && vertex_lon != NULL) {
		delete [] vertex_lon;
		delete [] vertex_lat;
	}
	if (mask_str != NULL && mask != NULL)
		delete [] mask;
	if (area_str != NULL && area != NULL)
		delete [] area;
	delete netcdf_file_object;
	h2d_grid_ids.push_back(grid_id);
	all_grid_ids.push_back(grid_id);
	h2d_grid_names.push_back(strdup(grid_name_str));
}

void Inout_datamodel::config_horizontal_grid_via_uniform_lonlat_grid(TiXmlNode *uniform_lonlat_grid_entry_node, const char *ori_grid_name, const char *grid_name) {
	int line_number, grid_id, num_lons, num_lats;
	char annotation[NAME_STR_SIZE], string_annotation[NAME_STR_SIZE];
	double min_lon, max_lon, min_lat, max_lat;
	char min_lon_buf[get_data_type_size(DATA_TYPE_DOUBLE)], min_lat_buf[get_data_type_size(DATA_TYPE_DOUBLE)], max_lon_buf[get_data_type_size(DATA_TYPE_DOUBLE)], max_lat_buf[get_data_type_size(DATA_TYPE_DOUBLE)];
	//char min_lon_buf[NAME_STR_SIZE], min_lat_buf[NAME_STR_SIZE], max_lon_buf[NAME_STR_SIZE], max_lat_buf[NAME_STR_SIZE];
	TiXmlElement *uniform_lonlat_grid_entry_element = uniform_lonlat_grid_entry_node->ToElement();
	const char *coord_unit_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of a grid_data_file_field", "datamodel configuration file", true);
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(coord_unit_str, "degrees"), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the parameter \"coord_unit\" (currently is \"%s\") is not \"degrees\". Please check the xml configuration file %s arround line number %d.", grid_name, datamodel_name, coord_unit_str, XML_file_name, uniform_lonlat_grid_entry_element->Row());
	const char *num_lons_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "num_lons", XML_file_name, line_number, "the \"num_lons\" of a uniform_lon_lat_grid", "datamodel configuration file", true);
	const char *num_lats_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "num_lats", XML_file_name, line_number, "the \"num_lats\" of a uniform_lon_lat_grid", "datamodel configuration file", true);
	const char *cyclic_or_acyclic_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "cyclic_or_acyclic", XML_file_name, line_number, "the \"cyclic_or_acyclic\" of a grid_data_file_field", "datamodel configuration file",true);
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(cyclic_or_acyclic_str, "cyclic") || words_are_the_same(cyclic_or_acyclic_str, "acyclic"), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the value (currently is \"%s\") of parameter \"cyclic_or_acyclic\" is not \"cyclic\" or \"acyclic\". Please check the xml configuration file %s around line number %d.", grid_name, datamodel_name, cyclic_or_acyclic_str, XML_file_name, uniform_lonlat_grid_entry_element->Row());
	num_lons = atoi(num_lons_str);
	num_lats = atoi(num_lats_str);
	bool cyclic = false;
	if (words_are_the_same(cyclic_or_acyclic_str, "cyclic"))
		cyclic = true;
	const char *min_lon_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "min_lon", XML_file_name, line_number, "the \"min_lon\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	const char *min_lat_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "min_lat", XML_file_name, line_number, "the \"min_lat\" of a grid_data_file_field", "datamodel configuration file", true);
	const char *max_lon_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "max_lon", XML_file_name, line_number, "the \"max_lon\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	const char *max_lat_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "max_lat", XML_file_name, line_number, "the \"max_lat\" of a grid_data_file_field", "datamodel configuration file", true);
	EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(min_lon_str, "%lf", &min_lon) == 1, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the parameter \"min_lon\" is not of float or double type, which is \"%s\", Please check the xml configuration file %s around line number %d.", grid_name, datamodel_name, min_lon_str, XML_file_name, uniform_lonlat_grid_entry_element->Row());
	EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(max_lon_str, "%lf", &max_lon) == 1, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the parameter \"max_lon\" is not of float or double type, which is \"%s\", Please check the xml configuration file %s around line number %d.", grid_name, datamodel_name, min_lon_str, XML_file_name, uniform_lonlat_grid_entry_element->Row());
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, !cyclic && !(min_lon == 0.0 && max_lon == 360.0) || cyclic && (min_lon == 0.0 && max_lon == 360.0), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": when the value of parameter \"cyclic_or_acyclic\" is specified as \"cyclic\", parameters \"min_lon\" and \"max_lon\" can only be specified as \"0.0\" and \"360.0\" (not \"%f\" and \"%f\" as specified in xml configuration file), Please check the xml configuration file %s around line number %d.", grid_name, datamodel_name, min_lon, max_lon, XML_file_name, uniform_lonlat_grid_entry_element->Row());
	EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(min_lat_str, "%lf", &min_lat) == 1, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the parameter \"min_lat\" is not of float or double type, which is \"%s\", Please check the xml configuration file %s around line number %d.", grid_name, datamodel_name, min_lon_str, XML_file_name, uniform_lonlat_grid_entry_element->Row());
	EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(max_lat_str, "%lf", &max_lat) == 1, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the parameter \"max_lat\" is not of float or double type, which is \"%s\", Please check the xml configuration file %s around line number %d.", grid_name, datamodel_name, min_lon_str, XML_file_name, uniform_lonlat_grid_entry_element->Row());
	memcpy(&min_lon_buf, &min_lon, get_data_type_size(DATA_TYPE_DOUBLE));
	memcpy(&min_lat_buf, &min_lat, get_data_type_size(DATA_TYPE_DOUBLE));
	memcpy(&max_lon_buf, &max_lon, get_data_type_size(DATA_TYPE_DOUBLE));
	memcpy(&max_lat_buf, &max_lat, get_data_type_size(DATA_TYPE_DOUBLE));
	sprintf(annotation, "grid %s for datamodel %s", grid_name, datamodel_name);
	char *center_lon = new char [num_lons*get_data_type_size(DATA_TYPE_DOUBLE)];
	char *center_lat = new char [num_lats*get_data_type_size(DATA_TYPE_DOUBLE)];
	double *center_lon_array = new double [num_lons];
	double *center_lat_array = new double [num_lats];
	double center_lat_buff;
	double center_lon_buff;
	if (cyclic) num_lons ++;
	for (int i = 0; i < num_lats; i++) {
		center_lat_array[i] = ((max_lat - min_lat)/(num_lats-1)) * i + min_lat;
	}
	for (int j = 0; j < num_lons; j++) {
		center_lon_array[j] = ((max_lon - min_lon)/(num_lons-1)) * j + min_lon;
	}
	memcpy(center_lon, center_lon_array, num_lons * sizeof(double));
	memcpy(center_lat, center_lat_array, num_lats * sizeof(double));
	char *cyclic_or_acyclic = strdup(cyclic_or_acyclic_str);
	sprintf(string_annotation, "register H2D grid %s for datamodel %s via CCPL_grid_file at line %d in xml configuration file %s", ori_grid_name, datamodel_name, uniform_lonlat_grid_entry_element->Row(), XML_file_name);
	grid_id = original_grid_mgr->register_H2D_grid_via_global_data(host_comp_id, grid_name, "LON_LAT", coord_unit_str, cyclic_or_acyclic, DATA_TYPE_DOUBLE, num_lons, num_lats, num_lons, num_lats, -1, -1, -1, -1, min_lon_buf, max_lon_buf, min_lat_buf, max_lat_buf, center_lon, center_lat, NULL, NULL, NULL, NULL, string_annotation, API_ID_HANDLER_DATAMODEL_OUTPUT);
	delete [] center_lon;
	delete [] center_lat;
	delete [] center_lon_array;
	delete [] center_lat_array;
	h2d_grid_ids.push_back(grid_id);
	all_grid_ids.push_back(grid_id);
	h2d_grid_names.push_back(strdup(grid_name));
}

void Inout_datamodel::config_vertical_grids_for_datamodel(TiXmlNode *vgs_node) {
	int line_number, dim_size2, grid_id, grid_type;
	char coord_unit_str[NAME_STR_SIZE], top_value_str[NAME_STR_SIZE];
	char data_type[16];
	void *coord_values=NULL, *top_value=NULL, *sigma_values=NULL, *Ap_values=NULL, *Bp_values=NULL;
	void *value2=NULL, *value3=NULL;
	char API_label[32], annotation[256], grid_name_str2[32];
	double temp_value1, *temp_value2, *temp_value3;
	for (TiXmlNode *vg_node = vgs_node->FirstChild(); vg_node != NULL; vg_node = vg_node->NextSibling()) {
		TiXmlElement *vg_element = vg_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, vg_element, XML_file_name, "the status of a \"vertical_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		double temp_value1=NULL, *temp_value2=NULL, *temp_value3=NULL;
		int API_id;
		TiXmlNode *entry_node = vg_node->FirstChild();
		TiXmlElement *entry_element = entry_node->ToElement();
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(entry_element->Value(), "entry"), "Error happens when reading the xml configuration file for datamodel \"%s\", the first child node of a \"vertical_grid\" node should be named \"entry\", Please check file %s.", datamodel_name, XML_file_name);
		const char *file_name_str2 = get_XML_attribute(host_comp_id, 80, entry_element, "file_name", XML_file_name, line_number, "the \"file_name\" of a output_datamodel vertical_grid", "datamodel configuration file",true);
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, vg_element, "grid_name", XML_file_name, line_number, "the \"grid_name\" of an output_datamodel vertical_grid","datamodel configuration file",true);
		const char *specification_str = get_XML_attribute(host_comp_id, 80, vg_element, "specification", XML_file_name, line_number, "the \"specification\" of an output_datamodel vertical_grid","datamodel configuration file",true);
		const char *grid_type_str = get_XML_attribute(host_comp_id, 80, vg_element, "grid_type", XML_file_name, line_number, "the \"grid_type\" of an output_datamodel vertical_grid", "datamodel configuration file",true);
		MPI_Comm comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id, "in register_V1D_grid for output_datamodel");
		bool is_root_proc = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(host_comp_id, "in register_V1D_grid for output_datamodel") == 0;
		char *file_name_str = new char [strlen(datamodel_data_dir) + strlen(file_name_str2) + 1];
		sprintf(file_name_str, "%s/%s", datamodel_data_dir, file_name_str2);
		IO_netcdf *netcdf_file_object = new IO_netcdf("V1D_grid_data", file_name_str, "r", false);
		EXECUTION_REPORT(REPORT_LOG, -1, true, "start to register V1D grid %s for output_datamodel", grid_name_str);
		sprintf(annotation,"V1D grid %s for datamodel %s", grid_name_str, datamodel_name);
		sprintf(grid_name_str2, "datamodel_%s", grid_name_str);
		if (words_are_the_same(specification_str, "grid_data_file_field")) {
			if (words_are_the_same(grid_type_str, "Z")) {
				grid_type = 1;
				config_vertical_z_grid(comm, entry_node, netcdf_file_object, coord_unit_str, &value2, dim_size2, data_type, is_root_proc);
				API_id = API_ID_GRID_MGT_REG_V1D_Z_GRID_VIA_MODEL;
			}
			else if (words_are_the_same(grid_type_str, "SIGMA")) {
				grid_type = 2;
				config_vertical_sigma_grid(comm, entry_node, netcdf_file_object, coord_unit_str, top_value_str, &value2, dim_size2, data_type, is_root_proc);
				memcpy((char*)value2, (char*)value3, dim_size2);
				API_id = API_ID_GRID_MGT_REG_V1D_SIGMA_GRID_VIA_MODEL;
			}
			else if (words_are_the_same(grid_type_str, "HYBRID")) {
				grid_type = 3;
				config_vertical_hybrid_grid(comm, entry_node, grid_name_str,  netcdf_file_object, coord_unit_str, top_value_str, &value2, &value3, dim_size2, data_type, is_root_proc);
				API_id = API_ID_GRID_MGT_REG_V1D_HYBRID_GRID_VIA_MODEL;
			}
			else EXECUTION_REPORT(REPORT_ERROR, -1, false, "The \"grid_type\" of a vertical_grid in a datamodel configuration file can only be one of \"Z\", \"SIGMA\", or \"HYBRID\", Please Verify the xml configuration file \"%s\".", XML_file_name);
		}
		else EXECUTION_REPORT(REPORT_ERROR, -1, false, "The \"specification\" of a vertical_grid in a datamodel configuration file can only be \"grid_data_file_field\", Please verify the xml configuration file \"%s\".", XML_file_name);
		temp_value2 = new double [dim_size2];
		temp_value3 = new double [dim_size2];
		double top_value;
		if (strlen(top_value_str) != 0) {
			EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(top_value_str, "%lf", &top_value) == 1, "Error happens when registering an vertical grid \"%s\" for output_datamodel \"%s\": the parameter \"top_value\" is not of float or double type, which is \"%s\", Please check the xml configuration file %s around line number %d.", grid_name_str, datamodel_name, top_value_str, XML_file_name, vg_node->FirstChild()->ToElement()->Row());
			transform_datatype_of_arrays((float*)value2, temp_value2, dim_size2);
			transform_datatype_of_arrays((float*)value3, temp_value3, dim_size2);
		}
		EXECUTION_REPORT(REPORT_ERROR, -1, is_array_in_sorting_order(temp_value3, dim_size2) != 0, "Error happens when registering V1D grid \"%s\" for datamodel \"%s\": some arrays of parameters are not in a descending/ascending order. Please check the data file \"%s\".", grid_name_str, datamodel_name, file_name_str);
		grid_id = original_grid_mgr->register_V1D_grid_via_data(API_id, host_comp_id, grid_name_str2, grid_type, coord_unit_str, dim_size2, top_value, temp_value2, temp_value3, annotation);
		v1d_grid_ids.push_back(grid_id);
		all_grid_ids.push_back(grid_id);
		delete [] temp_value2;
		delete [] temp_value3;
		delete [] file_name_str;
		delete netcdf_file_object;
	}
}

void Inout_datamodel::config_vertical_z_grid(MPI_Comm comm, TiXmlNode *entry_node, IO_netcdf *netcdf_file_object, char *coord_unit_str, void **coord_values, int &size_coord_values, char *data_type_for_coord_values, bool is_root_proc) {
	int line_number;
	TiXmlElement *entry_element = entry_node->ToElement();
	strcpy(coord_unit_str,get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of a datamodel vertical_grid","datamodel configuration file", true));
	const char *coord_values_str = get_XML_attribute(host_comp_id, 80, entry_element, "coord_values",XML_file_name, line_number, "the \"coord_values\" of a vertical_z_grid", "datamodel configuration file", true);
	netcdf_file_object->read_file_field(coord_unit_str, coord_values, &size_coord_values, data_type_for_coord_values, comm, is_root_proc);
}

void Inout_datamodel::config_vertical_sigma_grid(MPI_Comm comm, TiXmlNode *entry_node, IO_netcdf *netcdf_file_object, char *coord_unit_str, char *top_value_str, void **sigma_values, int &size_sigma_values, char *data_type_for_sigma_values, bool is_root_proc) {
	int line_number;
	TiXmlElement *entry_element = entry_node->ToElement();
	strcpy(coord_unit_str, get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of an datamodel vertical_grid", "datamodel configuration file", true));
	strcpy(top_value_str, get_XML_attribute(host_comp_id, 80, entry_element, "top_value", XML_file_name, line_number, "the \"top_value\" of an output_datmaodel vertical_grid", "datamodel configuration file", true));
	const char *sigma_values_str = get_XML_attribute(host_comp_id, 80, entry_element, "sigma_values", XML_file_name, line_number, "the \"sigma_values\" of a datamodel vertical grid", "datamodel configuration file", true);
	netcdf_file_object->read_file_field(sigma_values_str, sigma_values, &size_sigma_values, data_type_for_sigma_values, comm, is_root_proc);
}

void Inout_datamodel::config_vertical_hybrid_grid(MPI_Comm comm, TiXmlNode *entry_node, const char *grid_name_str, IO_netcdf *netcdf_file_object, char *coord_unit_str, char *top_value_str, void **Ap_values, void **Bp_values, int &size_Ap, char *data_type_for_ap_values, bool is_root_proc) {
	int line_number, size_Bp;
	char data_type_for_bp_values[NAME_STR_SIZE];
	TiXmlElement *entry_element = entry_node->ToElement();
	strcpy(coord_unit_str, get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of an datamodel vertical_grid", "datamodel configuration file", true));
	strcpy(top_value_str, get_XML_attribute(host_comp_id, 80, entry_element, "top_value", XML_file_name, line_number, "the \"top_value\" of an output_datmaodel vertical_grid", "datamodel configuration file", true));
	const char *Ap_str = get_XML_attribute(host_comp_id, 80, entry_element, "coef_A", XML_file_name, line_number, "the \"Ap\" values of an datamodel vertical grid", "datamodel configuration file", true);
	const char *Bp_str = get_XML_attribute(host_comp_id, 80, entry_element, "coef_B", XML_file_name, line_number, "the \"Bp\" values of an datamodel vertical grid", "datamodel configuration file", true);
	netcdf_file_object->read_file_field(Ap_str, Ap_values, &size_Ap, data_type_for_ap_values, comm, is_root_proc);
	netcdf_file_object->read_file_field(Bp_str, Bp_values, &size_Bp, data_type_for_bp_values, comm, is_root_proc);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_ap_values, data_type_for_bp_values), "Error happens when registering a V1D hybrid gird \"%s\" for datamodel %s, the data type for Ap of variable \"%s\" (which is %s) is different from the data type for Bp of variable \"%s\" (which is %s).", datamodel_name, grid_name_str, Ap_str, data_type_for_ap_values, Bp_str, data_type_for_bp_values);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, size_Ap == size_Bp, "Error happens when registering a V1D hybrid gird \"%s\" for datamodel %s, the array size for Ap of variable \"%s\" is different form the array size for Bp of variable \"%s\".", grid_name_str, Ap_str, Bp_str);
}

void Inout_datamodel::config_v3d_grids_for_datamodel(TiXmlNode *v3ds_node) {
	int line_number, v3d_grid_id, mid_3d_grid_id, mid_1d_grid_id;
	for (TiXmlNode *v3d_node = v3ds_node->FirstChild(); v3d_node != NULL; v3d_node = v3d_node->NextSibling()) {
		TiXmlElement *v3d_element = v3d_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, v3d_element, XML_file_name, "the status of a \"3d_grid\" node of a datamodel", "datamodel xml file"))
			continue;
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, v3d_element, "grid_name", XML_file_name, line_number, "the \"grid_name\" of a datamodel 3d_grid","datamodel configuration file",true);
		const char *mid_point_grid_name_str = get_XML_attribute(host_comp_id, 80, v3d_element, "mid_point_grid_name", XML_file_name, line_number, "the \"mid_point_grid_name\" of a datamodel vertical_grid","datamodel configuration file",false);
		TiXmlNode *horizontal_sub_grid_node = v3d_node->FirstChild();
		TiXmlElement *horizontal_grid_element = horizontal_sub_grid_node->ToElement();
		const char *horizontal_subgrid_name_str = get_XML_attribute(host_comp_id, 80, horizontal_grid_element, "grid_name",XML_file_name, line_number, "the \"grid_name\" of the horizontal_subgrid of a datamodel v3d_grid","datamodel configuration file",true);
		TiXmlNode *vertical_sub_grid_node = horizontal_sub_grid_node->NextSibling();
		TiXmlElement *vertical_sub_grid_element = vertical_sub_grid_node->ToElement();
		const char *vertical_subgrid_name_str = get_XML_attribute(host_comp_id, 80, vertical_sub_grid_element, "grid_name",XML_file_name, line_number, "the \"grid_name\" of the vertical_subgrid of a datamodel v3d_grid","datamodel configuration file", true);
		char *hg_subgrid_name = new char [10 + strlen(horizontal_subgrid_name_str)];
		char *vg_subgrid_name = new char [10 + strlen(vertical_subgrid_name_str)];
		char *vd_grid_name = new char [10 + strlen(grid_name_str)];
		sprintf(hg_subgrid_name, "datamodel_%s", horizontal_subgrid_name_str);
		sprintf(vg_subgrid_name, "datamodel_%s", vertical_subgrid_name_str);
		sprintf(vd_grid_name, "datamodel_%s", grid_name_str);
		char *report_string = new char [NAME_STR_SIZE * h2d_grid_ids.size()];
		for (int i = 0; i < h2d_grid_ids.size(); i++) {
			sprintf(report_string, "%s ", original_grid_mgr->search_grid_info(h2d_grid_ids[i])->get_grid_name());
		}
		int h2d_subgrid_id = original_grid_mgr->get_grid_id(host_comp_id, hg_subgrid_name, "get horizontal_subgrid name for datamodel");
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, h2d_subgrid_id != -1, "Error happens when registering a V3D_grid \"%s\" for datamodel \"%s\": the horizontal subgrid with name \"%s\" has not been specified (currently specified are \"%s\"), Please check the xml configuration file \"%s\".", vd_grid_name, datamodel_name, hg_subgrid_name, report_string, XML_file_name);
		for (int i = 0; i < v1d_grid_ids.size(); i++) {
			sprintf(report_string, "%s ", original_grid_mgr->search_grid_info(v1d_grid_ids[i])->get_grid_name());
		}
		int v1d_subgrid_id = original_grid_mgr->get_grid_id(host_comp_id, vg_subgrid_name, "get vertical_subgrid name for datamodel");
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, v1d_subgrid_id != -1, "Error happens when registering a V3D_grid \"%s\" for datamodel \"%s\": the verticlal subgrid with name \"%s\" has not been specified (currently specified are \"%s\"), Please check the xml configuration file \"%s\".", vd_grid_name, datamodel_name, vg_subgrid_name, report_string, XML_file_name);
		delete [] report_string;
		v3d_grid_id = original_grid_mgr->register_md_grid_via_multi_grids(host_comp_id, vd_grid_name, h2d_subgrid_id, v1d_subgrid_id, -1, -1, NULL, "register v3d_grid for datamodel");
		v3d_grid_ids.push_back(v3d_grid_id);
		all_grid_ids.push_back(v3d_grid_id);
		md_grid_names.push_back(strdup(vd_grid_name));

		if (mid_point_grid_name_str != NULL) {
			char API_label[256];
			get_API_hint(-1, API_ID_GRID_MGT_REG_MID_POINT_GRID, API_label);
			check_for_ccpl_managers_allocated(API_ID_GRID_MGT_REG_MID_POINT_GRID, "register mid point grid for datamodel");
			check_for_coupling_registration_stage(original_grid_mgr->get_comp_id_of_grid(v3d_grid_id), API_ID_GRID_MGT_REG_MID_POINT_GRID, true, "register mid point grid for datamodel");
			char *mid_point_grid_name = new char [10 + strlen(mid_point_grid_name_str)];
			sprintf(mid_point_grid_name, "datamodel_%s", mid_point_grid_name_str);
			original_grid_mgr->register_mid_point_grid(v3d_grid_id, &mid_3d_grid_id, &mid_1d_grid_id, -1, NULL, "register mid point grid for datamodel", API_label);
			md_grid_names.push_back(strdup(mid_point_grid_name));
			v3d_grid_ids.push_back(mid_3d_grid_id);
			all_grid_ids.push_back(mid_3d_grid_id);
			mid_1d_grid_ids.push_back(mid_1d_grid_id);
			delete [] mid_point_grid_name;
		}
		TiXmlNode *surface_field_node = vertical_sub_grid_node->FirstChild();
		if (surface_field_node != NULL) {//if surface_field_node exists, read it
			TiXmlElement *surface_field_element = surface_field_node->ToElement();
			const char *surface_field_type_str = get_XML_attribute(host_comp_id, 80, surface_field_element, "type", XML_file_name, line_number, "the surface_field \"type\" of a datamodel V3D_grid", "datamodel configuration file",false);
			const char *field_name_in_file_str = get_XML_attribute(host_comp_id, 80, surface_field_element, "field_name_in_file", XML_file_name, line_number, "the surface_field name of a datamodel V3D_grid", "datamodel configuration file", false);
			surface_field_names.push_back(strdup(field_name_in_file_str));
			//if (datamodel_type == INPUT_DATAMODEL) surface_field_type...
		}
		delete [] hg_subgrid_name;
		delete [] vg_subgrid_name;
		delete [] vd_grid_name;
		EXECUTION_REPORT_LOG(REPORT_LOG, host_comp_id, true, "Finished registering V3D_grid for datamodel %s, grid %s", datamodel_name, grid_name_str);
	}
}

void Inout_datamodel::config_field_output_settings_for_datamodel(TiXmlNode *field_output_settings_node) {
	int line_number, fields_output_setting_index = 0;
	for (TiXmlNode *field_output_setting_node = field_output_settings_node->FirstChild(); field_output_setting_node != NULL; field_output_setting_node = field_output_setting_node->NextSibling()) {
		TiXmlElement *field_output_setting_element = field_output_setting_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, field_output_setting_element, XML_file_name, "the status of a \"horizontal_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		config_fields_output_setting_attributes(field_output_setting_node);
		for (TiXmlNode *sub_node = field_output_setting_node->FirstChild(); sub_node != NULL; sub_node = sub_node->NextSibling()) {
			TiXmlElement *sub_element = sub_node->ToElement();
			if (words_are_the_same(sub_element->Value(), "output_frequency"))
				config_output_frequency(sub_node, fields_output_setting_index);
			if (words_are_the_same(sub_element->Value(), "fields"))
				config_field_info(sub_node, fields_output_setting_index);
		}
		fields_output_setting_index ++;
	}
}

void Inout_datamodel::config_fields_output_setting_attributes(TiXmlNode *default_settings_node) {
	int line_number;
	TiXmlElement *default_settings_element = default_settings_node->ToElement();
	file_mid_name = strdup(get_XML_attribute(host_comp_id, 80, default_settings_element, "file_mid_name", XML_file_name, line_number, "The \"file_mid_name\" of the output_datamodel","output datamodel xml file",false));
	const char *time_format_in_datafile_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "time_format_in_data_file", XML_file_name, line_number, "The \"time_format_in_data_file\" of the output_datamodel","output datamodel xml file",false);
	int id_time_format_in_data_file = check_time_format(time_format_in_datafile_str, "time_format in output_datamodel file name");
	id_time_format_in_data_files.push_back(id_time_format_in_data_file);
	const char *field_specification_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "field_specification", XML_file_name, line_number, "The \"field_specification\" of the output_datamodel","output datamodel xml file",false);
	EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(field_specification_str, "default") || words_are_the_same(field_specification_str, "fields") || words_are_the_same(field_specification_str, "hybrid"), "Error happens when configuring \"fields_output_settings\" node: parameter \"field_specification\" can only be one of \"default\" \"fields\" or \"hybrid\", Please check the xml configuration file %s around line number %d", XML_file_name, default_settings_element->Row());

	const char *default_operation_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_operation", XML_file_name, line_number, "The \"default_operation\" of the output_datamodel","output datamodel xml file",false);
	const char *default_h2d_grid_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_h2d_grid", XML_file_name, line_number, "The \"default_h2d_grid\" of the output_datamodel","output datamodel xml file",false);
	const char *default_v3d_grid_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_v3d_grid", XML_file_name, line_number, "The \"default_v3d_grid\" of the output_datamodel","output datamodel xml file",false);
	const char *default_float_type_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_float_type", XML_file_name, line_number, "The \"default_float_type\" of the output_datamodel","output datamodel xml file",false);
	const char *default_integer_type_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_integer_type", XML_file_name, line_number, "The \"default_integer_type\" of the output_datamodel","output datamodel xml file",false);
	if (field_specification_str != NULL)
		field_specifications.push_back(strdup(field_specification_str));
	else field_specifications.push_back("default");
	if (default_operation_str != NULL)
		default_operations.push_back(strdup(default_operation_str));
	else default_operations.push_back(NULL);

	if (default_float_type_str != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(default_float_type_str, "float") || words_are_the_same(default_float_type_str, "double"), "Error happens when configuring \"output_frequency\" node for datamodel \"%s\": parameter \"default_float_type\" must be one of \"float\" or \"double\", Please check the xml configuration file %s around line number %d.", datamodel_name, XML_file_name);
		default_float_types.push_back(strdup(default_float_type_str));
	}
	else default_float_types.push_back(NULL);

	if (default_integer_type_str != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(default_integer_type_str, "int") || words_are_the_same(default_integer_type_str, "short") , "Error happens when configuring \"output_frequency\" node for datamodel \"%s\": parameter \"default_integer_type_str\" must be one of \"int\" or \"short\", Please check the xml configuration file %s around line number %d.", datamodel_name, XML_file_name);
		default_integer_types.push_back(strdup(default_integer_type_str));
	}
	else default_integer_types.push_back(NULL);

	if (default_h2d_grid_str != NULL) {
		bool default_h2d_grid_valid = false;
		char *default_name = new char [16+strlen(default_h2d_grid_str)];
		sprintf(default_name, "datamodel_%s", default_h2d_grid_str);
		for (int i = 0; i < h2d_grid_names.size(); i++)
			if (words_are_the_same(default_name, h2d_grid_names[i]))
				{ default_h2d_grid_valid = true; break; }
		EXECUTION_REPORT(REPORT_ERROR, -1, default_h2d_grid_valid, "Error happens when configuring parameter \"default_h2d_grid\" for datamodel %s: grid name \"%s\" is not valid, Please check the xml configruation file around line number %d.", datamodel_name, default_h2d_grid_str, default_settings_element->Row());
		default_h2d_grids.push_back(strdup(default_name));
		delete default_name;
	}
	else default_h2d_grids.push_back(NULL);

	if (default_v3d_grid_str != NULL) {
		bool default_v3d_grid_valid = false;
		char *default_name = new char [16+strlen(default_v3d_grid_str)];
		sprintf(default_name, "datamodel_%s", default_v3d_grid_str);
		for (int i = 0; i < md_grid_names.size(); i++) {
			if (words_are_the_same(default_name, md_grid_names[i])) {
				default_v3d_grid_valid = true;
				break;
			}
		}
		EXECUTION_REPORT(REPORT_ERROR, -1, default_v3d_grid_valid, "Error happens when configuring parameter \"default_v3d_grid\" for datamodel %s: grid name \"%s\" is not valid, Please check the xml configruation file around line number %d.", datamodel_name, default_v3d_grid_str, default_settings_element->Row());
		default_v3d_grids.push_back(strdup(default_name));
		delete default_name;
	}
	else default_v3d_grids.push_back(NULL);
}

void Inout_datamodel::config_output_frequency(TiXmlNode *output_frequency_node, int index) {
	int line_number, freq_count, file_freq_count;
	TiXmlElement *output_frequency_element = output_frequency_node->ToElement();
	const char *file_freq_unit_str = get_XML_attribute(host_comp_id, 80, output_frequency_element, "file_freq_unit", XML_file_name, line_number, "The \"file_freq_unit\" of the output_datamodel","output datamodel xml file",false);
	const char *file_freq_count_str = get_XML_attribute(host_comp_id, 80, output_frequency_element, "file_freq_count", XML_file_name, line_number, "The \"file_freq_count\" of the output_datamodel","output datamodel xml file",false);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, (file_freq_count_str != NULL && file_freq_unit_str != NULL) || (file_freq_count_str == NULL || file_freq_unit_str == NULL), "Error happens when reading \"output_frequency\" configurations for output_datamodel \"%s\": parameters \"file_freq_count\" and \"file_freq_unit\" should be specified/unspecified at the same time, Please verify the xml file %s arround line_number %d", datamodel_name, XML_file_name, output_frequency_element->Row());
	if (file_freq_unit_str != NULL) {
		int id_file_freq_unit = set_unit(file_freq_unit_str, "file_freq_unit");
		EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(file_freq_count_str, "%d", &file_freq_count) == 1, "Error happens when reading \"output_frequency\" configurations for output_datamodel \"%s\": the parameter \"file_freq_count\" is not of integer type (which is \"%s\"), Please check the xml configuration file %s around line number %d.", datamodel_name, file_freq_count_str, XML_file_name, output_frequency_element->Row());
		file_freq_counts.push_back(file_freq_count);
		file_freq_units.push_back(strdup(file_freq_unit_str));
	}
	else {
		file_freq_counts.push_back(NULL);
		file_freq_units.push_back(NULL);//不可以push_back NULL
	}
	if (file_freq_units[index] != NULL) {
		int file_timer_id = timer_mgr->define_timer(host_comp_id, file_freq_units[index], file_freq_counts[index], 0, 0, this->annotation);
		file_timer_ids.push_back(file_timer_id);
	}
	else file_timer_ids.push_back(NULL);

	bool is_outer_segment_node_found = false;
	for (TiXmlNode *outer_segment_node = output_frequency_node->FirstChild(); outer_segment_node != NULL; outer_segment_node = outer_segment_node->NextSibling()) {
		TiXmlElement *outer_segment_element = outer_segment_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, outer_segment_element, XML_file_name, "the status of a \"segment\" node of an output_datamodel output_frequency_setting", "output_datamodel xml file"))
			continue;
		else if (is_outer_segment_node_found)
			EXECUTION_REPORT(REPORT_ERROR, -1, false, "In output_datamodel configuration file\"%s\", only one \"status\" of the outmost \"segment\" node can be set on, Please Verify.", XML_file_name);
		else is_outer_segment_node_found = true;
		const char *freq_count_str = get_XML_attribute(host_comp_id, 80, outer_segment_element, "freq_count", XML_file_name, line_number, "The \"freq_count\" of the output_datamodel","output datamodel xml file",false);
		const char *freq_unit_str = get_XML_attribute(host_comp_id, 80, outer_segment_element, "freq_unit", XML_file_name, line_number, "The \"freq_unit\" of the output_datamodel","output datamodel xml file",false);
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, (freq_count_str != NULL && freq_unit_str != NULL) || (freq_count_str == NULL || freq_unit_str == NULL), "Error happens when reading \"output_frequency\" configurations for output_datamodel \"%s\": parameters \"freq_count\" and \"freq_unit\" should be specified/unspecified at the same time, Please verify the xml file %s arround line_number %d", datamodel_name, XML_file_name, outer_segment_element->Row());
		if (freq_unit_str != NULL) {
			int id_freq_unit = set_unit(freq_unit_str, "freq_unit");
			EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(freq_count_str, "%d", &freq_count) == 1, "Error happens when reading \"output_frequency\" configurations for output_datamodel \"%s\": the parameter \"freq_count\" is not of integer type (which is \"%s\"), Please check the xml configuration file %s around line number %d.", datamodel_name, freq_count_str, XML_file_name, outer_segment_element->Row());
			output_freq_counts.push_back(freq_count);
			output_freq_units.push_back(strdup(freq_unit_str));
		}
		else {
			output_freq_counts.push_back(NULL);
			output_freq_units.push_back(NULL);
		}
		//read nested output_frequencies
		std::vector<TiXmlNode*> segment_vector;
		int valid_segment_start_pos = 0;
		//第一层可能是time_points_node
		TiXmlNode *time_segment_node = outer_segment_node->FirstChild();
		if (time_segment_node == NULL || !expected_segment_exists(time_segment_node, "time_points")) {
			time_point_units.push_back(NULL);
			time_points.push_back(0);
		} 
		else if (expected_segment_exists(time_segment_node, "time_slots"))
			get_all_sub_segment_time_slots(outer_segment_node, segment_vector);//create a queue
		else if (expected_segment_exists(time_segment_node, "time_points"))
			visit_time_points_node(time_segment_node);
		int num_segments = segment_vector.size();
		while (num_segments != 0) {
			for (int i = valid_segment_start_pos; i < num_segments; i++) {
				visit_time_slots_node(segment_vector[i]);
				get_all_sub_segment_time_slots(segment_vector[i], segment_vector);
			}
			valid_segment_start_pos += num_segments;
			num_segments = segment_vector.size() - num_segments;
		}
	}
	//define single timer
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, components_time_mgrs->get_time_mgr(host_comp_id)->get_time_step_in_second() > 0, "Error happers when configuring the \"output_frequency\" parameter for datamodel %s: the time step of the host component model \"%s\" has not been set yet. Please specify the time step before registering the output_datmaodel_handler with annotation \"%s\"", datamodel_name, 
		comp_comm_group_mgt_mgr->get_global_node_of_local_comp(host_comp_id, true, this->annotation)->get_comp_full_name(), this->annotation);
	if (output_freq_units[index] != NULL) {
		int output_timer_id = timer_mgr->define_timer(host_comp_id, output_freq_units[index], output_freq_counts[index], time_points[index], 0, this->annotation);//the index can only uesed under define_single_timer case
		output_timer_ids.push_back(output_timer_id);
	}
	else output_timer_ids.push_back(NULL);
}

bool Inout_datamodel::expected_segment_exists(TiXmlNode *&undecided_node, const char *expected_str) {
	int line_number;
	TiXmlNode *input_undecided_node = undecided_node;
	for (; undecided_node != NULL; undecided_node = undecided_node->NextSibling()) {
		TiXmlElement *undecided_element = undecided_node->ToElement();
		if(is_XML_setting_on(host_comp_id, undecided_element, XML_file_name, "the status of a \"time_segment\" node of an output_datamodel output_frequency_setting", "output_datamodel xml file")) {
			const char *undecided_str = get_XML_attribute(host_comp_id, 80, undecided_element, expected_str, XML_file_name, line_number, "The \"time_slots\" or \"time_points\" of the output_datamodel","output datamodel xml file",false);
			if (undecided_str == NULL)
				return false;
			else return true;
		}
	}
	undecided_node = input_undecided_node;
	return false;
}

void Inout_datamodel::get_all_sub_segment_time_slots(TiXmlNode *outer_segment_node, std::vector<TiXmlNode*> global_segment_vector) {
	for (TiXmlNode *internal_segment_node = outer_segment_node->FirstChild(); internal_segment_node != NULL; internal_segment_node = internal_segment_node->NextSibling()) {
		if (expected_segment_exists(internal_segment_node, "time_slots"))
			global_segment_vector.push_back(internal_segment_node);
	}
}

void Inout_datamodel::visit_time_slots_node(TiXmlNode *time_slot_segment_node) {
	int line_number;
	TiXmlElement *time_slot_segment_element = time_slot_segment_node->ToElement();
	const char *time_slots_str = get_XML_attribute(host_comp_id, 80, time_slot_segment_element, "time_slots", XML_file_name, line_number, "The \"time_slots\" of the output_datamodel","output datamodel xml file",true);
	const char *time_slot_format_str = get_XML_attribute(host_comp_id, 90, time_slot_segment_element, "time_slot_format", XML_file_name, line_number, "The \"time_slot_format\" of the output_datamodel","output datamodel xml file",true);
	const char *freq_count_str = get_XML_attribute(host_comp_id, 80, time_slot_segment_element, "freq_count", XML_file_name, line_number, "The \"freq_count\" of the output_datamodel","output datamodel xml file",false);
	const char *freq_unit_str = get_XML_attribute(host_comp_id, 80, time_slot_segment_element, "freq_unit", XML_file_name, line_number, "The \"freq_unit\" of the output_datamodel","output datamodel xml file",false);
	TiXmlNode *sub_segment_node = time_slot_segment_node->FirstChild();
	if (expected_segment_exists(sub_segment_node, "time_points"))
		visit_time_points_node(sub_segment_node);
}

void Inout_datamodel::visit_time_points_node(TiXmlNode *time_points_segment_node) {
	int line_number;
	TiXmlElement *time_points_segment_element = time_points_segment_node->ToElement();
	const char *time_points_str = get_XML_attribute(host_comp_id, 80, time_points_segment_element, "time_points", XML_file_name, line_number, "The \"time_points\" of the output_datamodel","output datamodel xml file",true);
	const char *time_point_format_str = get_XML_attribute(host_comp_id, 80, time_points_segment_element, "time_point_format", XML_file_name, line_number, "The \"time_point_format\" of the output_datamodel","output datamodel xml file",false);
	const char *time_point_unit_str = get_XML_attribute(host_comp_id, 80, time_points_segment_element, "time_point_unit", XML_file_name, line_number, "The \"time_point_unit\" of the output_datamodel","output datamodel xml file",false);
	//the following are for single time point case
	int time_point;
	EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(time_points_str, "%d", &time_point) == 1, "Error happens when reading \"output_frequency\" configurations for output_datamodel \"%s\": the parameter \"time_points\" is not of integer type (which is \"%s\"), Please check the xml configuration file %s around line number %d.", datamodel_name,time_points_str, XML_file_name, time_points_segment_element->Row());
	time_points.push_back(time_point);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, (time_point_format_str != NULL && time_point_unit_str == NULL) || (time_point_format_str == NULL || time_point_unit_str != NULL), "Error happens when reading \"output_frequency\" configurations for output_datamodel \"%s\": only one of parameters \"time_point_format\" and \"time_point_unit\" can be set and must be set, Please verify the xml file %s arround line_number %d", datamodel_name, XML_file_name, time_points_segment_element->Row());
	if (time_point_format_str != NULL)
		//time_point_units.push_back(transform_time_format_to_unit(time_point_format_str, "time_point_format"));
		time_point_units.push_back(NULL);
	else time_point_units.push_back(strdup(time_point_unit_str));
}

void Inout_datamodel::config_field_info(TiXmlNode *fields_node, int index) {
	std::vector<Field_config_info*> current_set_of_fields;
	for (TiXmlNode *field_node = fields_node->FirstChild(); field_node != NULL; field_node = field_node->NextSibling()) {
		TiXmlElement *field_element = field_node->ToElement();
		Field_config_info *output_field_info = new Field_config_info();
		output_field_info->name_in_model = strdup(get_XML_attribute(host_comp_id, 80, field_element, "name_in_model", XML_file_name, line_number, "The \"name_in_model\" of the output_datamodel field","output datamodel xml file",true));
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, field_element, "grid_name", XML_file_name, line_number, "The \"grid_name\" of the output_datamodel field","output datamodel xml file",false);
		const char *name_in_file_str = get_XML_attribute(host_comp_id, 80, field_element, "name_in_file", XML_file_name, line_number, "The \"name_in_file\" of the output_datamodel field","output datamodel xml file",false);
		const char *float_datatype_str = get_XML_attribute(host_comp_id, 80, field_element, "float_type", XML_file_name, line_number, "The \"float_datatype\" of the output_datamodel field","output datamodel xml file",false);
		const char *integer_datatype_str = get_XML_attribute(host_comp_id, 80, field_element, "integer_type", XML_file_name, line_number, "The \"integer_datatype\" of the output_datamodel field","output datamodel xml file",false);
		const char *operation_str = get_XML_attribute(host_comp_id, 80, field_element, "operation", XML_file_name, line_number, "The \"operation\" of the output_datamodel field","output datamodel xml file",false);
		const char *unit_str = get_XML_attribute(host_comp_id, 80, field_element, "unit", XML_file_name, line_number, "The \"unit\" of the output_datamodel field","output datamodel xml file",false);
		const char *add_offset_str = get_XML_attribute(host_comp_id, 80, field_element, "add_offset", XML_file_name, line_number, "The \"add_offset\" of the output_datamodel field","output datamodel xml file",false);
		const char *scale_factor_str = get_XML_attribute(host_comp_id, 80, field_element, "scale_factor", XML_file_name, line_number, "The \"scale_factor\" of the output_datamodel field","output datamodel xml file",false);

		if (grid_name_str != NULL) {
			output_field_info->grid_name = strdup(grid_name_str);
		}
		else output_field_info->grid_name = NULL;//设置起来有些复杂，要根据变量实例具体是二维的还是三维的

		if (name_in_file_str != NULL)
			output_field_info->name_in_file = strdup(name_in_file_str);
		else output_field_info->name_in_file = strdup(output_field_info->name_in_model);

		if (float_datatype_str != NULL) {
			EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(float_datatype_str, "float") || words_are_the_same(float_datatype_str, "double"), "Error happens when configuring \"field\" node for datamodel \"%s\": parameter \"float_type\" must be one of \"float\" or \"double\", Please check the xml configuration file %s around line number %d.", datamodel_name, XML_file_name, field_element->Row());
			output_field_info->float_datatype = strdup(float_datatype_str);
		}
		else if (default_float_types[index] != NULL)
			output_field_info->float_datatype = strdup(default_float_types[index]);
		else output_field_info->float_datatype = NULL;

		if (integer_datatype_str != NULL) {
			EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(integer_datatype_str, "int") || words_are_the_same(integer_datatype_str, "short"), "Error happens when configuring \"field\" node for datamodel \"%s\": parameter \"integer_type\" must be one of \"int\" or \"short\", Please check the xml configuration file %s around line number %d.", datamodel_name, XML_file_name, field_element->Row());
			output_field_info->integer_datatype = strdup(integer_datatype_str);
		}
		else if (default_integer_types[index] != NULL)
			output_field_info->integer_datatype = strdup(default_integer_types[index]);
		else output_field_info->integer_datatype = NULL;

		if (operation_str != NULL) {
			EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(operation_str, "aver") || words_are_the_same(operation_str, "inst") || words_are_the_same(operation_str, "min") || words_are_the_same(operation_str, "max") || words_are_the_same(operation_str, "accum"), "Error happens when configuring \"field\" node for datamodel \"%s\": parameter \"operation\" must be one of \"aver\" \"inst\" \"min\" \"max\" or \"accum\", Please check the xml configuration file %s around line number %d.", datamodel_name, XML_file_name, field_element->Row());
			output_field_info->operation = strdup(operation_str);
		}
		else if (default_operations[index] != NULL)
			output_field_info->operation = strdup(default_operations[index]);
		else output_field_info->operation = NULL;


		EXECUTION_REPORT(REPORT_ERROR, -1, (add_offset_str != NULL && scale_factor_str != NULL) || (add_offset_str == NULL && scale_factor_str == NULL), "Error happens when configuring \"field\" node for datamodel \"%s\": parameter \"add_offset\" and \"scale_factor\" must be specified/unspecified at the same time, Please check the xml configuration file %s around line number %d.", datamodel_name, XML_file_name, field_element->Row());
		if (add_offset_str != NULL) {
			EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(add_offset_str, "%lf", &(output_field_info->add_offset)) == 1, "Error happens when configuring \"field\" node for datamodel \"%s\": parameter \"add_offset\" is not of float or double type (which is \"%s\"), Please check the xml configuration file %s around line number %d.", datamodel_name, add_offset_str, XML_file_name, field_element->Row());
			EXECUTION_REPORT(REPORT_ERROR, -1, sscanf(scale_factor_str, "%lf", &(output_field_info->scale_factor)) == 1, "Error happens when configuring \"field\" node for datamodel \"%s\": the parameter \"scale_factor\" is not of float or double type (which is \"%s\"), Please check the xml configuration file %s around line number %d.", datamodel_name, scale_factor_str, XML_file_name, field_element->Row());
		}
		else {
			output_field_info->add_offset = 0.0;
			output_field_info->scale_factor = 1.0;
		}
		current_set_of_fields.push_back(output_field_info);
	}
	fields_config_info.push_back(current_set_of_fields);
}

Inout_datamodel::~Inout_datamodel() {
	delete [] datamodel_files_dir_name;
	delete [] file_dir;
	delete [] file_name_prefix;
	for (int i = 0; i <default_operations.size(); i++) {
		delete default_operations[i];
		delete default_integer_types[i];
		delete default_float_types[i];
		delete default_h2d_grids[i];
		delete default_v3d_grids[i];
		delete file_freq_units[i];
		delete time_point_units[i];
		delete output_freq_units[i];
	}
	for (int i = 0; i < h2d_grid_names.size(); i++) {
		delete h2d_grid_names[i];
	}
	for (int i = 0; i < md_grid_names.size(); i++) {
		delete md_grid_names[i];
	}
	for (int i = 0; i < surface_field_names.size(); i++) {
		delete surface_field_names[i];
	}
}

Datamodel_mgt::~Datamodel_mgt() {
	for (int i = 0; i < output_handlers.size(); i++) {
		delete [] output_handlers[i];
	}
	for (int i = 0; i < output_datamodels.size(); i++) {
		delete [] output_datamodels[i];
	}
	for (int i = 0; i < input_datamodels.size(); i++) {
		delete [] input_datamodels[i];
	}
}

void Datamodel_mgt::handle_normal_output(int handler_id, int bypass_timer, int API_id, const char *handler_annotation) {
	Output_handler *output_handler;
	char API_label[NAME_STR_SIZE];

	get_API_hint(-1, API_id, API_label);
	if (!is_legal_handler_id(handler_id))
		EXECUTION_REPORT(REPORT_ERROR, -1, false, "Error happens when handling a normal output through the API \"%s\": the given interface ID 0x%x is illegal. Please check the model code with the annotation \"%s\"", API_label, handler_id, handler_annotation);
	output_handler = get_handler(handler_id);
	EXECUTION_REPORT_LOG(REPORT_LOG, output_handler->get_host_comp_id(), true, "Begin to execute handler \"%s\" (model code annotation is \"%s\")", output_handler->get_handler_name(), handler_annotation);
	output_handler->execute_handler(bypass_timer, API_id, handler_annotation);
	EXECUTION_REPORT_LOG(REPORT_LOG, output_handler->get_host_comp_id(), true, "Finish executing handler \"%s\" (model code annotation is \"%s\")", output_handler->get_handler_name(), handler_annotation);
}

void Output_handler::execute_handler(int bypass_timer, int API_id, const char *annotation) {
	//coupling generation for output_data
	generate_interpolation();
}

void Output_handler::generate_interpolation(bool output_original_grid) {
	//
}

Output_handler *Datamodel_mgt::get_handler(int handler_id) {
	if (!is_legal_handler_id(handler_id))
		return NULL;
	return output_handlers[handler_id&TYPE_ID_PREFIX_MASK];
}

bool Datamodel_mgt::is_legal_handler_id(int handler_id) {
	if ((handler_id & TYPE_ID_PREFIX_MASK) != TYPE_OUTPUT_HANDLER_ID_PREFIX)
		return false;
	return (handler_id&TYPE_ID_SUFFIX_MASK) < output_handlers.size();
}