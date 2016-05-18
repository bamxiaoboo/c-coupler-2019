/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef EXECUTION_REPORT_H
#define EXECUTION_REPORT_H


#define REPORT_ERROR       ((int)1)
#define REPORT_LOG         ((int)2)
#define REPORT_WARNING     ((int)3)
#define REPORT_PROGRESS    ((int)4)


extern void wtime(double *t);

extern void EXECUTION_REPORT(int, bool);
extern void EXECUTION_REPORT(int, bool, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, long);
extern void EXECUTION_REPORT(int, bool, const char*, int, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, int, const char*, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, int, int);
extern void EXECUTION_REPORT(int, bool, const char*, int, int, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, int, int, int, int); 
extern void EXECUTION_REPORT(int, bool, const char*, int, const char*, int, long); 
extern void EXECUTION_REPORT(int, bool, const char*, const void*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, int); 
extern void EXECUTION_REPORT(int, bool, const char*, const char*, int, const char*); 
extern void EXECUTION_REPORT(int, bool, const char*, const char*, long, int, long, int); 
extern void EXECUTION_REPORT(int, bool, const char*, const char*, long, long); 
extern void EXECUTION_REPORT(int, bool, const char*, const char*, long, long, long); 
extern void EXECUTION_REPORT(int, bool, const char*, const void*, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, int); 
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, int, const char*); 
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, int, long);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, double, double); 
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, int, long, long);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char *);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char *, int);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char *, int, long);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char *, int, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char*, int, void*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char*, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char*, const char*, const char*, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char*, const char*, int);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char*, const char*, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char*, const char*, int, const char*);
extern void EXECUTION_REPORT(int, bool, const char*, const char*, const char*, const char*, const char*, int, int);


#endif
