/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"


int c_coupler_get_field_size(void *model_buf, const char *annotation)
{
    EXECUTION_REPORT(REPORT_ERROR, memory_manager != NULL, 
				     "C-Coupler interface coupling_interface_initialize has not been called before running the code corresponding to annotation \"%s\"\n", annotation); 
    return memory_manager->get_field_size(model_buf, annotation);
}



