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
   public :: c_coupler_initialize
   public :: c_coupler_finalize
   public :: c_coupler_abort
   public :: c_coupler_execute_procedure
   public :: c_coupler_do_restart_read
   public :: c_coupler_do_restart_write
   public :: c_coupler_advance_timer
   public :: c_coupler_check_coupled_run_finished
   public :: c_coupler_check_coupled_run_restart_time
   public :: c_coupler_get_current_date
   public :: c_coupler_get_global_sum_real16
   public :: c_coupler_register_decomposition
   public :: c_coupler_withdraw_model_data
   public :: c_coupler_get_current_time
   public :: c_coupler_get_num_elapsed_days_from_start
   public :: c_coupler_allreduce_real16
   public :: c_coupler_log_case_info_in_netcdf_file
   public :: c_coupler_check_sum_for_all_fields
   public :: c_coupler_export_field_instance

   interface c_coupler_register_model_data ; module procedure &
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
        c_coupler_register_model_integer_4D_data, &
        c_coupler_register_model_logical_0D_data, &
        c_coupler_register_model_logical_1D_data, &
        c_coupler_register_model_logical_2D_data, &
        c_coupler_register_model_logical_3D_data, &
        c_coupler_register_model_logical_4D_data, &
        c_coupler_register_model_string_data
   end interface


   interface c_coupler_check_sum_for_external_data ; module procedure &
        c_coupler_check_sum_for_external_double_0D_data, &
        c_coupler_check_sum_for_external_double_1D_data, &
        c_coupler_check_sum_for_external_double_2D_data, &
        c_coupler_check_sum_for_external_double_3D_data, &
        c_coupler_check_sum_for_external_double_4D_data, &
        c_coupler_check_sum_for_external_float_0D_data, &
        c_coupler_check_sum_for_external_float_1D_data, &
        c_coupler_check_sum_for_external_float_2D_data, &
        c_coupler_check_sum_for_external_float_3D_data, &
        c_coupler_check_sum_for_external_float_4D_data, &
        c_coupler_check_sum_for_external_integer_0D_data, &
        c_coupler_check_sum_for_external_integer_1D_data, &
        c_coupler_check_sum_for_external_integer_2D_data, &
        c_coupler_check_sum_for_external_integer_3D_data, &
        c_coupler_check_sum_for_external_integer_4D_data, &
        c_coupler_check_sum_for_external_logical_0D_data, &
        c_coupler_check_sum_for_external_logical_1D_data, &
        c_coupler_check_sum_for_external_logical_2D_data, &
        c_coupler_check_sum_for_external_logical_3D_data, &
        c_coupler_check_sum_for_external_logical_4D_data
   end interface


   interface c_coupler_is_model_data_renewed_in_current_time_step ; module procedure &
        c_coupler_is_model_double_0D_data_renewed_in_current_time_step,  &
        c_coupler_is_model_double_1D_data_renewed_in_current_time_step,  &
        c_coupler_is_model_double_2D_data_renewed_in_current_time_step,  &
        c_coupler_is_model_double_3D_data_renewed_in_current_time_step,  &
        c_coupler_is_model_double_4D_data_renewed_in_current_time_step,  &
        c_coupler_is_model_float_0D_data_renewed_in_current_time_step,   &
        c_coupler_is_model_float_1D_data_renewed_in_current_time_step,   &
        c_coupler_is_model_float_2D_data_renewed_in_current_time_step,   &
        c_coupler_is_model_float_3D_data_renewed_in_current_time_step,   &
        c_coupler_is_model_float_4D_data_renewed_in_current_time_step,   &
        c_coupler_is_model_integer_0D_data_renewed_in_current_time_step, &
        c_coupler_is_model_integer_1D_data_renewed_in_current_time_step, &
        c_coupler_is_model_integer_2D_data_renewed_in_current_time_step, &
        c_coupler_is_model_integer_3D_data_renewed_in_current_time_step, &
        c_coupler_is_model_integer_4D_data_renewed_in_current_time_step, &
        c_coupler_is_model_logical_0D_data_renewed_in_current_time_step, &
        c_coupler_is_model_logical_1D_data_renewed_in_current_time_step, &
        c_coupler_is_model_logical_2D_data_renewed_in_current_time_step, &
        c_coupler_is_model_logical_3D_data_renewed_in_current_time_step, &
        c_coupler_is_model_logical_4D_data_renewed_in_current_time_step
   end interface


   interface c_coupler_is_model_data_active_in_coupling ; module procedure &
        c_coupler_is_model_double_0D_data_active_in_coupling,  &
        c_coupler_is_model_double_1D_data_active_in_coupling,  &
        c_coupler_is_model_double_2D_data_active_in_coupling,  &
        c_coupler_is_model_double_3D_data_active_in_coupling,  &
        c_coupler_is_model_double_4D_data_active_in_coupling,  &
        c_coupler_is_model_float_0D_data_active_in_coupling,   &
        c_coupler_is_model_float_1D_data_active_in_coupling,   &
        c_coupler_is_model_float_2D_data_active_in_coupling,   &
        c_coupler_is_model_float_3D_data_active_in_coupling,   &
        c_coupler_is_model_float_4D_data_active_in_coupling,   &
        c_coupler_is_model_integer_0D_data_active_in_coupling, &
        c_coupler_is_model_integer_1D_data_active_in_coupling, &
        c_coupler_is_model_integer_2D_data_active_in_coupling, &
        c_coupler_is_model_integer_3D_data_active_in_coupling, &
        c_coupler_is_model_integer_4D_data_active_in_coupling, &
        c_coupler_is_model_logical_0D_data_active_in_coupling, &
        c_coupler_is_model_logical_1D_data_active_in_coupling, &
        c_coupler_is_model_logical_2D_data_active_in_coupling, &
        c_coupler_is_model_logical_3D_data_active_in_coupling, &
        c_coupler_is_model_logical_4D_data_active_in_coupling
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


   interface c_coupler_check_grid_values ; module procedure &
        c_coupler_check_float_grid_values, &
        c_coupler_check_double_grid_values, &
        c_coupler_check_integer_grid_values
   end interface

   interface c_coupler_get_current_calendar_time ; module procedure &
        c_coupler_get_double_current_calendar_time, &
        c_coupler_get_float_current_calendar_time 
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

   SUBROUTINE c_coupler_initialize(comp_comm,non_orbit)
   use parse_compset_nml_mod
   use orbital_params_mod
   use shr_orb_mod
   implicit none
   integer                 :: rcode
   integer,external        :: chdir   ! LINUX system call
   integer :: ierr
   integer :: comp_comm
   character *512    :: nml_filename
   logical, optional :: non_orbit
   integer have_random_seed_for_perturb_roundoff_errors
   logical mpi_running       ! returned value indicates if MPI_INIT has been called

   call mpi_initialized (mpi_running, ierr)
   if (.not.mpi_running) then
       call MPI_INIT(ierr)
   endif

   call getarg(1, nml_filename)
   call parse_compset_nml(nml_filename)
   rcode=chdir(comp_run_data_dir(1:len_trim(comp_run_data_dir))) ! change working dir of current component
   open(unit=5,file=comp_model_nml ,status='UNKNOWN')            ! open the namelist of component and connect it to unit 5
   open(unit=6,file=comp_log_filename,position='APPEND')         ! open the log file of component running and connect it to unit 6 
   call comm_initialize(trim(exp_model)//char(0), trim(component_name)//char(0), trim(compset_filename)//char(0), comp_comm, &
                        trim(case_name)//char(0), trim(case_desc)//char(0), trim(run_type)//char(0), &
                        trim(comp_model_nml)//char(0), trim(config_time)//char(0), &
                        trim(original_case_name)//char(0), trim(original_config_time)//char(0))
   call initialize_coupler_timer(start_date, start_second, stop_date, stop_second, reference_date, leap_year, cpl_interface_time_step, &
                                 trim(rest_freq_unit)//char(0), rest_freq_count, stop_latency_seconds)
   call initialize_coupling_managers(restart_date, restart_second, trim(restart_read_file)//char(0))

   if (.not.(present(non_orbit) .and. non_orbit)) then
      call parse_orb_nml(nml_filename)
      call c_coupler_register_model_data(iyear_AD, "NULL", "input_orbYear",.false.)
      call c_coupler_register_model_data(eccen, "NULL", "input_orbEccen",.false.)
      call c_coupler_register_model_data(obliq, "NULL", "input_orbObliq",.false.)
      call c_coupler_register_model_data(obliqr, "NULL", "input_orbObliqr",.false.)
      call c_coupler_register_model_data(mvelp, "NULL", "input_orbMvelp",.false.)
      call c_coupler_register_model_data(mvelpp, "NULL", "input_orbMvelpp",.false.)
      call c_coupler_register_model_data(lambm0, "NULL", "input_orbLambm0",.false.)
      call shr_orb_params(iyear_AD , eccen  , obliq , mvelp, obliqr   , lambm0 , mvelpp, .true.)
   end if

   reduce_buf_real16_size = 1
   allocate(reduce_buf_real16(reduce_buf_real16_size))

   if (ensemble_member_id .gt. 0) then
       have_random_seed_for_perturb_roundoff_errors = 0
       if (random_seed_for_perturb_roundoff_errors .ge. 0) then
           have_random_seed_for_perturb_roundoff_errors = 1
       endif
       call coupling_initialize_ensemble_manager(ensemble_member_id, have_random_seed_for_perturb_roundoff_errors, &
                          random_seed_for_perturb_roundoff_errors, trim(roundoff_errors_perturbation_type)//char(0))
   endif
   
   END SUBROUTINE  c_coupler_initialize



   SUBROUTINE c_coupler_finalize()
   include 'mpif.h'
   integer ierr
   call MPI_BARRIER(MPI_COMM_WORLD,ierr)
   call finalize_coupling_managers()
   call MPI_FINALIZE(ierr)
   END SUBROUTINE  c_coupler_finalize



   SUBROUTINE c_coupler_execute_procedure(procedure_name, procedure_stage)
   IMPLICIT none
   character(len=*),     intent(in)    ::  procedure_name
   character(len=*),     intent(in)    ::  procedure_stage

   call coupling_execute_procedure(trim(procedure_name)//char(0), trim(procedure_stage)//char(0))

   END SUBROUTINE c_coupler_execute_procedure



   SUBROUTINE c_coupler_withdraw_model_data(decomp_name, field_name, grid_name)
   implicit none
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character(len=*), optional   :: grid_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   call withdraw_model_data(trim(decomp_name)//char(0), trim(field_name)//char(0), trim(local_grid_name)//char(0))

   END SUBROUTINE c_coupler_withdraw_model_data




   SUBROUTINE c_coupler_export_field_instance(data_buf, field_name, decomp_name, grid_name)
   implicit none
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character(len=*), intent(in) :: grid_name
   real(R8),         intent(in) :: data_buf
  
   call export_field_data(data_buf, 1, trim(field_name)//char(0), &
      trim(decomp_name)//char(0), trim(grid_name)//char(0), trim("real8")//char(0))

   END SUBROUTINE c_coupler_export_field_instance



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



   SUBROUTINE c_coupler_register_model_double_0D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R8), INTENT(IN)         :: data_buf
   real(R8), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R8)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_double_0D_data


   SUBROUTINE c_coupler_register_model_double_1D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R8), DIMENSION(:)       :: data_buf
   real(R8), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R8)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_double_1D_data


   SUBROUTINE c_coupler_register_model_double_2D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R8), DIMENSION(:,:)     :: data_buf
   real(R8), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R8)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_double_2D_data


   SUBROUTINE c_coupler_register_model_double_3D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R8), DIMENSION(:,:,:)   :: data_buf
   real(R8), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R8)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_double_3D_data


   SUBROUTINE c_coupler_register_model_double_4D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R8), DIMENSION(:,:,:,:) :: data_buf
   real(R8), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R8)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real8")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_double_4D_data


   SUBROUTINE c_coupler_register_model_float_0D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R4)                     :: data_buf
   real(R4), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R4)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_float_0D_data


   SUBROUTINE c_coupler_register_model_float_1D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R4), DIMENSION(:)       :: data_buf
   real(R4), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R4)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_float_1D_data


   SUBROUTINE c_coupler_register_model_float_2D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R4), DIMENSION(:,:)     :: data_buf
   real(R4), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R4)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_float_2D_data


   SUBROUTINE c_coupler_register_model_float_3D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R4), DIMENSION(:,:,:)   :: data_buf
   real(R4), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R4)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_float_3D_data


   SUBROUTINE c_coupler_register_model_float_4D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   real(R4), DIMENSION(:,:,:,:) :: data_buf
   real(R4), optional           :: fill_value
   character(len=*), optional   :: grid_name
   real(R4)                     :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("real4")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_float_4D_data


   SUBROUTINE c_coupler_register_model_integer_0D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   integer                      :: data_buf
   integer, optional            :: fill_value
   character(len=*), optional   :: grid_name
   integer                      :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_integer_0D_data


   SUBROUTINE c_coupler_register_model_integer_1D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   integer,DIMENSION(:)         :: data_buf
   integer, optional            :: fill_value
   character(len=*), optional   :: grid_name
   integer                      :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_integer_1D_data


   SUBROUTINE c_coupler_register_model_integer_2D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   integer,DIMENSION(:,:)       :: data_buf
   integer, optional            :: fill_value
   character(len=*), optional   :: grid_name
   integer                      :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_integer_2D_data


   SUBROUTINE c_coupler_register_model_integer_3D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   integer,DIMENSION(:,:,:)     :: data_buf
   integer, optional            :: fill_value
   character(len=*), optional   :: grid_name
   integer                      :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_integer_3D_data


   SUBROUTINE c_coupler_register_model_integer_4D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   integer,DIMENSION(:,:,:,:)   :: data_buf
   integer, optional            :: fill_value
   character(len=*), optional   :: grid_name
   integer                      :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("integer")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_integer_4D_data


   SUBROUTINE c_coupler_register_model_logical_0D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   logical(1)                    :: data_buf
   logical(1), optional          :: fill_value
   character(len=*), optional    :: grid_name
   logical(1)                    :: local_fill_value
   logical                       :: is_restart_field
   integer                       :: have_fill_value
   character(len=*), intent(in)  :: decomp_name
   character(len=*), intent(in)  :: field_name
   character *512                :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_logical_0D_data


   SUBROUTINE c_coupler_register_model_logical_1D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   logical(1),DIMENSION(:)       :: data_buf
   logical(1), optional          :: fill_value
   character(len=*), optional    :: grid_name
   logical(1)                    :: local_fill_value
   logical                       :: is_restart_field
   integer                       :: have_fill_value
   character(len=*), intent(in)  :: decomp_name
   character(len=*), intent(in)  :: field_name
   character *512                :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_logical_1D_data


   SUBROUTINE c_coupler_register_model_logical_2D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   logical(1),DIMENSION(:,:)     :: data_buf
   logical(1), optional          :: fill_value
   character(len=*), optional    :: grid_name
   logical(1)                    :: local_fill_value
   logical                       :: is_restart_field
   integer                       :: have_fill_value
   character(len=*), intent(in)  :: decomp_name
   character(len=*), intent(in)  :: field_name
   character *512                :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_logical_2D_data


   SUBROUTINE c_coupler_register_model_logical_3D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   logical(1),DIMENSION(:,:,:)   :: data_buf
   logical(1), optional          :: fill_value
   character(len=*), optional    :: grid_name
   logical(1)                    :: local_fill_value
   logical                       :: is_restart_field
   integer                       :: have_fill_value
   character(len=*), intent(in)  :: decomp_name
   character(len=*), intent(in)  :: field_name
   character *512                :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_logical_3D_data


   SUBROUTINE c_coupler_register_model_logical_4D_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   logical(1),DIMENSION(:,:,:,:) :: data_buf
   logical(1), optional          :: fill_value
   character(len=*), optional    :: grid_name
   logical(1)                    :: local_fill_value
   logical                       :: is_restart_field
   integer                       :: have_fill_value
   character(len=*), intent(in)  :: decomp_name
   character(len=*), intent(in)  :: field_name
   character *512                :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, size(data_buf), trim(decomp_name)//char(0), trim(field_name)//char(0), trim("logical")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_logical_4D_data


   SUBROUTINE c_coupler_register_model_string_data(data_buf, decomp_name, field_name, is_restart_field, grid_name, fill_value)
   implicit none
   character(len=*)             :: data_buf
   character, optional          :: fill_value
   character(len=*), optional   :: grid_name
   character                    :: local_fill_value
   logical                      :: is_restart_field
   integer                      :: have_fill_value
   character(len=*), intent(in) :: decomp_name
   character(len=*), intent(in) :: field_name
   character *512               :: local_grid_name

   local_grid_name = "none"
   if (present(grid_name)) then
       local_grid_name = grid_name
   endif

   if (present(fill_value)) then
       have_fill_value = 1
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("string")//char(0), trim(local_grid_name)//char(0), have_fill_value, fill_value, is_restart_field)
   else
       have_fill_value = 0
       call register_model_data(data_buf, 1, trim(decomp_name)//char(0), trim(field_name)//char(0), trim("string")//char(0), trim(local_grid_name)//char(0), have_fill_value, local_fill_value, is_restart_field)
   end if
   END SUBROUTINE c_coupler_register_model_string_data


   SUBROUTINE c_coupler_check_sum_for_external_double_0D_data(data_buf, array_size, hint)
   implicit none
   real(R8)                      :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real8")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_double_0D_data


   SUBROUTINE c_coupler_check_sum_for_external_double_1D_data(data_buf, array_size, hint)
   implicit none
   real(R8),DIMENSION(:)         :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real8")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_double_1D_data


   SUBROUTINE c_coupler_check_sum_for_external_double_2D_data(data_buf, array_size, hint)
   implicit none
   real(R8),DIMENSION(:,:)       :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real8")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_double_2D_data


   SUBROUTINE c_coupler_check_sum_for_external_double_3D_data(data_buf, array_size, hint)
   implicit none
   real(R8),DIMENSION(:,:,:)     :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real8")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_double_3D_data


   SUBROUTINE c_coupler_check_sum_for_external_double_4D_data(data_buf, array_size, hint)
   implicit none
   real(R8),DIMENSION(:,:,:,:)   :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real8")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_double_4D_data


   SUBROUTINE c_coupler_check_sum_for_external_float_0D_data(data_buf, array_size, hint)
   implicit none
   real(R4)                      :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real4")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_float_0D_data


   SUBROUTINE c_coupler_check_sum_for_external_float_1D_data(data_buf, array_size, hint)
   implicit none
   real(R4),DIMENSION(:)         :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real4")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_float_1D_data


   SUBROUTINE c_coupler_check_sum_for_external_float_2D_data(data_buf, array_size, hint)
   implicit none
   real(R4),DIMENSION(:,:)       :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real4")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_float_2D_data


   SUBROUTINE c_coupler_check_sum_for_external_float_3D_data(data_buf, array_size, hint)
   implicit none
   real(R4),DIMENSION(:,:,:)     :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real4")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_float_3D_data


   SUBROUTINE c_coupler_check_sum_for_external_float_4D_data(data_buf, array_size, hint)
   implicit none
   real(R4),DIMENSION(:,:,:,:)   :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("real4")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_float_4D_data


   SUBROUTINE c_coupler_check_sum_for_external_integer_0D_data(data_buf, array_size, hint)
   implicit none
   integer                       :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("integer")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_integer_0D_data


   SUBROUTINE c_coupler_check_sum_for_external_integer_1D_data(data_buf, array_size, hint)
   implicit none
   integer,DIMENSION(:)          :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("integer")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_integer_1D_data


   SUBROUTINE c_coupler_check_sum_for_external_integer_2D_data(data_buf, array_size, hint)
   implicit none
   integer,DIMENSION(:,:)        :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("integer")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_integer_2D_data


   SUBROUTINE c_coupler_check_sum_for_external_integer_3D_data(data_buf, array_size, hint)
   implicit none
   integer,DIMENSION(:,:,:)      :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("integer")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_integer_3D_data


   SUBROUTINE c_coupler_check_sum_for_external_integer_4D_data(data_buf, array_size, hint)
   implicit none
   integer,DIMENSION(:,:,:,:)    :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("integer")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_integer_4D_data


   SUBROUTINE c_coupler_check_sum_for_external_logical_0D_data(data_buf, array_size, hint)
   implicit none
   logical(1)                    :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("logical")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_logical_0D_data


   SUBROUTINE c_coupler_check_sum_for_external_logical_1D_data(data_buf, array_size, hint)
   implicit none
   logical(1),DIMENSION(:)       :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("logical")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_logical_1D_data


   SUBROUTINE c_coupler_check_sum_for_external_logical_2D_data(data_buf, array_size, hint)
   implicit none
   logical(1),DIMENSION(:,:)     :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("logical")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_logical_2D_data


   SUBROUTINE c_coupler_check_sum_for_external_logical_3D_data(data_buf, array_size, hint)
   implicit none
   logical(1),DIMENSION(:,:,:)   :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("logical")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_logical_3D_data


   SUBROUTINE c_coupler_check_sum_for_external_logical_4D_data(data_buf, array_size, hint)
   implicit none
   logical(1),DIMENSION(:,:,:,:) :: data_buf
   integer                       :: array_size
   character(len=*), intent(in)  :: hint

   call coupling_check_sum_for_external_data(data_buf, array_size, trim("logical")//char(0), trim(hint)//char(0))

   END SUBROUTINE c_coupler_check_sum_for_external_logical_4D_data


   logical FUNCTION c_coupler_is_model_double_0D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R8)                     :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_0D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_double_0D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_double_0D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_double_1D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R8),DIMENSION(:)        :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_1D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_double_1D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_double_1D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_double_2D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R8),DIMENSION(:,:)      :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_2D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_double_2D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_double_2D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_double_3D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R8),DIMENSION(:,:,:)    :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_3D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_double_3D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_double_3D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_double_4D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R8),DIMENSION(:,:,:,:)  :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_4D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_double_4D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_double_4D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_float_0D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R4)                     :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_0D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_float_0D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_float_0D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_float_1D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R4),DIMENSION(:)        :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_1D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_float_1D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_float_1D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_float_2D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R4),DIMENSION(:,:)      :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_2D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_float_2D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_float_2D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_float_3D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R4),DIMENSION(:,:,:)    :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_3D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_float_3D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_float_3D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_float_4D_data_renewed_in_current_time_step(data_buf)
   implicit none
   real(R4),DIMENSION(:,:,:,:)  :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_4D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_float_4D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_float_4D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_integer_0D_data_renewed_in_current_time_step(data_buf)
   implicit none
   integer                      :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_0D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_integer_0D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_0D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_integer_1D_data_renewed_in_current_time_step(data_buf)
   implicit none
   integer,DIMENSION(:)         :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_1D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_integer_1D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_1D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_integer_2D_data_renewed_in_current_time_step(data_buf)
   implicit none
   integer,DIMENSION(:,:)       :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_2D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_integer_2D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_2D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_integer_3D_data_renewed_in_current_time_step(data_buf)
   implicit none
   integer,DIMENSION(:,:,:)     :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_3D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_integer_3D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_3D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_integer_4D_data_renewed_in_current_time_step(data_buf)
   implicit none
   integer,DIMENSION(:,:,:,:)   :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_4D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_integer_4D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_4D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_logical_0D_data_renewed_in_current_time_step(data_buf)
   implicit none
   logical(1)                   :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_0D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_logical_0D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_0D_data_renewed_in_current_time_step


   logical FUNCTION c_coupler_is_model_logical_1D_data_renewed_in_current_time_step(data_buf)
   implicit none
   logical(1),DIMENSION(:)      :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_1D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_logical_1D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_1D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_logical_2D_data_renewed_in_current_time_step(data_buf)
   implicit none
   logical(1),DIMENSION(:,:)    :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_2D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_logical_2D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_2D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_logical_3D_data_renewed_in_current_time_step(data_buf)
   implicit none
   logical(1),DIMENSION(:,:,:)  :: data_buf
   integer                      :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_3D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_logical_3D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_3D_data_renewed_in_current_time_step



   logical FUNCTION c_coupler_is_model_logical_4D_data_renewed_in_current_time_step(data_buf)
   implicit none
   logical(1),DIMENSION(:,:,:,:) :: data_buf
   integer                       :: result

   call coupling_is_model_data_renewed_in_current_time_step(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_4D_data_renewed_in_current_time_step = .true.
   else
       c_coupler_is_model_logical_4D_data_renewed_in_current_time_step = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_4D_data_renewed_in_current_time_step



   SUBROUTINE c_coupler_register_decomposition(decomp_name, grid_name, num_local_cells, local_cell_indexes)
   implicit none
   character(len=*),     intent(in)    ::  decomp_name
   character(len=*),     intent(in)    ::  grid_name
   integer, intent(in)                 ::  num_local_cells
   integer, intent(in)                 ::  local_cell_indexes(:)
   
   call coupling_add_decomposition(trim(decomp_name)//char(0), trim(grid_name)//char(0), num_local_cells, local_cell_indexes);

   END SUBROUTINE c_coupler_register_decomposition


 SUBROUTINE c_coupler_do_restart_read

   call coupling_do_restart_read()

 END SUBROUTINE c_coupler_do_restart_read


 SUBROUTINE c_coupler_do_restart_write

   call coupling_do_restart_write()

 END SUBROUTINE c_coupler_do_restart_write



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



 SUBROUTINE c_coupler_advance_timer

   call coupling_advance_timer()
 
 END SUBROUTINE c_coupler_advance_timer


 
 logical FUNCTION c_coupler_check_coupled_run_finished
   implicit none
   logical is_coupled_run_ended

   call coupling_check_coupled_run_finished(is_coupled_run_ended)
   c_coupler_check_coupled_run_finished = is_coupled_run_ended

 END FUNCTION c_coupler_check_coupled_run_finished


 logical FUNCTION c_coupler_check_coupled_run_restart_time
   implicit none
   logical is_coupled_run_restart_time

   call coupling_check_coupled_run_restart_time(is_coupled_run_restart_time)
   c_coupler_check_coupled_run_restart_time = is_coupled_run_restart_time

 END FUNCTION c_coupler_check_coupled_run_restart_time



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


 SUBROUTINE c_coupler_check_float_grid_values(decomp_name, grid_name, value_label, grid_values)
   implicit none
   character(len=*),     intent(in)    ::  decomp_name
   character(len=*),     intent(in)    ::  grid_name
   character(len=*),     intent(in)    ::  value_label
   real(R4),             intent(in)    ::  grid_values(:)

   call coupling_check_grid_values_consistency(trim(decomp_name)//char(0), trim(grid_name)//char(0), trim(value_label)//char(0), trim('real4')//char(0), grid_values)
 END SUBROUTINE c_coupler_check_float_grid_values


 SUBROUTINE c_coupler_check_double_grid_values(decomp_name, grid_name, value_label, grid_values)
   implicit none
   character(len=*),     intent(in)    ::  decomp_name
   character(len=*),     intent(in)    ::  grid_name
   character(len=*),     intent(in)    ::  value_label
   real(R8),             intent(in)    ::  grid_values(:)

   call coupling_check_grid_values_consistency(trim(decomp_name)//char(0), trim(grid_name)//char(0), trim(value_label)//char(0), trim('real8')//char(0), grid_values)
 END SUBROUTINE c_coupler_check_double_grid_values


 SUBROUTINE c_coupler_check_integer_grid_values(decomp_name, grid_name, value_label, grid_values)
   implicit none
   character(len=*),     intent(in)    ::  decomp_name
   character(len=*),     intent(in)    ::  grid_name
   character(len=*),     intent(in)    ::  value_label
   integer,              intent(in)    ::  grid_values(:)

   call coupling_check_grid_values_consistency(trim(decomp_name)//char(0), trim(grid_name)//char(0), trim(value_label)//char(0), trim('integer')//char(0), grid_values)
 END SUBROUTINE c_coupler_check_integer_grid_values



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
 


 SUBROUTINE c_coupler_get_global_sum_real16(local_sums, global_sums, num_data)
   implicit none
   include 'mpif.h'
   real(R16)       :: local_sums(:)
   real(R16)       :: global_sums(:)
   integer         :: num_data
   integer         :: local_comm
   integer         :: ierr

   call coupling_get_current_comp_comm(local_comm)
   call c_coupler_allreduce_real16(local_sums,global_sums,num_data,local_comm)
 END SUBROUTINE c_coupler_get_global_sum_real16



 SUBROUTINE c_coupler_log_case_info_in_netcdf_file(ncfile_id, not_at_def_mode)
   implicit none
   integer           :: ncfile_id
   logical, optional :: not_at_def_mode
   integer           :: local_not_at_def_mode

   local_not_at_def_mode = 0
   if (present(not_at_def_mode) .and. not_at_def_mode) then
       local_not_at_def_mode = 1
   end if
   call coupling_log_case_info_in_netcdf_file(ncfile_id, local_not_at_def_mode)
 END SUBROUTINE c_coupler_log_case_info_in_netcdf_file



 SUBROUTINE c_coupler_check_sum_for_all_fields
   implicit none
   CALL coupling_check_sum_for_all_fields
 END SUBROUTINE c_coupler_check_sum_for_all_fields



 SUBROUTINE c_coupler_abort(error_string)
   implicit none
   character(len=*),     intent(in)    ::  error_string

   call coupling_abort(trim(error_string)//char(0))

 END SUBROUTINE c_coupler_abort


   logical FUNCTION c_coupler_is_model_double_0D_data_active_in_coupling(data_buf)
   implicit none
   real(R8)                     :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_0D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_double_0D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_double_0D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_double_1D_data_active_in_coupling(data_buf)
   implicit none
   real(R8),DIMENSION(:)        :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_1D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_double_1D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_double_1D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_double_2D_data_active_in_coupling(data_buf)
   implicit none
   real(R8),DIMENSION(:,:)      :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_2D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_double_2D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_double_2D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_double_3D_data_active_in_coupling(data_buf)
   implicit none
   real(R8),DIMENSION(:,:,:)    :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_3D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_double_3D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_double_3D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_double_4D_data_active_in_coupling(data_buf)
   implicit none
   real(R8),DIMENSION(:,:,:,:)  :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_double_4D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_double_4D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_double_4D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_float_0D_data_active_in_coupling(data_buf)
   implicit none
   real(R4)                     :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_0D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_float_0D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_float_0D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_float_1D_data_active_in_coupling(data_buf)
   implicit none
   real(R4),DIMENSION(:)        :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_1D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_float_1D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_float_1D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_float_2D_data_active_in_coupling(data_buf)
   implicit none
   real(R4),DIMENSION(:,:)      :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_2D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_float_2D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_float_2D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_float_3D_data_active_in_coupling(data_buf)
   implicit none
   real(R4),DIMENSION(:,:,:)    :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_3D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_float_3D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_float_3D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_float_4D_data_active_in_coupling(data_buf)
   implicit none
   real(R4),DIMENSION(:,:,:,:)  :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_float_4D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_float_4D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_float_4D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_integer_0D_data_active_in_coupling(data_buf)
   implicit none
   integer                      :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_0D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_integer_0D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_0D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_integer_1D_data_active_in_coupling(data_buf)
   implicit none
   integer,DIMENSION(:)         :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_1D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_integer_1D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_1D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_integer_2D_data_active_in_coupling(data_buf)
   implicit none
   integer,DIMENSION(:,:)       :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_2D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_integer_2D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_2D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_integer_3D_data_active_in_coupling(data_buf)
   implicit none
   integer,DIMENSION(:,:,:)     :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_3D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_integer_3D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_3D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_integer_4D_data_active_in_coupling(data_buf)
   implicit none
   integer,DIMENSION(:,:,:,:)   :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_integer_4D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_integer_4D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_integer_4D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_logical_0D_data_active_in_coupling(data_buf)
   implicit none
   logical(1)                   :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_0D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_logical_0D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_0D_data_active_in_coupling


   logical FUNCTION c_coupler_is_model_logical_1D_data_active_in_coupling(data_buf)
   implicit none
   logical(1),DIMENSION(:)      :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_1D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_logical_1D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_1D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_logical_2D_data_active_in_coupling(data_buf)
   implicit none
   logical(1),DIMENSION(:,:)    :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_2D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_logical_2D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_2D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_logical_3D_data_active_in_coupling(data_buf)
   implicit none
   logical(1),DIMENSION(:,:,:)  :: data_buf
   integer                      :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_3D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_logical_3D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_3D_data_active_in_coupling



   logical FUNCTION c_coupler_is_model_logical_4D_data_active_in_coupling(data_buf)
   implicit none
   logical(1),DIMENSION(:,:,:,:) :: data_buf
   integer                       :: result

   call coupling_is_model_data_active_in_coupling(data_buf, result)
   if (result .eq. 1) then
       c_coupler_is_model_logical_4D_data_active_in_coupling = .true.
   else
       c_coupler_is_model_logical_4D_data_active_in_coupling = .false.
   end if

   END FUNCTION c_coupler_is_model_logical_4D_data_active_in_coupling



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



 END MODULE c_coupler_interface_mod
