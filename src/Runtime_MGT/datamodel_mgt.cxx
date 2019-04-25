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

int Datamodel_mgt::register_datamodel_output_handler(int num_fields, int *field_ids, const char *output_datamodel_name, int implicit_or_explicit, int sampling_timer_id, int field_instance_ids_size, const char *annotation) {
	int API_id = API_ID_HANDLER_DATAMODEL_OUTPUT, i;
	char datamodel_keyword[NAME_STR_SIZE];
	Inout_datamodel *output_datamodel;
	common_checking_for_datamodel_handler_registration(num_fields, field_ids, implicit_or_explicit, sampling_timer_id, field_instance_ids_size, annotation);
	int host_comp_id = memory_manager->get_field_instance(field_ids[0])->get_comp_id();
	for (i=0; i < output_datamodels.size(); i++) {
		if (words_are_the_same(datamodel_keyword, output_datamodels[i]->get_datamodel_keyword())) {
			output_datamodel = new Inout_datamodel(output_datamodels[i]->get_datamodel_keyword());
			break;
		}
	}
	if (i == datamodel_mgr->output_datamodels.size()) 
		output_datamodel = new Inout_datamodel(host_comp_id, datamodel_keyword, output_datamodel_name, OUTPUT_DATAMODEL);
	output_datamodels.push_back(output_datamodel);
	Output_handler *new_output_handler = new Output_handler(datamodel_keyword, output_datamodel_name, get_next_handler_id(), num_fields, field_ids, implicit_or_explicit, sampling_timer_id, field_instance_ids_size, annotation);
	output_handlers.push_back(new_output_handler);
	return new_output_handler->get_handler_id();
}

Output_handler::Output_handler(char *datamodel_keyword_str, const char *datamodel_name_str, int handler_id, int num_fields, int *field_ids,int implicit_or_explicit, int sampling_timer_id, int field_instance_ids_size, const char *annotation) {
	this->sampling_timer_id = sampling_timer_id;
	this->handler_id = handler_id;
	this->num_fields = num_fields;
	this->sampling_timer_id = sampling_timer_id;
	strcpy(this->datamodel_keyword, datamodel_keyword_str);
	strcpy(this->datamodel_name, datamodel_name_str);
	strcpy(this->annotation, annotation);
	for (int j = 0; j < num_fields; j ++) {
		handler_fields_id.push_back(field_ids[j]);
	}
	this->host_comp_id = memory_manager->get_field_instance(field_ids[0])->get_comp_id();
}

void Datamodel_mgt::common_checking_for_datamodel_handler_registration(int num_fields, int *field_ids, int implicit_or_explicit, int sampling_timer_id, int field_instance_ids_size, const char *annotation) {
	int comp_id = -1;
	char str[NAME_STR_SIZE], API_label[NAME_STR_SIZE];

	get_API_hint(-1, API_ID_HANDLER_DATAMODEL_OUTPUT, API_label);
	EXECUTION_REPORT(REPORT_ERROR, -1, num_fields > 0, "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\", the parameter \"num_field_instances\" (currently is %d) cannot be smaller than 1. Please verify the model code.", API_label, annotation, num_fields);
	EXECUTION_REPORT(REPORT_ERROR, -1, num_fields <= field_instance_ids_size, "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the array size (currently is %d) of parameter \"%s\" cannot be smaller than the parameter \"num_field_instances\" (currently is %d). Please verify the corresponding model code.", API_label, annotation, field_instance_ids_size, "field_instance_ids", num_fields);
	for (int i = 0; i < num_fields; i ++) {
		EXECUTION_REPORT(REPORT_ERROR, -1, memory_manager->check_is_legal_field_instance_id(field_ids[i]) && memory_manager->get_field_instance(field_ids[i])->get_is_registered_model_buf(), "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the parameter \"%s\" contains wrong field instance id (the %dth element of the array is wrong). Please verify the corresponding model code.", API_label, annotation, "field_instance_ids", i+1);
		if (i == 0)
			comp_id = memory_manager->get_field_instance(field_ids[i])->get_comp_id();
		EXECUTION_REPORT_ERROR_OPTIONALLY(REPORT_ERROR, comp_id, comp_id == memory_manager->get_field_instance(field_ids[i])->get_comp_id(), "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the field instances specified via the parameter \"%s\" should but not correspond to the same component model crrently: the first field instance corresponds to the component model \"%s\" while the %dth field instance corresponds to the component model \"%s\". Please verify the model code with the annotation \"%s\".", API_label, annotation, "field_instance_ids", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, false, "")->get_comp_full_name(), i+1, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(memory_manager->get_field_instance(field_ids[i])->get_comp_id(), false, "")->get_comp_full_name());
	}
	if (sampling_timer_id != -1) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, timer_mgr->check_is_legal_timer_id(sampling_timer_id), "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the parameter \"sampling_timer_id\" (currently is 0x%x) is not the legal id of a timer. Please verify the corresponding model code.", API_label, annotation, sampling_timer_id);check_API_parameter_timer(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), "registering a datamodel handler", sampling_timer_id, "sampling_timer_id (the information of the timer)", annotation);
	}
	check_API_parameter_int(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), "registering a datamodel handler", sampling_timer_id, "sampling_timer_id", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_id == timer_mgr->get_timer(sampling_timer_id)->get_comp_id(), "Error happens when calling the API \"%s\" to register a datamodel handler with the annotation \"%s\": the parameter \"sampling_timer_id\" and the parameter \"%s\" do not correspond to the same component model (the parameter \"sampling_timer_id\" corresponds to the component model \"%s\" while \"%s\" corresponds to the component model \"%s\"). Please verify.", API_label, annotation, "field_instance_ids", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(timer_mgr->get_timer(sampling_timer_id)->get_comp_id(), false, "")->get_comp_full_name(), "field_instance_ids", comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, false, "")->get_comp_full_name());
	sprintf(str, "registering a datamodel handler with the annotation \"%s\"", annotation);
	synchronize_comp_processes_for_API(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), str, annotation);
	check_API_parameter_int(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), NULL, num_fields, "num_field_instances", annotation);
	sprintf(str, "\"%s\" (the information of the field instances)", "field_instance_ids");
	for (int i = 0; i < num_fields; i++)
		check_API_parameter_field_instance(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), "registering a datamodel handler", field_ids[i], str, annotation);
	check_API_parameter_int(comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Output_handler:Output_handler"), "registering a datamodel handler", implicit_or_explicit, "implicit_or_explicit (the tag of executing the handler implicitly or explicitly)", annotation);
}

Inout_datamodel::Inout_datamodel(int host_comp_id, char *datamodel_keyword, const char *output_datamodel_name, bool datamodel_type_label) {
	int line_number;
	this->host_comp_id = host_comp_id;
	this->datamodel_type = datamodel_type_label;
	strcpy(this->datamodel_keyword, datamodel_keyword);
	strcpy(datamodel_name, output_datamodel_name);
	sprintf(datamodel_config_dir, "%s/CCPL_dir/datamodel/config",comp_comm_group_mgt_mgr->get_root_working_dir());
	sprintf(datamodel_file_name,"output_datamodel_%s.xml",output_datamodel_name);
	sprintf(XML_file_name,"%s/%s", datamodel_config_dir, datamodel_file_name);
	sprintf(datamodel_data_dir, "%s/CCPL_dir/datamodel/data",comp_comm_group_mgt_mgr->get_root_working_dir());
	strcpy(this->XML_file_name,XML_file_name);
	MPI_Comm comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id, "in register_datamodel_output_handler H2D_grid");

	TiXmlDocument *XML_file = open_XML_file_to_read(host_comp_id, XML_file_name, comm, false);
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
        strcpy(time_format_in_file_names, get_XML_attribute(host_comp_id, NAME_STR_SIZE, data_file_element, "time_format_in_filename", XML_file_name, line_number, "the \"time_format\" in the file name of a datamodel", "datamodel configuration file", true));
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
	char grid_name_str2[NAME_STR_SIZE];
	for (TiXmlNode *hg_node = hgs_node->FirstChild(); hg_node != NULL; hg_node = hg_node->NextSibling()) {
		TiXmlElement *hg_element = hg_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, hg_element, XML_file_name, "the status of a \"horizontal_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, hg_element, "grid_name", XML_file_name, line_number, "the \"grid_name\" of an output_datamodel horizontal_grid","datamodel configuration file",true);
		sprintf(grid_name_str2, "datamodel_%s", grid_name_str);
		const char *specification_str = get_XML_attribute(host_comp_id, 80, hg_element, "specification", XML_file_name, line_number, "the \"specification\" of an output_datamodel horizontal_grid","datamodel configuration file",true);
		if (words_are_the_same(specification_str, "CCPL_grid_file"))
			config_horizontal_grid_via_CCPL_grid_file(hg_node->FirstChild(), grid_name_str2);
		else if (words_are_the_same(specification_str, "grid_data_file_field"))
			config_horizontal_grid_via_grid_data_file_field(hg_node->FirstChild(), grid_name_str2);
		else if (words_are_the_same(specification_str, "uniform_lonlat_grid"))
			config_horizontal_grid_via_uniform_lonlat_grid(hg_node->FirstChild(), grid_name_str2);
		else EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "The \"specification\" of a horizontal_grid in a datamodel configuration file can only be one of \"CCPL_grid_file\", \"grid_data_file_field\", or \"uniform_lonlat_grid\", Please Verify the xml configuration file \"%s\".", XML_file_name);
	}
}

void Inout_datamodel::config_horizontal_grid_via_CCPL_grid_file(TiXmlNode *grid_file_entry_node, const char *grid_name) {
	int line_number, grid_id;
	char file_name[NAME_STR_SIZE];
	const char *CCPL_grid_file_name_str = get_XML_attribute(host_comp_id, 80, grid_file_entry_node->ToElement(), "file_name", XML_file_name, line_number, "the \"file_name\" of an CCPL_grid_file","datamodel configuration file", true);
	sprintf(file_name, "%s/%s", datamodel_data_dir, CCPL_grid_file_name_str);
	grid_id = original_grid_mgr->register_H2D_grid_via_file(host_comp_id, grid_name, file_name, "register H2D grid for datamodel via CCPL_grid_file");
	h2d_grid_ids.push_back(grid_id);
}

void Inout_datamodel::config_horizontal_grid_via_grid_data_file_field(TiXmlNode *file_field_entry_node, const char *grid_name) {
	int line_number;
	char grid_name_str[NAME_STR_SIZE], file_name_str2[NAME_STR_SIZE];
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
	sprintf(file_name_str2, "%s/%s", datamodel_data_dir, file_name_str);
	register_common_h2d_grid_for_datamodel(grid_name, file_name_str2, edge_type_str, coord_unit_str, cyclic_or_acyclic_str, dim_size1_str, dim_size2_str, min_lon_str, min_lat_str, max_lon_str, max_lat_str, center_lon_str, center_lat_str, mask_str, area_str, vertex_lon_str, vertex_lat_str);
}

void Inout_datamodel::register_common_h2d_grid_for_datamodel(const char *grid_name_str, const char *file_name_str, const char *edge_type_str, const char *coord_unit_str, const char *cyclic_or_acyclic_str, const char *dim_size1_str, const char *dim_size2_str, const char *min_lon_str, const char *min_lat_str, const char *max_lon_str, const char *max_lat_str, const char *center_lon_str, const char *center_lat_str, const char *mask_str, const char *area_str, const char *vertex_lon_str, const char *vertex_lat_str) {
	int rcode, ncfile_id, grid_id;
	int size_center_lon=-1, size_center_lat=-1, size_mask=-1, size_area=-1, size_vertex_lon=-1, size_vertex_lat=-1;
	int *mask=NULL;
	long dim_lon_size, dim_lat_size, dim_H2D_size, dim_size1, dim_size2;
	char *center_lon = NULL, *center_lat = NULL, *vertex_lon = NULL, *vertex_lat = NULL, *area = NULL;
	char min_lon[NAME_STR_SIZE], max_lon[NAME_STR_SIZE], min_lat[NAME_STR_SIZE], max_lat[NAME_STR_SIZE];
	char data_type_for_center_lat[NAME_STR_SIZE], data_type_for_center_lon[NAME_STR_SIZE], data_type_for_vertex_lon[NAME_STR_SIZE], data_type_for_vertex_lat[NAME_STR_SIZE], data_type_for_mask[NAME_STR_SIZE], data_type_for_area[NAME_STR_SIZE];
	char data_type_temp[NAME_STR_SIZE];
	char edge_type[NAME_STR_SIZE], cyclic_or_acyclic[NAME_STR_SIZE], coord_unit[NAME_STR_SIZE], annotation[NAME_STR_SIZE];

	MPI_Comm comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id, "in register_datamodel_output_handler H2D_grid");
	bool is_root_proc = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(host_comp_id, "in register_datamodel_output_handler H2D_grid") == 0;
	check_API_parameter_string(host_comp_id, API_ID_HANDLER_DATAMODEL_OUTPUT, comm, "registering an H2D grid for output_datamodel handler", file_name_str, "file_name", "registering an H2D grid for output_datamodel handler");

	IO_netcdf *netcdf_file_object = new IO_netcdf("datamodel_H2D_grid_data", file_name_str, "r", false);
	netcdf_file_object->read_file_field(center_lon_str, (void**)(&center_lon), &size_center_lon, data_type_for_center_lon, comm, is_root_proc);
	netcdf_file_object->read_file_field(center_lat_str, (void**)(&center_lat), &size_center_lat, data_type_for_center_lat, comm, is_root_proc);
	if (vertex_lon_str != NULL)
		netcdf_file_object->read_file_field(vertex_lon_str, (void**)(&vertex_lon), &size_vertex_lon, data_type_for_vertex_lon, comm, is_root_proc);	
	if (vertex_lat_str != NULL)
		netcdf_file_object->read_file_field(vertex_lat_str, (void**)(&vertex_lat), &size_vertex_lat, data_type_for_vertex_lat, comm, is_root_proc);	
	if (area_str != NULL)
		netcdf_file_object->read_file_field(area_str, (void**)(&area), &size_area, data_type_for_area, comm, is_root_proc);
	if (mask_str != NULL)
		netcdf_file_object->read_file_field(mask_str, (void**)(&mask), &size_mask, data_type_for_mask, comm, is_root_proc);
	if (!varname_or_value(dim_size1_str))
		dim_size1 = netcdf_file_object->get_dimension_size(dim_size1_str, comm, is_root_proc);
	if (!varname_or_value(dim_size2_str))
		dim_size2 = netcdf_file_object->get_dimension_size(dim_size2_str, comm, is_root_proc);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, center_lon != NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the longitude value for the center of each grid point (variable \"%s\" in the file) is not specified in the grid file \"%s\".",grid_name_str, datamodel_name, center_lon_str, file_name_str);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, center_lat != NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": the latitude value for the center of each grid point (variable \"%s\" in the file) is not specified in the grid file \"%s\".", grid_name_str, datamodel_name, center_lat_str, file_name_str);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, vertex_lon != NULL && vertex_lat != NULL || vertex_lon == NULL && vertex_lat == NULL, "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the longitude and latitude values for each vertex (variables \"%s\" and \"\"%s in the file) of each grid point must be specified/unspecified at the same time", grid_name_str, datamodel_name, vertex_lon_str, vertex_lat_str, file_name_str);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, data_type_for_center_lat), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\", in the data file \"%s\", the data type of variables \"%s\" and \"%s\" are not the same", grid_name_str, datamodel_name, file_name_str, center_lon_str, center_lat_str);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, DATA_TYPE_FLOAT) || words_are_the_same(data_type_for_center_lon, DATA_TYPE_DOUBLE), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\", in the data file \"%s\", the data type of variables \"%s\" is not floating-point", grid_name_str, datamodel_name, file_name_str, center_lon_str);
	if (vertex_lon != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, data_type_for_vertex_lon), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the data type of varaible \"%s\" is different from the data type of variable \"%s\".", grid_name_str, file_name_str, center_lon_str, vertex_lon_str);
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, data_type_for_vertex_lat),"Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the data type of varaible \"%s\" is different from the data type of variable \"%s\".", grid_name_str, file_name_str, center_lon_str, vertex_lat_str);
	}
	if (area != NULL)
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_center_lon, data_type_for_area), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the data type of varaible \"%s\" is different from the data type of variable \"%s\".", grid_name_str, file_name_str, center_lon_str, vertex_lat_str);
	if (mask != NULL)
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_mask, DATA_TYPE_INT), "Error happens when registering an H2D grid \"%s\" for output_datamodel \"%s\": in the data file \"%s\", the data type of variable \"%s\" is not \"integer\".", grid_name_str, datamodel_name, file_name_str, mask_str);
	strcpy(edge_type, edge_type_str);
	strcpy(cyclic_or_acyclic, cyclic_or_acyclic_str);
	strcpy(coord_unit, coord_unit_str);
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
	sprintf(annotation, "grid %s for datamodel %s", grid_name_str, datamodel_name);
	grid_id = original_grid_mgr->register_H2D_grid_via_global_data(host_comp_id, grid_name_str, edge_type, coord_unit_str, cyclic_or_acyclic, data_type_for_center_lon, dim_size1, dim_size2, size_center_lon, size_center_lat, size_mask, size_area, size_vertex_lon, size_vertex_lat, min_lon, max_lon, min_lat, max_lat, center_lon, center_lat, mask, area, vertex_lon, vertex_lat, annotation, API_ID_HANDLER_DATAMODEL_OUTPUT);
	delete [] center_lon;
	delete [] center_lat;
	if (vertex_lon != NULL) {
		delete [] vertex_lon;
		delete [] vertex_lat;
	}
	if (mask != NULL)
		delete [] mask;
	if (area != NULL)
		delete [] area;
	h2d_grid_ids.push_back(grid_id);
}

void Inout_datamodel::config_horizontal_grid_via_uniform_lonlat_grid(TiXmlNode *uniform_lonlat_grid_entry_node, const char *grid_name) {
	int line_number, grid_id, num_lons, num_lats, dim_h2d_size;
	char annotation[NAME_STR_SIZE], cyclic_or_acyclic[NAME_STR_SIZE];
	char min_lon[NAME_STR_SIZE], min_lat[NAME_STR_SIZE], max_lon[NAME_STR_SIZE], max_lat[NAME_STR_SIZE];
	char *center_lon, *center_lat, *vertex_lon, *vertex_lat, *area;
	char data_type_for_center_lat[NAME_STR_SIZE], data_type_for_center_lon[NAME_STR_SIZE];
	int *mask;
	TiXmlElement *uniform_lonlat_grid_entry_element = uniform_lonlat_grid_entry_node->ToElement();
	const char *num_lons_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "num_lons", XML_file_name, line_number, "the \"num_lons\" of a uniform_lon_lat_grid", "datamodel configuration file", true);
	const char *num_lats_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "num_lats", XML_file_name, line_number, "the \"num_lats\" of a uniform_lon_lat_grid", "datamodel configuration file", true);
	const char *cyclic_or_acyclic_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "cyclic_or_acyclic", XML_file_name, line_number, "the \"cyclic_or_acyclic\" of a grid_data_file_field", "datamodel configuration file",true);
	num_lons = atoi(num_lons_str);
	num_lats = atoi(num_lats_str);
	dim_h2d_size = num_lons * num_lats;
	bool cyclic = false;
	if (words_are_the_same(cyclic_or_acyclic_str, "cyclic"))
		bool cyclic = true;
	const char *min_lon_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "min_lon", XML_file_name, line_number, "the \"min_lon\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	const char *min_lat_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "min_lat", XML_file_name, line_number, "the \"min_lat\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	const char *max_lon_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "max_lon", XML_file_name, line_number, "the \"max_lon\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	const char *max_lat_str = get_XML_attribute(host_comp_id, 80, uniform_lonlat_grid_entry_element, "max_lat", XML_file_name, line_number, "the \"max_lat\" of a grid_data_file_field", "datamodel configuration file", !cyclic);
	transform_datatype_of_arrays(min_lon_str, min_lon, 1);
	transform_datatype_of_arrays(min_lat_str, min_lat, 1);
	transform_datatype_of_arrays(max_lon_str, max_lat, 1);
	transform_datatype_of_arrays(max_lat_str, max_lat, 1);
	sprintf(annotation, "grid %s for datamodel %s", grid_name, datamodel_name);
	center_lon = new char [dim_h2d_size*get_data_type_size(DATA_TYPE_DOUBLE)];
	center_lat = new char [dim_h2d_size*get_data_type_size(DATA_TYPE_DOUBLE)];
	for (int i = 0; i < num_lats; i++) {
		for (int j = 0; j < num_lons; j++) {
			strcpy(center_lon, (j * ((max_lon - min_lon)/(num_lons-1))+min_lon));
			center_lon += (i*num_lons+j)*get_data_type_size(DATA_TYPE_DOUBLE);
			strcpy(center_lat, (i * ((max_lat - min_lat)/(num_lats-1))+min_lat));
			center_lat += (i*num_lons+j)*get_data_type_size(DATA_TYPE_DOUBLE);
		}
	}
	strcpy(cyclic_or_acyclic, cyclic_or_acyclic_str);
	grid_id = original_grid_mgr->register_H2D_grid_via_global_data(host_comp_id, grid_name, "LON_LAT", "degrees", cyclic_or_acyclic, DATA_TYPE_DOUBLE, num_lons, num_lats, dim_h2d_size, dim_h2d_size, 0, 0, dim_h2d_size, dim_h2d_size, min_lon, max_lon, min_lat, max_lat, center_lon, center_lat, NULL, NULL, NULL, NULL, annotation, API_ID_HANDLER_DATAMODEL_OUTPUT);//DATA_TYPE_DOUBLE?no vertexes specified
	delete [] center_lon;
	delete [] center_lat;
	h2d_grid_ids.push_back(grid_id);
}

void Inout_datamodel::config_vertical_grids_for_datamodel(TiXmlNode *vgs_node) {
	int line_number, dim_size2, grid_id, grid_type;
	char coord_unit_str[NAME_STR_SIZE], file_name_str[NAME_STR_SIZE], top_value_str[NAME_STR_SIZE];
	char data_type[NAME_STR_SIZE];
	void *coord_values=NULL, *top_value=NULL, *sigma_values=NULL, *Ap_values=NULL, *Bp_values=NULL;
	void *value2=NULL, *value3=NULL;
	char API_label[NAME_STR_SIZE], annotation[NAME_STR_SIZE], grid_name_str2[NAME_STR_SIZE];
	double temp_value1, *temp_value2, *temp_value3;
	for (TiXmlNode *vg_node = vgs_node->FirstChild(); vg_node != NULL; vg_node = vg_node->NextSibling()) {
		TiXmlElement *vg_element = vg_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, vg_element, XML_file_name, "the status of a \"vertical_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		double temp_value1=NULL, *temp_value2=NULL, *temp_value3=NULL;
		int API_id;
		TiXmlNode *entry_node = vg_node->FirstChild();
		TiXmlElement *entry_element = entry_node->ToElement();
		const char *file_name_str2 = get_XML_attribute(host_comp_id, 80, entry_element, "file_name", XML_file_name, line_number, "the \"file_name\" of a output_datamodel vertical_grid", "datamodel configuration file",true);
		const char *grid_name_str = get_XML_attribute(host_comp_id, 80, vg_element, "grid_name", XML_file_name, line_number, "the \"grid_name\" of an output_datamodel vertical_grid","datamodel configuration file",true);
		const char *specification_str = get_XML_attribute(host_comp_id, 80, vg_element, "specification", XML_file_name, line_number, "the \"specification\" of an output_datamodel vertical_grid","datamodel configuration file",true);
		const char *grid_type_str = get_XML_attribute(host_comp_id, 80, vg_element, "grid_type", XML_file_name, line_number, "the \"grid_type\" of an output_datamodel vertical_grid", "datamodel configuration file",true);
		MPI_Comm comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(host_comp_id, "in register_V1D_grid for output_datamodel");
		bool is_root_proc = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(host_comp_id, "in register_V1D_grid for output_datamodel") == 0;
		sprintf(file_name_str, "%s/%s", datamodel_data_dir, file_name_str2);
		IO_netcdf *netcdf_file_object = new IO_netcdf("V1D_grid_data", file_name_str, "r", false);
		EXECUTION_REPORT(REPORT_LOG, -1, true, "start to register V1D grid %s for output_datamodel", grid_name_str);
		sprintf(annotation,"V1D grid %s for datamodel %s", grid_name_str, datamodel_name);
		sprintf(grid_name_str2, "datamodel_%s", grid_name_str);
		if (words_are_the_same(specification_str, "grid_data_file_field")) {
			if (words_are_the_same(grid_type_str, "Z")) {
				grid_type = 1;
				config_vertical_z_grid(comm, vg_node->FirstChild(), netcdf_file_object, coord_unit_str, &value2, dim_size2, data_type, is_root_proc);
				API_id = API_ID_GRID_MGT_REG_V1D_Z_GRID_VIA_MODEL;
			}
			else if (words_are_the_same(grid_type_str, "SIGMA")) {
				grid_type = 2;
				config_vertical_sigma_grid(comm, vg_node->FirstChild(), netcdf_file_object, coord_unit_str, top_value_str, &value2, dim_size2, data_type, is_root_proc);
				memcpy((char*)value2, (char*)value3, dim_size2);
				API_id = API_ID_GRID_MGT_REG_V1D_SIGMA_GRID_VIA_MODEL;
			}
			else if (words_are_the_same(grid_type_str, "HYBRID")) {
				grid_type = 3;
				config_vertical_hybrid_grid(comm, vg_node->FirstChild(), grid_name_str,  netcdf_file_object, coord_unit_str, top_value_str, &value2, &value3, dim_size2, data_type, is_root_proc);
				API_id = API_ID_GRID_MGT_REG_V1D_HYBRID_GRID_VIA_MODEL;
			}
			else EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "The \"grid_type\" of a vertical_grid in a datamodel configuration file can only be one of \"Z\", \"SIGMA\", or \"HYBRID\", Please Verify the xml configuration file \"%s\".", XML_file_name);
		}
		else EXECUTION_REPORT(REPORT_ERROR, host_comp_id, false, "The \"specification\" of a vertical_grid in a datamodel configuration file can only be \"grid_data_file_field\", Please verify the xml configuration file \"%s\".", XML_file_name);
		temp_value2 = new double [dim_size2];
		temp_value3 = new double [dim_size2];
		if (words_are_the_same(data_type, DATA_TYPE_FLOAT)) {
			if (strlen(top_value_str) != 0)
				float top_value = atof(top_value_str);
			transform_datatype_of_arrays((float*)value2, temp_value2, dim_size2);
			transform_datatype_of_arrays((float*)value3, temp_value3, dim_size2);
		}
		else {
			if (strlen(top_value_str) != 0)
				double top_value = atof(top_value_str);
			transform_datatype_of_arrays((double*)value2, temp_value2, dim_size2);
			transform_datatype_of_arrays((double*)value3, temp_value3, dim_size2);
		}
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, is_array_in_sorting_order(temp_value3, dim_size2) != 0, "Error happens when registering V1D grid \"%s\" for datamodel \"%s\": some arrays of parameters are not in a descending/ascending order. Please check the data file \"%s\".", grid_name_str, datamodel_name, file_name_str);
		grid_id = original_grid_mgr->register_V1D_grid_via_data(API_id, host_comp_id, grid_name_str2, grid_type, coord_unit_str, dim_size2, temp_value1, temp_value2, temp_value3, annotation);
		v1d_grid_ids.push_back(grid_id);
		delete [] temp_value2;
		delete [] temp_value3;
	}
}

void Inout_datamodel::config_vertical_z_grid(MPI_Comm comm, TiXmlNode *entry_node, IO_netcdf *netcdf_file_object, char *coord_unit_str, void **coord_values, int &size_coord_values, char *data_type_for_coord_values, bool is_root_proc) {
	int line_number;
	TiXmlElement *entry_element = entry_node->ToElement();
	strcpy(coord_unit_str,get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of a datamodel vertical_grid","datamodel configuration file", true));
	const char *coord_values_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "coord_values",XML_file_name, line_number, "the \"coord_values\" of a vertical_z_grid", "datamodel configuration file", true);
	netcdf_file_object->read_file_field(coord_unit_str, coord_values, &size_coord_values, data_type_for_coord_values, comm, is_root_proc);
}

void Inout_datamodel::config_vertical_sigma_grid(MPI_Comm comm, TiXmlNode *entry_node, IO_netcdf *netcdf_file_object, char *coord_unit_str, char *top_value_str, void **sigma_values, int &size_sigma_values, char *data_type_for_sigma_values, bool is_root_proc) {
	int line_number;
	TiXmlElement *entry_element = entry_node->ToElement();
	strcpy(coord_unit_str, get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of an datamodel vertical_grid", "datamodel configuration file", true));
	strcpy(top_value_str, get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "top_value", XML_file_name, line_number, "the \"top_value\" of an output_datmaodel vertical_grid", "datamodel configuration file", true));
	const char *sigma_values_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "sigma_values", XML_file_name, line_number, "the \"sigma_values\" of a datamodel vertical grid", "datamodel configuration file", true);
	netcdf_file_object->read_file_field(sigma_values_str, sigma_values, &size_sigma_values, data_type_for_sigma_values, comm, is_root_proc);
}

void Inout_datamodel::config_vertical_hybrid_grid(MPI_Comm comm, TiXmlNode *entry_node, const char *grid_name_str, IO_netcdf *netcdf_file_object, char *coord_unit_str, char *top_value_str, void **Ap_values, void **Bp_values, int &size_Ap, char *data_type_for_ap_values, bool is_root_proc) {
	int line_number, size_Bp;
	char data_type_for_bp_values[NAME_STR_SIZE];
	TiXmlElement *entry_element = entry_node->ToElement();
	strcpy(coord_unit_str, get_XML_attribute(host_comp_id, 80, entry_element, "coord_unit", XML_file_name, line_number, "the \"coord_unit\" of an datamodel vertical_grid", "datamodel configuration file", true));
	strcpy(top_value_str, get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "top_value", XML_file_name, line_number, "the \"top_value\" of an output_datmaodel vertical_grid", "datamodel configuration file", true));
	const char *Ap_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "coef_A", XML_file_name, line_number, "the \"Ap\" values of an datamodel vertical grid", "datamodel configuration file", true);
	const char *Bp_str = get_XML_attribute(host_comp_id, NAME_STR_SIZE, entry_element, "coef_B", XML_file_name, line_number, "the \"Bp\" values of an datamodel vertical grid", "datamodel configuration file", true);
	netcdf_file_object->read_file_field(Ap_str, Ap_values, &size_Ap, data_type_for_ap_values, comm, is_root_proc);
	netcdf_file_object->read_file_field(Bp_str, Bp_values, &size_Bp, data_type_for_bp_values, comm, is_root_proc);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, words_are_the_same(data_type_for_ap_values, data_type_for_bp_values), "Error happens when registering a V1D hybrid gird \"%s\" for datamodel %s, the data type for Ap of variable \"%s\" is different form the data type for Bp of variable \"%s\".", grid_name_str, Ap_str, Bp_str);
	EXECUTION_REPORT(REPORT_ERROR, host_comp_id, size_Ap == size_Bp, "Error happens when registering a V1D hybrid gird \"%s\" for datamodel %s, the array size for Ap of variable \"%s\" is different form the array size for Bp of variable \"%s\".", grid_name_str, Ap_str, Bp_str);
}

void Inout_datamodel::config_v3d_grids_for_datamodel(TiXmlNode *v3ds_node) {
	int line_number, v3d_grid_id, mid_3d_grid_id, mid_1d_grid_id;
	char hg_subgrid_name[NAME_STR_SIZE], vg_subgrid_name[NAME_STR_SIZE];
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
		sprintf(hg_subgrid_name, "datamodel_%s", horizontal_subgrid_name_str);
		sprintf(vg_subgrid_name, "datamodel_%s", vertical_subgrid_name_str);
		int h2d_subgrid_id = original_grid_mgr->get_grid_id(host_comp_id, hg_subgrid_name, "get horizontal_subgrid name for datamodel");
		int v1d_subgrid_id = original_grid_mgr->get_grid_id(host_comp_id, vg_subgrid_name, "get vertical_subgrid name for datamodel");
		bool h2d_registered = false, v1d_registered = false;
		for (int i = 0; i < h2d_grid_ids.size(); i++) {
			if (h2d_grid_ids[i] == h2d_subgrid_id) { 
				h2d_registered = true; 
				break;
			}
		}
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, h2d_registered, "Error happens when registering a V3D_grid \"%s\" for datamodel \"%s\": the horizontal subgrid with name \"%s\" has not been specified in the xml configuration file \"%s\", Please check.", grid_name_str, datamodel_name, horizontal_subgrid_name_str, XML_file_name);
		for (int i = 0; i < v1d_grid_ids.size(); i++) {
			if (v1d_grid_ids[i] == v1d_subgrid_id) {
				v1d_registered = true;
				break;
			}
		}
		EXECUTION_REPORT(REPORT_ERROR, host_comp_id, v1d_registered, "Error happens when registering a V3D_grid \"%s\" for datamodel \"%s\": the verticlal subgrid with name \"%s\" has not been specified in the xml configuration file \"%s\", Please check.", grid_name_str, datamodel_name, vertical_subgrid_name_str, XML_file_name);
		v3d_grid_id = original_grid_mgr->register_md_grid_via_multi_grids(host_comp_id, grid_name_str, h2d_subgrid_id, v1d_subgrid_id, -1, -1, NULL, "register v3d_grid for datamodel");

		if (mid_point_grid_name_str != NULL) {
			char API_label[NAME_STR_SIZE];
			get_API_hint(-1, API_ID_GRID_MGT_REG_MID_POINT_GRID, API_label);
			check_for_ccpl_managers_allocated(API_ID_GRID_MGT_REG_MID_POINT_GRID, "register mid point grid for datamodel");
			check_for_coupling_registration_stage(original_grid_mgr->get_comp_id_of_grid(v3d_grid_id), API_ID_GRID_MGT_REG_MID_POINT_GRID, true, "register mid point grid for datamodel");
			original_grid_mgr->register_mid_point_grid(v3d_grid_id, &mid_3d_grid_id, &mid_1d_grid_id, -1, NULL, "register mid point grid for datamodel", API_label);
			mid_3d_grid_ids.push_back(mid_3d_grid_id);
			mid_1d_grid_ids.push_back(mid_1d_grid_id);
		}

		TiXmlNode *surface_field_node = vertical_sub_grid_node->FirstChild();
		if (surface_field_node != NULL) {//if surface_field_node exists, read it
			TiXmlElement *surface_field_element = surface_field_node->ToElement();
			const char *surface_field_type_str = get_XML_attribute(host_comp_id, 80, surface_field_element, "type", XML_file_name, line_number, "the surface_field \"type\" of a datamodel V3D_grid", "datamodel configuration file",false);
			const char *field_name_in_file_str = get_XML_attribute(host_comp_id, 80, surface_field_element, "field_name_in_file", XML_file_name, line_number, "the surface_field name of a datamodel V3D_grid", "datamodel configuration file", false);
			surface_field_names.push_back(strdup(field_name_in_file_str));
			//if (datamodel_type == INPUT_DATAMODEL) surface_field_type...
		}
	}
}

void Inout_datamodel::config_field_output_settings_for_datamodel(TiXmlNode *field_output_settings_node) {
	int line_number;
	for (TiXmlNode *field_output_setting_node = field_output_settings_node->FirstChild(); field_output_setting_node != NULL; field_output_setting_node = field_output_setting_node->NextSibling()) {
		TiXmlElement *field_output_setting_element = field_output_setting_node->ToElement();
		if (!is_XML_setting_on(host_comp_id, field_output_setting_element, XML_file_name, "the status of a \"horizontal_grid\" node of an output_datamodel", "output_datamodel xml file"))
			continue;
		initialize_output_setting_configurations();
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

void Inout_datamodel::initialize_output_setting_configurations() {
	//
}

void Inout_datamodel::config_default_settings(TiXmlNode *default_settings_node) {
	int line_number;
	TiXmlElement *default_settings_element = default_settings_node->ToElement();

	const char *default_operation_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_operation", XML_file_name, line_number, "The \"default_operation\" of the output_datamodel","output datamodel xml file",false);
	const char *default_h2d_grid_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_h2d_grid", XML_file_name, line_number, "The \"default_h2d_grid\" of the output_datamodel","output datamodel xml file",false);
	const char *default_v3d_grid_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_v3d_grid", XML_file_name, line_number, "The \"default_v3d_grid\" of the output_datamodel","output datamodel xml file",false);
	const char *default_float_type_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_float_type", XML_file_name, line_number, "The \"default_float_type\" of the output_datamodel","output datamodel xml file",false);
	const char *default_integer_type_str = get_XML_attribute(host_comp_id, 80, default_settings_element, "default_integer_type", XML_file_name, line_number, "The \"default_integer_type\" of the output_datamodel","output datamodel xml file",false);
	if (default_operation_str != NULL)
		default_operations.push_back(strdup(default_operation_str));
	else default_operations.push_back(NULL);
	if (default_float_type_str != NULL)
		default_float_types.push_back(strdup(default_float_type_str));
	else default_float_types.push_back(NULL);
	if (default_integer_type_str != NULL)
		default_integer_types.push_back(strdup(default_integer_type_str));
	else default_integer_types.push_back(NULL);
	if (default_h2d_grid_str != NULL)
		default_h2d_grids.push_back(strdup(default_h2d_grid_str));
	else default_h2d_grids.push_back(NULL);
	if (default_v3d_grid_str != NULL)
		default_v3d_grids.push_back(strdup(default_v3d_grid_str));
	else default_v3d_grids.push_back(NULL);
}

void Inout_datamodel::config_output_frequency(TiXmlNode *output_frequency_node) {
	int line_number;
	TiXmlElement *output_frequency_element = output_frequency_node->ToElement();
	const char *file_freq_unit_str = get_XML_attribute(host_comp_id, 80, output_frequency_element, "file_freq_unit", XML_file_name, line_number, "The \"file_freq_unit\" of the output_datamodel","output datamodel xml file",false);
	const char *file_freq_count_str = get_XML_attribute(host_comp_id, 80, output_frequency_element, "file_freq_count", XML_file_name, line_number, "The \"file_freq_count\" of the output_datamodel","output datamodel xml file",false);
	if (file_freq_unit_str != NULL)
		file_freq_units.push_back(strdup(file_freq_unit_str));
	else file_freq_units.push_back(NULL);
	if (file_freq_count_str != NULL)
		file_freq_counts.push_back(strdup(file_freq_count_str));
	else file_freq_counts.push_back(NULL);
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
			get_all_sub_segment_time_slots(outer_segment_node, segment_vector);//create a queue
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

Inout_datamodel::Inout_datamodel(const char *src_output_datamodel_keyword) {
	//
}