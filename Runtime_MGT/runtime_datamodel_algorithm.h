/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef _RUNTIME_DATAMODEL_ALGORITHM_H_
#define _RUNTIME_DATAMODEL_ALGORITHM_H_

#include "Runtime_Algorithm_Basis.h"
#include "field_info_mgt.h"
#include "decomp_info_mgt.h"
#include "timer_mgt.h"
#include "cor_global_data.h"
#include "memory_mgt.h"
#include <vector>


struct Datamodel_field_info {
    char field_name_in_IO_file[NAME_STR_SIZE];
    char field_datatype_IO_file[NAME_STR_SIZE];
    Field_mem_info *field_data_mem;
    double scale_factor;
    double add_offset;
    bool have_scale_factor;
};


class Runtime_datamodel_algorithm : public Runtime_algorithm_basis
{
    private:
        std::vector<Datamodel_field_info*> datamodel_fields;
        char datamodel_type[NAME_STR_SIZE];
        char IO_file_type[NAME_STR_SIZE];
        char IO_file_name[NAME_STR_SIZE];
        char write_type[NAME_STR_SIZE];
        IO_netcdf *netcdf_file_object;
        Coupling_timer *io_timer;
        Coupling_timer *change_file_timer;

        void initialize_datamodel(const char * in_file);
        void datamodel_read(void);
        void datamodel_check(void);
        void datamodel_write(void);

    public:
        Runtime_datamodel_algorithm(const char *);    
        ~Runtime_datamodel_algorithm();
        char *get_datamodel_type() { return datamodel_type; }
        void change_IO_file_name_for_restart(const char *);
        virtual void run(bool);
};


#endif

