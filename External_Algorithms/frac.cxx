/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <stdio.h>
#include <math.h>
#include <string.h>
#include "c_coupler_interface.h"

///The index of data_buf
#define	ICE_MASK	0
#define	ICE_IFRAC	1
#define	ICE_OFRAC	2
#define	ICE_AFRAC	3
#define	ICE_LFRAC	4
#define	OCN_IFRAC	5
#define	OCN_OFRAC	6
#define	OCN_AFRAC	7
#define	OCN_LFRAC	8
#define	ATM_IFRAC	9
#define	ATM_OFRAC	10
#define	ATM_AFRAC	11
#define	ATM_LFRAC	12
#define	LND_IFRAC	13
#define	LND_OFRAC	14
#define	LND_AFRAC	15
#define	LND_LFRAC	16


///The index of length
#define ICE_LENGTH	0
#define	OCN_LENGTH	0
#define	ATM_LENGTH	1
#define	LND_LENGTH	1

int nint(double a)
{
	int b = floor(a);
	if((a - b) < 0.5)
		return b;
	else
		return (b+1);
}

/**
 * Initialize fraction values on ocn grid (based on zero ice fraction).
 * \param data_buf compress the fraction datas to data_buf.
 * \param length the length of the grids.
 */
void frac_init_ocn(void ** data_buf,void ** data_dst,int * length)
{
	double ** frac_buf = (double **) data_dst;
	bool *mask = (bool *) frac_buf[ICE_MASK];
	int i;
        int atm_field_size = c_coupler_get_field_size(frac_buf[ATM_OFRAC], "Get atm and lnd field size in frac_init_ocn");
        int ocn_field_size = c_coupler_get_field_size(frac_buf[ICE_IFRAC], "Get ocn and sea ice field size in frac_init_ocn");
	
	/**
	 * Initialize values on ice grid (based on zero ice fraction).
	 */
	memset(frac_buf[ICE_AFRAC], 0, sizeof(double) * ocn_field_size);
	memset(frac_buf[ICE_IFRAC], 0, sizeof(double) * ocn_field_size);
	memset(frac_buf[ICE_LFRAC], 0, sizeof(double) * ocn_field_size);
	memset(frac_buf[ICE_OFRAC], 0, sizeof(double) * ocn_field_size);
	memset(frac_buf[ATM_AFRAC], 0, sizeof(double) * atm_field_size);
	memset(frac_buf[ATM_IFRAC], 0, sizeof(double) * atm_field_size);
	memset(frac_buf[ATM_OFRAC], 0, sizeof(double) * atm_field_size);
	memset(frac_buf[ATM_LFRAC], 0, sizeof(double) * atm_field_size);
	memset(frac_buf[LND_AFRAC], 0, sizeof(double) * atm_field_size);
	memset(frac_buf[LND_IFRAC], 0, sizeof(double) * atm_field_size);
	memset(frac_buf[LND_OFRAC], 0, sizeof(double) * atm_field_size);
	memset(frac_buf[LND_LFRAC], 0, sizeof(double) * atm_field_size);
	int count = 0;
	for(i = 0; i < ocn_field_size; i ++)
		if (mask[i])
			count ++;
	for(i = 0; i < ocn_field_size; i ++) {
		if(mask[i]) {
			frac_buf[ICE_AFRAC][i] = 1.0;
			frac_buf[ICE_OFRAC][i] = 1.0;
		}
	}
	
	/**
	 * Initialize values on ocn grid (same as for ice grid).
	 */
	memcpy(frac_buf[OCN_AFRAC], frac_buf[ICE_AFRAC], sizeof(double) * ocn_field_size);
	memcpy(frac_buf[OCN_IFRAC], frac_buf[ICE_IFRAC], sizeof(double) * ocn_field_size);
	memcpy(frac_buf[OCN_LFRAC], frac_buf[ICE_LFRAC], sizeof(double) * ocn_field_size);
	memcpy(frac_buf[OCN_OFRAC], frac_buf[ICE_OFRAC], sizeof(double) * ocn_field_size);
}

/**
 * Initialize fraction values on atm grid (needs/assumes zero ice).
 * \param data_buf compress the fraction datas to data_buf.
 * \param length the length of the grids.
 */
void frac_init_atm(void ** data_buf, void ** data_dst, int * length)
{
	double ** frac_buf = (double **) data_buf;
        int atm_field_size = c_coupler_get_field_size(frac_buf[ATM_OFRAC], "Get atm and lnd field size in frac_init_ocn");


	/**
	 * Clean up atm fraction on atm grid: must be 1 everywhere.
	 * Clean up ice fraction on atm grid: must be 0 everywhere.
	 */
	for(int i = 0; i < atm_field_size; i ++)
	{
		frac_buf[ATM_AFRAC][i] = 1.0;
		frac_buf[ATM_IFRAC][i] = 0.0;
	}
	/**
	 * Clean up ocn fraction on atm grid: must be in [0,1].
	 * Compute lnd fraction on atm grid: lnd = 1 - ocn.
	 * It's a requirement to elminate land points smaller than .001
	 */
	for(int i = 0; i < atm_field_size; i ++)
	{
		if(frac_buf[ATM_OFRAC][i] > 1.0)
			frac_buf[ATM_OFRAC][i] = 1.0;
		else if(frac_buf[ATM_OFRAC][i] < 0.0)
			frac_buf[ATM_OFRAC][i] = 0.0;
		frac_buf[ATM_LFRAC][i] = 1.0 - frac_buf[ATM_OFRAC][i];
		if(frac_buf[ATM_LFRAC][i] < 0.001)
			frac_buf[ATM_LFRAC][i] = 0.0;
	}
	/**
	 * Copy fraction on atm grid to fraction on lnd grid.
	 * Then clean up the ocn and ice fraction on lnd grid.
	 */
	memcpy(frac_buf[LND_AFRAC], frac_buf[ATM_AFRAC], sizeof(double) * atm_field_size);
	memcpy(frac_buf[LND_LFRAC], frac_buf[ATM_LFRAC], sizeof(double) * atm_field_size);
	memset(frac_buf[LND_IFRAC], 0, sizeof(double) * atm_field_size);
	memset(frac_buf[LND_OFRAC], 0, sizeof(double) * atm_field_size);
}

/**
 * Set/Update the surface fraction.
 * \param data_buf compress the fraction datas to data_buf.
 * \param length the length of grids.
 */
void frac_set_ocn(void ** data_buf,void ** data_dst,int * length)
{
	double ** frac_buf = (double **) data_buf;
	bool *mask = (bool *) data_buf[ICE_MASK];
	int i;
        int ocn_field_size = c_coupler_get_field_size(frac_buf[ICE_IFRAC], "Get ocn and sea ice field size in frac_set_ocn");


	/**
	 * Set/Update values on ice grid, confine values into [0,1], mask =0 => frac=0
	 */
	for(i = 0; i < ocn_field_size; i ++)
	{
		if(frac_buf[ICE_IFRAC][i] > 1.0)
			frac_buf[ICE_IFRAC][i] = 1.0;
		else if(frac_buf[ICE_IFRAC][i] < 0.0)
			frac_buf[ICE_IFRAC][i] = 0.0;

		if(!mask[i])
			frac_buf[ICE_IFRAC][i] = 0.0;
	}

	/**
	 * Set/Update values on ocn grid (assum ice & ocn have same domain)
	 */
	for(i = 0; i < ocn_field_size; i ++)
	{
		frac_buf[OCN_IFRAC][i] = frac_buf[ICE_IFRAC][i];
		frac_buf[OCN_AFRAC][i] = 1.0 - frac_buf[ICE_IFRAC][i];
	}
}

/**
 * Set/Update the surface fraction.
 * \param data_buf compress the fraction datas to data_buf.
 * \param length the length of grids.
 */
void frac_set_atm(void ** data_buf,void ** data_dst,int * length)
{
	double ** frac_buf = (double **) data_buf;
        int atm_field_size = c_coupler_get_field_size(frac_buf[ATM_OFRAC], "Get atm and lnd field size in frac_set_atm");

	/**
	 * Set/Update values on atm grid.
	 */
	for(int i = 0; i < atm_field_size; i ++)
	{
		if(frac_buf[ATM_IFRAC][i] > 1.0)
			frac_buf[ATM_IFRAC][i] = 1.0;
		else if(frac_buf[ATM_IFRAC][i] < 0.0)
			frac_buf[ATM_IFRAC][i] = 0.0;
		frac_buf[ATM_OFRAC][i] = 1.0 - frac_buf[ATM_IFRAC][i] - frac_buf[ATM_LFRAC][i];
		if(frac_buf[ATM_OFRAC][i] > 1.0)
			frac_buf[ATM_OFRAC][i] = 1.0;
		else if(frac_buf[ATM_OFRAC][i] < 0.0)
			frac_buf[ATM_OFRAC][i] = 0.0;
	}
}

