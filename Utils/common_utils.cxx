/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "common_utils.h"
#include <string.h>
#include <stdlib.h>


bool get_next_line(char *line, FILE *fp)
{
    char c;
    int iter = 0;
    

    while (!feof(fp) && (c = getc(fp)) != -1) {
        if (c == '\n') 
            break;
        line[iter ++] = c;
    }
    line[iter ++] = '\0';
    if (iter == 1)
        return false;

    return true;
}


bool get_next_attr(char *attr, char **line)
{
    if ((*line)[0] == '\0') {
        (*line) = NULL;
        return false;
    }

    while ((*line)[0] == ' ' || (*line)[0] == '\t')
        (*line) ++;
    
    while ((*line)[0] != '\0' && (*line)[0] != '\t') {
        *attr = (*line)[0];
        attr ++;
        (*line) ++;
    }
    if ((*line)[0] == '\t')
        (*line) ++;

    if (*attr == ' ' || *attr == '\t') {
        while (*attr == ' ' || *attr == '\t')
            attr --;
        attr ++;
    }
    *attr = '\0';
    
    return true;
}


bool is_end_of_file(FILE *fp)
{
    long offset;

    offset = ftell(fp);
    getc(fp);
    if (feof(fp))
        return true;

    fseek(fp, offset, SEEK_SET);
    return false;
}


int get_next_integer_attr(char **line)
{
    char attr[NAME_STR_SIZE];


    EXECUTION_REPORT(REPORT_ERROR, get_next_attr(attr, line), "Can not read integer value from config file");

    return atoi(attr);
}


int get_next_integer_attr(char **line, bool &has_integer)
{
    char attr[NAME_STR_SIZE];


    has_integer = get_next_attr(attr, line);

	if (has_integer) 
        return atoi(attr);
	else return -1;
}


double get_next_double_attr(char **line)
{
    char attr[NAME_STR_SIZE];


    EXECUTION_REPORT(REPORT_ERROR, get_next_attr(attr, line), "Can not read double value from config file");

    return atof(attr);
}


FILE *open_config_file(const char *config_file_name, const char *config_file_dir)
{
    FILE *cfg_fp;
    char config_file_full_name[NAME_STR_SIZE];

	
    sprintf(config_file_full_name, "%s/%s\0", C_COUPLER_CONFIG_DIR, config_file_dir);
    strcat(config_file_full_name, config_file_name);

    EXECUTION_REPORT(REPORT_ERROR, (cfg_fp = fopen(config_file_full_name, "rb")) != NULL, 
		         "Config file %s under dir %s can not be opened\n", config_file_name, config_file_dir);

    return cfg_fp;
}



FILE *open_config_file(const char *config_file_name)
{
    FILE *cfg_fp;
    char config_file_full_name[NAME_STR_SIZE];

	
    sprintf(config_file_full_name, "%s/\0", C_COUPLER_CONFIG_DIR);
    strcat(config_file_full_name, config_file_name);

    EXECUTION_REPORT(REPORT_ERROR, (cfg_fp = fopen(config_file_full_name, "rb")) != NULL, 
		         "Config file %s can not be opened\n", config_file_name);	    

    return cfg_fp;
}


int get_num_fields_in_config_file(const char *config_file_name, const char *config_file_dir)
{
    FILE *cfg_fp;
    char tmp[NAME_STR_SIZE];
    int num_fields;


	if (words_are_the_same(config_file_name, "NULL"))
		return 0;
	
    cfg_fp = open_config_file(config_file_name, config_file_dir);
    num_fields = 0;
    while (get_next_line(tmp, cfg_fp))
        num_fields ++;

    fclose(cfg_fp);

    return num_fields;
}


