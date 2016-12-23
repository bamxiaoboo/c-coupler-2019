!*************************************************************
!  Copyright (c) 2013, Tsinghua University.
!  This is a source file of C-Coupler.
!  If you have any problem, 
!  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
!*************************************************************


 MODULE c_coupler_interface_mod
   

   IMPLICIT none
!
!  PUBLIC: interfaces
!
   public :: c_coupler_abort
   public :: c_coupler_get_current_date
   public :: c_coupler_get_current_time
   public :: c_coupler_get_num_elapsed_days_from_start
   public :: c_coupler_allreduce_real16
   public :: c_coupler_check_sum_for_all_fields

   interface CCPL_register_field_instance ; module procedure &
        c_coupler_register_model_double_0D_data, &
        c_coupler_register_model_double_1D_data, &
        c_coupler_register_model_double_2D_data, &
        c_coupler_register_model_double_3D_data, &
        c_coupler_register_model_double_4D_data, &
        c_coupler_register_model_float_0D_data, &
        c_coupler_register_model_float_1D_data, &
        c_coupler_register_model_float_2D_data, &
        c_coupler_register_model_float_3D_data, &
        c_coupler_register_model_float_4D_data, &
        c_coupler_register_model_integer_0D_data, &
        c_coupler_register_model_integer_1D_data, &
        c_coupler_register_model_integer_2D_data, &
        c_coupler_register_model_integer_3D_data, &
        c_coupler_register_model_integer_4D_data
   end interface



   interface CCPL_register_IO_field ; module procedure &
        c_coupler_register_one_IO_field_from_field_instance_new_name, &
        c_coupler_register_one_IO_field_from_field_instance_no_name, &
        c_coupler_register_new_IO_field_double_0D_data, &
        c_coupler_register_new_IO_field_double_1D_data, &
        c_coupler_register_new_IO_field_double_2D_data, &
        c_coupler_register_new_IO_field_double_3D_data, &
        c_coupler_register_new_IO_field_double_4D_data, &
        c_coupler_register_new_IO_field_float_0D_data, &
        c_coupler_register_new_IO_field_float_1D_data, &
        c_coupler_register_new_IO_field_float_2D_data, &
        c_coupler_register_new_IO_field_float_3D_data, &
        c_coupler_register_new_IO_field_float_4D_data, &
        c_coupler_register_new_IO_field_integer_0D_data, &
        c_coupler_register_new_IO_field_integer_1D_data, &
        c_coupler_register_new_IO_field_integer_2D_data, &
        c_coupler_register_new_IO_field_integer_3D_data, &
        c_coupler_register_new_IO_field_integer_4D_data
   end interface



   interface CCPL_register_import_interface ; module procedure &
        CCPL_register_import_interface_1, &
        CCPL_register_import_interface_2, &
        CCPL_register_import_interface_3, &
        CCPL_register_import_interface_4
   end interface




   interface CCPL_register_export_interface ; module procedure &
        CCPL_register_export_interface_1, &
        CCPL_register_export_interface_2
   end interface



   interface c_coupler_register_sigma_grid_bottom_field ; module procedure &
        c_coupler_register_sigma_grid_bottom_field_1d_float,   &
        c_coupler_register_sigma_grid_bottom_field_2d_float,   &
        c_coupler_register_sigma_grid_bottom_field_1d_double,  &
        c_coupler_register_sigma_grid_bottom_field_2d_double
   end interface



   interface c_coupler_add_field_for_perturbing_roundoff_errors ; module procedure &
        c_coupler_add_field_for_perturbing_roundoff_errors_double_0D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_double_1D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_double_2D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_double_3D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_double_4D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_float_0D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_float_1D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_float_2D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_float_3D, &
        c_coupler_add_field_for_perturbing_roundoff_errors_float_4D
   end interface

   interface c_coupler_perturb_roundoff_errors_for_an_array ; module procedure &
        c_coupler_perturb_roundoff_errors_for_an_array_double_0D, &
        c_coupler_perturb_roundoff_errors_for_an_array_double_1D, &
        c_coupler_perturb_roundoff_errors_for_an_array_double_2D, &
        c_coupler_perturb_roundoff_errors_for_an_array_double_3D, &
        c_coupler_perturb_roundoff_errors_for_an_array_double_4D, &
        c_coupler_perturb_roundoff_errors_for_an_array_float_0D, &
        c_coupler_perturb_roundoff_errors_for_an_array_float_1D, &
        c_coupler_perturb_roundoff_errors_for_an_array_float_2D, &
        c_coupler_perturb_roundoff_errors_for_an_array_float_3D, &
        c_coupler_perturb_roundoff_errors_for_an_array_float_4D
   end interface



   interface c_coupler_get_current_calendar_time ; module procedure &
        c_coupler_get_double_current_calendar_time, &
        c_coupler_get_float_current_calendar_time 
   end interface


   interface CCPL_execute_interface ; module procedure &
        CCPL_execute_interface_using_id, &
        CCPL_execute_interface_using_name
   end interface


   REAL, parameter, public :: coupling_fill_value = 1.0e30 
   integer,parameter,private :: R16 = selected_real_kind(24) ! 16 byte real
   integer,parameter,private :: R8  = selected_real_kind(12) ! 8 byte real
   integer,parameter,private :: R4  = selected_real_kind( 6) ! 4 byte real

   REAL(R16), allocatable, private :: reduce_buf_real16(:)
   integer,                private :: reduce_buf_real16_size
   
   contains   
!   
!  SUBROUTINE below
! 

   SUBROUTINE  c_coupler_register_sigma_grid_bottom_field_1d_float(data_buf, grid_name)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:)         :: data_buf
   character(len=*)                           :: grid_name
   
   call register_sigma_grid_bottom_field(data_buf, trim(grid_name)//char(0))
   
   END SUBROUTINE  c_coupler_register_sigma_grid_bottom_field_1d_float



   SUBROUTINE c_coupler_register_sigma_grid_bottom_field_2d_float(data_buf,grid_name)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:)       :: data_buf
   character(len=*)                           :: grid_name
   
   call register_sigma_grid_bottom_field(data_buf, trim(grid_name)//char(0))
   
   END SUBROUTINE  c_coupler_register_sigma_grid_bottom_field_2d_float



   SUBROUTINE c_coupler_register_sigma_grid_bottom_field_1d_double(data_buf,grid_name)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:)         :: data_buf
   character(len=*)                           :: grid_name
   
   call register_sigma_grid_bottom_field(data_buf, trim(grid_name)//char(0))
   
   END SUBROUTINE  c_coupler_register_sigma_grid_bottom_field_1d_double



   SUBROUTINE c_coupler_register_sigma_grid_bottom_field_2d_double(data_buf,grid_name)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:)       :: data_buf
   character(len=*)                           :: grid_name
   
   call register_sigma_grid_bottom_field(data_buf, trim(grid_name)//char(0))
   
   END SUBROUTINE  c_coupler_register_sigma_grid_bottom_field_2d_double



   integer FUNCTION c_coupler_register_model_double_0D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R8), INTENT(IN)                    :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, 1, decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real8")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_double_0D_data = field_instance_id

   END FUNCTION c_coupler_register_model_double_0D_data



   integer FUNCTION c_coupler_register_model_double_1D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:)      :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real8")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_double_1D_data = field_instance_id

   END FUNCTION c_coupler_register_model_double_1D_data



   integer FUNCTION c_coupler_register_model_double_2D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:)    :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real8")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_double_2D_data = field_instance_id

   END FUNCTION c_coupler_register_model_double_2D_data



   integer FUNCTION c_coupler_register_model_double_3D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:,:)  :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real8")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_double_3D_data = field_instance_id

   END FUNCTION c_coupler_register_model_double_3D_data



   integer FUNCTION c_coupler_register_model_double_4D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:,:,:)  :: data_buf
   character(len=*), intent(in)              :: field_name
   character(len=*), intent(in), optional    :: field_unit
   character(len=*), intent(in), optional    :: annotation
   integer,          intent(in)              :: decomp_id, comp_or_grid_id, buf_mark
   character *512                            :: local_field_unit, local_annotation
   integer                                   :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real8")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_double_4D_data = field_instance_id

   END FUNCTION c_coupler_register_model_double_4D_data



   integer FUNCTION c_coupler_register_model_float_0D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R4), INTENT(IN)                    :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, 1, decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real4")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_float_0D_data = field_instance_id

   END FUNCTION c_coupler_register_model_float_0D_data



   integer FUNCTION c_coupler_register_model_float_1D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:)      :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real4")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_float_1D_data = field_instance_id

   END FUNCTION c_coupler_register_model_float_1D_data



   integer FUNCTION c_coupler_register_model_float_2D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:)    :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real4")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_float_2D_data = field_instance_id

   END FUNCTION c_coupler_register_model_float_2D_data



   integer FUNCTION c_coupler_register_model_float_3D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:,:)  :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real4")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_float_3D_data = field_instance_id

   END FUNCTION c_coupler_register_model_float_3D_data



   integer FUNCTION c_coupler_register_model_float_4D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:,:,:)  :: data_buf
   character(len=*), intent(in)              :: field_name
   character(len=*), intent(in), optional    :: field_unit
   character(len=*), intent(in), optional    :: annotation
   integer,          intent(in)              :: decomp_id, comp_or_grid_id, buf_mark
   character *512                            :: local_field_unit, local_annotation
   integer                                   :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("real4")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_float_4D_data = field_instance_id

   END FUNCTION c_coupler_register_model_float_4D_data



   integer FUNCTION c_coupler_register_model_integer_0D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   integer, INTENT(IN)                     :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, 1, decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("integer")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_integer_0D_data = field_instance_id

   END FUNCTION c_coupler_register_model_integer_0D_data



   integer FUNCTION c_coupler_register_model_integer_1D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   integer, INTENT(IN), DIMENSION(:)       :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("integer")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_integer_1D_data = field_instance_id

   END FUNCTION c_coupler_register_model_integer_1D_data



   integer FUNCTION c_coupler_register_model_integer_2D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   integer, INTENT(IN), DIMENSION(:,:)     :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("integer")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_integer_2D_data = field_instance_id

   END FUNCTION c_coupler_register_model_integer_2D_data



   integer FUNCTION c_coupler_register_model_integer_3D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   integer, INTENT(IN), DIMENSION(:,:,:)   :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("integer")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_integer_3D_data = field_instance_id

   END FUNCTION c_coupler_register_model_integer_3D_data



   integer FUNCTION c_coupler_register_model_integer_4D_data(data_buf, field_name, decomp_id, comp_or_grid_id, buf_mark, field_unit, annotation)
   implicit none
   integer, INTENT(IN), DIMENSION(:,:,:,:) :: data_buf
   character(len=*), intent(in)            :: field_name
   character(len=*), intent(in), optional  :: field_unit
   character(len=*), intent(in), optional  :: annotation
   integer,          intent(in)            :: decomp_id, comp_or_grid_id, buf_mark
   character *512                          :: local_field_unit, local_annotation
   integer                                 :: field_instance_id

   local_field_unit = ""
   if (present(field_unit)) then
       local_field_unit = field_unit
   endif
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_external_field_instance(field_instance_id, trim(field_name)//char(0), data_buf, size(data_buf), decomp_id, comp_or_grid_id, buf_mark, trim(local_field_unit)//char(0), trim("integer")//char(0), trim(local_annotation)//char(0))
   c_coupler_register_model_integer_4D_data = field_instance_id

   END FUNCTION c_coupler_register_model_integer_4D_data



   SUBROUTINE c_coupler_register_one_IO_field_from_field_instance_new_name(field_IO_name, field_inst_id, annotation)
   implicit none
   character(len=*), intent(in)            :: field_IO_name
   integer,          intent(in)            :: field_inst_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_an_io_field_from_field_instance(field_inst_id, trim(field_IO_name)//char(0), trim(annotation)//char(0))
   else
       call register_an_io_field_from_field_instance(field_inst_id, trim(field_IO_name)//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_one_IO_field_from_field_instance_new_name



   SUBROUTINE c_coupler_register_one_IO_field_from_field_instance_no_name(field_inst_id, annotation)
   implicit none
   integer,          intent(in)            :: field_inst_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_an_io_field_from_field_instance(field_inst_id, trim("")//char(0), trim(annotation)//char(0))
   else
       call register_an_io_field_from_field_instance(field_inst_id, trim("")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_one_IO_field_from_field_instance_no_name



   SUBROUTINE c_coupler_register_new_IO_field_double_0D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R8), INTENT(IN)                    :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, 1, data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, 1, data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_double_0D_data 



   SUBROUTINE c_coupler_register_new_IO_field_double_1D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:)      :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_double_1D_data 



   SUBROUTINE c_coupler_register_new_IO_field_double_2D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:)    :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_double_2D_data 



   SUBROUTINE c_coupler_register_new_IO_field_double_3D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:,:)  :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_double_3D_data 



   SUBROUTINE c_coupler_register_new_IO_field_double_4D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:,:,:):: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real8")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_double_4D_data 



   subroutine c_coupler_register_new_IO_field_float_0D_data(data_buf, field_io_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(r4), intent(in)                    :: data_buf
   character(len=*), intent(in)            :: field_io_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, 1, data_buf, trim(field_io_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, 1, data_buf, trim(field_io_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim("")//char(0))
   endif

   end subroutine c_coupler_register_new_IO_field_float_0D_data 



   SUBROUTINE c_coupler_register_new_IO_field_float_1D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:)      :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_float_1D_data 



   SUBROUTINE c_coupler_register_new_IO_field_float_2D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:)    :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_float_2D_data 



   SUBROUTINE c_coupler_register_new_IO_field_float_3D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:,:)  :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_float_3D_data 



   SUBROUTINE c_coupler_register_new_IO_field_float_4D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:,:,:):: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("real4")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_float_4D_data 



   subroutine c_coupler_register_new_IO_field_integer_0D_data(data_buf, field_io_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   integer, intent(in)                     :: data_buf
   character(len=*), intent(in)            :: field_io_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, 1, data_buf, trim(field_io_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, 1, data_buf, trim(field_io_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim("")//char(0))
   endif

   end subroutine c_coupler_register_new_IO_field_integer_0D_data 



   SUBROUTINE c_coupler_register_new_IO_field_integer_1D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   integer, INTENT(IN), DIMENSION(:)       :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_integer_1D_data 



   SUBROUTINE c_coupler_register_new_IO_field_integer_2D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   integer, INTENT(IN), DIMENSION(:,:)     :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_integer_2D_data 



   SUBROUTINE c_coupler_register_new_IO_field_integer_3D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   integer, INTENT(IN), DIMENSION(:,:,:)   :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_integer_3D_data 



   SUBROUTINE c_coupler_register_new_IO_field_integer_4D_data(data_buf, field_IO_name, &
              field_long_name, field_unit, comp_or_grid_id, decomp_id, annotation)
   implicit none
   integer, INTENT(IN), DIMENSION(:,:,:,:) :: data_buf
   character(len=*), intent(in)            :: field_IO_name
   character(len=*), intent(in)            :: field_long_name
   character(len=*), intent(in)            :: field_unit
   integer,          intent(in)            :: comp_or_grid_id
   integer,          intent(in)            :: decomp_id
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim(annotation)//char(0))
   else
       call register_a_new_io_field(comp_or_grid_id, decomp_id, size(data_buf), data_buf, trim(field_IO_name)//char(0), trim(field_long_name)//char(0), &
                                     trim(field_unit)//char(0), trim("integer")//char(0), trim("")//char(0))
   endif

   END SUBROUTINE c_coupler_register_new_IO_field_integer_4D_data 



 SUBROUTINE c_coupler_reset_timer

   call coupling_reset_timer

 END SUBROUTINE c_coupler_reset_timer



 integer FUNCTION c_coupler_get_nstep
   implicit none  
   integer :: nstep

   call coupling_get_current_nstep(nstep)
   c_coupler_get_nstep = nstep

 END FUNCTION c_coupler_get_nstep



 integer FUNCTION c_coupler_get_num_total_step
   implicit none  
   integer :: nstep

   call coupling_get_num_total_step(nstep)
   c_coupler_get_num_total_step = nstep

 END FUNCTION c_coupler_get_num_total_step



 integer FUNCTION c_coupler_get_step_size
   implicit none  
   integer :: step_size

   call coupling_get_step_size(step_size)
   c_coupler_get_step_size = step_size

 END FUNCTION c_coupler_get_step_size



 integer FUNCTION c_coupler_get_field_size(data_buf, annotation)
   implicit none  
   real(R8),DIMENSION(:)      :: data_buf
   character(len=*), intent(in) :: annotation
   integer :: field_size

   call coupling_get_field_size(data_buf, trim(annotation)//char(0), field_size)
   c_coupler_get_field_size = field_size

 END FUNCTION c_coupler_get_field_size



 logical FUNCTION c_coupler_is_first_restart_step
   implicit none
   logical is_first_restart_step

   call coupling_is_first_restart_step(is_first_restart_step)
   c_coupler_is_first_restart_step = is_first_restart_step

 END FUNCTION c_coupler_is_first_restart_step



 logical FUNCTION c_coupler_is_first_step
   implicit none
   logical is_first_step

   call coupling_is_first_step(is_first_step)
   c_coupler_is_first_step = is_first_step

 END FUNCTION c_coupler_is_first_step



 SUBROUTINE c_coupler_get_current_num_days_in_year(days)
   implicit none
   integer  days

   call coupling_get_current_num_days_in_year(days)
 END SUBROUTINE c_coupler_get_current_num_days_in_year



 SUBROUTINE c_coupler_get_current_year(year)
   implicit none
   integer  year

   call coupling_get_current_year(year)
 END SUBROUTINE c_coupler_get_current_year



 SUBROUTINE c_coupler_get_current_date(date)
   implicit none
   integer  date

   call coupling_get_current_date(date)
 END SUBROUTINE c_coupler_get_current_date


 SUBROUTINE c_coupler_get_current_second(second)
   implicit none
   integer  second

   call coupling_get_current_second(second)
 END SUBROUTINE c_coupler_get_current_second



 SUBROUTINE c_coupler_get_start_time(year, month, day, second)
    implicit none
    integer year, month, day, second

    call coupling_get_start_time(year, month, day, second)

 END SUBROUTINE c_coupler_get_start_time



 SUBROUTINE c_coupler_get_stop_time(year, month, day, second)
    implicit none
    integer year, month, day, second

    call coupling_get_stop_time(year, month, day, second)

 END SUBROUTINE c_coupler_get_stop_time



 SUBROUTINE c_coupler_get_previous_time(year, month, day, second)
    implicit none
    integer year, month, day, second

    call coupling_get_previous_time(year, month, day, second)

 END SUBROUTINE c_coupler_get_previous_time



 SUBROUTINE c_coupler_get_current_time(year, month, day, second, shift_second)
    implicit none
    integer year, month, day, second
    integer, optional ::  shift_second
    integer           ::  local_shift

    local_shift = 0
    if (present(shift_second)) local_shift = shift_second

    call coupling_get_current_time(year, month, day, second, local_shift)

 END SUBROUTINE c_coupler_get_current_time



 SUBROUTINE c_coupler_get_num_elapsed_days_from_reference(days, seconds)
    implicit none
    integer days, seconds

    call coupling_get_elapsed_days_from_reference_date(days, seconds)

 END SUBROUTINE c_coupler_get_num_elapsed_days_from_reference



 SUBROUTINE c_coupler_get_num_elapsed_days_from_start(days, seconds)
    implicit none
    integer days, seconds

    call coupling_get_elapsed_days_from_start_date(days, seconds)

 END SUBROUTINE c_coupler_get_num_elapsed_days_from_start



 logical FUNCTION c_coupler_is_end_current_day()
    implicit none
    integer year, month, day, second

    call coupling_get_current_time(year, month, day, second, 0)
    c_coupler_is_end_current_day = (second == 0)

 END FUNCTION c_coupler_is_end_current_day



 logical FUNCTION c_coupler_is_end_current_month()
    implicit none
    integer year, month, day, second

    call coupling_get_current_time(year, month, day, second, 0)
    if (second .eq. 0 .and. day .eq. 1) then
       c_coupler_is_end_current_month = .true.
    else 
       c_coupler_is_end_current_month = .false.
    end if

 END FUNCTION c_coupler_is_end_current_month



 SUBROUTINE c_coupler_get_double_current_calendar_time(cal_time, shift_second)
   implicit none
   real(R8)  cal_time
   integer, optional ::  shift_second
   integer           ::  local_shift

   local_shift = 0
   if (present(shift_second)) local_shift = shift_second
   call coupling_get_double_current_calendar_time(cal_time, local_shift)

 END SUBROUTINE c_coupler_get_double_current_calendar_time


 SUBROUTINE c_coupler_get_float_current_calendar_time(cal_time, shift_second)
   implicit none
   real(R4)  cal_time
   integer, optional ::  shift_second
   integer           ::  local_shift

   local_shift = 0
   if (present(shift_second)) local_shift = shift_second
   call coupling_get_float_current_calendar_time(cal_time, local_shift)

 END SUBROUTINE c_coupler_get_float_current_calendar_time



 SUBROUTINE c_coupler_allreduce_real16(input_data, output_data, num_data, comm, num_proc)
   implicit none
   include 'mpif.h'
   real(R16)         :: input_data(:), output_data(:)
   integer           :: num_data, comm
   integer           :: ierr
   integer,optional  :: num_proc
   integer           :: local_num_proc, i, k


#if ( defined  NO_MPI_REAL16 )
   if (present(num_proc)) then
       local_num_proc = num_proc
   else
       call MPI_COMM_SIZE(comm, local_num_proc, ierr)
   end if
   if (reduce_buf_real16_size < num_data*local_num_proc) then
      reduce_buf_real16_size = num_data*local_num_proc*2
      deallocate(reduce_buf_real16)
      allocate(reduce_buf_real16(reduce_buf_real16_size))
   end if
   call mpi_allgather(input_data,num_data*2,MPI_REAL8,reduce_buf_real16,num_data*2,MPI_REAL8,comm,ierr)
   do k=1,num_data
       output_data(k) = 0.0
       do i=1,local_num_proc
          output_data(k) = output_data(k) + reduce_buf_real16((i-1)*num_data+k)
       end do
    enddo
#else
   call mpi_allreduce(input_data,output_data,num_data,MPI_REAL16,MPI_SUM,comm,ierr)
#endif
 END SUBROUTINE c_coupler_allreduce_real16
 


 SUBROUTINE c_coupler_check_sum_for_all_fields
   implicit none
   CALL coupling_check_sum_for_all_fields
 END SUBROUTINE c_coupler_check_sum_for_all_fields



 SUBROUTINE c_coupler_abort(error_string)
   implicit none
   character(len=*),     intent(in)    ::  error_string

   call coupling_abort(trim(error_string)//char(0))

 END SUBROUTINE c_coupler_abort



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_0D(data_buf)
   implicit none
   real(R8), INTENT(IN)         :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_0D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_1D(data_buf)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:)         :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_1D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_2D(data_buf)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:)         :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_2D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_3D(data_buf)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:,:)       :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_3D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_4D(data_buf)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:,:,:)     :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_double_4D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_0D(data_buf)
   implicit none
   real(R4), INTENT(IN)         :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_0D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_1D(data_buf)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:)         :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_1D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_2D(data_buf)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:)       :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_2D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_3D(data_buf)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:,:)     :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_3D



   SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_4D(data_buf)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:,:,:)   :: data_buf

   call coupling_add_field_for_perturbing_roundoff_errors(data_buf)

   END SUBROUTINE c_coupler_add_field_for_perturbing_roundoff_errors_float_4D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_0D(data_buf, array_size)
   implicit none
   real(R8), INTENT(IN)         :: data_buf
   integer, INTENT(IN)          :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real8")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_0D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_1D(data_buf, array_size)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:)         :: data_buf
   integer, INTENT(IN)                        :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real8")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_1D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_2D(data_buf, array_size)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:)         :: data_buf
   integer, INTENT(IN)                          :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real8")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_2D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_3D(data_buf, array_size)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:,:)       :: data_buf
   integer, INTENT(IN)                          :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real8")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_3D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_4D(data_buf, array_size)
   implicit none
   real(R8), INTENT(IN), DIMENSION(:,:,:,:)     :: data_buf
   integer, INTENT(IN)                          :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real8")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_double_4D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_0D(data_buf, array_size)
   implicit none
   real(R4), INTENT(IN)         :: data_buf
   integer, INTENT(IN)          :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real4")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_0D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_1D(data_buf, array_size)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:)         :: data_buf
   integer, INTENT(IN)                        :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real4")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_1D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_2D(data_buf, array_size)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:)       :: data_buf
   integer, INTENT(IN)                        :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real4")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_2D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_3D(data_buf, array_size)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:,:)     :: data_buf
   integer, INTENT(IN)                        :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real4")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_3D



   SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_4D(data_buf, array_size)
   implicit none
   real(R4), INTENT(IN), DIMENSION(:,:,:,:)   :: data_buf
   integer, INTENT(IN)                        :: array_size

   call coupling_perturb_roundoff_errors_for_an_array(data_buf, trim("real4")//char(0), array_size)

   END SUBROUTINE c_coupler_perturb_roundoff_errors_for_an_array_float_4D


   SUBROUTINE c_coupler_register_field_info(fld_name, units, long_name)
   implicit none
   character(len=*), intent(in) :: fld_name      
   character(len=*), intent(in) :: units 
   character(len=*), intent(in) :: long_name

   call coupling_add_field_info(trim(fld_name)//char(0), trim(units)//char(0), trim(long_name)//char(0)) 

   END SUBROUTINE c_coupler_register_field_info



   SUBROUTINE c_coupler_perturb_roundoff_errors
   implicit none

   call coupling_perturb_roundoff_errors
   
   END SUBROUTINE c_coupler_perturb_roundoff_errors



   integer FUNCTION CCPL_register_root_component(comp_comm, comp_type, annotation)
   use parse_compset_nml_mod
   implicit none
   integer                     :: rcode
   integer,external            :: chdir   ! LINUX system call
   integer                     :: comp_id
   integer :: ierr
   integer :: comp_comm
   character(len=*)            :: comp_type
   character(len=*), optional  :: annotation
   character *1024             :: local_annotation
   character *512    :: nml_filename
   character *512    :: exe_name
   integer have_random_seed_for_perturb_roundoff_errors
   logical mpi_running       ! returned value indicates if MPI_INIT has been called


   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call getarg(0, exe_name)
   call getarg(1, nml_filename)
   call parse_compset_nml(nml_filename)
   rcode=chdir(comp_run_data_dir(1:len_trim(comp_run_data_dir))) ! change working dir of current component
   open(unit=5,file=comp_model_nml ,status='UNKNOWN')            ! open the namelist of component and connect it to unit 5
   open(unit=6,file=comp_log_filename,position='APPEND')         ! open the log file of component running and connect it to unit 6 
   write(6,*) 'before call c++ interface'
   flush(6)
   call register_root_component(comp_comm, trim(component_name)//char(0), trim(comp_type)//char(0), trim(local_annotation)//char(0), comp_id, &
                        trim(exe_name)//char(0), trim(exp_model)//char(0),trim(case_name)//char(0), trim(case_desc)//char(0), trim(run_type)//char(0), &
                        trim(comp_model_nml)//char(0), trim(config_time)//char(0), &
                        trim(original_case_name)//char(0), trim(original_config_time)//char(0))
   CCPL_register_root_component = comp_id

   END FUNCTION CCPL_register_root_component



   integer FUNCTION CCPL_register_component(parent_id, comp_name, comp_type, comp_comm, annotation)
   implicit none
   integer                     :: parent_id
   integer                     :: comp_id
   character(len=*), optional  :: annotation
   character *1024             :: local_annotation
   integer                     :: comp_comm
   character(len=*)            :: comp_name
   character(len=*)            :: comp_type
   

   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call register_component(parent_id, trim(comp_name)//char(0), trim(comp_type)//char(0), comp_comm, trim(local_annotation)//char(0), comp_id)
   CCPL_register_component = comp_id

   END FUNCTION CCPL_register_component


   
   integer FUNCTION CCPL_get_component_id(comp_name, annotation)
   implicit none
   character(len=*), intent(in)            :: comp_name
   character(len=*), intent(in), optional  :: annotation
   character *1024                         :: local_annotation
   integer                     :: comp_id

   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif
 
   call get_id_of_component(trim(comp_name)//char(0), trim(local_annotation)//char(0), comp_id)
   CCPL_get_component_id = comp_id

   end FUNCTION CCPL_get_component_id



   integer FUNCTION CCPL_get_current_process_id_in_component(comp_id, annotation)
   implicit none
   integer, intent(in)                     :: comp_id
   character(len=*), intent(in), optional  :: annotation
   character *1024                         :: local_annotation
   integer                                 :: proc_id
   
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call get_current_proc_id_in_comp(comp_id, proc_id, local_annotation)
   CCPL_get_current_process_id_in_component = proc_id

   END FUNCTION CCPL_get_current_process_id_in_component
   


   integer FUNCTION CCPL_get_num_process_in_component(comp_id, annotation)
   implicit none
   integer, intent(in)                     :: comp_id
   character(len=*), intent(in), optional  :: annotation
   character *1024                         :: local_annotation
   integer                                 :: num_proc
   
   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif
   call get_num_proc_in_comp(comp_id, num_proc, annotation)
   CCPL_get_num_process_in_component = num_proc

   END FUNCTION CCPL_get_num_process_in_component



   SUBROUTINE CCPL_end_coupling_configuration(comp_id, annotation)
   implicit none
   integer                     :: comp_id
   character(len=*), optional  :: annotation
   character *1024             :: local_annotation

   local_annotation = ""
   if (present(annotation)) then
       local_annotation = annotation
   endif

   call end_registration(comp_id, trim(local_annotation)//char(0))

   END SUBROUTINE CCPL_end_coupling_configuration



   integer FUNCTION CCPL_get_CoR_defined_grid(comp_id, CCPL_grid_name, CoR_grid_name, annotation) 
   implicit none
   integer, intent(in)                     :: comp_id
   character(len=*), intent(in)            :: CCPL_grid_name
   character(len=*), intent(in)            :: CoR_grid_name
   character(len=*), intent(in), optional  :: annotation
   integer                                 :: grid_id

   if (present(annotation)) then
       call register_cor_defined_grid(comp_id, trim(CCPL_grid_name)//char(0), trim(CoR_grid_name)//char(0), trim(annotation)//char(0), grid_id)
   else 
       call register_cor_defined_grid(comp_id, trim(CCPL_grid_name)//char(0), trim(CoR_grid_name)//char(0), trim("")//char(0), grid_id)
   endif

   write(*,*) 'fortran grid id is ', grid_id

   CCPL_get_CoR_defined_grid = grid_id

   END FUNCTION CCPL_get_CoR_defined_grid



   integer FUNCTION CCPL_get_grid_size(grid_id, annotation) 
   implicit none
   integer, intent(in)                     :: grid_id
   integer                                 :: grid_size
   character(len=*), intent(in), optional  :: annotation

   if (present(annotation)) then
       call get_grid_size(grid_id, grid_size, trim(annotation)//char(0))
   else 
       call get_grid_size(grid_id, grid_size, trim("")//char(0))
   endif
   
   CCPL_get_grid_size = grid_size
    
   END FUNCTION CCPL_get_grid_size


   integer FUNCTION CCPL_register_parallel_decomp(decomp_name, grid_id, num_local_cells, local_cells_global_indx, annotation) 
   implicit none
   character(len=*), intent(in)                :: decomp_name
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in)                :: grid_id
   integer,          intent(in)                :: num_local_cells
   integer,          intent(in), dimension(:)  :: local_cells_global_indx(:)
   integer                                     :: decomp_id


   if (present(annotation)) then
       call register_parallel_decomposition(decomp_id, grid_id, num_local_cells, size(local_cells_global_indx), local_cells_global_indx, trim(decomp_name)//char(0), trim(annotation)//char(0))
   else 
       call register_parallel_decomposition(decomp_id, grid_id, num_local_cells, size(local_cells_global_indx), local_cells_global_indx, trim(decomp_name)//char(0), trim("")//char(0))
   endif

   CCPL_register_parallel_decomp = decomp_id

   end FUNCTION CCPL_register_parallel_decomp



   integer FUNCTION CCPL_define_single_timer(comp_id, frequency_unit, frequency_count, delay_count, annotation) 
   implicit none
   integer,          intent(in)                :: comp_id
   character(len=*), intent(in)                :: frequency_unit
   integer,          intent(in)                :: frequency_count
   integer,          intent(in), optional      :: delay_count
   character(len=*), intent(in), optional      :: annotation
   integer                                     :: local_delay_count
   integer                                     :: timer_id
 
   local_delay_count = 0
   if (present(delay_count)) local_delay_count = delay_count
   if (present(annotation)) then
        call define_single_timer(comp_id, timer_id, frequency_unit, frequency_count, local_delay_count, trim(annotation)//char(0))
   else
        call define_single_timer(comp_id, timer_id, frequency_unit, frequency_count, local_delay_count, trim("")//char(0))
        
   endif
   CCPL_define_single_timer = timer_id

   end FUNCTION CCPL_define_single_timer 



   integer FUNCTION CCPL_define_complex_timer(comp_id, children_timers_id, num_children_timers, OR_or_AND, annotation)
   implicit none
   integer,          intent(in)                :: comp_id
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in)                :: OR_or_AND
   integer,          intent(in)                :: num_children_timers
   integer                                     :: timer_id
   integer,          intent(in), dimension(:)  :: children_timers_id


   if (present(annotation)) then
       call define_complex_timer(comp_id, timer_id, children_timers_id, num_children_timers, OR_or_AND, trim(annotation)//char(0))
   else
       call define_complex_timer(comp_id, timer_id, children_timers_id, num_children_timers, OR_or_AND, trim("")//char(0))
   endif

   CCPL_define_complex_timer = timer_id

   end FUNCTION CCPL_define_complex_timer



   SUBROUTINE CCPL_set_time_step(comp_id, time_step_in_second, annotation)
   implicit none
   integer,          intent(in)                :: comp_id
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in)                :: time_step_in_second

   if (present(annotation)) then
       call set_component_time_step(comp_id, time_step_in_second, trim(annotation)//char(0))
   else
       call set_component_time_step(comp_id, time_step_in_second, trim("")//char(0))
   endif

   end SUBROUTINE CCPL_set_time_step



   SUBROUTINE CCPL_advance_time(comp_id, annotation)
   implicit none
   integer,          intent(in)                :: comp_id
   character(len=*), intent(in), optional      :: annotation

   if (present(annotation)) then
        call advance_component_time(comp_id, trim(annotation)//char(0))
   else
        call advance_component_time(comp_id, trim("")//char(0))
   endif

   END SUBROUTINE CCPL_advance_time



   logical FUNCTION CCPL_is_timer_on(timer_id, annotation)
   implicit none
   integer,          intent(in)                :: timer_id
   character(len=*), intent(in), optional      :: annotation
   integer                                     :: is_on

   if ((present(annotation))) then
       call is_ccpl_timer_on(timer_id, is_on, trim(annotation)//char(0))
   else
       call is_ccpl_timer_on(timer_id, is_on, trim("")//char(0))
   endif 
   
   if (is_on .eq. 1) then
       CCPL_is_timer_on = .true.
   else
       CCPL_is_timer_on = .false.
   endif

   end FUNCTION CCPL_is_timer_on



   SUBROUTINE CCPL_check_current_time(comp_id, date, second, annotation)
   implicit none
   integer,          intent(in)                :: comp_id
   integer,          intent(in)                :: date
   integer,          intent(in)                :: second
   character(len=*), intent(in), optional      :: annotation

   if (present(annotation)) then
        call check_ccpl_component_current_time(comp_id, date, second, trim(annotation)//char(0))
   else
        call check_ccpl_component_current_time(comp_id, date, second, trim("")//char(0))
   endif

   end SUBROUTINE CCPL_check_current_time


   SUBROUTINE CCPL_finalize()
   include 'mpif.h'
   integer ierr
   call MPI_BARRIER(MPI_COMM_WORLD,ierr)
   call MPI_FINALIZE(ierr)
   END SUBROUTINE  CCPL_finalize



   logical FUNCTION CCPL_is_model_run_ended(comp_id, annotation)
   implicit none
   integer,          intent(in)                :: comp_id
   character(len=*), intent(in), optional      :: annotation
   integer                                     :: is_ended

   if (present(annotation)) then
       call check_is_ccpl_model_run_ended(comp_id, is_ended, trim(annotation)//char(0))
   else
       call check_is_ccpl_model_run_ended(comp_id, is_ended, trim("")//char(0))
   endif

   if (is_ended .eq. 1) then
       CCPL_is_model_run_ended = .true.
   else 
       CCPL_is_model_run_ended = .false.
   endif

   end FUNCTION CCPL_is_model_run_ended



   integer FUNCTION CCPL_register_import_interface_1(interface_name, num_field_instances, field_instance_IDs, timer_IDs, inst_or_aver, annotation)
   implicit none
   character(len=*), intent(in)                :: interface_name
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in), dimension(:)  :: timer_IDs
   integer,          intent(in), dimension(:)  :: inst_or_aver
   integer,          intent(in), dimension(:)  :: field_instance_IDs
   integer,          intent(in)                :: num_field_instances
   integer                                     :: interface_id

   
   if (present(annotation)) then
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 0, num_field_instances, field_instance_IDs, timer_IDs, inst_or_aver, trim(annotation)//char(0), size(field_instance_IDs), size(timer_IDs), size(inst_or_aver))
   else
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 0, num_field_instances, field_instance_IDs, timer_IDs, inst_or_aver, trim("")//char(0), size(field_instance_IDs), size(timer_IDs), size(inst_or_aver))
   endif
   CCPL_register_import_interface_1 = interface_id;

   end FUNCTION CCPL_register_import_interface_1



   integer FUNCTION CCPL_register_import_interface_2(interface_name, num_field_instances,  field_instance_IDs, timer_ID, inst_or_aver, annotation)
   implicit none
   character(len=*), intent(in)                :: interface_name
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in)                :: timer_ID
   integer,          intent(in), dimension(:)  :: inst_or_aver
   integer,          intent(in), dimension(:)  :: field_instance_IDs
   integer,          intent(in)                :: num_field_instances
   integer                                     :: interface_id

   
   if (present(annotation)) then
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 0, num_field_instances, field_instance_IDs, timer_ID, inst_or_aver, trim(annotation)//char(0), size(field_instance_IDs), 1, size(inst_or_aver))
   else
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 0, num_field_instances, field_instance_IDs, timer_ID, inst_or_aver, trim("")//char(0), size(field_instance_IDs), 1, size(inst_or_aver))
   endif
   CCPL_register_import_interface_2 = interface_id;

   end FUNCTION CCPL_register_import_interface_2



   integer FUNCTION CCPL_register_import_interface_3(interface_name, num_field_instances, field_instance_IDs, timer_IDs, inst_or_aver, annotation)
   implicit none
   character(len=*), intent(in)                :: interface_name
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in), dimension(:)  :: timer_IDs
   integer,          intent(in)                :: inst_or_aver
   integer,          intent(in), dimension(:)  :: field_instance_IDs
   integer,          intent(in)                :: num_field_instances
   integer                                     :: interface_id

   
   if (present(annotation)) then
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 0, num_field_instances, field_instance_IDs, timer_IDs, inst_or_aver, trim(annotation)//char(0), size(field_instance_IDs), size(timer_IDs), 1)
   else
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 0, num_field_instances, field_instance_IDs, timer_IDs, inst_or_aver, trim("")//char(0), size(field_instance_IDs), size(timer_IDs), 1)
   endif
   CCPL_register_import_interface_3 = interface_id;

   end FUNCTION CCPL_register_import_interface_3



   integer FUNCTION CCPL_register_import_interface_4(interface_name, num_field_instances,  field_instance_IDs, timer_ID, inst_or_aver, annotation)
   implicit none
   character(len=*), intent(in)                :: interface_name
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in)                :: timer_ID
   integer,          intent(in)                :: inst_or_aver
   integer,          intent(in), dimension(:)  :: field_instance_IDs
   integer,          intent(in)                :: num_field_instances
   integer                                     :: interface_id

   
   if (present(annotation)) then
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 0, num_field_instances, field_instance_IDs, timer_ID, inst_or_aver, trim(annotation)//char(0), size(field_instance_IDs), 1, 1)
   else
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 0, num_field_instances, field_instance_IDs, timer_ID, inst_or_aver, trim("")//char(0), size(field_instance_IDs), 1, 1)
   endif
   CCPL_register_import_interface_4 = interface_id;

   end FUNCTION CCPL_register_import_interface_4




   integer FUNCTION CCPL_register_export_interface_1(interface_name, num_field_instances, field_instance_IDs, timer_IDs, annotation)
   implicit none
   character(len=*), intent(in)                :: interface_name
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in), dimension(:)  :: timer_IDs
   integer,          intent(in), dimension(:)  :: field_instance_IDs
   integer,          intent(in)                :: num_field_instances
   integer                                     :: interface_id

   
   if (present(annotation)) then
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 1, num_field_instances, field_instance_IDs, timer_IDs, field_instance_IDs, trim(annotation)//char(0), size(field_instance_IDs), size(timer_IDs), 0)
   else
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 1, num_field_instances, field_instance_IDs, timer_IDs, field_instance_IDs, trim("")//char(0), size(field_instance_IDs), size(timer_IDs), 0)
   endif
   CCPL_register_export_interface_1 = interface_id;

   end FUNCTION CCPL_register_export_interface_1



   integer FUNCTION CCPL_register_export_interface_2(interface_name, num_field_instances, field_instance_IDs, timer_ID, annotation)
   implicit none
   character(len=*), intent(in)                :: interface_name
   character(len=*), intent(in), optional      :: annotation
   integer,          intent(in)                :: timer_ID
   integer,          intent(in), dimension(:)  :: field_instance_IDs
   integer,          intent(in)                :: num_field_instances
   integer                                     :: interface_id

   
   if (present(annotation)) then
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 1, num_field_instances, field_instance_IDs, timer_ID, field_instance_IDs, trim(annotation)//char(0), size(field_instance_IDs), 1, 0)
   else
       call register_inout_interface(trim(interface_name)//char(0), interface_id, 1, num_field_instances, field_instance_IDs, timer_ID, field_instance_IDs, trim("")//char(0), size(field_instance_IDs), 1, 0)
   endif
   CCPL_register_export_interface_2 = interface_id;

   end FUNCTION CCPL_register_export_interface_2



   SUBROUTINE CCPL_execute_interface_using_id(interface_id, bypass_timers, annotation)
   implicit none
   integer,          intent(in)                :: interface_id
   logical,          intent(in)                :: bypass_timers
   character(len=*), intent(in), optional      :: annotation
   integer                                     :: local_bypass_timers


   if (bypass_timers) then
       local_bypass_timers = 1
   else
       local_bypass_timers = 0
   endif
   if (present(annotation)) then
       call execute_inout_interface_with_id(interface_id, local_bypass_timers, trim(annotation)//char(0))
   else 
       call execute_inout_interface_with_id(interface_id, local_bypass_timers, trim("")//char(0))
   endif

   END SUBROUTINE CCPL_execute_interface_using_id



   SUBROUTINE CCPL_execute_interface_using_name(component_id, interface_name, bypass_timers, annotation)
   implicit none
   integer,          intent(in)                :: component_id
   logical,          intent(in)                :: bypass_timers
   character(len=*), intent(in), optional      :: annotation
   character(len=*), intent(in)                :: interface_name
   integer                                     :: local_bypass_timers


   if (bypass_timers) then
       local_bypass_timers = 1
   else
       local_bypass_timers = 0
   endif
   if (present(annotation)) then
       call execute_inout_interface_with_name(component_id, trim(interface_name)//char(0), local_bypass_timers, trim(annotation)//char(0))
   else 
       call execute_inout_interface_with_name(component_id, trim(interface_name)//char(0), local_bypass_timers, trim("")//char(0))
   endif

   END SUBROUTINE CCPL_execute_interface_using_name



 END MODULE c_coupler_interface_mod
