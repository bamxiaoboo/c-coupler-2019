

!===============================================================================
!BOP ===========================================================================
!
! !IROUTINE: flux_atmOcn_thucpl - wrapper to atm/ocn flux calculation
!
! !DESCRIPTION:
!     wrapper to atm/ocn flux calculation
!
! !REMARKS:
!     All data must be on the ocean domain (note: a domain includes a 
!     particular decomposition).
!
! !REVISION HISTORY:
!     2002-Jun-10 - B. Kauffman - first version
!
! !INTERFACE: ------------------------------------------------------------------

subroutine flux_atmOcn_thucpl(ocn_mask, ocn_u, ocn_v, ocn_t, atm_z, atm_u, atm_v, atm_ptem, &
                              atm_shum, atm_dens, atm_tbot, ao_sen, ao_lat, ao_lwup, &
                              ao_evap, ao_taux, ao_tauy, ao_tref, ao_qref, ao_duu10n, &
                              field_size)

! !INPUT/OUTPUT PARAMETERS:

   integer,intent(in)  :: field_size
   logical(KIND=1),intent(in)  :: ocn_mask(field_size)
   real,intent(in)     :: ocn_u(field_size)
   real,intent(in)     :: ocn_v(field_size)
   real,intent(in)     :: ocn_t(field_size)
   real,intent(in)     :: atm_z(field_size)
   real,intent(in)     :: atm_u(field_size)
   real,intent(in)     :: atm_v(field_size)
   real,intent(in)     :: atm_ptem(field_size)
   real,intent(in)     :: atm_shum(field_size)
   real,intent(in)     :: atm_dens(field_size)
   real,intent(in)     :: atm_tbot(field_size)
   real,intent(out)    :: ao_sen(field_size)
   real,intent(out)    :: ao_lat(field_size)
   real,intent(out)    :: ao_lwup(field_size)
   real,intent(out)    :: ao_evap(field_size)
   real,intent(out)    :: ao_taux(field_size)
   real,intent(out)    :: ao_tauy(field_size)
   real,intent(out)    :: ao_tref(field_size)
   real,intent(out)    :: ao_qref(field_size)
   real,intent(out)    :: ao_duu10n(field_size)

!EOP

!-------------------------------------------------------------------------------
! NOTES:
! o all data is on ocn grid
!-------------------------------------------------------------------------------

   !----------------------------------------------------------------------------
   ! call the scientist-written physics routine
   !----------------------------------------------------------------------------
     call srfflx_ao_thucpl( field_size    ,   atm_z   ,   atm_u   ,   atm_v   , atm_ptem   , &
      &              atm_shum     , atm_dens   , atm_tbot   ,ocn_u   ,ocn_v   , &
      &              ocn_t     ,ocn_mask   , ao_sen   , ao_lat   , ao_lwup   , &
      &              ao_evap     ,ao_taux   , ao_tauy  , ao_tref   , ao_qref   , &
      &              ao_duu10n                                     )

end subroutine flux_atmOcn_thucpl


!===============================================================================
!BOP ===========================================================================
!
! !IROUTINE: srfflx_ao_thucpl -- internal atm/ocn flux calculation
!
! !DESCRIPTION:
!
!     Internal atm/ocn flux calculation
!     
! !REVISION HISTORY:
!     2002-Jun-10 - B. Kauffman - brought in from cpl5.
!     2003-Apr-02 - B. Kauffman - taux & tauy now utilize ocn velocity
!     2003-Apr-02 - B. Kauffman - tref,qref,duu10n mods as per Bill Large
!
! !INTERFACE: ------------------------------------------------------------------

SUBROUTINE srfflx_ao_thucpl(imax  ,zbot  ,ubot  ,vbot  ,thbot ,   & 
           &         qbot  ,rbot  ,tbot  ,us    ,vs    ,   &
           &         ts    ,mask  ,sen   ,lat   ,lwup  ,   &
           &         evap  ,taux  ,tauy  ,tref  ,qref  ,   &
           &         duu10n                                )

! !USES:
   use cpl_cpl_const_mod

   implicit none

! !INPUT/OUTPUT PARAMETERS:

   !--- input arguments --------------------------------
   integer,intent(in) ::       imax  ! array dimensions
   logical(KIND=1),intent(in)     :: mask (imax) ! ocn domain mask 0
   real   ,intent(in) :: zbot (imax) ! atm level height      (m)
   real   ,intent(in) :: ubot (imax) ! atm u wind            (m/s)
   real   ,intent(in) :: vbot (imax) ! atm v wind            (m/s)
   real   ,intent(in) :: thbot(imax) ! atm potential T       (K)
   real   ,intent(in) :: qbot (imax) ! atm specific humidity (kg/kg)
   real   ,intent(in) :: rbot (imax) ! atm air density       (kg/m^3)
   real   ,intent(in) :: tbot (imax) ! atm T                 (K) 
   real   ,intent(in) :: us   (imax) ! ocn u-velocity        (m/s)
   real   ,intent(in) :: vs   (imax) ! ocn v-velocity        (m/s)
   real   ,intent(in) :: ts   (imax) ! ocn temperature       (K)

   !--- output arguments -------------------------------
   real,intent(out)  ::  sen  (imax) ! heat flux: sensible    (W/m^2)
   real,intent(out)  ::  lat  (imax) ! heat flux: latent      (W/m^2)
   real,intent(out)  ::  lwup (imax) ! heat flux: lw upward   (W/m^2)
   real,intent(out)  ::  evap (imax) ! water flux: evap  ((kg/s)/m^2)
   real,intent(out)  ::  taux (imax) ! surface stress, zonal      (N)
   real,intent(out)  ::  tauy (imax) ! surface stress, maridional (N)
   real,intent(out)  ::  tref (imax) ! diag:  2m ref height T     (K)
   real,intent(out)  ::  qref (imax) ! diag:  2m ref humidity (kg/kg)
   real,intent(out)  :: duu10n(imax) ! diag: 10m wind speed squared (m/s)^2
 
!EOP

   !--- local constants --------------------------------
   real,parameter :: umin  =  0.5    ! minimum wind speed       (m/s)
   real,parameter :: zref  = 10.0    ! reference height           (m)
   real,parameter :: ztref =  2.0    ! reference height for air T (m)

   !--- local variables --------------------------------
   integer     :: i      ! vector loop index
   real    :: vmag   ! surface wind magnitude   (m/s)
   real    :: thvbot ! virtual temperature      (K)
   real    :: ssq    ! sea surface humidity     (kg/kg)
   real    :: delt   ! potential T difference   (K)
   real    :: delq   ! humidity difference      (kg/kg)
   real    :: stable ! stability factor
   real    :: rdn    ! sqrt of neutral exchange coeff (momentum) 
   real    :: rhn    ! sqrt of neutral exchange coeff (heat)     
   real    :: ren    ! sqrt of neutral exchange coeff (water)    
   real    :: rd     ! sqrt of exchange coefficient (momentum)         
   real    :: rh     ! sqrt of exchange coefficient (heat)             
   real    :: re     ! sqrt of exchange coefficient (water)            
   real    :: ustar  ! ustar             
   real    :: qstar  ! qstar             
   real    :: tstar  ! tstar             
   real    :: hol    ! H (at zbot) over L
   real    :: xsq    ! ?
   real    :: xqq    ! ?
   real    :: psimh  ! stability function at zbot (momentum)
   real    :: psixh  ! stability function at zbot (heat and water)
   real    :: psix2  ! stability function at ztref reference height
   real    :: alz    ! ln(zbot/zref)
   real    :: al2    ! ln(zref/ztref)
   real    :: u10n   ! 10m neutral wind 
   real    :: tau    ! stress at zbot
   real    :: cp     ! specific heat of moist air
   real    :: bn     ! exchange coef funct for interpolation
   real    :: bh     ! exchange coef funct for interpolation
   real    :: fac    ! vertical interpolation factor

   !--- local functions --------------------------------
   real    :: qsat   ! function: the saturation humididty of air (kg/m^3)
   real    :: cdn    ! function: neutral drag coeff at 10m
   real    :: psimhu ! function: unstable part of psimh
   real    :: psixhu ! function: unstable part of psimx
   real    :: Umps   ! dummy arg ~ wind velocity (m/s)
   real    :: Tk     ! dummy arg ~ temperature (K)
   real    :: xd     ! dummy arg ~ ?
 
   qsat(Tk)   = 640380.0 / exp(5107.4/Tk)
   cdn(Umps)  = 0.0027 / Umps + 0.000142 + 0.0000764 * Umps
   psimhu(xd) = log((1.0+xd*(2.0+xd))*(1.0+xd*xd)/8.0) - 2.0*atan(xd) + 1.571
   psixhu(xd) = 2.0 * log((1.0 + xd*xd)/2.0)
 
!-------------------------------------------------------------------------------
! PURPOSE:
!   computes atm/ocn surface fluxes
!
! NOTES: 
!   o all fluxes are positive downward
!   o net heat flux = net sw + lw up + lw down + sen + lat
!   o here, tstar = <WT>/U*, and qstar = <WQ>/U*.
!   o wind speeds should all be above a minimum speed (eg. 1.0 m/s)
! 
! ASSUMPTIONS:
!   o Neutral 10m drag coeff: cdn = .0027/U10 + .000142 + .0000764 U10
!   o Neutral 10m stanton number: ctn = .0327 sqrt(cdn), unstable
!                                 ctn = .0180 sqrt(cdn), stable
!   o Neutral 10m dalton number:  cen = .0346 sqrt(cdn)
!   o The saturation humidity of air at T(K): qsat(T)  (kg/m^3)
!-------------------------------------------------------------------------------
 
   al2 = log(zref/ztref)

   DO i=1,imax
     if (mask(i)) then
    
        !--- compute some needed quantities ---
        vmag   = max(umin, sqrt( (ubot(i)-us(i))**2 + (vbot(i)-vs(i))**2) )
        thvbot = thbot(i) * (1.0 + cpl_const_zvir * qbot(i)) ! virtual temp (K)
        ssq    = 0.98 * qsat(ts(i)) / rbot(i)      ! sea surf hum (kg/kg)
        delt   = thbot(i) - ts(i)                  ! pot temp diff (K)
        delq   = qbot(i) - ssq                     ! spec hum dif (kg/kg)
        alz    = log(zbot(i)/zref) 
        cp     = cpl_const_cpdair*(1.0 + cpl_const_cpvir*ssq) 
   
        !------------------------------------------------------------
        ! first estimate of Z/L and ustar, tstar and qstar
        !------------------------------------------------------------
   
        !--- neutral coefficients, z/L = 0.0 ---
        stable = 0.5 + sign(0.5 , delt)
        rdn    = sqrt(cdn(vmag))
        rhn    = (1.0-stable) * 0.0327 + stable * 0.018 
        ren    = 0.0346 
   
        !--- ustar, tstar, qstar ---
        ustar = rdn * vmag
        tstar = rhn * delt  
        qstar = ren * delq  
   
        !--- compute stability & evaluate all stability functions ---
        hol  = cpl_const_karman*cpl_const_g*zbot(i)*  &
               (tstar/thvbot+qstar/(1.0/cpl_const_zvir+qbot(i)))/ustar**2
        hol  = sign( min(abs(hol),10.0), hol )
        stable = 0.5 + sign(0.5 , hol)
        xsq    = max(sqrt(abs(1.0 - 16.0*hol)) , 1.0)
        xqq    = sqrt(xsq)
        psimh  = -5.0*hol*stable + (1.0-stable)*psimhu(xqq)
        psixh  = -5.0*hol*stable + (1.0-stable)*psixhu(xqq)
   
        !--- shift wind speed using old coefficient ---
        rd   = rdn / (1.0 + rdn/cpl_const_karman*(alz-psimh))
        u10n = vmag * rd / rdn 
   
        !--- update transfer coeffs at 10m and neutral stability ---
        rdn = sqrt(cdn(u10n))
        ren = 0.0346
        rhn = (1.0-stable)*0.0327 + stable * 0.018 
    
        !--- shift all coeffs to measurement height and stability ---
        rd = rdn / (1.0 + rdn/cpl_const_karman*(alz-psimh)) 
        rh = rhn / (1.0 + rhn/cpl_const_karman*(alz-psixh)) 
        re = ren / (1.0 + ren/cpl_const_karman*(alz-psixh)) 
   
        !--- update ustar, tstar, qstar using updated, shifted coeffs --
        ustar = rd * vmag 
        tstar = rh * delt 
        qstar = re * delq 
    
        !------------------------------------------------------------
        ! iterate to converge on Z/L, ustar, tstar and qstar
        !------------------------------------------------------------
    
        !--- compute stability & evaluate all stability functions ---
        hol  = cpl_const_karman*cpl_const_g*zbot(i)* &
               (tstar/thvbot+qstar/(1.0/cpl_const_zvir+qbot(i)))/ustar**2
        hol  = sign( min(abs(hol),10.0), hol )
        stable = 0.5 + sign(0.5 , hol)
        xsq    = max(sqrt(abs(1.0 - 16.0*hol)) , 1.0)
        xqq    = sqrt(xsq)
        psimh  = -5.0*hol*stable + (1.0-stable)*psimhu(xqq)
        psixh  = -5.0*hol*stable + (1.0-stable)*psixhu(xqq)
    
        !--- shift wind speed using old coeffs ---
        rd   = rdn / (1.0 + rdn/cpl_const_karman*(alz-psimh))
        u10n = vmag * rd/rdn 
    
        !--- update transfer coeffs at 10m and neutral stability ---
        rdn = sqrt(cdn(u10n))
        ren = 0.0346
        rhn = (1.0 - stable)*0.0327 + stable * 0.018 
   
        !--- shift all coeffs to measurement height and stability ---
        rd = rdn / (1.0 + rdn/cpl_const_karman*(alz-psimh)) 
        rh = rhn / (1.0 + rhn/cpl_const_karman*(alz-psixh)) 
        re = ren / (1.0 + ren/cpl_const_karman*(alz-psixh)) 
    
        !--- update ustar, tstar, qstar using updated, shifted coeffs ---
        ustar = rd * vmag 
        tstar = rh * delt 
        qstar = re * delq 
    
        !------------------------------------------------------------
        ! compute the fluxes
        !------------------------------------------------------------
    
        tau = rbot(i) * ustar * ustar 
       
        !--- momentum flux ---
        taux(i) = tau * (ubot(i)-us(i)) / vmag 
        tauy(i) = tau * (vbot(i)-vs(i)) / vmag 
        
        !--- heat flux ---
        sen (i) =                cp * tau * tstar / ustar 
        lat (i) =  cpl_const_latvap * tau * qstar / ustar
        lwup(i) = -cpl_const_stebol * ts(i)**4 
      
        !--- water flux ---
        evap(i) = lat(i)/cpl_const_latvap 
    
        !------------------------------------------------------------
        ! compute diagnositcs: 2m ref T & Q, 10m wind speed squared
        !------------------------------------------------------------
        hol = hol*ztref/zbot(i)
        xsq = max( 1.0, sqrt(abs(1.0-16.0*hol)) )
        xqq = sqrt(xsq)
        psix2   = -5.0*hol*stable + (1.0-stable)*psixhu(xqq)
        fac     = (rh/cpl_const_karman) * (alz + al2 - psixh + psix2 )
        tref(i) = thbot(i) - delt*fac 
        tref(i) = tref(i) - 0.01*ztref   ! pot temp to temp correction
        fac     = (re/cpl_const_karman) * (alz + al2 - psixh + psix2 )
        qref(i) =  qbot(i) - delq*fac
    
        duu10n(i) = u10n*u10n ! 10m wind speed squared
     endif
   ENDDO 

END subroutine srfflx_ao_thucpl
