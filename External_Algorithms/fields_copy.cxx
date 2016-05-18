/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "fields_copy.h"
#include <stdio.h>


void fields_copy(void **data_src, void **data_dst, int *length)
{
    int npnts;
    int i;

    npnts = length[0];
    printf("okok npnts: %d\n", npnts);
    fflush(NULL);
    for (i = 0; i < npnts; i ++) {
      ((double **) data_dst)[0][i] = ((double **) data_src)[0][i];
    }
}
