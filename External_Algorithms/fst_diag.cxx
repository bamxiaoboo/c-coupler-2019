/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <cstdio>
#include "fst_diag.h"

#define MAX(a,b) ((a)>(b) ? (a) : (b))

//constant list
#define CONST_OCN_REF_SALT  34.7
#define CONST_ICE_REF_SALT  4.0
#define CONST_LAT_HEAT_FSN  (3.337e5)                    // latent heat of fusion

#define HEAT_TO_WATER  -(CONST_OCN_REF_SALT-CONST_ICE_REF_SALT) /(CONST_OCN_REF_SALT*CONST_LAT_HEAT_FSN) 

//number of phyical data for each diag_component
#define NUM_DIAG_ATM   18
#define NUM_DIAG_LND    21   
#define NUM_DIAG_ICE     23
#define NUM_DIAG_OCN    25

//number of phyical data that each model provides
#define IDX_A2C_SIZE     0 
#define IDX_L2C_SIZE     0
#define IDX_R2C_SIZE     1
#define IDX_I2C_SIZE     2
#define IDX_O2C_SIZE     2  


#define IDX_HFRZ      0  // heat : latent, freezing 
#define IDX_HMELT    1  // heat : latent, melting 
#define IDX_HSWNET 2  // heat : short wave, net
#define IDX_HLWDN   3  // heat : longwave down
#define IDX_HLWUP   4  // heat : longwave up
#define IDX_HLAT      5  // heat : latent, vaporization
#define IDX_HSEN      6  // heat : sensible
#define IDX_HNET      7  // heat : sum of all heat

#define IDX_WFRZ     0  // water: freezing
#define IDX_WMELT   1  // water: melting
#define IDX_WRAIN   2  // water: precip, liquid
#define IDX_WSNOW  3  // water: precip, frozen
#define IDX_WEVAP   4  // water: evaporation
#define IDX_WROFF   5  // water: runoff
#define IDX_WNET     6  // water: sum of all water

#define IDX_AREA      0  // area (wrt to unit sphere)

#define IDX_ATM        0  // model index: atm
#define IDX_ICE_N     1  // model index: ice, northern
#define IDX_ICE_S     2  // model index: ice, southern
#define IDX_LND        3  // model index: lnd
#define IDX_OCN        4  // model index: ocn
#define IDX_SUM        5  // sum of all

#define  IDX_AVSDR_DA       0     // Sx_avsdr from bun_c2a%data
#define  IDX_ANIDR_DA       1     // Sx_anidr from bun_c2a%data
#define  IDX_AVSDF_DA       2     // Sx_avsdf from bun_c2a%data
#define  IDX_ANIDF_DA       3     // Sx_anidf from bun_c2a%data
#define  IDX_CMLWUP_DA    4     // Faxx_lwup  from bun_c2a%data
#define  IDX_CMLAT_DA       5     // Faxx_lat    from bun_c2a%data
#define  IDX_CMSEN_DA       6     // Faxx_sen   from bun_c2a%data
#define  IDX_CMEVAP_DA     7     // Faxx_evap from bun_c2a%data
#define  IDX_SWNDR_DA      8     // Faxa_swndr from bun_a2c%data
#define  IDX_SWVDR_DA      9     // Faxa_swvdr from bun_a2c%data
#define  IDX_SWVDF_DA      10   // Faxa_swvdf from bun_a2c%data
#define  IDX_SWNDF_DA      11   // Faxa_swndf from bun_a2c%data
#define  IDX_ARAINC_DA     12   // Faxa_rainc  from bun_a2c%data
#define  IDX_ARAINL_DA     13   // Faxa_rainl  from bun_a2c%data
#define  IDX_ASNOWC_DA   14   // Faxa_snowc from bun_a2c%data
#define  IDX_ASNOWL_DA   15   // Faxa_snowl from bun_a2c%data
#define  IDX_ALWDN_DA     16   // Faxa_lwdn from bun_a2c%data
#define  IDX_AREA_DA        17   // aream from bun_a2c%dom%lGrid

#define  IDX_AVSDR_DL      0     // Sl_avsdr  from bun_l2c%data
#define  IDX_ANIDR_DL      1     // Sl_anidr  from bun_l2c%data
#define  IDX_AVSDF_DL      2     // Sl_avsdf from bun_l2c%data
#define  IDX_ANIDF_DL      3     // Sl_anidf from bun_l2c%data
#define  IDX_LLWUP_DL      4     // Fall_lwup  from bun_l2c%data
#define  IDX_LLAT_DL         5     // Fall_lat  from bun_l2c%data
#define  IDX_LSEN_DL         6     // Fall_sen from bun_l2c%data
#define  IDX_LEVAP_DL       7     // Fall_evap from bun_l2c%data
#define  IDX_SWNDR_DL     8     // Faxa_swndr  from bun_c2l%data
#define  IDX_SWVDR_DL     9     // Faxa_swvdr  from bun_c2l%data
#define  IDX_SWVDF_DL     10   // Faxa_swvdf from bun_c2l%data
#define  IDX_SWNDF_DL     11   // Faxa_swndf from bun_c2l%data
#define  IDX_ARAINC_DL    12   // Faxa_rainc  from bun_c2l%data
#define  IDX_ARAINL_DL    13   // Faxa_rainl  from bun_c2l%data
#define  IDX_ASNOWC_DL  14   // Faxa_snowc from bun_c2l%data
#define  IDX_ASNOWL_DL  15   // Faxa_snowl from bun_c2l%data
#define  IDX_ALWDN_DL    16   // Faxa_lwdn from bun_c2l%data
#define  IDX_RROFF_DL     17   // Forr_roff from bun_r2c%data
#define  IDX_LFRAC_DL     18   // lfrac from bun_lfrac%data 
#define  IDX_AREA_DL      19   // aream from bun_l2c%dom%lGrid
#define  IDX_MASK_DL      20   // mask from bun_l2c%dom%lGrid
#define  IDX_AREAR_DL    21   // aream from bun_r2c%dom%lGrid
#define  IDX_MASKR_DL    22   // mask from bun_r2c%dom%lGrid

#define  IDX_AVSDR_DI      0     // Sl_avsdr  from bun_i2c%data
#define  IDX_ANIDR_DI      1     // Sl_anidr  from bun_i2c%data
#define  IDX_AVSDF_DI      2     // Sl_avsdf from bun_i2c%data
#define  IDX_ANIDF_DI      3     // Sl_anidf from bun_i2c%data
#define  IDX_MELTH_DI      4      // Fioi_melth from bun_i2c%data
#define  IDX_ASWNET_DI   5      // Faii_swnet  from bun_i2c%data
#define  IDX_PSWNET_DI   6      // Fioi_swpen from bun_i2c%data
#define  IDX_LWUP_DI       7      // Faii_lwup from bun_i2c%data
#define  IDX_LAT_DI          8      // Faii_lat from bun_i2c%data
#define  IDX_SEN_DI          9      // Faii_sen from bun_i2c%data
#define  IDX_MELTW_DI     10    // Fioi_meltw from bun_i2c%data
#define  IDX_EVAP_DI        11    // Faii_evap from bun_i2c%data
#define  IDX_SWNDR_DI     12   // Faxa_swndr  from bun_c2i%data
#define  IDX_SWVDR_DI     13   // Faxa_swvdr from bun_c2i%data
#define  IDX_SWVDF_DI     14   // Faxa_swvdf from bun_c2i%data
#define  IDX_SWNDF_DI     15   // Faxa_swndf  from bun_c2i%data
#define  IDX_LWDN_DI       16   // Faxa_lwdn from bun_c2i%data
#define  IDX_RAIN_DI        17   // Faxc_rain from bun_c2i%data
#define  IDX_SNOW_DI       18    // Faxc_snow from bun_c2i%data
#define  IDX_IFRAC_DI      19    // ifrac from bun_ifrac%data      
#define  IDX_OQ_DI            20   // Fioo_q from bun_o2c%data
#define  IDX_AREA_DI       21   // aream from bun_i2c%dom%lGrid
#define  IDX_MASK_DI       22    // mask from bun_i2c%dom%lGrid
#define  IDX_LATS_DI        23    // lat from bun_i2c%dom%lGrid

#define  IDX_AVSDR_DO       0      //  So_avsdr  from bun_alb%data
#define  IDX_ANIDR_DO       1      //  So_anidr  from bun_alb%data
#define  IDX_AVSDF_DO       2      //  So_avsdf  from bun_alb%data
#define  IDX_ANIDF_DO       3      //  So_anidf  from bun_alb%data
#define  IDX_MELTH_DO       4      // Foxx_melth  from bun_c2o%data
#define  IDX_LWDN_DO        5      // Foxx_lwdn  from bun_c2o%data
#define  IDX_LWUP_DO        6      // Foxx_lwup  from bun_c2o%data
#define  IDX_LAT_DO           7      // Foxx_lat  from bun_c2o%data
#define  IDX_SEN_DO           8      // Foxx_sen  from bun_c2o%data
#define  IDX_MELTW_DO      9      // Foxx_meltw  from bun_c2o%data
#define  IDX_EVAP_DO        10     // Foxx_evap  from bun_c2o%data
#define  IDX_RAIN_DO        11     // Foxx_rain  from bun_c2o%data
#define  IDX_SNOW_DO       12     // Foxx_snow  from bun_c2o%data
#define  IDX_RROFF_DO      13     // Forr_roff  from bun_c2o%data
#define  IDX_IFRAC_DO       14     // ifrac  from bun_ofrac%data
#define  IDX_AFRAC_DO      15     // Fioo_q  from bun_ofrac%data
#define  IDX_SWNDR_DO     16     // Faxa_swndr from bun_a2c_o%data
#define  IDX_SWVDR_DO     17     // Faxa_swvdr from bun_a2c_o%data
#define  IDX_SWVDF_DO     18     // Faxa_swvdf from bun_a2c_o%data
#define  IDX_SWNDF_DO     19     // Faxa_swndf from bun_a2c_o%data
#define  IDX_AREA_DO        20     // aream  from bun_o2c%dom%lGrid
#define  IDX_MASK_DO        21    // mask  from bun_o2c%dom%lGrid   
#define  IDX_PSWNET_DO   22     // Fioi_swpen  from bun_i2c%data
#define  IDX_OQ_DO            23     // Fioo_q  from bun_o2c%data

#define  IDX_DATA_SRC_FAXA_SWNDR            8       // bun_a2c%data Faxa_swndr from data_src
#define  IDX_DATA_SRC_FAXA_SWVDR            9       // bun_a2c%data Faxa_swndr from data_src
#define  IDX_DATA_SRC_FAXA_SWVDF            10     // bun_a2c%data Faxa_swndr from data_src
#define  IDX_DATA_SRC_FAXA_SWNDF            11     // bun_a2c%data Faxa_swndf from data_src
#define  IDX_DATA_SRC_FIOI_SWPEN             42     // bun_i2c%data  Fioi_swpen from data_src
#define  IDX_DATA_SRC_FIOI_Q                      56     // bun_o2c%data Fioo_q from data_src

//constant list
#define CONST_PI  3.14159265358979323846

//number of phyical data that each model provides
#define IDX_SOLAR_A2C_SIZE     0 
#define IDX_SOLAR_L2C_SIZE     0
#define IDX_SOLAR_I2C_SIZE     1

// index of data in pointer_list
#define  IDX_ASWNET_S    0     // Faxa_swnet from bun_a2c%data
#define  IDX_LSWNET_S    1     // Fall_swnet from bun_l2c%data
#define  IDX_LFRAC_S     2     // lfrac from bun_lfrac%data
#define  IDX_ISWNET_S    3     // Faii_swnet from bun_i2c%data
#define  IDX_IFRAC_S     4     // ifrac from bun_ifrac%data
#define  IDX_AREA_A2C    5     // aream from bun_a2c%dom%lGrid
#define  IDX_AREA_L2C    6     // aream from bun_l2c%dom%lGrid
#define  IDX_MASK_L2C    7     // mask from bun_l2c%dom%lGrid
#define  IDX_AREA_I2C    8     // aream from bun_i2c%dom%lGrid
#define  IDX_MASK_I2C    9     // mask from bun_i2c%dom%lGrid
#define  IDX_LATS_I2C    10    // lats from bun_i2c%dom%lGrid


static double swnet_atm;            // cpl's expected swnet from atm
static double swnet_lnd;             // cpl's expected swnet from lnd
static double swnet_ice_nh;       // cpl's expected swnet from ice, northern hemi
static double swnet_ice_sh;       // cpl's expected swnet from ice, southern hemi
static double swnet_ocn;            // cpl's expected swnet from ocn

//double *data_rst;
static double mdl_heat[6*8];    //  7( 6 mdls + 1 sum) * 8 (7 heats + 1 sum)
static double mdl_water[6*7];  //  7( 6 mdls + 1 sum) * 7 (6 water + 1 sum)
static double mdl_area[6*1];    //  7( 6 mdls + 1 sum) * 1 (1 area)

static void diag_atm(double **data, int *length);
static void diag_lnd(double **data, int *length);
static void diag_ice(double **data, int *length);
static void diag_ocn(double **data, int *length);


void diag_dodiag(void ** buf_src, void ** buf_dst, int *length)
{
    int i, j, indx;
    int *atm_s, *lnd_s, *ice_s, *ocn_s;
    double *heat_p;

    // diag_atm
    double * atm_p[18];
    for (i=0;i<18;i++)	
        atm_p[i] = (double *)buf_src[i];
    atm_s = &length[IDX_A2C_SIZE];
    diag_atm(atm_p, atm_s);

    // diag_lnd
    double * lnd_p[23];
    for (i=18;i<41;i++)
        lnd_p[i-18] = (double *)buf_src[i];
    lnd_s = &length[IDX_L2C_SIZE];
    diag_lnd(lnd_p, lnd_s);

    // diag_ice
    double * ice_p[24];
    for (i=41;i<65;i++)
        ice_p[i-41] = (double *)buf_src[i];
    ice_s = &length[IDX_I2C_SIZE];
    diag_ice(ice_p, ice_s);

    // diag_ocn
    double * ocn_p[24];
    for (i=65; i<87; i++)
        ocn_p[i-65] = (double *)buf_src[i];

    // get Fioi_swpen from bun_i2c%data     i=22         data_src     indx is 42(+5) 47
    ocn_p[22] = (double *)buf_src[47];
    // get Fioo_q  from bun_c2o%data         i=23         data_src     indx is 56(+5) 61
    ocn_p[23] = (double *)buf_src[61];	 


    ocn_s = &length[IDX_O2C_SIZE];
    diag_ocn(ocn_p, ocn_s);

    // sum of instantaneous, inter-model data
    // heat flux
    heat_p = &mdl_heat[IDX_LND*8];

    indx = IDX_SUM*8;
    for (i=0; i<8; i++){
        mdl_heat[indx+i] = 0;
        for (j=0; j<5; j++)
            mdl_heat[indx+i] +=mdl_heat[j*8+i];
    }

    //water flux
    indx = IDX_SUM*7;
    for (i=0; i<7; i++){
        mdl_water[indx+i] = 0;
        for (j=0; j<5; j++)
            mdl_water[indx+i] +=mdl_water[j*7+i];
    }
    // area
    indx = IDX_SUM;
    mdl_area[indx] = 0;
    for (j=0; j<5; j++)
        mdl_area[indx] +=mdl_area[j];
}


static void diag_atm(double **data, int *length)
{
    int nloc;
    int i;
    double darea;
    double *heat_p, *water_p, *area_p;  // heat/water/area data for this model


    nloc = length[0];

    // do global sum
    heat_p = &mdl_heat[IDX_ATM*8];
    water_p = &mdl_water[IDX_ATM*7];
    area_p = &mdl_area[IDX_ATM];

    for (i=0; i<8; i++)
        heat_p[i] = 0;
    for (i=0; i<7; i++)
        water_p[i] = 0;
    *area_p = 0;

    for (i=0; i<nloc; i++){
        // area
        darea = *(data[IDX_AREA_DA]+i);
        *area_p = *area_p+darea;
        // heat flux
        heat_p[IDX_HFRZ] = 0;
        heat_p[IDX_HMELT] = 0;
        heat_p[IDX_HSWNET] = heat_p[IDX_HSWNET] - darea *
            ((1 - (*(data[IDX_AVSDR_DA]+i))) * (*(data[IDX_SWVDR_DA]+i))
             + (1 - (*(data[IDX_ANIDR_DA]+i))) * (*(data[IDX_SWNDR_DA]+i))
             + (1 - (*(data[IDX_AVSDF_DA]+i))) * (*(data[IDX_SWVDF_DA]+i))
             + (1 - (*(data[IDX_ANIDF_DA]+i))) * (*(data[IDX_SWNDF_DA]+i)));
        heat_p[IDX_HLWDN] = heat_p[IDX_HLWDN] - darea * (*(data[IDX_ALWDN_DA]+i));
        heat_p[IDX_HLWUP] = heat_p[IDX_HLWUP] - darea * (*(data[IDX_CMLWUP_DA]+i));
        heat_p[IDX_HLAT] = heat_p[IDX_HLAT] - darea * (*(data[IDX_CMLAT_DA]+i));
        heat_p[IDX_HSEN] = heat_p[IDX_HSEN] - darea * (*(data[IDX_CMSEN_DA]+i));

        // water flux
        water_p[IDX_WFRZ] = 0;
        water_p[IDX_WMELT] = 0;
        water_p[IDX_WRAIN] = water_p[IDX_WRAIN] - darea *  ((*(data[IDX_ARAINC_DA]+i))+(*(data[IDX_ARAINL_DA]+i)));
        water_p[IDX_WSNOW] = water_p[IDX_WSNOW] - darea * ((*(data[IDX_ASNOWC_DA]+i))+(*(data[IDX_ASNOWL_DA]+i)));
        water_p[IDX_WEVAP] = water_p[IDX_WEVAP]  - darea * (*(data[IDX_CMEVAP_DA]+i));
        water_p[IDX_WROFF] = 0;         
    }

    // sum over sources/sinks
    for (i=0; i<7; i++)
        heat_p[IDX_HNET] = heat_p[IDX_HNET]  + heat_p[i];
    for (i=0; i<6; i++)
        water_p[IDX_WNET] = water_p[IDX_WNET] + water_p[i];

    swnet_atm = heat_p[IDX_HSWNET];
}

static void diag_lnd(double **data, int *length)
{
    int nloc, nlocr;
    int i;
    double darea;
    double *heat_p, *water_p, *area_p;  // heat/water/area data for this model
    bool *river_mask, *land_mask;


    nloc = length[0];
    nlocr = length[1];

    // do global sum
    heat_p = &mdl_heat[IDX_LND*8];
    water_p = &mdl_water[IDX_LND*7];
    area_p = &mdl_area[IDX_LND];


    river_mask = (bool *)data[IDX_MASKR_DL];
    land_mask = (bool *)data[IDX_MASK_DL];

    for (i=0; i<8; i++)
        heat_p[i] = 0;
    for (i=0; i<7; i++)
        water_p[i] = 0;
    *area_p = 0;

    //tmp_frac = 0;
    //tmp_area = 0;

    for (i=0; i<nloc; i++){
        if(land_mask[i]){
            // area
            darea =(*(data[IDX_AREA_DL]+i))*(*(data[IDX_LFRAC_DL]+i));	
            *area_p = *area_p+darea;

            // heat flux
            heat_p[IDX_HFRZ] = 0;
            heat_p[IDX_HMELT] = 0;
            heat_p[IDX_HSWNET] = heat_p[IDX_HSWNET] + darea * 
                ((1 - (*(data[IDX_AVSDR_DL]+i))) * (*(data[IDX_SWVDR_DL]+i))
                 + (1 - (*(data[IDX_ANIDR_DL]+i))) * (*(data[IDX_SWNDR_DL]+i))
                 + (1 - (*(data[IDX_AVSDF_DL]+i))) * (*(data[IDX_SWVDF_DL]+i))
                 + (1 - (*(data[IDX_ANIDF_DL]+i))) * (*(data[IDX_SWNDF_DL]+i)));

            heat_p[IDX_HLWDN] = heat_p[IDX_HLWDN] + darea * (*(data[IDX_ALWDN_DL]+i));
            heat_p[IDX_HLWUP] = heat_p[IDX_HLWUP] + darea * (*(data[IDX_LLWUP_DL]+i));
            heat_p[IDX_HLAT] = heat_p[IDX_HLAT] + darea * (*(data[IDX_LLAT_DL]+i));
            heat_p[IDX_HSEN] = heat_p[IDX_HSEN] + darea * (*(data[IDX_LSEN_DL]+i));

            // water flux
            water_p[IDX_WFRZ] = 0;
            water_p[IDX_WMELT] = 0;
            water_p[IDX_WRAIN] = water_p[IDX_WRAIN] + darea * ((*(data[IDX_ARAINC_DL]+i))+(*(data[IDX_ARAINL_DL]+i)));
            water_p[IDX_WSNOW] = water_p[IDX_WSNOW] + darea * ((*(data[IDX_ASNOWC_DL]+i))+(*(data[IDX_ASNOWL_DL]+i)));
            water_p[IDX_WEVAP] = water_p[IDX_WEVAP]  + darea * (*(data[IDX_LEVAP_DL]+i));				
        }
    }

    // do river model data
    for (i=0; i<nlocr; i++)
        if(river_mask[i])
            water_p[IDX_WROFF] = water_p[IDX_WROFF] -(*(data[IDX_AREAR_DL]+i))*(*(data[IDX_RROFF_DL]+i)); 

    // form sum across all heat/water sources/sinks
    for (i=0; i<7; i++)
        heat_p[IDX_HNET] = heat_p[IDX_HNET]  + heat_p[i];
    for (i=0; i<6; i++)
        water_p[IDX_WNET] = water_p[IDX_WNET] + water_p[i];

    swnet_lnd = heat_p[IDX_HSWNET];
}


static void diag_ice(double **data, int *length)
{
    int nloc;
    int i;
    double darea,dareai;
    //double  heatf;  // heat from freezing xor melt pot
    double *heat_np, *water_np, *area_np;  // heat/water/area  data for n-hemi
    double *heat_sp, *water_sp, *area_sp;  // heat/water/area  data for s-hemi
    bool *ice_mask;


    nloc = length[0];	
    // do global sum
    heat_np = &mdl_heat[IDX_ICE_N*8];
    water_np = &mdl_water[IDX_ICE_N*7];
    area_np = &mdl_area[IDX_ICE_N];

    heat_sp = &mdl_heat[IDX_ICE_S*8];
    water_sp = &mdl_water[IDX_ICE_S*7];
    area_sp = &mdl_area[IDX_ICE_S];

    ice_mask = (bool *)data[IDX_MASK_DI];

    for (i=0; i<8; i++){
        heat_np[i] = 0;
        heat_sp[i] = 0;
    }
    for (i=0; i<7; i++){
        water_np[i] = 0;
        water_sp[i] = 0;
    }
    *area_np = 0;
    *area_sp = 0;

    swnet_ice_nh = 0;    
    swnet_ice_sh = 0;   

    for (i=0; i<nloc; i++){
        darea = *(data[IDX_AREA_DI]+i);
        dareai = darea*(*(data[IDX_IFRAC_DI]+i));
        // northern hemisphere
        if (ice_mask[i] && data[IDX_LATS_DI][i]>0){
            // area
            *area_np = *area_np+dareai;

            // heat flux
            heat_np[IDX_HFRZ] = heat_np[IDX_HFRZ] - darea *MAX((*(data[IDX_OQ_DI]+i)), 0.0);
            heat_np[IDX_HMELT] = heat_np[IDX_HMELT] - dareai*(*(data[IDX_MELTH_DI]+i));

            swnet_ice_nh = swnet_ice_nh + dareai*
                ((1 - (*(data[IDX_AVSDR_DI]+i))) *(*(data[IDX_SWVDR_DI]+i))
                 + (1 - (*(data[IDX_ANIDR_DI]+i))) * (*(data[IDX_SWNDR_DI]+i))
                 + (1 - (*(data[IDX_AVSDF_DI]+i))) * (*(data[IDX_SWVDF_DI]+i))
                 + (1 - (*(data[IDX_ANIDF_DI]+i))) * (*(data[IDX_SWNDF_DI]+i)));

            heat_np[IDX_HSWNET] = heat_np[IDX_HSWNET] - dareai * (*(data[IDX_PSWNET_DI]+i));
            heat_np[IDX_HLWDN] = heat_np[IDX_HLWDN] + dareai * (*(data[IDX_LWDN_DI]+i));
            heat_np[IDX_HLWUP] = heat_np[IDX_HLWUP] + dareai * (*(data[IDX_LWUP_DI]+i));
            heat_np[IDX_HLAT] = heat_np[IDX_HLAT] + dareai * (*(data[IDX_LAT_DI]+i));
            heat_np[IDX_HSEN] = heat_np[IDX_HSEN] + dareai * (*(data[IDX_SEN_DI]+i));

            // water flux
            water_np[IDX_WMELT] = water_np[IDX_WMELT] -dareai * (*(data[IDX_MELTW_DI]+i));
            water_np[IDX_WRAIN] = water_np[IDX_WRAIN] + dareai * (*(data[IDX_RAIN_DI]+i));
            water_np[IDX_WSNOW] = water_np[IDX_WSNOW] + dareai * (*(data[IDX_SNOW_DI]+i));
            water_np[IDX_WEVAP] = water_np[IDX_WEVAP]  + dareai * (*(data[IDX_EVAP_DI]+i));
            water_np[IDX_WROFF] = 0;         
        }
        // southern hemisphere
        else if (ice_mask[i] && 
                data[IDX_LATS_DI][i]<0){
            // area
            *area_sp = *area_sp+dareai;

            // heat flux
            heat_sp[IDX_HFRZ] = heat_sp[IDX_HFRZ] - darea *MAX((*(data[IDX_OQ_DI]+i)), 0.0);
            heat_sp[IDX_HMELT] = heat_sp[IDX_HMELT] - dareai*(*(data[IDX_MELTH_DI]+i));

            swnet_ice_sh = swnet_ice_sh + dareai*
                ((1 - (*(data[IDX_AVSDR_DI]+i))) *(*(data[IDX_SWVDR_DI]+i))
                 + (1 - (*(data[IDX_ANIDR_DI]+i))) * (*(data[IDX_SWNDR_DI]+i))
                 + (1 - (*(data[IDX_AVSDF_DI]+i))) * (*(data[IDX_SWVDF_DI]+i))
                 + (1 - (*(data[IDX_ANIDF_DI]+i))) * (*(data[IDX_SWNDF_DI]+i)));

            heat_sp[IDX_HSWNET] = heat_sp[IDX_HSWNET] - dareai * (*(data[IDX_PSWNET_DI]+i));
            heat_sp[IDX_HLWDN] = heat_sp[IDX_HLWDN] + dareai * (*(data[IDX_LWDN_DI]+i));
            heat_sp[IDX_HLWUP] = heat_sp[IDX_HLWUP] + dareai * (*(data[IDX_LWUP_DI]+i));
            heat_sp[IDX_HLAT] = heat_sp[IDX_HLAT] + dareai * (*(data[IDX_LAT_DI]+i));
            heat_sp[IDX_HSEN] = heat_sp[IDX_HSEN] + dareai * (*(data[IDX_SEN_DI]+i));

            // water flux
            water_sp[IDX_WMELT] = water_sp[IDX_WMELT] -dareai * (*(data[IDX_MELTW_DI]+i));
            water_sp[IDX_WRAIN] = water_sp[IDX_WRAIN] + dareai * (*(data[IDX_RAIN_DI]+i));
            water_sp[IDX_WSNOW] = water_sp[IDX_WSNOW] + dareai * (*(data[IDX_SNOW_DI]+i));
            water_sp[IDX_WEVAP] = water_sp[IDX_WEVAP]  + dareai * (*(data[IDX_EVAP_DI]+i));
            water_sp[IDX_WROFF] = 0;         	
        }
    }

    water_np[IDX_WFRZ] = heat_np[IDX_HFRZ]*HEAT_TO_WATER;  // implied water flux
    water_sp[IDX_WFRZ] = heat_sp[IDX_HFRZ]*HEAT_TO_WATER;  // implied water flux

    heat_np[IDX_HSWNET]  = heat_np[IDX_HSWNET]  +  swnet_ice_nh;
    heat_sp[IDX_HSWNET]  = heat_sp[IDX_HSWNET]  +  swnet_ice_sh;

    // form sum across all heat/water sources/sinks
    for (i=0; i<7; i++){
        heat_np[IDX_HNET] = heat_np[IDX_HNET]  + heat_np[i];
        heat_sp[IDX_HNET] = heat_sp[IDX_HNET]  + heat_sp[i];
    }
    for (i=0; i<6; i++){
        water_np[IDX_WNET] = water_np[IDX_WNET] + water_np[i];
        water_sp[IDX_WNET] = water_sp[IDX_WNET] + water_sp[i];
    }
}


static void diag_ocn(double **data, int *length)
{
    int nloc;
    int i;
    double darea,darea_a,darea_i;
    double *heat_p, *water_p, *area_p;  // heat/water/area data for this model
    bool *ocn_mask;


    nloc = length[0];

    // do global sum
    heat_p = &mdl_heat[IDX_OCN*8];
    water_p = &mdl_water[IDX_OCN*7];
    area_p = &mdl_area[IDX_OCN];

    for (i=0; i<8; i++)
        heat_p[i] = 0;
    for (i=0; i<7; i++)
        water_p[i] = 0;
    *area_p = 0;

    ocn_mask = (bool *)data[IDX_MASK_DO];

    swnet_ocn = 0;  // need swnet w/o penetrating sw for solar verificatio

    for (i=0; i<nloc; i++){
        if (ocn_mask[i]) {
            // area
            darea = *(data[IDX_AREA_DO]+i);
            darea_a = darea*(*(data[IDX_AFRAC_DO]+i));
            darea_i = darea*(*(data[IDX_IFRAC_DO]+i));
            *area_p = *area_p+darea_a;
            // heat flux
            heat_p[IDX_HFRZ] = heat_p[IDX_HFRZ] + darea *MAX((*(data[IDX_OQ_DO]+i)), 0.0);
            heat_p[IDX_HMELT] = heat_p[IDX_HMELT] + darea*(*(data[IDX_MELTH_DO]+i));
            heat_p[IDX_HSWNET] = heat_p[IDX_HSWNET] + darea_i * (*(data[IDX_PSWNET_DO]+i));                              
            swnet_ocn = swnet_ocn + darea_a*
                ((1 - (*(data[IDX_AVSDR_DO]+i))) *(*(data[IDX_SWVDR_DO]+i))
                 + (1 - (*(data[IDX_ANIDR_DO]+i))) * (*(data[IDX_SWNDR_DO]+i))
                 + (1 - (*(data[IDX_AVSDF_DO]+i))) * (*(data[IDX_SWVDF_DO]+i))
                 + (1 - (*(data[IDX_ANIDF_DO]+i))) * (*(data[IDX_SWNDF_DO]+i)));

            heat_p[IDX_HLWDN] = heat_p[IDX_HLWDN] + darea * (*(data[IDX_LWDN_DO]+i));
            heat_p[IDX_HLWUP] = heat_p[IDX_HLWUP] + darea * (*(data[IDX_LWUP_DO]+i));
            heat_p[IDX_HLAT] = heat_p[IDX_HLAT] + darea * (*(data[IDX_LAT_DO]+i));
            heat_p[IDX_HSEN] = heat_p[IDX_HSEN] + darea * (*(data[IDX_SEN_DO]+i));
            // water flux
            water_p[IDX_WMELT] = water_p[IDX_WMELT] -darea * (*(data[IDX_MELTW_DO]+i));
            water_p[IDX_WRAIN] = water_p[IDX_WRAIN] + darea* (*(data[IDX_RAIN_DO]+i));
            water_p[IDX_WSNOW] = water_p[IDX_WSNOW] + darea * (*(data[IDX_SNOW_DO]+i));
            water_p[IDX_WEVAP] = water_p[IDX_WEVAP]  + darea * (*(data[IDX_EVAP_DO]+i));
            water_p[IDX_WROFF] = water_p[IDX_WROFF]+darea * (*(data[IDX_RROFF_DO]+i));   
        }
    }

    water_p[IDX_WFRZ] = heat_p[IDX_HFRZ]*HEAT_TO_WATER;   // implied water flux

    heat_p[IDX_HSWNET]  = heat_p[IDX_HSWNET]  + swnet_ocn;

    // sum over sources/sinks
    for (i=0; i<7; i++)
        heat_p[IDX_HNET] = heat_p[IDX_HNET]  + heat_p[i];
    for (i=0; i<6; i++)
        water_p[IDX_WNET] = water_p[IDX_WNET] + water_p[i];
}

void diag_solar(void ** buf_src, void ** buf_dst, int *length) {
    int i;
    int nloca, nlocl, nloci; 
    double sa1, sl1, sin1, sis1, so1;   // net solar computed in cpl diagnostics
    double sa2, sl2, sin2, sis2, so2;   // net solar sent to cpl from comps, local sum
    double sa2g, sl2g, sin2g, sis2g;   // net solar sent to cpl from comps, global

    int tmp_count1;
    int tmp_count2;
    int tmp_count3;
    int tmp_count4;
    double tmp_sum1;
    double tmp_sum2;
    double tmp_sum3;
    double tmp_sum4;
    double tmp_sum5;

    double ** data = (double **)buf_src;
    bool * maskl = (bool *)buf_src[IDX_MASK_L2C];
    bool * maski = (bool *)buf_src[IDX_MASK_I2C];

    nloca = length[IDX_SOLAR_A2C_SIZE]; 
    nlocl  = length[IDX_SOLAR_L2C_SIZE]; 
    nloci  = length[IDX_SOLAR_I2C_SIZE]; 

    // atm
    tmp_sum1=0;
    sa2 = 0;
    for (i=0; i<nloca; i++){
        sa2 = sa2 - data[IDX_AREA_A2C][i] * data[IDX_ASWNET_S][i];
        tmp_sum1 = tmp_sum1 + data[IDX_ASWNET_S][i];
    }

    // sa1 is the sum of swnet_atm through MPI_REDUCE
    sa1 = swnet_atm;
    // sa2g is the sum of sa2 through MPI_REDUCE
    sa2g = sa2;

    // if (cpl_comm_comp_pid == 0) 
    sa1  = sa1 /(4.0*CONST_PI);
    sa2g = sa2g/(4.0*CONST_PI);

    // lnd
    sl2 = 0;
    tmp_count1=0;
    tmp_sum2=0;
    tmp_sum3=0;
    for (i=0; i<nlocl; i++)
        if(maskl[i]){
            tmp_count1++;
            sl2 = sl2 + data[IDX_AREA_L2C][i] * data[IDX_LFRAC_S][i] * data[IDX_LSWNET_S][i];
            tmp_sum2 +=tmp_sum2 +(*(data[IDX_LSWNET_S]+i));
            tmp_sum3 +=tmp_sum3 +(*(data[IDX_LFRAC_S]+i));
        }

    // sl1 is the sum of swnet_lnd through MPI_REDUCE
    sl1 = swnet_lnd;
    // sl2g is the sum of sl2 through MPI_REDUCE
    sl2g = sl2;

    // if (cpl_comm_comp_pid == 0) 
    sl1  = sl1 /(4.0*CONST_PI);
    sl2g = sl2g/(4.0*CONST_PI);

    // ice-nh and ice-sh
    sin2 = 0;
    sis2 = 0;
    tmp_count2=0;
    tmp_count3=0;
    tmp_count4=0;
    tmp_sum4=0;
    tmp_sum5=0;
    for (i=0; i<nloci; i++){
        tmp_count2++;
        if (maski[i]){			
            if(data[IDX_LATS_I2C][i] > 0.0){
                sin2 = sin2 + data[IDX_AREA_I2C][i] * data[IDX_IFRAC_S][i] * data[IDX_ISWNET_S][i];
                tmp_count3++;
                tmp_sum4+=(*(data[IDX_ISWNET_S]+i));
                tmp_sum5+=(*(data[IDX_IFRAC_S]+i));
            }
            else if(data[IDX_LATS_I2C][i] < 0.0){
                sis2 = sis2 + data[IDX_AREA_I2C][i] * data[IDX_IFRAC_S][i] * data[IDX_ISWNET_S][i];
                tmp_count4++;
            }
        }
    }
    // sin1, sis1 is the sum of swnet_ice_nh through MPI_REDUCE
    sin1 = swnet_ice_nh;
    sis1 = swnet_ice_sh;
    // sin2g, sis2g is the sum of sin2 through MPI_REDUCE
    sin2g =  sin2;
    sis2g =  sis2;

    // if (cpl_comm_comp_pid == 0)   
    sin1  = sin1 /(4.0*CONST_PI);
    sis1  = sis1 /(4.0*CONST_PI);
    sin2g = sin2g/(4.0*CONST_PI);
    sis2g = sis2g/(4.0*CONST_PI);

    // ocn   < note: ocn does not provide cpl with any sw-net info >

    // so1 is the sum of swnet_ocn through MPI_REDUCE
    so1 = swnet_ocn;

    so2 = -999.0;     // flags invalid/missing data

    // if (cpl_comm_comp_pid == 0)
    so1=so1/(4.0*CONST_PI);

/*
    printf("c_solar_diag------------------------\n");
    printf("sa1 = %.10lf\n",sa1);
    printf("sl1 = %.10lf\n",sl1);
    printf("sin1 = %.10lf\n",sin1);
    printf("sis1 = %.10lf\n",sis1);
    printf("so1 = %.10lf\n",so1);
    printf("sa2g = %.10lf\n", sa2g);
    printf("sl2g = %.10lf\n",sl2g);
    printf("sin2g = %.10lf\n",sin2g );
    printf("sis2g = %.10lf\n",sis2g);	

    printf("++++++++++++++++++++++++\n");
    printf("tmp_sum1 : %.10lf\n",tmp_sum1);
    printf("tmp_sum2 : %.10lf\n",tmp_sum2);
    printf("tmp_sum3 : %.10lf\n",tmp_sum3);
    printf("tmp_sum4 : %.10lf\n",tmp_sum4);
    printf("tmp_sum5 : %.10lf\n",tmp_sum5);

    printf("tmp_count1 : %d\n",tmp_count1);
    printf("tmp_count2 : %d\n",tmp_count2);
    printf("tmp_count3 : %d\n",tmp_count3);
    printf("tmp_count4 : %d\n",tmp_count4);
    printf("c_solar_diag------------------------\n");
*/
}
