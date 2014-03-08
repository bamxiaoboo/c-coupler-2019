/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


/*input field list*/
#define MASK 0
#define SO_U 1
#define SO_V 2
#define SO_T 3
#define SA_Z 4
#define SA_U 5
#define SA_V 6
#define SA_TBOT 7
#define SA_PTEM 8
#define SA_SHUM 9
#define SA_DENS 10

/*output field list*/
#define FAOC_SEN 0
#define FAOC_LAT 1
#define FAOC_LWUP 2
#define FAOC_EVAP 3
#define FAOC_TAUX 4
#define FAOC_TAUY 5
#define FAOC_TREF 6
#define FAOC_QREF 7
#define FAOC_DUU10N 8

extern "C" void flux_atmocn_thucpl_(bool*, double*, double*, double*, double*, double*, double*, double*,
                                    double*, double*, double*, double*, double*, double*, double*, double*, 
                                    double*, double*, double*, double*, int*);

void flux_atmOcn(void ** buf_src, void ** buf_dst, int * length)
{
    int *field_size;
    bool *ocn_mask;
    double *ocn_u, *ocn_v, *ocn_t;
    double *atm_z, *atm_u, *atm_v, *atm_ptem, *atm_shum, *atm_dens, *atm_tbot;
    double *ao_sen, *ao_lat, *ao_lwup, *ao_evap, *ao_taux, *ao_tauy, *ao_tref, *ao_qref, *ao_duu10n;
    
    /*decompress the buf_src*/
    ocn_mask = (bool *) buf_src[MASK];
    ocn_u = (double *) buf_src[SO_U];
    ocn_v = (double *) buf_src[SO_V];
    ocn_t = (double *) buf_src[SO_T];
    atm_z = (double *) buf_src[SA_Z];
    atm_u = (double *) buf_src[SA_U];
    atm_v = (double *) buf_src[SA_V];
    atm_tbot = (double *) buf_src[SA_TBOT];
    atm_ptem = (double *) buf_src[SA_PTEM];
    atm_shum = (double *) buf_src[SA_SHUM];
    atm_dens = (double *) buf_src[SA_DENS];

    /*decompress the data_dst*/
    ao_sen = (double *) buf_dst[FAOC_SEN];
    ao_lat = (double *) buf_dst[FAOC_LAT];
    ao_lwup = (double *) buf_dst[FAOC_LWUP];
    ao_evap = (double *) buf_dst[FAOC_EVAP];
    ao_taux = (double *) buf_dst[FAOC_TAUX];
    ao_tauy = (double *) buf_dst[FAOC_TAUY];
    ao_tref = (double *) buf_dst[FAOC_TREF];
    ao_qref = (double *) buf_dst[FAOC_QREF];
    ao_duu10n = (double *) buf_dst[FAOC_DUU10N];
    
    field_size = length;

    flux_atmocn_thucpl_(ocn_mask, ocn_u, ocn_v, ocn_t, atm_z, atm_u, atm_v, atm_ptem, 
                              atm_shum, atm_dens, atm_tbot, ao_sen, ao_lat, ao_lwup, 
                              ao_evap, ao_taux, ao_tauy, ao_tref, ao_qref, ao_duu10n, 
                              field_size);
}
