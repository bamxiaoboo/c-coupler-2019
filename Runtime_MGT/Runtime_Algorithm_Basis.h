/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef _ALGORITHM_RUNTIME_BASIS_H_
#define _ALGORITHM_RUNTIME_BASIS_H_


class Runtime_algorithm_basis
{
    protected:
        int num_src_fields;
        int num_dst_fields;
        void **src_fields_data_buffers;
        void **dst_fields_data_buffers;
        int *num_elements_in_field_buffers;
        void runtime_algorithm_common_initialize(const int, const int, const int);

    public:
        Runtime_algorithm_basis();
        virtual ~Runtime_algorithm_basis();
        virtual void run(bool) = 0;
};

#endif

