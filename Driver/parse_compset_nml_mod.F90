!*************************************************************
!  Copyright (c) 2013, Tsinghua University.
!  This is a source file of C-Coupler.
!  If you have any problem, 
!  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
!*************************************************************


module parse_compset_nml_mod

   character *512, public :: exp_model
   character *512, public :: case_name
   character *512, public :: original_case_name
   character *512, public :: case_desc
   character *512, public :: config_time
   character *512, public :: original_config_time
   character *512, public :: run_type              ! initial run, restart run
   integer       , public :: start_date            ! yyyymmdd
   integer       , public :: start_second          
   integer       , public :: stop_date             ! yyyymmdd
   integer       , public :: stop_second          
   integer       , public :: restart_date          ! yyyymmdd
   integer       , public :: restart_second         
   character *512, public :: rest_freq_unit
   integer       , public :: rest_freq_count
   character *512, public :: component_name
   character *512, public :: compset_filename
   character *512, public :: comp_run_data_dir
   character *512, public :: comp_model_nml
   character *512, public :: comp_log_filename
   integer       , public :: cpl_interface_time_step
   integer       , public :: stop_latency_seconds
   logical       , public :: leap_year
   character *512, public :: restart_read_file


contains
subroutine parse_compset_nml(compset_nml_filename)
   implicit none
   include 'mpif.h'
   integer :: rcode
   character *512         :: compset_nml_filename

   namelist /compset_nml/ exp_model, case_name, case_desc, run_type, start_date, start_second, stop_date, &
                      stop_second, rest_freq_unit, rest_freq_count, component_name, compset_filename, &
                      config_time, comp_run_data_dir, comp_model_nml, comp_log_filename, &
                      restart_date, restart_second, original_case_name, original_config_time, &
                      restart_read_file, cpl_interface_time_step, stop_latency_seconds, leap_year

   exp_model = "none"
   case_name = "none"
   original_case_name = "none"
   case_desc = "none"
   config_time = "none"
   original_config_time = "none"
   run_type = "none"
   start_date = -1
   restart_date = -1
   stop_date = -1
   rest_freq_unit = "none"
   rest_freq_count = -1
   component_name = "none"
   compset_filename = "none"
   comp_run_data_dir = "none"
   comp_model_nml = "none"
   comp_log_filename = "none"
   cpl_interface_time_step = -1
   stop_latency_seconds = -1
   leap_year = .false.
   restart_read_file = "none"

   open (10,file=compset_nml_filename,form='formatted',status='OLD')
   read (10,nml=compset_nml,iostat=rcode)
   close(10)
   
   if (rcode /= 0) then
      write(6,*)'read coupling namelist error, returns ', rcode
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (exp_model .eq. "none") then
      write(6,*) "The name of experiment model is unknown in namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (case_name .eq. "none") then
      write(6,*) "case_name has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (config_time .eq. "none") then
      write(6,*) "config_time has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (run_type .eq. "restart") then
      if (original_config_time .eq. "none") then
         write(6,*) "original_config_time has not been set in coupling namelist:", compset_nml_filename
         call mpi_abort (MPI_COMM_WORLD, 1)
      end if
      if (restart_read_file .eq. "none") then
         write(6,*) "restart_read_file has not been set in coupling namelist:", compset_nml_filename
         call mpi_abort (MPI_COMM_WORLD, 1)
      end if
   end if

   if (run_type .eq. "restart") then
      if (original_case_name .eq. "none") then
         write(6,*) "original_case_name has not been set in coupling namelist:", compset_nml_filename
         call mpi_abort (MPI_COMM_WORLD, 1)
      end if
   end if

   if (case_desc .eq. "none") then
      write(6,*) "case_desc has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (.not.(run_type .eq. "initial" .or. run_type .eq. "restart" .or. run_type .eq. "hybrid")) then
      write(6,*) "run_type must be set to one of initial, restart and hybrid in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if
   
   if (.not.(rest_freq_unit .eq. "seconds" .or. rest_freq_unit .eq. "days" .or. &
             rest_freq_unit .eq. "months" .or. rest_freq_unit .eq. "years")) then
      write(6,*) "rest_freq_unit must be set to one of seconds, days, months and years in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (compset_filename .eq. "none") then
      write(6,*) "compset_filename has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if
     
   if (comp_run_data_dir .eq. "none") then
      write(6,*) "comp_run_data_dir has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (comp_model_nml .eq. "none") then
      write(6,*) "comp_model_nml has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (comp_log_filename .eq. "none") then
      write(6,*) "comp_log_filename has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (start_date .lt. 0) then
      write(6,*) "start_date has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (stop_date .lt. 0) then
      write(6,*) "stop_date has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (stop_date .le. start_date) then
      write(6,*) "stop_date must be at least one day after the start_date in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (rest_freq_count .lt. 0) then
      write(6,*) "rest_freq_count has not been set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (cpl_interface_time_step .lt. 0) then
      write(6,*) "cpl_interface_time_step has not been correctly set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

   if (stop_latency_seconds .lt. 0) then
      write(6,*) "stop_latency_seconds has not been correctly set in coupling namelist:", compset_nml_filename
      call mpi_abort (MPI_COMM_WORLD, 1)
   end if

end subroutine parse_compset_nml

end module parse_compset_nml_mod 
