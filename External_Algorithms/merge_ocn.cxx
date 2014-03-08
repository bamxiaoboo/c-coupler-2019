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
#include "merge_ocn.h"

/*input data list*/

#define	CPL_WTAUX	0
#define	CPL_WTAUY	1
#define	CPL_SWNET	2
#define	CPL_LATENT	3
#define	CPL_SEN		4
#define	CPL_LWUP	5
#define	CPL_EVAP	6
#define	CPL_RAIN	7
#define	CPL_SNOW	8
#define	CPL_DUU10N	9
#define	ATM_LWDN	10
#define	ATM_PSLV	11

#define	ICE_WTAUX	0   //12
#define	ICE_WTAUY	1   //13
#define	ICE_SWPEN	2   //14
#define	ICE_MELTH	3   //15
#define	ICE_MELTW	4   //16
#define	ICE_SALT	5   //17

#define	RIVER_ROFF	0   //18

#define	AFRAC		0   //19
#define	IFRAC		1   //20

/*output data list*/

#define	OCN_WTAUX	0
#define	OCN_WTAUY	1
#define	OCN_SWNET	2
#define	OCN_LATENT	3
#define	OCN_SEN		4
#define	OCN_LWUP	5
#define	OCN_EVAP	6
#define	OCN_LWDN	7
#define	OCN_RAIN	8
#define	OCN_SNOW	9
#define	OCN_PREC	10
#define	OCN_MELTH	11
#define	OCN_MELTW	12
#define	OCN_SALT	13
#define	OCN_IFRAC	14
#define	OCN_PSLV	15
#define	OCN_DUU10N	16
#define	OCN_ROFF	17

void merge_ocn(void ** buf_src, void ** buf_dst, int * length)
{
	double ** data_src = (double **)buf_src;
	double ** data_dst = (double **)buf_dst;

	///The variables used to decompress the data_src and data_dst.
	double ** data_atm, ** data_ice, ** data_river, * afrac, * ifrac, ** data_ocn;

	/*decompress the data_src*/
	data_atm = data_src;
	data_ice = & data_src[12];
	data_river = & data_src[18];
	afrac = data_src[19];
	ifrac = data_src[20];
	
	///decompress the data-dst
	data_ocn = data_dst;

    for(int n = 0; n < (* length); n++)
    {
        data_ocn[OCN_WTAUX][n] = data_atm[CPL_WTAUX][n] * afrac[n] + data_ice[ICE_WTAUX][n] * ifrac[n];
        data_ocn[OCN_WTAUY][n] = data_atm[CPL_WTAUY][n] * afrac[n] + data_ice[ICE_WTAUY][n] * ifrac[n];
        data_ocn[OCN_SWNET][n] = data_atm[CPL_SWNET][n] * afrac[n] + data_ice[ICE_SWPEN][n] * ifrac[n];
        data_ocn[OCN_LATENT][n] = data_atm[CPL_LATENT][n] * afrac[n];
        data_ocn[OCN_SEN][n] = data_atm[CPL_SEN][n] * afrac[n];
        data_ocn[OCN_LWUP][n] = data_atm[CPL_LWUP][n] * afrac[n];
        data_ocn[OCN_EVAP][n] = data_atm[CPL_EVAP][n] * afrac[n];
        data_ocn[OCN_LWDN][n] = data_atm[ATM_LWDN][n] * afrac[n];
        data_ocn[OCN_RAIN][n] = data_atm[CPL_RAIN][n] * afrac[n];
        data_ocn[OCN_SNOW][n] = data_atm[CPL_SNOW][n] * afrac[n];
        data_ocn[OCN_PREC][n] = data_ocn[OCN_RAIN][n] + data_ocn[OCN_SNOW][n];
        data_ocn[OCN_MELTH][n] = data_ice[ICE_MELTH][n] * ifrac[n];
        data_ocn[OCN_MELTW][n] = data_ice[ICE_MELTW][n] * ifrac[n];
        data_ocn[OCN_SALT][n] = data_ice[ICE_SALT][n] * ifrac[n];
        data_ocn[OCN_IFRAC][n] = ifrac[n];
		data_ocn[OCN_PSLV][n] = data_atm[ATM_PSLV][n];
		data_ocn[OCN_DUU10N][n] = data_atm[CPL_DUU10N][n];
		data_ocn[OCN_ROFF][n] = data_river[RIVER_ROFF][n];
    }
}


