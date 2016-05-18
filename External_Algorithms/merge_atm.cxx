/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <iostream>
#include <cstring>
#include <cstdio>

/*input data list*/

#define	CPL_WTAUX	0
#define	CPL_WTAUY	1
#define	CPL_LATENT	2
#define	CPL_SEN		3
#define	CPL_LWUP	4
#define	CPL_EVAP	5
#define	CPL_TREF	6
#define	CPL_QREF	7
#define	CPL_AVSDR	8
#define	CPL_ANIDR	9
#define	CPL_AVSDF	10
#define	CPL_ANIDF	11
#define	OCN_SST		12

#define	LND_WTAUX	0
#define	LND_WTAUY	1
#define	LND_LATENT	2
#define	LND_SEN		3
#define	LND_LWUP	4
#define	LND_EVAP	5
#define	LND_TREF	6
#define	LND_QREF	7
#define	LND_AVSDR	8
#define	LND_ANIDR	9
#define	LND_AVSDF	10
#define	LND_ANIDF	11
#define	LND_T		12
#define	LND_SNOWH	13

#define	ICE_WTAUX	0
#define	ICE_WTAUY	1
#define	ICE_LATENT	2
#define	ICE_SEN		3
#define	ICE_LWUP	4
#define	ICE_EVAP	5
#define	ICE_TREF	6
#define	ICE_QREF	7
#define	ICE_AVSDR	8
#define	ICE_ANIDR	9
#define	ICE_AVSDF	10
#define	ICE_ANIDF	11
#define	ICE_T		12

#define	OFRAC		0
#define	LFRAC		1
#define	IFRAC		2

/*output data list*/
#define	ATM_WTAUX	0
#define	ATM_WTAUY	1
#define	ATM_LATENT	2
#define	ATM_SEN		3
#define	ATM_LWUP	4
#define	ATM_EVAP	5
#define	ATM_TREF	6
#define	ATM_QREF	7
#define	ATM_AVSDR	8
#define	ATM_ANIDR	9
#define	ATM_AVSDF	10
#define	ATM_ANIDF	11
#define	ATM_T		12
#define	ATM_SNOWH	13
#define	ATM_IFRAC	14
#define	ATM_OFRAC	15
#define	ATM_SST		16

void bundle_add(double * dst, double * src, double * frac, int length)
{
    for(int i = 0; i < length; i++)
        dst[i] = dst[i] + src[i] * frac[i];
}
/**
 * Merge the values of atm.
 * \param buf_src_src compress the datas of input config file to buf_src
 * \param buf_dst compress the datas of output config file to buf_dst
 * \param length recorde the lengths of grids used in this function.
 */
void merge_atm(void ** buf_src, void ** buf_dst, int * length)
{
    double ** data_src = (double **) buf_src;
    double ** data_dst = (double **) buf_dst;

    ///The variables used to decompress the data_src and data_dst
    double ** data_ocn, ** data_lnd, ** data_ice, ** data_atm, ** bun_frac_a;
    /*decompress the data_src*/
    data_ocn = data_src;
    data_lnd = & data_src[13];
    data_ice = & data_src[27];
    bun_frac_a = & data_src[40];

    /*decompress the data_dst*/
    data_atm = data_dst;

	for (int i = 0; i < 13; i ++) {
		memset(data_atm[i], 0, (* length)*sizeof(double));
		bundle_add(data_atm[i], data_ocn[i], bun_frac_a[0], (*length));
		bundle_add(data_atm[i], data_lnd[i], bun_frac_a[1], (*length));
		bundle_add(data_atm[i], data_ice[i], bun_frac_a[2], (*length));
	}

    memset(data_atm[ATM_SNOWH], 0, (* length)*sizeof(double));
    bundle_add(data_atm[ATM_SNOWH], data_lnd[LND_SNOWH], bun_frac_a[1], (*length));

    memcpy(data_atm[ATM_IFRAC], bun_frac_a[IFRAC], (* length)*sizeof(double));
    memcpy(data_atm[ATM_OFRAC], bun_frac_a[OFRAC], (* length)*sizeof(double));
    memcpy(data_atm[ATM_SST], data_ocn[OCN_SST], (* length)*sizeof(double));
}



void merge_carbon_flux(void ** buf_src, void ** buf_dst, int * length)
{
    double *carbon_flux_ocn = ((double**) buf_src)[0];
    double *carbon_flux_lnd = ((double**) buf_src)[1];
    double *carbon_flux_atm = ((double**) buf_dst)[0];
    double *frac_o = ((double**) buf_src)[2];
    double *frac_l = ((double**) buf_src)[3];
    int field_size = length[0];


    for (int i = 0; i < field_size; i ++)
         carbon_flux_atm[i] = carbon_flux_ocn[i]*frac_o[i] + carbon_flux_lnd[i]*frac_l[i];

}
