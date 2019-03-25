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

bool varname_or_value(const char* string) {
    for (int i=0;i<strlen(string);i++) {
        if (!(((string[i]>='0') && (string[i]<='9')) || (string[i]=='.') || string[0] == '-')) 
            return false;//not decimal value
    }
    return true;//decimal value
}

int set_unit(const char* input_unit) {
    int output_unit;
    if (words_are_the_same(input_unit,"year") || words_are_the_same(input_unit,"years") || words_are_the_same(input_unit,"nyear") || words_are_the_same(input_unit,"nyears"))
        output_unit=1;
    else if (words_are_the_same(input_unit,"month") || words_are_the_same(input_unit,"months") || words_are_the_same(input_unit,"nmonth") || words_are_the_same(input_unit,"nmonths"))
        output_unit=2;
    else if (words_are_the_same(input_unit,"day") || words_are_the_same(input_unit,"days") || words_are_the_same(input_unit,"nday") || words_are_the_same(input_unit,"ndays"))
        output_unit=3;
    else if (words_are_the_same(input_unit,"second") || words_are_the_same(input_unit,"seconds") || words_are_the_same(input_unit,"nsecond") || words_are_the_same(input_unit,"nseconds"))
        output_unit=4;
    return output_unit;
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
    else EXECUTION_REPORT(REPORT_ERROR, false, "The time format \"%s\" for \"%s\" is incorrect", time_format, report_hint);

    return id_time_format;
}

Datamodel_instance_info::Datamodel_instance_info() {
	//initialization
	is_coupling_connection_file_read = false;
	is_datamodel_needed = false;
	host_comp_id = -1;
	id_period_time_format = -1;
	period_unit = -1;
	period_count = -1;
	offset_unit = -1;
	offset_count = -1;
	printf("initializing datamodel_instance_info");
}

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
				strncpy(this->datamodel_instance_name,datamodel_interface_str+10,strlen(datamodel_interface_str)-9);
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
		bool is_period_set=false;
		if (words_are_the_same(specification,"period")) {
			for (TiXmlNode *period_setting_element_node = time_mapping_element->FirstChild(); period_setting_element_node != NULL; period_setting_element_node = period_setting_element_node->NextSibling()) {
				TiXmlElement *period_setting_element = period_setting_element_node->ToElement();
				if (!is_XML_setting_on(host_comp_id, period_setting_element, XML_file_name, "the status of period setting configuration for a datamodel_instance","datamodel_instance configuration file"))
					continue;
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,!is_period_set,"the XML element for specifying the period_setting information of a Datamodel instance in file %s has already been set,Please verify the wanted period_setting version around line number %d",XML_file_name,period_setting_element->Row());
				const char *period_unit_config = get_XML_attribute(host_comp_id, 80, period_setting_element, "period_unit", XML_file_name, line_number, "period unit of a datamodel instance", "datamodel instance configuration file", true);
				const char *period_start_time_config = get_XML_attribute(host_comp_id, 80, period_setting_element, "period_start_time", XML_file_name, line_number, "period start time of a datamodel instance", "datamodel instance configuration file", true);
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,varname_or_value(period_start_time_config),"Error happens when reading the coupling_connection configuration file, the \"period_start_time\" of datamodel_instance %s should be an integer value, Please verify file %s arround line %d.", datamodel_instance_name, XML_file_name, period_setting_element->Row());
				const char *period_time_format = get_XML_attribute(host_comp_id, 80, period_setting_element, "period_time_format", XML_file_name, line_number, "period format of a datamodel instance", "datamodel instance configuration file", true);
				const char *period_count_config = get_XML_attribute(host_comp_id, 80, period_setting_element, "period_count", XML_file_name, line_number, "period count of a datamodel instance", "datamodel instance configuration file", true);
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,varname_or_value(period_count_config),"Error happens when reading the coupling connection configuration file, the \"period_count\" of datamodel_instance %s should be an integer value, Please verify file %s arround line %d.", datamodel_instance_name, XML_file_name, period_setting_element->Row());
				this->period_unit = set_unit(period_unit_config);
				strcpy(this->period_start_time,period_start_time_config);
				this->id_period_time_format=check_time_format(period_time_format,"period_time_format");
				strcpy(this->period_time_format,period_time_format);
				EXECUTION_REPORT(REPORT_ERROR,host_comp_id,check_time_format(period_time_format,"check_period_time_format"),"The period_time_format for datamodel_instance \"%s\" is not legal, Please check the xml configuration file %s arround line number %d.", datamodel_instance_name, XML_file_name, period_setting_element->Row());
				const char *report_hint = "period_time_format";
				this->period_count = atoi(period_count_config);
				/*check if period_unit and period_time_format match with each other*/
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
	}
}