/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "flux_albi.h"
#include <math.h>
#include <iostream>
#include <string.h>

/*input field list*/
#define LAT				0
#define LON				1
#define CPL_FLUXALBAV	2
#define CPL_ORBECCEN	3
#define CPL_ORBOBLIQR	4
#define CPL_ORBLAMBM0	5
#define CPL_ORBMVELPP	6
#define CALDAY			7

/*output field list*/
#define ANIDR 0
#define AVSDR 1
#define ANIDF 2
#define AVSDF 3

extern "C" void flux_albi_thucpl_(double*, double*, bool*, double*, double*, 
                                  double*, double*, double*, int*, double*,
                                  double*, double*, double*);


void flux_albi(void ** buf_src, void ** buf_dst, int *length)
{
    double ** data_src;
    double ** data_dst;
    bool *fluxAlbav;
    double *orbEccen, *orbObliqr, *orbLambm0, *orbMvelpp;
    double *albedo_shift;
    double *lats, *lons;

    data_src = (double **) buf_src;
    data_dst = (double **) buf_dst;

    lats = data_src[LAT];
    lons = data_src[LON];
    fluxAlbav = (bool *) data_src[CPL_FLUXALBAV];
    orbEccen = data_src[CPL_ORBECCEN];
    orbObliqr = data_src[CPL_ORBOBLIQR];
    orbLambm0 = data_src[CPL_ORBLAMBM0];
    orbMvelpp = data_src[CPL_ORBMVELPP];
    albedo_shift = data_src[CALDAY];

    flux_albi_thucpl_(lats, lons, fluxAlbav, orbEccen, orbMvelpp,
                                  orbLambm0, orbObliqr, albedo_shift, length, data_dst[ANIDR],
                                  data_dst[AVSDR], data_dst[ANIDF], data_dst[AVSDF]);
}

