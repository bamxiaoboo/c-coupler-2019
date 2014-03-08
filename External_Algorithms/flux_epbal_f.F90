subroutine flux_epbal(ievap, oevap, rain, snow, roff, ifrac, afrac, area, mask, &
                      nloc, control_fluxEPbal, control_fluxEPfac)

! !USES:
   use c_coupler_interface_mod

   implicit none

   !--- local ---
   integer,parameter     :: k_area = 1 ! index: area of ocn domain
   integer,parameter     :: k_prec = 2 ! index: water ~ precipitation
   integer,parameter     :: k_evap = 3 ! index: water ~ evaporation
   integer,parameter     :: k_roff = 4 ! index: water ~ runoff
   real*16              :: psum(4)    ! partial/local sum of area,prec,evap,roff
   real*16              :: gsum(4)    !   full/global sum of area,prec,evap,roff
   real              :: tprec      ! total precip
   real              :: tevap      ! total evap
   real              :: troff      ! total runoff
   real              :: tarea      ! total area
   real              :: da         ! area of one ocn grid cell = dth*dph
   real              :: dai        ! area of ocn grid covered by ice
   real              :: dao        ! area of ocn grid covered by atm
   real              :: factor     ! prec adjustment factor: evap/prec
   real              :: ievap(nloc)  
   real              :: oevap(nloc)  
   real              :: rain(nloc)  
   real              :: snow(nloc)  
   real              :: roff(nloc)  
   real              :: ifrac(nloc)  
   real              :: afrac(nloc)  
   real              :: area(nloc)  ! cell area
   logical(1)               :: mask(nloc)  ! domain mask
   integer               :: nloc       ! size of local data array
   integer               :: n                  ! generic loop index
   real                :: control_fluxEPfac
   integer            :: counter

   !----- formats -----
   character(*),parameter :: F00 = "('(flux_epbal) ',4a)"
   character(*),parameter :: F01 = "('(flux_epbal) ',a,3e11.3,a,f9.6)"
   character(len=*)       :: control_fluxEPbal


!-------------------------------------------------------------------------------
!
!-------------------------------------------------------------------------------
     
   if (control_fluxEPbal(1:3) == 'off') return

   !----------------------------------------------------------------------------
   ! compute local integrals
   !----------------------------------------------------------------------------
   psum(:) = 0 ! local/parial sum
   counter = 0
   do n=1,nloc
      if (mask(n)) then
         da  = area(n)
         dai = da*ifrac(n) 
         dao = da*afrac(n)
         psum(k_area) = psum(k_area) + dai + dao
         psum(k_prec) = psum(k_prec) + dai*(rain(n) + snow(n))
         psum(k_prec) = psum(k_prec) + dao*(rain(n) + snow(n))
         psum(k_evap) = psum(k_evap) + dai*ievap(n)
         psum(k_evap) = psum(k_evap) + dao*oevap(n)
         psum(k_roff) = psum(k_roff) + da *roff(n)
         counter = counter + 1
      endif
   enddo 

   !----------------------------------------------------------------------------
   ! compute factor (on master process only)
   !----------------------------------------------------------------------------
   gsum(:) = 0 ! global sum
   call c_coupler_get_global_sum_real16(psum, gsum, 4)

      tarea = gsum(k_area)
      tprec = gsum(k_prec)/tarea
      tevap = gsum(k_evap)/tarea
      troff = gsum(k_roff)/tarea

      if (control_fluxEPbal(1:3) == 'ocn') then
         !-------------------------------------------------------------
         ! use factor supplied by ocn, NOTE: may not cause E+f(P+R)=0 
         !-------------------------------------------------------------

         factor = control_fluxEPfac
         if (factor .le. 0.0) then
            write(6,F01) 'WARNING: factor from ocn = ',factor
            write(6,F01) 'WARNING: resetting factor to 1.0'
            factor = 1.0
         end if

      else if (control_fluxEPbal(1:4) == 'inst') then
         !-------------------------------------------------------------
         ! compute factor st f(P+R)+E=0 at every timestep 
         !-------------------------------------------------------------
 
         if ( (tprec+troff) > 0.0) then
            factor = -tevap/(tprec+troff)
         else
            factor=1.0
            write(6,F01) 'WARNING: avg  P,R,(P+R) = ',tprec,troff,tprec+troff
            write(6,F01) 'WARNING: setting factor = 1.0'
         end if
      else
         !-------------------------------------------------------------
         ! invalid option
         !-------------------------------------------------------------
         write(6,F00) 'ERROR: unknown epbal option: ',control_fluxEPbal
      end if
!
   factor = 0.98*factor
!
!DIR$ CONCURRENT
   do n=1,nloc
      if (mask(n)) then
         rain(n) = rain(n)*factor
         snow(n) = snow(n)*factor
         roff(n) = roff(n)*factor
      end if
   end do

end subroutine flux_epbal
