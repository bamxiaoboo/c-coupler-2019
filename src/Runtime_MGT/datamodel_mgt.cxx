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

void tolower(char *string) {
	for (int i = 0; i < strlen(string); i++) {
		if (string[i]<='Z' || string[i]>='A')
			string[i]+=32;
	}
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
}

int Datamodel_mgt::register_datamodel_output_handler(int num_fields, int *field_ids, const char *output_datamodel_name, bool implicit_or_explicit, int sampling_timer_id, const char *annotation) {
	int API_id = API_ID_HANDLER_DATAMODEL_OUTPUT;
	Output_handler *new_output_handler = new Output_handler(output_datamodel_name, get_next_handler_id(), num_fields, field_ids, sampling_timer_id, annotation);
	output_handlers.push_back(new_output_handler);
	return new_output_handler->get_handler_id();
}

Output_handler::Output_handler(const char *datamodel_name, int handler_id, int num_fields, int *field_ids, int sampling_timer_id, const char *annotation) {
	int i;
	for (i=0; i < datamodel_mgr->output_datamodels.size(); i++) {
		if (words_are_the_same(datamodel_name, datamodel_mgr->output_datamodels[i]->get_datamodel_name())) {
			output_datamodel = new Inout_datamodel(datamodel_mgr->output_datamodels[i]);
			break;
		}
	}

	if (i == datamodel_mgr->output_datamodels.size())
		output_datamodel = new Inout_datamodel(host_comp_id, datamodel_name);

	this->handler_id = handler_id;
	this->num_fields = num_fields;
	for (int j = 0; j < num_fields; j ++) {
		handler_fields_id.push_back(field_ids[j]);
	}
	this->sampling_timer_id = sampling_timer_id;
	strcpy(this->annotation, annotation);
	
	this->host_comp_id = memory_manager->get_field_instance(field_ids[1])->get_comp_id();
}

Inout_datamodel::Inout_datamodel(int host_comp_id, const char *output_datamodel_name) {
	int line_number;
	this->host_comp_id = host_comp_id;
	strcpy(datamodel_name, output_datamodel_name);
	sprintf(datamodel_config_dir, "%s/CCPL_dir/datamodel/config",comp_comm_group_mgt_mgr->get_root_working_dir());
	sprintf(datamodel_file_name,"output_datamodel_%s.xml",output_datamodel_name);
	sprintf(XML_file_name,"%s/%s", datamodel_config_dir, datamodel_file_name);
	sprintf(datamodel_data_dir, "%s/CCPL_dir/datamodel/data",comp_comm_group_mgt_mgr->get_root_working_dir());
	strcpy(this->XML_file_name,XML_file_name);

	TiXmlDocument *XML_file = open_XML_file_to_read(host_comp_id, XML_file_name, MPI_COMM_NULL, false);
	if (XML_file == NULL) {
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "Can't find the Datamodel configuration file named %s, Please check directory %s.", datamodel_file_name, datamodel_config_dir);
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
	const char *name_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, output_datamodel_element, "name", XML_file_name, line_number, "The \"name\" of the output_datamodel","output datamodel xml file",true);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(name_str, output_datamodel_name),"The name of the Output_datamodel should be %s, Please verify.",output_datamodel_name);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, is_XML_setting_on(host_comp_id, output_datamodel_element, XML_file_name,"the \"status\" of output_datamodel","output_datamodel config file"),"The status of \"output_datmodel\" node of datamodel \"%s\" should be on, Please check.",output_datamodel_name);
	TiXmlNode *data_files_node = output_datamodel_node->FirstChild();
	TiXmlElement *data_files_element = data_files_node->ToElement();
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_files_element->Value(), "data_files"),"The first node in an Output_datamodel configuration file should be \"data_files\", not \"%s\" as is specified in the file, Please verify.", data_files_element->Value());
	config_data_files_for_datamodel(data_files_node);

	for (TiXmlNode *sub_node = data_files_node->NextSibling(); sub_node != NULL; sub_node = sub_node->NextSibling()) {
		if (words_are_the_same(sub_node->ToElement()->Value(),"horizontal_grids")) 
			config_horizontal_grids_for_datamodel(sub_node);
		if (words_are_the_same(sub_node->ToElement()->Value(),"vertical_grids"))
			config_vertical_grids_for_datamodel(sub_node);
		if (words_are_the_same(sub_node->ToElement()->Value(),"V3D_grids"))
			config_v3d_grids_for_datamodel(sub_node);
		if (words_are_the_same(sub_node->ToElement()->Value(), "fields_output_settings"))
			config_field_output_settings_for_datamodel(sub_node);
	}
}

void Inout_datamodel::config_data_files_for_datamodel(TiXmlNode *data_files_node) {
	TiXmlNode *data_file_node;
	int line_number,pos_last_star,i, num_stars=0;
	bool is_data_file_configured = false;
	char file_name[NAME_STR_SIZE],file_type_str[NAME_STR_SIZE];

	for (data_file_node = data_files_node->FirstChild(); data_file_node != NULL; data_file_node = data_file_node->NextSibling()) {
		TiXmlElement *data_file_element = data_file_node->ToElement();
		if (is_data_file_configured && is_XML_setting_on(host_comp_id, data_file_element, XML_file_name,"the status of a data_file node of an output_datamodel", "output_datamodel xml file"))
			EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "In output_datamodel configuration file\"%s\", only one \"status\" of the \"data_file\" node can be set on, Please Verify.", XML_file_name);
		if (is_data_file_configured)
			continue;
		is_data_file_configured = true;
		const char *file_names_str = get_XML_attribute(host_comp_id, 80, data_file_element, "file_names", XML_file_name, line_number, "the \"file_names\" of an output_datamodel", "output_datamodel configuration file", true);
		if (file_names_str[0] != '/')
			sprintf(datamodel_files_dir_name, "%s/%s", datamodel_data_dir, file_names_str);
		else
			strcpy(datamodel_files_dir_name, file_names_str);
		for (i = strlen(datamodel_files_dir_name); i != 0; i--) {
			if (datamodel_files_dir_name[i] == '/')
				break;
		}
		strncpy(file_dir, datamodel_files_dir_name, i+1);
		file_dir[i+1]='\0';
		strcpy(file_name, datamodel_files_dir_name+i+1);
		for (i = 0; i < strlen(file_name); i++) {
			if (file_name[i] == '*') {
				num_stars ++;
				pos_last_star = i;
			}
		}
		strncpy(file_name_prefix, file_name, pos_last_star);
		file_name_prefix[pos_last_star] = '\0';
		strcpy(file_name_suffix, file_name+pos_last_star+1);
        strcpy(time_format_in_file_names,get_XML_attribute(host_comp_id, NAME_STR_SIZE, data_file_element, "time_format_in_filename", XML_file_name, line_number, "the \"time_format\" in the file name of a datamodel", "datamodel configuration file", true));
        //int id_time_format_in_file_names = check_time_format(host_comp_id, time_format_in_file_names, "time_format_in_file_name");
        EXECUTION_REPORT(REPORT_ERROR,host_comp_id,num_stars == 1,"Error happens when reading the XML configuration file %s arround line number %d, only one * can be specified in data_file names, there are %d detected, Pleas Verify.", XML_file_name, data_file_element->Row(), num_stars);//if there are no * s in filename, do not read time_format_in_filename
        strcpy(file_type_str,get_XML_attribute(host_comp_id, 80, data_file_element, "file_type", XML_file_name, line_number, "the \"file_type\" of an output_datamodel", "datamodel configuration file", true));
        tolower(strdup(file_type_str));
        if (words_are_the_same(file_type_str, "netcdf"))
        	this->file_type = 1;
	}
}

void Inout_datamodel::config_horizontal_grids_for_datamodel(TiXmlNode *hgs_node) {
	int line_number;
	for (TiXmlNode *hg_node = hgs_node->FirstChild(); hg_node != NULL; hg_node = hg_node->NextSibling()) {
		TiXmlElement *hg_element = hg_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, hg_element, XML_file_name, "the status of a \"horizontal_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, hg_element, "grid_name", XML_file_name, line_number, "the \"grid_name\" of an output_datamodel horizontal_grid","datamodel configuration file",true);
		const char *specification_str = get_XML_attribute(host_comp_id, 80, hg_element, "specification", XML_file_name, line_number, "the \"specification\" of an output_datamodel horizontal_grid","datamodel configuration file",true);
		if (words_are_the_same(specification_str, "CCPL_grid_file"))
			config_horizontal_grid_via_CCPL_grid_file(hg_node->FirstChild());
		else if (words_are_the_same(specification_str, "grid_data_file_field"))
			config_horizontal_grid_via_grid_data_file_field(hg_node->FirstChild());
		else if (words_are_the_same(specification_str, "uniform_lonlat_grid"))
			config_horizontal_grid_via_uniform_lonlat_grid(hg_node->FirstChild());
		else EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "The \"specification\" of a horizontal_grid in a datamodel configuration file can only be one of \"CCPL_grid_file\", \"grid_data_file_field\", or \"uniform_lonlat_grid\", Please Verify the xml configuration file \"%s\".", XML_file_name);
	}
}

void Inout_datamodel::config_horizontal_grid_via_CCPL_grid_file(TiXmlNode *grid_file_entry_node) {
	int line_number;
	const char *CCPL_grid_file_name_str = get_XML_attribute(host_comp_id, 80, grid_file_entry_node->ToElement(), "file_name", XML_file_name, line_number, "the \"file_name\" of an CCPL_grid_file","datamodel configuration file", true);
}

void Inout_datamodel::config_horizontal_grid_via_grid_data_file_field(TiXmlNode *file_field_entry_node) {
	int line_number;
	TiXmlElement *file_field_entry_element = file_field_entry_node->ToElement();
	const char *file_name_str = get_XML_attribute(host_comp_id, 80, file_field_entry_element, "file_name", XML_file_name, line_number, "the \"file_name\" of a grid_data_file_field", "datamodel configuration file",false);
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
}

void Inout_datamodel::config_horizontal_grid_via_uniform_lonlat_grid(TiXmlNode *uniform_lonlat_grid_entry_node) {
	int line_number;
	TiXmlElement *uniform_lonlat_grid_entry_element = uniform_lonlat_grid_entry_node->ToElement();
	const char *num_lons = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "num_lons", XML_file_name, line_number, "the \"num_lons\" of a uniform_lon_lat_grid", "datamodel configuration file", true);
	const char *num_lats = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "num_lats", XML_file_name, line_number, "the \"num_lats\" of a uniform_lon_lat_grid", "datamodel configuration file", true);
	const char *cyclic_or_acyclic_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "cyclic_or_acyclic", XML_file_name, line_number, "the \"cyclic_or_acyclic\" of a grid_data_file_field", "datamodel configuration file",true);
	bool cyclic = false;
	if (words_are_the_same(cyclic_or_acyclic_str, "cyclic"))
		bool cyclic = true;
	const char *min_lon_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "min_lon", XML_file_name, line_number, "the \"min_lon\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	const char *min_lat_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "min_lat", XML_file_name, line_number, "the \"min_lat\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	const char *max_lon_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "max_lon", XML_file_name, line_number, "the \"max_lon\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	const char *max_lat_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "max_lat", XML_file_name, line_number, "the \"max_lat\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
}

void Inout_datamodel::config_vertical_grids_for_datamodel(TiXmlNode *vgs_node) {
	int line_number;
	for (TiXmlNode *vg_node = vgs_node->FirstChild(); vg_node != NULL; vg_node = vg_node->NextSibling()) {
		TiXmlElement *vg_element = vg_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, vg_element, XML_file_name, "the status of a \"vertical_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		const char *file_name_str = get_XML_attribute(host_comp_id, 80, vg_element, "file_name", XML_file_name, line_number, "the \"file_name\" of a output_datamodel vertical_grid", "datamodel configuration file",false);
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, vg_element, "grid_name", XML_file_name, line_number, "the \"grid_name\" of an output_datamodel vertical_grid","datamodel configuration file",true);
		const char *specification_str = get_XML_attribute(host_comp_id, 80, vg_element, "specification", XML_file_name, line_number, "the \"specification\" of an output_datamodel vertical_grid","datamodel configuration file",true);
		const char *grid_type_str = get_XML_attribute(host_comp_id, 80, vg_element, "grid_type", XML_file_name, line_number, "the \"grid_type\" of an output_datamodel vertical_grid", "datamodel configuration file",true);
		if (words_are_the_same(specification_str, "grid_data_file_field")) {
			if (words_are_the_same(grid_type_str, "Z"))
				config_vertical_z_grid(vg_node->FirstChild());
			else if (words_are_the_same(grid_type_str, "SIGMA"))
				config_vertical_sigma_grid(vg_node->FirstChild());
			else if (words_are_the_same(grid_type_str, "HYBRID"))
				config_vertical_hybrid_grid(vg_node->FirstChild());
			else EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "The \"grid_type\" of a horizontal_grid in a datamodel configuration file can only be one of \"Z\", \"SIGMA\", or \"HYBRID\", Please Verify the xml configuration file \"%s\".", XML_file_name);
		}
	}
}

void Inout_datamodel::config_vertical_z_grid(TiXmlNode *entry_node) {
	int line_number;
	TiXmlElement *entry_element = entry_node->ToElement();
	const char *coord_unit_str = get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of an output_datamodel vertical_grid","datamodel configuration file", true);
	const char *coord_values_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "coord_values",XML_file_name, line_number, "the \"coord_values\" of a vertical_z_grid", "datamodel configuration file", true);
}

void Inout_datamodel::config_vertical_sigma_grid(TiXmlNode *entry_node) {
	int line_number;
	TiXmlElement *entry_element = entry_node->ToElement();
	const char *coord_unit_str = get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of an output_datamodel vertical_grid", "datamodel configuration file", true);
	const char *top_value_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "top_value", XML_file_name, line_number, "the \"top_value\" of an output_datmaodel vertical_grid", "datamodel configuration file", true);
	const char *sigma_values_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "sigma_values", XML_file_name, line_number, "the \"sigma_values\" of an output_datamodel vertical grid", "datamodel configuration file", true);
}

void Inout_datamodel::config_vertical_hybrid_grid(TiXmlNode *entry_node) {
	int line_number;
	TiXmlElement *entry_element = entry_node->ToElement();
	const char *coord_unit_str = get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of an output_datamodel vertical_grid", "datamodel configuration file", true);
	const char *top_value_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "top_value", XML_file_name, line_number, "the \"top_value\" of an output_datmaodel vertical_grid", "datamodel configuration file", true);
	const char *Ap_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "Ap", XML_file_name, line_number, "the \"Ap\" values of an output_datamodel vertical grid", "datamodel configuration file", true);
	const char *Bp_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "Bp", XML_file_name, line_number, "the \"Bp\" values of an output_datamodel vertical grid", "datamodel configuration file", true);
}

void Inout_datamodel::config_v3d_grids_for_datamodel(TiXmlNode *v3ds_node) {
	int line_number;
	for (TiXmlNode *v3d_node = v3ds_node->FirstChild(); v3d_node != NULL; v3d_node = v3d_node->NextSibling()) {
		TiXmlElement *v3d_element = v3d_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, v3d_element, XML_file_name, "the status of a \"3d_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, v3d_element, "grid_name", XML_file_name, line_number, "the \"grid_name\" of an output_datamodel 3d_grid","datamodel configuration file",true);
		const char *mid_point_grid_name_str = get_XML_attribute(host_comp_id, 80, v3d_element, "mid_point_grid_name", XML_file_name, line_number, "the \"mid_point_grid_name\" of an output_datamodel vertical_grid","datamodel configuration file",false);
		TiXmlNode *horizontal_sub_grid_node = v3d_node->FirstChild();
		TiXmlElement *horizontal_grid_element = horizontal_sub_grid_node->ToElement();
		const char *horizontal_subgrid_name_str = get_XML_attribute(host_comp_id, 80, horizontal_grid_element, "grid_name",XML_file_name, line_number, "the \"grid_name\" of the horizontal_subgrid of an output_datamodel v3d_grid","datamodel configuration file",true);
		TiXmlNode *vertical_sub_grid_node = horizontal_sub_grid_node->NextSibling();
		TiXmlElement *vertical_sub_grid_element = vertical_sub_grid_node->ToElement();
		const char *vertical_subgrid_name_str = get_XML_attribute(host_comp_id, 80, vertical_sub_grid_element, "grid_name",XML_file_name, line_number, "the \"grid_name\" of the vertical_subgrid of an output_datamodel v3d_grid","datamodel configuration file", true);
		TiXmlNode *surface_field_node = vertical_sub_grid_node->FirstChild();
		if (surface_field_node != NULL) {
			TiXmlElement *surface_field_element = surface_field_node->ToElement();
			const char *surface_field_type_str = get_XML_attribute(host_comp_id, 80, surface_field_element, "type", XML_file_name, line_number, "the surface_field \"type\" of an output_datamodel V3D_grid", "datamodel configuration file",false);
			const char *field_name_in_file_str = get_XML_attribute(host_comp_id, 80, surface_field_element, "field_name_in_file", XML_file_name, line_number, "the surface_field name of an output_datamodel V3D_grid", "datamodel configuration file", false);
		}
	}
}

void Inout_datamodel::config_field_output_settings_for_datamodel(TiXmlNode *field_output_settings_node) {
	int line_number;
	for (TiXmlNode *field_output_setting_node = field_output_settings_node->FirstChild(); field_output_setting_node != NULL; field_output_setting_node = field_output_setting_node->NextSibling()) {
		TiXmlElement *field_output_setting_element = field_output_setting_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, field_output_setting_element, XML_file_name, "the status of a \"horizontal_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		for (TiXmlNode *sub_node = field_output_setting_node->FirstChild(); sub_node != NULL; sub_node = sub_node->NextSibling()) {
			TiXmlElement *sub_element = sub_node->ToElement();
			if (words_are_the_same(sub_element->Value(), "default_settings"))
				config_default_settings(sub_node);
			if (words_are_the_same(sub_element->Value(), "output_frequency"))
				config_output_frequency(sub_node);
			if (words_are_the_same(sub_element->Value(), "fields"))
				config_field_info(sub_node);
		}
	}
}

void Inout_datamodel::config_default_settings(TiXmlNode *default_settings_node) {
	int line_number;
	TiXmlElement *default_settings_element = default_settings_node->ToElement();
	const char *default_operation_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_operation", XML_file_name, line_number, "The \"default_operation\" of the output_datamodel","output datamodel xml file",false);
	const char *default_h2d_grid_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_h2d_grid", XML_file_name, line_number, "The \"default_h2d_grid\" of the output_datamodel","output datamodel xml file",false);
	const char *default_float_type_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_float_type", XML_file_name, line_number, "The \"default_float_type\" of the output_datamodel","output datamodel xml file",false);
	const char *default_integer_type_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_integer_type", XML_file_name, line_number, "The \"default_integer_type\" of the output_datamodel","output datamodel xml file",false);
}

void Inout_datamodel::config_output_frequency(TiXmlNode *output_frequency_node) {
	int line_number;
	TiXmlElement *output_frequency_element = output_frequency_node->ToElement();
	const char *file_freq_unit_str = get_XML_attribute(host_comp_id, 80, output_frequency_element, "file_freq_unit", XML_file_name, line_number, "The \"file_freq_unit\" of the output_datamodel","output datamodel xml file",false);
	const char *file_freq_count_str = get_XML_attribute(host_comp_id, 80, output_frequency_element, "file_freq_count", XML_file_name, line_number, "The \"file_freq_count\" of the output_datamodel","output datamodel xml file",false);
	bool is_outer_segment_node_found = false;
	for (TiXmlNode *outer_segment_node = output_frequency_node->FirstChild(); outer_segment_node != NULL; outer_segment_node = outer_segment_node->NextSibling()) {
		TiXmlElement *outer_segment_element = outer_segment_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, outer_segment_element, XML_file_name, "the status of a \"segment\" node of an output_datamodel output_frequency_setting", "output_datamodel xml file"))
			continue;
		else if (is_outer_segment_node_found)
			EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "In output_datamodel configuration file\"%s\", only one \"status\" of the outmost \"segment\" node can be set on, Please Verify.", XML_file_name);
		else is_outer_segment_node_found = true;
		const char *freq_count_str = get_XML_attribute(host_comp_id, 80, outer_segment_element, "freq_count", XML_file_name, line_number, "The \"freq_count\" of the output_datamodel","output datamodel xml file",false);
		const char *freq_unit_str = get_XML_attribute(host_comp_id, 80, outer_segment_element, "freq_unit", XML_file_name, line_number, "The \"freq_unit\" of the output_datamodel","output datamodel xml file",false);
		std::vector<TiXmlNode*> segment_vector;
		int valid_segment_start_pos = 0;
		//第一层可能是time_points_node
		if (is_expected_segment(outer_segment_node->FirstChild(), "time_slots"))
			get_all_sub_segment_time_slots(outer_segment_node, segment_vector);
		else if (is_expected_segment(outer_segment_node->FirstChild(), "time_points"))
			visit_time_points_node(outer_segment_node->FirstChild());
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
}

bool Inout_datamodel::is_expected_segment(TiXmlNode *undecided_node, const char *expected_str) {
	int line_number;
	TiXmlElement *undecided_element = undecided_node->ToElement();
	const char *undecided_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, undecided_element, expected_str, XML_file_name, line_number, "The \"time_slots\" or \"time_points\" of the output_datamodel","output datamodel xml file",false);
	if (undecided_str == NULL)
		return false;
	else return true;
}

void Inout_datamodel::get_all_sub_segment_time_slots(TiXmlNode *outer_segment_node, std::vector<TiXmlNode*> global_segment_vector) {
	for (TiXmlNode *internal_segment_node = outer_segment_node->FirstChild(); internal_segment_node != NULL; internal_segment_node = internal_segment_node->NextSibling()) {
		if (is_expected_segment(internal_segment_node, "time_slots"))
			global_segment_vector.push_back(internal_segment_node);
	}
}

void Inout_datamodel::visit_time_slots_node(TiXmlNode *time_slot_segment_node) {
	int line_number;
	TiXmlElement *time_slot_segment_element = time_slot_segment_node->ToElement();
	if (!is_XML_setting_on(host_comp_id, time_slot_segment_element, XML_file_name,"the status of the time_points \"segment\" of an output_datamodel", "output_datamodel configuration file"))
		return;
	const char *time_slots_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, time_slot_segment_element, "time_slots", XML_file_name, line_number, "The \"time_slots\" of the output_datamodel","output datamodel xml file",true);
	const char *time_slot_format_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, time_slot_segment_element, "time_slot_format", XML_file_name, line_number, "The \"time_slot_format\" of the output_datamodel","output datamodel xml file",true);
	const char *freq_count_str = get_XML_attribute(host_comp_id, 80, time_slot_segment_element, "freq_count", XML_file_name, line_number, "The \"freq_count\" of the output_datamodel","output datamodel xml file",false);
	const char *freq_unit_str = get_XML_attribute(host_comp_id, 80, time_slot_segment_element, "freq_unit", XML_file_name, line_number, "The \"freq_unit\" of the output_datamodel","output datamodel xml file",false);
	TiXmlNode *sub_segment_node = time_slot_segment_node->FirstChild();
	if (is_expected_segment(sub_segment_node, "time_points"))
		visit_time_points_node(sub_segment_node);
}

void Inout_datamodel::visit_time_points_node(TiXmlNode *time_points_segment_node) {
	int line_number;
	TiXmlElement *time_points_segment_element = time_points_segment_node->ToElement();
	if (!is_XML_setting_on(host_comp_id, time_points_segment_element, XML_file_name,"the status of the time_points \"segment\" of an output_datamodel", "output_datamodel configuration file"))
		return;
	const char *time_points_str = get_XML_attribute(host_comp_id, 80, time_points_segment_element, "time_points", XML_file_name, line_number, "The \"time_points\" of the output_datamodel","output datamodel xml file",true);
	const char *time_point_format = get_XML_attribute(host_comp_id, 80, time_points_segment_element, "time_point_format", XML_file_name, line_number, "The \"time_point_format\" of the output_datamodel","output datamodel xml file",false);
	const char *time_point_unit = get_XML_attribute(host_comp_id, 80, time_points_segment_element, "time_point_unit", XML_file_name, line_number, "The \"time_point_unit\" of the output_datamodel","output datamodel xml file",false);
}

void Inout_datamodel::config_field_info(TiXmlNode *fields_node) {
	for (TiXmlNode *field_node = fields_node->FirstChild(); field_node != NULL; field_node = field_node->NextSibling()) {
		TiXmlElement *field_element = field_node->ToElement();
		const char *name_in_model_str = get_XML_attribute(host_comp_id, 80, field_element, "name_in_model", XML_file_name, line_number, "The \"name_in_model\" of the output_datamodel field","output datamodel xml file",true);
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, field_element, "grid_name", XML_file_name, line_number, "The \"grid_name\" of the output_datamodel field","output datamodel xml file",false);
		const char *name_in_file_str = get_XML_attribute(host_comp_id, 80, field_element, "name_in_file", XML_file_name, line_number, "The \"name_in_file\" of the output_datamodel field","output datamodel xml file",false);
		const char *float_datatype_str = get_XML_attribute(host_comp_id, 80, field_element, "float_datatype", XML_file_name, line_number, "The \"float_datatype\" of the output_datamodel field","output datamodel xml file",false);
		const char *integer_datatype_str = get_XML_attribute(host_comp_id, 80, field_element, "integer_datatype", XML_file_name, line_number, "The \"integer_datatype\" of the output_datamodel field","output datamodel xml file",false);
		const char *operation_str = get_XML_attribute(host_comp_id, 80, field_element, "operation", XML_file_name, line_number, "The \"operation\" of the output_datamodel field","output datamodel xml file",false);
		const char *unit_str = get_XML_attribute(host_comp_id, 80, field_element, "unit", XML_file_name, line_number, "The \"unit\" of the output_datamodel field","output datamodel xml file",false);
	}
}

Inout_datamodel::Inout_datamodel(Inout_datamodel *src_output_datamodel) {
	//
}