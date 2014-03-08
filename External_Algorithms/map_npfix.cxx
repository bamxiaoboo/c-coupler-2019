/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <stdio.h>
#include "map_npfix.h"

extern "C" void map_npfixnew_(double *, double *, double *, double *, double *, double *, double *, double *, double *, int *, int *, int *, int *);

void map_npfix(void ** buf_src, void ** buf_dst, int * length)
{
	double ** data_src = (double **) buf_src;
	double ** data_dst = (double **) buf_dst;
	int * ni = (int *)data_src[7]; 
	int * nj = (int *)data_src[8];
	
	map_npfixnew_(data_src[0], data_src[1], data_src[2], data_src[3], data_src[4], data_src[5], data_src[6], data_dst[0], data_dst[1], ni, nj, &length[0], &length[1]);
}
