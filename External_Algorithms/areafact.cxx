/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <stdio.h>
#include <math.h>
#include "areafact.h"

///The list of output
#define    COMP2CPL    0
#define    CPL2COMP    1
///The list of input
#define    AREAR    0
#define    AREAC    1
#define    MASK    2


/**
 * Initialize the area factor bundle
 * \param the input of one comp.
 * \param the areafact of the comp.
 * \param the length of the comp.
 */
void areafact_set(void ** buf_src, void ** buf_dst, int * length)
{
    double ** data_src = (double **) buf_src;
    double ** data_dst = (double **) buf_dst;
    double * frac = (double *)data_src[MASK];
    int i;
    
    for(i = 0; i < length[0]; i ++){
                if (frac[i] > 1.0e-6) {
            data_dst[COMP2CPL][i] = data_src[AREAC][i] / data_src[AREAR][i];
            data_dst[CPL2COMP][i] = 1.0 / data_dst[COMP2CPL][i];
            //data_dst[CPL2COMP][i] = data_src[AREAR][i] / data_src[AREAC][i];
                }
        else{
            data_dst[COMP2CPL][i] = 1.0;
            data_dst[CPL2COMP][i] = 1.0;
        }
    }
}


/**
 * Initialize the area factor bundle.
 * \param buf_src compress the input of atm, lnd, ocn, ice and river.
 * \param buf_dst compress the areafact of atm, lnd, ocn, ice and river.
 * \param length the length of atm, lnd, ocn, ice and river.
 */
void areafact_init(void ** buf_src, void ** buf_dst, int * length)
{
    ///The parameters of atm areafact.
    void ** data_atm_src = buf_src;
    void ** data_atm_dst = buf_dst;
    int * data_atm_length = &length[0];
    ///The parameters of ice areafact.
    void ** data_ice_src = &buf_src[3];
    void ** data_ice_dst = &buf_dst[2];
    int * data_ice_length = &length[1];
    ///The parameters of lnd areafact.
    void ** data_lnd_src = &buf_src[6];
    void ** data_lnd_dst = &buf_dst[4];
    int * data_lnd_length = &length[0];
    ///The parameters of ocn areafact.
    void ** data_ocn_src = &buf_src[9];
    void ** data_ocn_dst = &buf_dst[6];
    int * data_ocn_length = &length[1];
    ///The parameters of river areafact.
    void ** data_river_src = &buf_src[12];
    void ** data_river_dst = &buf_dst[8];
    int * data_river_length = &length[2];
    areafact_set(data_atm_src, data_atm_dst, data_atm_length);
    areafact_set(data_ice_src, data_ice_dst, data_ice_length);
    areafact_set(data_lnd_src, data_lnd_dst, data_lnd_length);
    areafact_set(data_ocn_src, data_ocn_dst, data_ocn_length);
    areafact_set(data_river_src, data_river_dst, data_river_length);
}

