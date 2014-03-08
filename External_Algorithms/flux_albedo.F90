!===============================================================================
!BOP ===========================================================================
!
! !IROUTINE: flux_albo_thucpl - ocean albedo calculation
!
! !DESCRIPTION:
!   if flux\_albav/=0 (ie. "on" or "true") \\
!      Compute four effective daily avg surface albedos for all
!      combinations of visible/near-infrared and direct/diffuse radiation
!      without accounting for zenith angle (ie. a "daily average" albedo) \\
!   else \\
!      Compute four surface albedos for all combinations of visible/
!      near-infrared and direct/diffuse radiation, accounting for
!      instantaneous zenith angle
!
! !REMARKS:
!   o upon input, albedos are assumed to be a 60 degree reference albedo
!   o albedos are computed by taking the 60 deg reference albedo
!     and then adjusting this value based on zenith angle
!   o Albedos are independent of spectral interval and other physical
!     factors such as surface wind speed.
!
!   For more details see Briegleb, Bruce P., 1992: Delta-Eddington
!   Approximation for Solar Radiation in the NCAR Community Climate
!   Model, Journal of Geophysical Research, Vol 97, D7, pp7603-7612.
!
! !REVISION HISTORY:
!    198x        - CCM1, original version
!    1992-Jun    - J. Rosinski -- standardized
!    1994-May    - J. Rosinski -- rewritten for land only
!    1994-Jul    - B. Kauffman -- rewritten for ocean only
!    2002-Oct-26 - R. Jacob -- Rewritten for cpl6
!
! !INTERFACE: ------------------------------------------------------------------

subroutine flux_albo_thucpl(lats, lons, cpl_fluxAlbav, orbEccen, orbMvelpp, &
                     orbLambm0, orbObliqr, albedo_shift, field_size, &
                     field_anidr, field_avsdr, field_anidf, field_avsdf)

! !USES:

   use shr_orb_mod ! orbital constants and methods
   use c_coupler_interface_mod

   implicit none

! !INPUT/OUTPUT PARAMETERS:

   integer,intent(in)  :: field_size
   real,intent(in) :: lats(field_size)
   real,intent(in) :: lons(field_size)
   logical(KIND=1),intent(in)  :: cpl_fluxAlbav
   real,intent(in) :: orbEccen
   real,intent(in) :: orbObliqr
   real,intent(in) :: orbLambm0
   real,intent(in) :: orbMvelpp
   integer,intent(in) :: albedo_shift
   real :: albedo_time
   real,intent(inout) :: field_anidr(field_size)
   real,intent(inout) :: field_avsdr(field_size)
   real,intent(inout) :: field_anidf(field_size)
   real,intent(inout) :: field_avsdf(field_size)

!EOP
   !--- local ---
   integer     :: n                   ! loop index
   real    :: rlat                ! gridcell latitude in radians
   real    :: rlon                ! gridcell longitude in radians
   real    :: cosz                ! Cosine of solar zenith angle 
   real    :: delta               ! Solar declination angle in radians
   real    :: eccf                ! Earth-sun distance factor
   real,parameter :: albdif = 0.06 ! 60 deg reference albedo, diffuse
   real,parameter :: albdir = 0.07 ! 60 deg reference albedo, direct 
   real,parameter :: cpl_const_deg2rad = 3.14159265358979323846 / 180; 

!-------------------------------------------------------------------------------
!
!-------------------------------------------------------------------------------

   IF (cpl_fluxAlbav) THEN
      do n=1,field_size
         field_anidr(n) = albdir
         field_avsdr(n) = albdir
         field_anidf(n) = albdif
         field_avsdf(n) = albdif
      end do
   ELSE
      !--- solar declination ---
      if (c_coupler_is_first_step()) then
          call c_coupler_get_current_calendar_time(albedo_time, 0)
      else
          call c_coupler_get_current_calendar_time(albedo_time, albedo_shift)
      endif
      call shr_orb_decl(albedo_time,orbEccen,orbMvelpp,orbLambm0,orbObliqr,delta,eccf)

      do n=1,field_size
         rlat = cpl_const_deg2rad * lats(n)
         rlon = cpl_const_deg2rad * lons(n)
         cosz = shr_orb_cosz(albedo_time, rlat, rlon, delta )
         if (cosz  >  0.0) then !--- sun hit --
            field_anidr(n) = (.026/(cosz**1.7 + 0.065)) +   &
                             (.150*(cosz      - 0.10 )  *   &
                             (cosz      - 0.50 )  *   &
                             (cosz      - 1.00 )  )
            field_avsdr(n) = field_anidr(n)
            field_anidf(n) = albdif
            field_avsdf(n) = albdif
         else !--- dark side of earth ---
            field_anidr(n) = 1.0
            field_avsdr(n) = 1.0
            field_anidf(n) = 1.0
            field_avsdf(n) = 1.0
         end if
      end do
   END IF 
END subroutine flux_albo_thucpl


!===============================================================================
!BOP ===========================================================================
!
! !IROUTINE: flux_albi_thucpl - ice albedo modification
!
! !DESCRIPTION:
!   if flux\_albav/=0 (ie. "on" or "true") \\
!     Impose a zenith angle dependance on the ice model "reference albedo".
!     Currently this only involves setting albedos to zero
!     on the dark side of the earth. \\
!   else \\
!     do not alter ice albedos
!
! !REMARKS:
!   o upon input, albedos are assumed to be a 60 degree reference albedo
!
! !REVISION HISTORY:
!   199x-       - B. Kauffman -- original cpl5 version
!   2002-Oct-26 - R. Jacob -- rewritten for cpl6
!
! !INTERFACE: ------------------------------------------------------------------
subroutine flux_albi_thucpl(lats, lons, cpl_fluxAlbav, orbEccen, orbMvelpp, &
                     orbLambm0, orbObliqr, albedo_shift, field_size, &
                     field_anidr, field_avsdr, field_anidf, field_avsdf)

! !USES:

   use shr_orb_mod ! orbital constants and methods
   use c_coupler_interface_mod

   implicit none

! !INPUT/OUTPUT PARAMETERS:

   integer,intent(in)  :: field_size
   real,intent(in) :: lats(field_size)
   real,intent(in) :: lons(field_size)
   logical(KIND=1),intent(in)  :: cpl_fluxAlbav
   real,intent(in) :: orbEccen
   real,intent(in) :: orbObliqr
   real,intent(in) :: orbLambm0
   real,intent(in) :: orbMvelpp
   integer,intent(in) :: albedo_shift
   real,intent(inout) :: field_anidr(field_size)
   real,intent(inout) :: field_avsdr(field_size)
   real,intent(inout) :: field_anidf(field_size)
   real,intent(inout) :: field_avsdf(field_size)
   real :: albedo_time

!EOP
   !--- local ---
   integer     :: n                   ! loop index
   real    :: rlat                ! gridcell latitude in radians
   real    :: rlon                ! gridcell longitude in radians
   real    :: cosz                ! Cosine of solar zenith angle 
   real    :: delta               ! Solar declination angle in radians
   real    :: eccf                ! Earth-sun distance factor
   real,parameter :: albdif = 0.06 ! 60 deg reference albedo, diffuse
   real,parameter :: albdir = 0.07 ! 60 deg reference albedo, direct 
   real,parameter :: cpl_const_deg2rad = 3.14159265358979323846 / 180; 

!-------------------------------------------------------------------------------
!
!-------------------------------------------------------------------------------


   IF (cpl_fluxAlbav) THEN
   ELSE
      !--- solar declination ---
      if (c_coupler_is_first_step()) then
          call c_coupler_get_current_calendar_time(albedo_time, 0)
      else
          call c_coupler_get_current_calendar_time(albedo_time, albedo_shift)
      endif

      call shr_orb_decl(albedo_time,orbEccen,orbMvelpp,orbLambm0,orbObliqr,delta,eccf)

      do n=1,field_size
         rlat = cpl_const_deg2rad * lats(n)
         rlon = cpl_const_deg2rad * lons(n)
         cosz = shr_orb_cosz(albedo_time, rlat, rlon, delta )
         if (cosz  <  0.0) then !--- dark side of earth --
            field_anidr(n) = 1.0
            field_avsdr(n) = 1.0
            field_anidf(n) = 1.0
            field_avsdf(n) = 1.0
         end if
      end do
   END IF 
END subroutine flux_albi_thucpl


