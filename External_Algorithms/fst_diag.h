/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was transformed from NCAR coupler CPL6. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef _FST_DISG_H_
#define _FST_DISG_H_

extern void diag_dodiag(void ** buf_src, void ** buf_dst, int * len);
extern void diag_solar(void ** buf_src, void ** buf_dst, int * len);

#endif //_FST_DISG_H_
