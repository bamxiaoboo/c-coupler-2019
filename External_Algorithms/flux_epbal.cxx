/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <iostream>
#include <cmath>
#include <cstring>
#include "flux_epbal.h"

/*input field list*/
#define FAXC_RAIN 0
#define FAXC_SNOW 1
#define FAII_EVAP 2
#define FAOC_EVAP 3 
#define FORR_ROFF 4
#define IFRAC 5
#define AFRAC 6
#define MASK 7
#define AREAR 8
//TODO: ADD THIS CONTROL FIELD
#define CPL_FLUXEPBAL 9
#define CPL_FLUXEPFAC 10

/*output field list*/
#define K_XRAIN 0
#define K_XSNOW 1
#define K_OROFF 2


extern "C" void flux_epbal_(double*, double*, double*, double*, double*, double*, double*, double*, bool*, int*, char*, double*);

void flux_epbal(void ** buf_src, void ** buf_dst, int * len)
{
 
    ///Point to the parameter.
    int * length = len;
    double ** data_src = (double **) buf_src;
    double ** data_dst = (double **) buf_dst;

    flux_epbal_(data_src[FAII_EVAP], data_src[FAOC_EVAP], data_src[FAXC_RAIN], data_src[FAXC_SNOW], data_src[FORR_ROFF], data_src[IFRAC], data_src[AFRAC],
              data_src[AREAR], (bool*) data_src[MASK], len, (char *) data_src[CPL_FLUXEPBAL], data_src[CPL_FLUXEPFAC]);
}

