!===============================================================================
! CVS: $Id: cpl_map_mod.F90,v 1.4.6.1 2005/04/21 19:56:29 tcraig Exp $
! CVS: $Source: /fs/cgd/csm/models/CVS.REPOS/shared/csm_share/cpl/cpl_map_mod.F90,v $
! CVS: $Name: ccsm3_0_1_beta14 $
!===============================================================================
!BOP ===========================================================================
!
! !MODULE: cpl_map_mod -- mapping subsystem module
!
! !DESCRIPTION:
!    This is the module represents a major subsystem of cpl6. "Mapping" refers 
!    to the transfer of 2d field data from one domain/grid to another.  It is 
!    often desirable that maps have the properties of being {\it smooth} and 
!    {\it conservative}.  Common mapping techniques are bilinear interplation
!    and area-averaging.  Mapping is implemented by a sparse matrix multiply.
!    This module defines the sparse matrix data type used for mapping and 
!    handles the actual mapping of data (matrix multiply).  This module also 
!    handles the initialization and error checking of sparse matrix data.
!
! !REVISION HISTORY:
!    2001-Aug-14 - B. Kauffman -- gathered all mapping routines into this module
!    2001-May-20 - T. Craig -- first prototype
!
! !INTERFACE:  -----------------------------------------------------------------

!module cpl_map_mod

! !USES:

!   interface cpl_map_npFix; module procedure cpl_map_npFixNew3; end interface

!===============================================================================
!BOP ===========================================================================
!
! !IROUTINE: cpl_map_npFixNew3 - correct the north pole mapping of velocity fields
!
! !DESCRIPTION:
!    Correct the north pole mapping of velocity fields from the atm to ocn
!    grids.  This assumes the input grid is a regular lat/lon with the north
!    pole surrounded by the last latitude line in the input array.  The
!    longitudes in the last latitude must be ordered and equally spaced.
!
!    This version (New3) is the same as New except it saves the gGrid data
!    type from the first call.  This assumes the gGrid used in all calls
!    to npfix is the same.  It is different from New2 in that it doesn't
!    use gather to compute npu and npv.  This is bfb with New2 and New on 
!    9/1/2003.
!
! !REVISION HISTORY:
!    29Aug03 - T. Craig -- first prototype
!
! !INTERFACE:  -----------------------------------------------------------------

subroutine map_npfixnew(buni_ku, buni_kv, buni_index, buni_lat, buni_lon, &
							 buno_lat, buno_lon, buno_ku, buno_kv, &
							 buni_ni, buni_nj, length_i, length_o)

! !USES:

!#if (! defined HIDE_MPI)
!#include <mpif.h>         ! mpi library include file
!#endif

! !INPUT/OUTPUT PARAMETERS:
   integer  :: length_i, length_o
   real     :: buni_ku(length_i)        ! input bundle fields 1
   real     :: buni_kv(length_i)        ! input bundle fields 2
   integer  :: buni_index(length_i)     ! input bundle index
   real     :: buni_lat(length_i)       ! input bundle lat
   real     :: buni_lon(length_i)       ! input bundle lon

   real     :: buno_ku(length_o)        ! output bundle fields 1
   real     :: buno_kv(length_o)        ! output bundle fields 2
   real     :: buno_lat(length_o)       ! output bundle lat
   real     :: buno_lon(length_o)       ! output bundle lon
   integer      :: buni_ni, buni_nj         ! input grid length of lat and lon

!EOP

   !--- local ---
   integer  :: n,m                   ! generic indices
   integer  :: n1,n2,n3              ! generic indices
   integer  :: nmin,nmax             ! indices of highest latitude in input
   integer  :: npts                  ! local number of points in an aV
   integer  :: num                   ! number of points at highest latitude
   integer  :: index                 ! index value
   integer  :: nn1,nn2               ! local n to global n values
   real     :: rindex                ! index value
   real     :: latmax                ! value of highest latitude
   real     :: olon,olat             ! output bundle lon/lat
   real     :: ilon,ilat             ! input bundle lon/lat
   real     :: npu,npv               ! np velocity fields relative to lon
   real     :: theta1,theta2         ! angles for trig functions
   real,allocatable :: ilon1(:)      ! lon of input grid at highest latitude
   real,allocatable :: ilat1(:)      ! lat of input grid at highest latitude
   real,allocatable :: ilon2(:)      ! lon of input grid at highest latitude
   real,allocatable :: ilat2(:)      ! lat of input grid at highest latitude
   real     :: w1,w2,w3,w4           ! weights
   real     :: f1,f2,f3,f4           ! function values
   real     :: alpha,beta            ! used to generate weights
   real     :: rtmp                  ! real temporary
   real,allocatable :: lData(:,:)    ! last lat local input bundle data
                                         ! also compressed global data
   real,allocatable :: gData(:,:,:)  ! last lat gathered input bundle data
   !type(cpl_mct_aVect),save :: gGrid     ! global/gathered input bundle data
   integer,parameter :: pid0 = 0     ! root process pid = zero
   logical      :: found                 ! search for new interpolation
   integer  :: rcode                 ! error code
   integer  :: np1                   ! n+1 or tmp
   real     :: ilon1x                ! tmp
   logical,save :: first_call = .true.   ! flags 1st invocation of routine
   integer,save :: tnpf1,tnpf2,tnpf3,tnpf4,tnpf5,tnpf6,tnpf7,tnpf8,tnpf9
   real, save   :: cpl_const_deg2rad

   !--- formats ---
   character(*),parameter :: subName = '(cpl_map_npFixNew3) '
   character(*),parameter :: F00 = "('(cpl_map_npFixNew3) ',8a)"

!-------------------------------------------------------------------------------
! 
!-------------------------------------------------------------------------------


   return

   if (first_call) then
     write(6,F00) " compute bilinear weights & indicies for NP region."
     !call cpl_mct_aVect_gather(buni%dom%lGrid,gGrid,buni%dom%gsMap,pid0,cpl_comm_comp,rcode)
     !if (cpl_comm_comp_pid /= pid0) call cpl_mct_aVect_clean(gGrid)
     !call cpl_mct_aVect_bcast(gGrid,pid0,cpl_comm_comp,rcode)
     first_call = .false.
	 cpl_const_deg2rad = 3.14159265358979323846 / 180
   endif

   nmin = (buni_ni)*(buni_nj-1) + 1
   nmax = length_i
   num  = buni_ni


!  barrier not required but interesting for timing.
!  call shr_mpi_barrier(cpl_comm_comp,subName//" barrier")

   allocate(lData(3,num))
   !allocate(gData(3,num,cpl_comm_comp_npe))
   allocate(gData(3,num,1))
   lData = 0.
   gData = 0.
   npts = length_i
   m = 0   
   do n=1,npts
     ! Changed by zhangcheng
     ! The index of c/c++ begin with 0, but the index of fortran begin with 1
     rindex = buni_index(n)
     if (rindex.ge.nmin) then
       m=m+1
       lData(1,m) = rindex
       lData(2,m) = buni_ku(n)
       lData(3,m) = buni_kv(n)
       write(*,*) 'north pole uv', n, buni_ku(n), buni_kv(n)
     endif
   enddo

   !call MPI_ALLGATHER(lData,3*num,MPI_REAL8, &
   !  gData,3*num,MPI_REAL8,cpl_comm_comp,rcode)
   gData(:,:,1) = lData(:,:)

   !if (rcode.ne.0) then
     !write(6,*) trim(subName),' rcode error ',rcode
     !call shr_sys_abort()
   !endif

   m = 0
   lData = 0.
   do n2=1,num
   !do n3=1,cpl_comm_comp_npe
     if (gData(1,n2,1).gt.0.1) then
       m = m+1
       index = nint(gData(1,n2,1)) - nmin + 1
       lData(1:3,index) = gData(1:3,n2,1)
     endif
   !enddo
   enddo
   if (m.ne.num) write(6,*) trim(subName),' error allgather ',m,num
   do n2=1,num
     if (lData(1,n2).lt.0.1) then
       write(6,*) trim(subName),' error allgather2 ',n2
     endif
   enddo

   allocate(ilon1(num))
   allocate(ilon2(num))
   allocate(ilat1(num))
   allocate(ilat2(num))

   latmax = buni_lat(nmin)
   npu = 0.
   npv = 0.
   do n = 1,num
     np1 = mod(n,num)+1
     nn1 = nmin + n - 1
     nn2 = nmin + np1 - 1 
     rtmp = buni_lon(nn1)
     ilon1(n) = mod(rtmp+360.,360.)
     rtmp = buni_lon(nn2)
     ilon2(n) = mod(rtmp+360.,360.)
     ilat1(n) =     buni_lat(nn1)
     ilat2(n) =     buni_lat(nn2)
     if (ilon2(n) < ilon1(n)) ilon2(n) = ilon2(n) + 360.

     latmax = max(latmax,ilat1(n))

     theta1 = ilon1(n)*cpl_const_deg2rad
     npu = npu + cos(theta1)*lData(2,n) &
               - sin(theta1)*lData(3,n)
     npv = npv + sin(theta1)*lData(2,n) &
               + cos(theta1)*lData(3,n)
   enddo
   npu = npu / float(num)
   npv = npv / float(num)

   npts = length_o
   do m = 1,npts
     olat = buno_lat(m)
     if (olat >= latmax) then
       rtmp = buno_lon(m)
       olon = mod(rtmp,360.)
       n = 1
       found = .false.
       do while (n <= num .and. .not.found )
         if (    olon >= ilon1(n) .and. olon < ilon2(n) .or.   &
            olon+360. >= ilon1(n) .and. olon < ilon2(n)) then
 !          write(6,*) 'pos', n, m
           np1 = mod(n,num)+1
           ilat = (ilat1(n) + ilat2(n)) * 0.5
           if (ilon2(n) == ilon1(n)) then
             alpha = 0.5
           else if (    olon >= ilon1(n) .and. olon < ilon2(n)) then
             alpha = (olon - ilon1(n)) / (ilon2(n) - ilon1(n))
           else if (olon+360.>= ilon1(n) .and. olon < ilon2(n)) then
             alpha = (olon+360. - ilon1(n)) / (ilon2(n) - ilon1(n))
           else
             write(6,*) subName,' ERROR: olon ',olon,ilon1(n),ilon2(n)
           endif
           if (ilat >= 90.) then
             beta  = 1.0
           else
             beta  = (olat - ilat) / (90. - ilat)
           endif
           w1 = (1.0-alpha)*(1.0-beta)
           w2 = (    alpha)*(1.0-beta)
           w3 = (    alpha)*(    beta)
           w4 = (1.0-alpha)*(    beta)

           theta1 = ilon1(n)*cpl_const_deg2rad
           theta2 = ilon2(n)*cpl_const_deg2rad

           f1 = lData(2,n)
           f2 = lData(2,np1)
           f3 =  cos(theta1)*npu + sin(theta1)*npv
           f4 =  cos(theta2)*npu + sin(theta2)*npv
           rtmp = w1*f1 + w2*f2 + w3*f3 + w4*f4
           write(*, *) 'npfix output uv before mapping is', m, buno_ku(m), buno_kv(m)
           buno_ku(m) = w1*f1 + w2*f2 + w3*f3 + w4*f4

           f1 = lData(3,n)
           f2 = lData(3,np1)
           f3 = -sin(theta1)*npu + cos(theta1)*npv
           f4 = -sin(theta2)*npu + cos(theta2)*npv
           rtmp = w1*f1 + w2*f2 + w3*f3 + w4*f4
           buno_kv(m) = w1*f1 + w2*f2 + w3*f3 + w4*f4
           write(*, *) 'npfix output uv after mapping is', m, buno_ku(m), buno_kv(m)
           found = .true.
         endif
         n = n + 1     ! normal increment
       enddo
       if ( .not.found ) then
         write(6,*) subName,' ERROR: found = false ',found,m,olon,olat
       endif
     endif
   end do

   deallocate(gData)
   deallocate(lData)
   deallocate(ilon1)
   deallocate(ilon2)
   deallocate(ilat1)
   deallocate(ilat2)

end subroutine map_npfixnew

!end module cpl_map_mod

