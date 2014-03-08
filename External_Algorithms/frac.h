/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef FRAC_H
#define FRAC_H

extern void frac_init_ocn(void **,void **, int *);
extern void frac_init_atm(void **,void **, int *);
extern void frac_set_ocn(void **,void **, int *);
extern void frac_set_atm(void **,void **, int *);

#endif
