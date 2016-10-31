/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef CCPL_API_MGT
#define CCPL_API_MGT


#include <mpi.h>


#define API_ID_FINALIZE                                 ((int)(0x00100001))
#define API_ID_COMP_MGT_REG_ROOT_COMP                   ((int)(0x00200001))
#define API_ID_COMP_MGT_REG_COMP                        ((int)(0x00200002))
#define API_ID_COMP_MGT_END_COMP_REG                    ((int)(0x00200004))
#define API_ID_COMP_MGT_GET_CURRENT_PROC_ID_IN_COMP     ((int)(0x00200008))
#define API_ID_COMP_MGT_GET_NUM_PROC_IN_COMP            ((int)(0x00200010))
#define API_ID_COMP_MGT_GET_COMP_ID                     ((int)(0x00200020))
#define API_ID_GRID_MGT_REG_H2D_GRID_ONLINE             ((int)(0X00400001))
#define API_ID_GRID_MGT_REG_H2D_GRID_VIA_FILE           ((int)(0X00400002))
#define API_ID_GRID_MGT_REG_1D_GRID_ONLINE              ((int)(0X00400004))
#define API_ID_GRID_MGT_REG_MD_GRID                     ((int)(0X00400008))
#define API_ID_GRID_MGT_REG_GRID_VIA_COR                ((int)(0X00400010))
#define API_ID_GRID_MGT_REG_GRID_VIA_LOCAL              ((int)(0X00400020))
#define API_ID_GRID_MGT_REG_GRID_VIA_REMOTE             ((int)(0X00400040))
#define API_ID_GRID_MGT_CMP_GRID_VIA_REMOTE             ((int)(0X00400080))
#define API_ID_GRID_MGT_GET_GRID_ID                     ((int)(0X00400100))
#define API_ID_GRID_MGT_SET_GRID_DATA                   ((int)(0X00400200))
#define API_ID_GRID_MGT_SET_3D_GRID_DYN_REF_FLD         ((int)(0X00400400))
#define API_ID_GRID_MGT_SET_3D_GRID_STATIC_REF_FLD      ((int)(0X00400800))
#define API_ID_GRID_MGT_GET_GRID_GLOBAL_DATA            ((int)(0X00401000))
#define API_ID_GRID_MGT_GET_GRID_LOCAL_DATA             ((int)(0X00402000))
#define API_ID_GRID_MGT_GET_MID_LAYER_GRID              ((int)(0X00404000))
#define API_ID_GRID_MGT_GET_GRID_SIZE                   ((int)(0X00408000))
#define API_ID_DECOMP_MGT_REG_DECOMP                    ((int)(0X00800001))
#define API_ID_FIELD_MGT_REG_FIELD_INST                 ((int)(0X01000001))
#define API_ID_TIME_MGT_SET_TIME_STEP                   ((int)(0X02000001))
#define API_ID_TIME_MGT_ADVANCE_TIME                    ((int)(0X02000002))
#define API_ID_TIME_MGT_DEFINE_SINGLE_TIMER             ((int)(0X02000004))
#define API_ID_TIME_MGT_DEFINE_COMPLEX_TIMER            ((int)(0X02000008))
#define API_ID_INTERFACE_REG_IMPORT                     ((int)(0X04000001))
#define API_ID_INTERFACE_REG_EXPORT                     ((int)(0X04000002))
#define API_ID_INTERFACE_EXECUTE                        ((int)(0X04000004))


extern void synchronize_comp_processes_for_API(int, int, MPI_Comm, const char *, const char *);
extern void check_API_parameter_string(int, int, MPI_Comm, const char*, const char*, const char*, const char*);
extern void check_API_parameter_int(int, int, MPI_Comm, const char*, int, const char*, const char*);
extern void check_API_parameter_timer(int, int, MPI_Comm, const char*, int, const char*, const char*);
extern void check_API_parameter_field_instance(int, int, MPI_Comm, const char*, int, const char*, const char*);
extern void get_API_hint(int, int, char*);
extern void check_and_verify_name_format_of_string_for_API(int, char*, int, const char*, const char*);
extern void check_and_verify_name_format_of_string_for_XML(int, char*, const char*, const char*, int);

#endif

