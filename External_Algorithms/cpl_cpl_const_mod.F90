!===============================================================================
! CVS $Id: cpl_const_mod.F90,v 1.2 2003/11/22 00:27:04 tcraig Exp $
! CVS $Source: /fs/cgd/csm/models/CVS.REPOS/shared/csm_share/cpl/cpl_const_mod.F90,v $
! CVS $Name: ccsm3_0_1_beta14 $
!===============================================================================
!BOP ===========================================================================
!
! !MODULE: cpl_const_mod - defines/provides common constants.
!
! !DESCRIPTION:
!    Defines/provides common constants.
!
! !REVISION HISTORY:
!     2002-jun-10 - B. Kauffman - created module
!     2002-dec-5  - T. Craig    - names consistent with convention, cpl_const_*
!
! !INTERFACE: ------------------------------------------------------------------

module cpl_cpl_const_mod

! !USES:

   use cpl_cpl_kind_mod   ! kinds
   use shr_const_mod  ! shared physical constants

   implicit none

! !PUBLIC TYPES:

  ! none

! !PUBLIC MEMBER FUNCTIONS:

  ! none

! !PUBLIC DATA MEMBERS:

   public

   !----------------------------------------------------------------------------
   ! physical constants
   !----------------------------------------------------------------------------
   real(R8),parameter :: cpl_const_pi      = SHR_CONST_PI     ! pi
   real(R8),parameter :: cpl_const_rearth  = SHR_CONST_REARTH ! radius of earth ~ m
   real(R8),parameter :: cpl_const_rearth2 = SHR_CONST_REARTH*SHR_CONST_REARTH ! rad**2
   real(R8),parameter :: cpl_const_g       = SHR_CONST_G      ! gravity
   real(R8),parameter :: cpl_const_deg2rad = cpl_const_pi/180.0_R8  ! deg to rads
   real(R8),parameter :: cpl_const_rad2deg = 180.0_R8/cpl_const_pi  ! rad to degs

   real(R8),parameter :: cpl_const_cpdair  = SHR_CONST_CPDAIR  ! spec heat of dry air
   real(R8),parameter :: cpl_const_cpwv    = SHR_CONST_CPWV    ! spec heat of h2o vapor
   real(R8),parameter :: cpl_const_cpvir   = cpl_const_cpwv/cpl_const_cpdair - 1.0_R8 
   real(R8),parameter :: cpl_const_zvir    = SHR_CONST_ZVIR    ! rh2o/rair   - 1.0
   real(R8),parameter :: cpl_const_latvap  = SHR_CONST_LATVAP  ! latent heat of evap
   real(R8),parameter :: cpl_const_latice  = SHR_CONST_LATICE  ! latent heat of fusion
   real(R8),parameter :: cpl_const_stebol  = SHR_CONST_STEBOL  ! Stefan-Boltzmann 
   real(R8),parameter :: cpl_const_karman  = SHR_CONST_KARMAN  ! Von Karman constant

   real(R8),parameter :: cpl_const_ocn_ref_sal = SHR_CONST_OCN_REF_SAL ! ocn ref salt
   real(R8),parameter :: cpl_const_ice_ref_sal = SHR_CONST_ICE_REF_SAL ! ice ref salt

   real(R8),parameter :: cpl_const_spval       = SHR_CONST_SPVAL       ! special value

!EOP

end module cpl_cpl_const_mod
