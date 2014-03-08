/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "fields_add.h"


void fields_add(void **data_src, void **data_dst, int *length)
{
    int npnts;
    int i;
    double *fld_dst = (double *) data_dst[0];
    double *fld0_src = (double *) data_src[0]; 
    double *fld1_src = (double *) data_src[1]; 


    npnts = length[0];
    for (i = 0; i < npnts; i ++) 
        fld_dst[i] = fld0_src[i] + fld1_src[i];
}

