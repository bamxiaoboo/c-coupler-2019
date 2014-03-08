/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "fields_mult.h"
#include <stdio.h>


void fields_mult(void **data_src, void **data_dst, int *length)
{
    int num_fields;
    int npnts;
    int i,j;
    double *mult_factor;


    npnts = length[0];
    num_fields = length[1];
    mult_factor = (double *) data_src[num_fields-1];
    for (i = 0; i < npnts; i ++) {
        for (j = 0; j < num_fields-1; j ++) { 
            ((double **) data_dst)[j][i] = mult_factor[i] * ((double **) data_src)[j][i];
        }
    }
}
