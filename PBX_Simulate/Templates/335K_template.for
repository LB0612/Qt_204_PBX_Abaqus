      SUBROUTINE USDFLD(FIELD, STATEV, PNEWDT, DIRECT, T, CELENT,
     1 time,dtime,cmname,orname,nfield,nstatv,noel,npt,layer,
     2  kspt,kstep,kinc,ndi,nshr,coord,jmac,jmtyp,matlayo,laccfla)
C 在材料积分点重新定义场变量
      include 'aba_param.inc'
C
      character*80 cmname,orname
      character*3  flgray(15)
      dimension FIELD(NFIELD),STATEV(NSTATV),direct(3,3),
     1 t(3,3),time(2)
      dimension array(15),jarray(15),jmac(*),jmtyp(*),coord(*)
C 调用GETVRM读取材料积分点数据
C 获取上一增量步的温度
      call getvrm('TEMP',array,jarray,flgray,jrcd,jmac, jmtyp, 
     1 matlayo, laccfla)
      temp = array(1)
C FIELD(1)为当前积分点的固化度场变量，STATEV(1)保存固化度状态
C KINC为当前增量步编号
      if(kinc.eq.1)then
	statev(1)=5.0E-4
	else  
      field(1)=statev(1)
      end if
C
      return
      end


      SUBROUTINE DISP(U,KSTEP,KINC,TIME,NODE,NOEL,JDOF,COORDS)
C 定义指定边界条件，此处用于定义固化温度循环
C 温度边界施加在与模具接触的表面
      INCLUDE 'ABA_PARAM.INC'
C
      DIMENSION U(3), TIME(2),COORDS(3)
C TIME(2)为当前总时间
C U(1)为当前点的指定温度值
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
C H(1)为当前点的表面对流换热系数
C SINK为固化温度循环对应的环境温度

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
C 定义固化化学反应产生的内部热流

      INCLUDE'ABA_PARAM.INC'
C
      CHARACTER*80 CMNAME
      DIMENSION TEMP(2),STATEV(3),PREDEF(1),TIME(2),FLUX(2),DPRED(1)
      DOUBLE PRECISION statev
      

C TEMP(1)为当前温度
      IF(TEMP(1).LT.304.)THEN
        STATEV(2) = 0.0
      ELSE
      
      STATEV(2) =5.9E6*EXP(-9012/TEMP(1))*(1.-STATEV(1))**1.2
c     &*((STATEV(1))**0.45)      

C STATEV(1)为固化度
C STATEV(2)为固化度变化速率da/dt
C 固化反应速率根据当前温度和固化度计算
      STATEV(1) = STATEV(1)+ STATEV(2)* DTIME
       
      END IF
C FLUX(1)为当前材料积分点的体积热流
C      STATEV(1) = STATEV(1)+ STATEV(2)* DTIME
      FLUX(1) = 0.00167*100000 * STATEV(2)

      RETURN
      END



      
