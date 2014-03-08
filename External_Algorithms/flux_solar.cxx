/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <iostream>
#include <cstring>
#include <cmath>
#include "flux_solar.h"

/*input field list*/
#define ANIDR 0
#define AVSDR 1
#define ANIDF 2
#define AVSDF 3
#define SWNDR 4
#define SWVDR 5
#define SWNDF 6
#define SWVDF 7

/*output field list*/
#define SWNET 0

/**
 * Compute the flux of solar.
 * \param buf_src point to the source datas. 
 * \param buf_dst point to the destination data.
 * \param len the array of the length of grids used in this function.
 */

void flux_solar(void ** buf_src, void ** buf_dst, int * len)
{
	/**
	 * Decompress the parameters.
	 */
	int * length = len;
	double ** data_src = (double **)buf_src;
	double ** data_dst = (double **)buf_dst;

	///Compute the data_dst using data_src.
    for(int i = 0; i < (* length); i++)
    {
        data_dst[SWNET][i] = ((1.0 - data_src[ANIDR][i]) * data_src[SWNDR][i]) + ((1.0 - data_src[AVSDR][i]) * data_src[SWVDR][i]) + ((1.0 - data_src[ANIDF][i]) * data_src[SWNDF][i]) + ((1.0 - data_src[AVSDF][i]) * data_src[SWVDF][i]);
    }
}
