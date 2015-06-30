/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/



#ifndef IO_NETCDF
#define IO_NETCDF


#include "io_basis.h"
#include <netcdf.h>
#include "remap_weight_of_strategy_class.h"


class IO_netcdf: public IO_basis
{
    private:
        int ncfile_id;
        int rcode;
        bool io_with_time_info;
        int time_dim_id;
        int time_count;
		bool is_external_file;
        
        void write_grid(Remap_grid_class*, bool);
        void write_field_data(Remap_grid_data_class*, Remap_grid_class*, bool, const char*, int, bool);
        void datatype_from_netcdf_to_application(nc_type, char*, const char*);
        void datatype_from_application_to_netcdf(const char*, nc_type*);
        void report_nc_error();

    public:
		IO_netcdf(int);
        IO_netcdf(const char*, const char*, const char*, bool);
        ~IO_netcdf();
        void read_data(Remap_data_field*, int);
        void write_grided_data(Remap_grid_data_class*, bool, int, int, bool);
        void write_remap_weights(Remap_weight_of_strategy_class*);
        long get_dimension_size(const char*);
        void read_remap_weights(Remap_weight_of_strategy_class*, Remap_strategy_class*, bool);
        void put_global_text(const char*, const char*);
        void get_global_text(const char*, char*, int);
		void read_file_field(const char*, void**, int*, int**, char*);
};


#endif 
