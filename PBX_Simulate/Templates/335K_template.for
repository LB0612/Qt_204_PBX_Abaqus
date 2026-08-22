      SUBROUTINE USDFLD(FIELD, STATEV, PNEWDT, DIRECT, T, CELENT,
     1 time,dtime,cmname,orname,nfield,nstatv,noel,npt,layer,
     2  kspt,kstep,kinc,ndi,nshr,coord,jmac,jmtyp,matlayo,laccfla)
c redefine field variables at a material point.
      include 'aba_param.inc'
c
      character*80 cmname,orname
      character*3  flgray(15)
      dimension FIELD(NFIELD),STATEV(NSTATV),direct(3,3),
     1 t(3,3),time(2)
      dimension array(15),jarray(15),jmac(*),jmtyp(*),coord(*)
c this subroutine must call utility routine GETVRM to access material point data.
c Get temperatures from previous increment
      call getvrm('TEMP',array,jarray,flgray,jrcd,jmac, jmtyp, 
     1 matlayo, laccfla)
      temp = array(1)
c field(1) : la valeur du degr?? de cuisson dans chaque point d'integration de l'incr??ment. 0.0001 est la valeur initiale d??finie.
c kinc: Increment number
      if(kinc.eq.1)then
	statev(1)=5.0E-4
	else  
      field(1)=statev(1)
      end if
c
      return
      end


      SUBROUTINE DISP(U,KSTEP,KINC,TIME,NODE,NOEL,JDOF,COORDS)
C  define the magnitudes of prescribed BCs /   ici d??finir le cycle de cuisson. 
C  appliquer sur la surface qui a contact du moule.
      INCLUDE 'ABA_PARAM.INC'
C
      DIMENSION U(3), TIME(2),COORDS(3)
C TIME(2): current value of total time.
C U(1): total value of the prescribed variable at this point, here,it is the temperature.
      IF(TIME(2).LE.19000.)THEN
      U(1) = 335.+TIME(2)*2/19000.
      ELSE IF(TIME(2).LE.600000.)THEN
      U(1) = 337.
      ELSE IF(TIME(2).LE.632000.)THEN
      U(1) = 337.-(TIME(2)-600000.)*44/32000.
      ELSE 
      U(1) = 293.
      END IF

      RETURN
      END



      SUBROUTINE FILM(H,SINK,TEMP,JSTEP,JINC,TIME,NOEL,NPT,COORDS,
     1 JLTYP,FIELD,NFIELD,SNAME,JUSERNODE,AREA)
C
      INCLUDE'ABA_PARAM.INC'
C
      DIMENSION H(2),COORDS(3),TIME(2),FIELD(NFIELD)
      CHARACTER*80 SNAME
C H(1) FILM COEFFICIENT AT THIS POINT.
C SINK : THE SAME TEMPETATURE OF THE CURE CYCLE.

      IF(TIME(2).LE.19000.)THEN
      SINK = 335.+TIME(2)*2/19000.
      ELSE IF(TIME(2).LE.600000.)THEN
      SINK = 337.
      ELSE IF(TIME(2).LE.632000.)THEN
      SINK = 337.-(TIME(2)-600000.)*44/32000.
      ELSE 
      SINK = 293.
      END IF
C
      RETURN
      END

      SUBROUTINE HETVAL(CMNAME,TEMP,TIME,DTIME,STATEV,FLUX,
     1 PREDEF, DPRED)
C DEFINE THE HEAT FLUX DUE TO INTERNAL HEAT GENERATION (THE CHEMICAL REACTION)

      INCLUDE'ABA_PARAM.INC'
C
      CHARACTER*80 CMNAME
      DIMENSION TEMP(2),STATEV(3),PREDEF(1),TIME(2),FLUX(2),DPRED(1)
      DOUBLE PRECISION statev
      

C TEMP(1): CURRENT TEMPERATURE
      IF(TEMP(1).LT.304.)THEN
        STATEV(2) = 0.0
      ELSE
      
      STATEV(2) =5.9E6*EXP(-9012/TEMP(1))*(1.-STATEV(1))**1.2
c     &*((STATEV(1))**0.45)      

C STATEV(1): THE VALUE OF DEGRE OF CURE PASSED FROM THE SUBROUTINE USDFLD, 
C use approximation  equation  da/dt=K3(1-a)=3270*EXP(-6820/Temperature)(1-a)
C STATEV(2): da/dt  (a=degre of cure)
      STATEV(1) = STATEV(1)+ STATEV(2)* DTIME
       
      END IF
C FLUX(1) HEAT FLUX J/TIME/VOLUME, AT THIS MATERIAL CALCULATION POINT.
C      STATEV(1) = STATEV(1)+ STATEV(2)* DTIME
      FLUX(1) = 0.00167*100000 * STATEV(2)

      RETURN
      END



      