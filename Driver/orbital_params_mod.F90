!*************************************************************
!  Copyright (c) 2013, Tsinghua University.
!  This is a source file of C-Coupler.
!  If you have any problem, 
!  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
!*************************************************************


module orbital_params_mod

   integer , parameter, private :: R8  = selected_real_kind(12) ! 8 byte real
   integer ,            public  :: iyear_AD                     ! Year to calculate orbit for
   real(R8),            public  :: eccen                        ! orbital eccentricity
   real(R8),            public  :: obliq                        ! obliquity in degrees
   real(R8),            public  :: mvelp                        ! moving vernal equinox long
   real(R8),            public  :: obliqr                       ! Earths obliquity in rad
   real(R8),            public  :: lambm0                       ! Mean long of perihelion at vernal equinox (radians)
   real(R8),            public  :: mvelpp                       ! moving vernal equinox long


contains


   subroutine parse_orb_nml(nml_filename)
   use shr_orb_mod
   implicit none
   integer :: rcode
   character *512         :: nml_filename

   namelist /orb_nml/ iyear_AD, eccen, obliq, mvelp, obliqr, lambm0, mvelpp

   iyear_AD = SHR_ORB_UNDEF_INT 
   eccen    = SHR_ORB_UNDEF_REAL 
   obliq    = SHR_ORB_UNDEF_REAL 
   mvelp    = SHR_ORB_UNDEF_REAL 
   obliqr   = SHR_ORB_UNDEF_REAL 
   lambm0   = SHR_ORB_UNDEF_REAL 
   mvelpp   = SHR_ORB_UNDEF_REAL 

   open (10,file=nml_filename,form='formatted',status='OLD')
   read (10,nml=orb_nml,iostat=rcode)
   close(10)

   end subroutine parse_orb_nml


end module orbital_params_mod

