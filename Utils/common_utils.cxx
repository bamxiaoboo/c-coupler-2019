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
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


void write_data_into_array_buffer(const void *data, int data_size, char **temp_array_buffer, int &buffer_max_size, int &buffer_content_size)
{
	if (*temp_array_buffer == NULL) {
		buffer_max_size = 2 * data_size;
		buffer_content_size = 0;
		*temp_array_buffer = new char [buffer_max_size];
	}

	if (buffer_max_size < buffer_content_size+data_size) {
		buffer_max_size = (buffer_content_size+data_size) * 2;
		char *temp_buffer = new char [buffer_max_size];
		for (int i = 0; i < buffer_content_size; i ++)
			temp_buffer[i] = (*temp_array_buffer)[i];
		delete [] *temp_array_buffer;
		*temp_array_buffer = temp_buffer;
	}

	for (int i = 0; i < data_size; i ++)
		(*temp_array_buffer)[buffer_content_size++] = ((char*)data)[i];
}


void read_data_from_array_buffer(void *data, int data_size, const char *temp_array_buffer, int &buffer_content_iter)
{
	EXECUTION_REPORT(REPORT_ERROR,-1, data_size <= buffer_content_iter, "Software error in read_data_from_array_buffer");
	
	for (int i = 0; i < data_size; i ++)
		((char*) data)[i] = temp_array_buffer[buffer_content_iter-data_size+i];
	
	buffer_content_iter -= data_size;
}


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
	EXECUTION_REPORT(REPORT_ERROR, *line != NULL, "Can not get next attribute from the configuration file. There may be problem in the configuration file");
	
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


bool get_next_integer_attr(char **line, int &value)
{
    char attr[NAME_STR_SIZE];

	
	value = -1;

    if (!get_next_attr(attr, line))
		return false;
	for (int i = 0; i < strlen(attr); i ++) {
		if (i == 0 && attr[0] == '-')
			continue;
        if (attr[i]-'0'< 0 || attr[i]-'9' > 0) 
            return false;		
	}
    value = atoi(attr);

	return true;
}


bool get_next_double_attr(char **line, double &value)
{
    char attr[NAME_STR_SIZE];
	int dot_number = 0;
	int figure_number = 0;


	if (!get_next_attr(attr, line))
		return false;

	for (int i = 0; i < strlen(attr); i ++) {
		if (i == 0 && attr[0] == '-')
			continue;
		if (attr[i] == '.') {
			dot_number ++;
			if (figure_number == 0)
				return false;
			if (dot_number > 1)
				return false;
			continue;
		}
        if (attr[i]-'0' < 0 || attr[i]-'9' > 0) 
            return false;
		figure_number ++;
	}
    value = atof(attr);

	return true;
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


void create_directory(const char *path, bool is_root_proc)
{
	if (is_root_proc) {
		printf("path1 is %s\n", path);
		DIR *dir=opendir(path);
		if (dir == NULL) {
			printf("path2 is %s\n", path);
			int is_create = mkdir(path, 0777);
			EXECUTION_REPORT(REPORT_ERROR, is_create == 0, "directory \"%s\" cannot be created. Please check why.", path);
		}
	}	
}

