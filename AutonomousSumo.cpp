#include "QTRSensors.h"
#include <PS2X_lib.h>
#include "RoboClaw.h"
#define LED_PIN 13

#define _Atacar_ 1 //Activa o Desactiva que el Robot Ataque
#define _Atacar_Tactica_ 1 //Activa tactica de retroceder un poco cuando ataca 
#define _Buscar_ 1 //Activa o Desactiva que el Robot Ande Buscando
#define _Buscar_Tactica 1  //Activa tactica de Busqueda paso a paso, no continuo
#define QTR_Activados 1 //Activados-> 1 , Desactivados-> 0. Activa o Desactiva Sensores de Piso.

#define ESTADO_INICIAL 30 //Estado Normal-> 1 , Arranque Zig Zag-> 30, Arranque Retroceso a Linea-> 31

//Sharp - Declaraciones
#define sensibilidad_Sharp 160//->necesita probars pero parece mejor aun//178->funciona medianamente bien//150//130//100//350
#define num_sensores_sharp 6

//Sensores de Piso QTR - Declaraciones
#define NUM_SENSORS   6     // number of sensors used
#define TIMEOUT       2500  // waits for 2500 microseconds for sensor outputs to go low
#define EMITTER_PIN   QTR_NO_EMITTER_PIN     // emitter is controlled by digital pin 2
#define sensibilidad_P0_P2 80//200//500//100
#define sensibilidad_P1 60//150//300//80//100//10//7 
#define sensibilidad_P4 200//150//80//100//10//7 
#define sensibilidad_P3_P5 500//200//400//180//150 
QTRSensorsRC qtrrc((unsigned char[]) {A8,A9,2,12,3,11},NUM_SENSORS, TIMEOUT, EMITTER_PIN); 
unsigned int sensorValues[NUM_SENSORS];


//Control Play Station 2
#include <PS2X_lib.h>
#define PS2_DAT      9        
#define PS2_CMD      10
#define PS2_SEL      5
#define PS2_CLK      6

PS2X ps2x; // create PS2 Controller Class

//RoboClaw Packet Serial Mode
#define address 0x80
RoboClaw roboclaw(&Serial1, 10000);


#define pwm_ubicar 25//3//25//(0-100)//85 (64-127)
#define pwm_ataque 70//100///64-127
#define pwm_buscar 40//80//64-127
#define pwm_piso_giro 30
#define pwm_piso_retroceder 20//48//0-64
//#define pwm_pulso_inverso 3//60//0-64
//#define pwm_giro_45 3//75//64-127

#define t_atacando 2000
#define t_atacando_retroceso 50
#define t_giro45 20
#define t_giro90 40
#define t_giro_borde 80
#define t_giro_borde_doble_sensor_piso 10
#define t_retroceso_piso 5
//#define t_retroceso_piso_seguridad 10

#define t_retroceso_piso_Enemigo_frente 20 //tiempo que retrocede cuando tiene el enemigo en frente y toca la linea del borde
#define t_pulso_inverso 5//30 //tiempo que se le da el pulso inverso para frenar el motor.
#define t_retroceso_piso_Enemigo_frente 250
#define t_avance_buscar 80
#define t_avance_espera_buscar 1000
#define t_cambio_giro_motor 10
#define t_actualizacion_PS2 300

//Arranque 30 Zig Zag
#define t_giro_diagonal 60
#define pmw_giro_arranque_1 25
#define pmw_avance_arranque_1 50//25
#define t_avance_arranque 300
#define t_giro_centrar 130

//Arranque 31 Retroceso a Linea
#define pmw_arranque_retroceso 40
#define t_arranque_retroceso 300
#define pwm_arranque_retroceso_recuperacion 10
#define t_arranque_recuperacion 1

int dato_Roboclaw_M1=0, dato_Roboclaw_M2=0;

// Retardos
elapsedMillis mseg_elapsed, mseg_elapsed2 ;
elapsedMillis mseg_lectura_PS2;

int i=0;
int estado=1, estado_anterior=1;
int flag1=0, flag2=0, flag3=0, flag4=0; //Banderas
//int flag1_ataque=0, flag2_ataque=0; //Banderas estado ataque
//int flag1_buscar=0, flag2_buscar=0, flag3_buscar=0; //Banderas estado Buscar
int direccion_arranque=0;
int lectura_sharp[num_sensores_sharp];

void setup() {
 pinMode(LED_PIN, OUTPUT);
 roboclaw.begin(38400);
 setSpeeds(0,0);
 //Configuracion Control PS2 Inalambrico
 int error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT);  // Setup gamepad (clock, command, attention, data) pins
 mseg_lectura_PS2=0; //Iniciamos contador para chequear el control de PS2 a cierto intervalo
 //Inicializamos sensores de piso como si leyera negro
 for(i=0;i<NUM_SENSORS;i++)
  sensorValues[i]=2500;
 
 //Esperamos a que se pulse X para iniciar conteo de 5 seg
 while(ps2x.ButtonDataByte()!= 0xFFFF4000){
  ps2x.read_gamepad();
  mseg_elapsed=0;
  while(mseg_elapsed<10);
 }
 //Iniciamos conteo de 5 Seg
 mseg_elapsed=0;
 while(mseg_elapsed<3000){
  led_on();
  mseg_elapsed2=0;
  while(mseg_elapsed2<100);
  led_off();
  mseg_elapsed2=0;
  while(mseg_elapsed2<100);
 }


estado=ESTADO_INICIAL;
randomSeed(analogRead(0));
direccion_arranque=random(2);
}

void loop() {
  
 if(mseg_lectura_PS2>t_actualizacion_PS2){
  mseg_lectura_PS2=0;
  ps2x.read_gamepad();
  if(ps2x.ButtonDataByte()== 0xFFFF0001){ 
   estado=51;//Detener Robot
  }
  else if(ps2x.ButtonDataByte()== 0xFFFF4000 && estado_anterior==51){
   estado=1;
  }
 } 
  
 switch(estado){
  
  case 1: // Leemos Sensores Sharp y Piso QTR
    for(i=0;i<num_sensores_sharp;i++){
    lectura_sharp[i] = analogRead(i);
    }
    qtrrc.read(sensorValues);
    estado=5; //Evaluamos Lectura de Sharp y Piso QTR
  break;
  
  case 2: // Leemos Solo Sensores Sharp 
    for(i=0;i<num_sensores_sharp;i++){
    lectura_sharp[i] = analogRead(i);
    }
    estado=estado_anterior;
  break;
  
  case 3: // Leemos Solo Sensores de Piso QTR y volvemos al anterior estado
    qtrrc.read(sensorValues);
    estado=estado_anterior;
  break;

  case 4: // Leemos Control PS2
  /*
    ps2x.read_gamepad();
    if(ps2x.ButtonDataByte()== 0xFFFF0001){ 
    estado=51;//Detener Robot
    }
    else if(ps2x.ButtonDataByte()== 0xFFFF4000 && estado_anterior==51){
      estado=1;
    }
    else{
     estado=estado_anterior; 
    }
    */
    estado=100;
  break;

  case 5: //Evaluamos lectura Sensores Sharp y Piso  

    //------------------------------Ningun Sensor-------------------------------//
    if((lectura_sharp[0]&&lectura_sharp[1]&&lectura_sharp[2]&&lectura_sharp[3]&&lectura_sharp[4]&&lectura_sharp[5])<sensibilidad_Sharp)
    estado=20; //Buscando

    //------------------------------Sensores en Diagonal Traseros---------------//
    if(lectura_sharp[5]>sensibilidad_Sharp)
    estado=11;//Gire Izquierda 45°
    //if(lectura_sharp[6]>sensibilidad_Sharp) //Desactivado por que no esta conectado este sensor
    //estado=12;//Gire Derecha 45°
    
    //------------------------------Sensores Laterales--------------------------//
    if(lectura_sharp[0]>sensibilidad_Sharp)
    estado=13;//Gire Izquierda 90°
    if(lectura_sharp[4]>sensibilidad_Sharp)
    estado=14;//Gire Derecha 90°

    //------------------------------Sensores en Diagonal Frente-----------------//
    if(lectura_sharp[1]>sensibilidad_Sharp)
    estado=11;//Gire Izquierda 45° 
    if(lectura_sharp[3]>sensibilidad_Sharp)
    estado=12;//Gire Derecha 45°
 
    //------------------------------Sensores del frente-------------------------//
    if(lectura_sharp[2]>sensibilidad_Sharp)
    estado=10; //Ataque 
    
    //------------------------------Sensores Piso-------------------------------//
    if(sensorValues[0]<sensibilidad_P0_P2 && QTR_Activados==1){ 
    estado=16;//Atras, gire Derecha
    }
    if(sensorValues[2]<sensibilidad_P0_P2 && QTR_Activados==1){ 
    estado=17;//Atras, gire Derecha
    }
    if(sensorValues[1]<sensibilidad_P1 && QTR_Activados==1){ 
    estado=18;//Atras, gire Derecha
    }
    if(sensorValues[0]<sensibilidad_P1 && sensorValues[3]<sensibilidad_P3_P5 && QTR_Activados==1){ 
    estado=21;//Doble sensor lado IZQ,  gire Derecha
    led_on();
    }
    if(sensorValues[2]<sensibilidad_P1 && sensorValues[5]<sensibilidad_P3_P5 && QTR_Activados==1){ 
    estado=22;//Doble sensor lado DER,  gire Izquierda
    led_on();
    }

    if(sensorValues[0]<sensibilidad_P0_P2 && sensorValues[2]<sensibilidad_P0_P2 && sensorValues[3]<sensibilidad_P3_P5 && sensorValues[5]<sensibilidad_P3_P5 && QTR_Activados==1) 
    estado=50;//se salió del Dojo, Quedese Quieto!
     
    //Limpiamos flags si se salio del estado de busqueda antes de terminar rutina de busqueda por que se activo algun sensor
 
    if(estado_anterior==10 && estado!=10){
      flag1=0;
      flag2=0;
    }
    if(estado_anterior==20 && estado!=20){
      flag1=0;
      flag2=0;
   
    }
  break;

  
  
  case 10:      // Ataque
    estado_anterior=10;
    led_on();
    if(_Atacar_){
     if(_Atacar_Tactica_){    
      if(flag1==0){
       setSpeeds(pwm_ataque,pwm_ataque);
       mseg_elapsed=0;
       flag1=1;
      }
      if(flag2==0 && flag1==1){
       if(mseg_elapsed>t_atacando){
        setSpeeds(-pwm_ataque,-pwm_ataque);       
        mseg_elapsed=0;
        flag2=1;
       }
      }
      if(flag2==1){
       if(mseg_elapsed>t_atacando_retroceso || sensorValues[3]<sensibilidad_P3_P5 || sensorValues[4]<sensibilidad_P4 || sensorValues[5]<sensibilidad_P3_P5){
        setSpeeds(pwm_ataque,pwm_ataque);
        mseg_elapsed=0;
        flag1=0;
        flag2=0;
       }
      }
     }
     else
     setSpeeds(pwm_ataque,pwm_ataque);  
    }
    else
     setSpeeds(0,0);
    estado=1;   
  break;
  
  case 11:      //Gire Izquierda 45°
  estado_anterior=11;
   if(flag1==0){
    setSpeeds(-pwm_ubicar,pwm_ubicar);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag1==1){
    estado=2;
    if(mseg_elapsed>t_giro45 || lectura_sharp[2]>sensibilidad_Sharp){
      flag1=0;
      estado=1;
    }
   }  
  break;

  case 12:       //Gire Derecha 45°
   estado_anterior=12;
   if(flag1==0){
    setSpeeds(pwm_ubicar,-pwm_ubicar);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag1==1){
    estado=2;
    if(mseg_elapsed>t_giro45 || lectura_sharp[2]>sensibilidad_Sharp){
      flag1=0;
      estado=1;
    }
   }
  break;

  case 13:      //Gire Izquierda 90°
   estado_anterior=13;
   if(flag1==0){
    setSpeeds(-pwm_ubicar,pwm_ubicar);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag1==1){
    estado=2;
    if(mseg_elapsed>t_giro90 || lectura_sharp[2]>sensibilidad_Sharp || lectura_sharp[1]>sensibilidad_Sharp){
      flag1=0;
      estado=1;
    }
   }
  break;

  case 14:       //Gire Derecha 90°
   estado_anterior=14;
   if(flag1==0){
    setSpeeds(pwm_ubicar,-pwm_ubicar);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag1==1){
    estado=2;
    if(mseg_elapsed>t_giro90 || lectura_sharp[2]>sensibilidad_Sharp || lectura_sharp[3]>sensibilidad_Sharp){
      flag1=0;
      estado=1;
    }
   }
  break;

  case 16:       //Piso QTR IZQ, Gire Derecha
   estado_anterior=16;
   if(flag1==0){
    setSpeeds(-pwm_piso_retroceder,-pwm_piso_retroceder);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag2==0 && flag1==1){
    estado=3;
    if(mseg_elapsed>t_retroceso_piso && sensorValues[0]>sensibilidad_P0_P2 && sensorValues[1]>sensibilidad_P1){
      /*mseg_elapsed=0;
      led_on();
      while(mseg_elapsed<t_retroceso_piso_seguridad);
      led_off();*/
      setSpeeds(pwm_piso_giro,-pwm_piso_giro);
      mseg_elapsed=0;
      flag2=1;
    }
   }
   if(flag2==1){
    estado=2;
    if(mseg_elapsed>t_giro_borde || lectura_sharp[0]>sensibilidad_Sharp || lectura_sharp[1]>sensibilidad_Sharp || lectura_sharp[2]>sensibilidad_Sharp || lectura_sharp[3]>sensibilidad_Sharp || lectura_sharp[4]>sensibilidad_Sharp || lectura_sharp[5]>sensibilidad_Sharp){
      flag1=0;
      flag2=0;
      estado=1;
    }
   }
  break;

  case 17:        //Piso QTR Der, Gire Izquierda
   estado_anterior=17;
   if(flag1==0){
    setSpeeds(-pwm_piso_retroceder,-pwm_piso_retroceder);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag2==0 && flag1==1){
    estado=3;
    if(mseg_elapsed>t_retroceso_piso && sensorValues[2]>sensibilidad_P0_P2 && sensorValues[1]>sensibilidad_P1){
      /*mseg_elapsed=0;
      led_on();
      while(mseg_elapsed<t_retroceso_piso_seguridad);
      led_off();*/
      setSpeeds(-pwm_piso_giro,pwm_piso_giro);
      mseg_elapsed=0;
      flag2=1;
    }
   }
   if(flag2==1){
    estado=2;
    if(mseg_elapsed>t_giro_borde || lectura_sharp[0]>sensibilidad_Sharp || lectura_sharp[1]>sensibilidad_Sharp || lectura_sharp[2]>sensibilidad_Sharp || lectura_sharp[3]>sensibilidad_Sharp || lectura_sharp[4]>sensibilidad_Sharp  || lectura_sharp[5]>sensibilidad_Sharp){
      flag1=0;
      flag2=0;
      estado=1;
    }
   }
  break;
  
  case 18:        //Piso QTR Centro, Retroceda

    estado_anterior=18;
   if(flag1==0){
    setSpeeds(-pwm_piso_retroceder,-pwm_piso_retroceder);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag2==0 && flag1==1){
    estado=3;
    if(mseg_elapsed>t_retroceso_piso && sensorValues[0]>sensibilidad_P0_P2 && sensorValues[1]>sensibilidad_P1 && sensorValues[2]>sensibilidad_P0_P2 ){
      /*mseg_elapsed=0;
      led_on();
      while(mseg_elapsed<t_retroceso_piso_seguridad);
      led_off();*/
      setSpeeds(-pwm_piso_giro,pwm_piso_giro);
      mseg_elapsed=0;
      flag2=1;
    }
   }
   if(flag2==1){
    if(mseg_elapsed>t_giro_borde || lectura_sharp[0]>sensibilidad_Sharp || lectura_sharp[1]>sensibilidad_Sharp || lectura_sharp[2]>sensibilidad_Sharp || lectura_sharp[3]>sensibilidad_Sharp || lectura_sharp[4]>sensibilidad_Sharp  || lectura_sharp[5]>sensibilidad_Sharp){
      flag1=0;
      flag2=0;
      estado=1;
    }
   }
  break;
  

  
//-------------------------------------------------------  
  case 20:      //Buscando
    estado_anterior=20;
    led_off();
    if(_Buscar_){
      if(_Buscar_Tactica){
       if(flag1==0){      
        setSpeeds(pwm_buscar,pwm_buscar);
        mseg_elapsed=0;
        flag1=1;
       }
       if(flag2==0 && flag1==1){
        if(mseg_elapsed>t_avance_buscar){
          setSpeeds(0,0);
          mseg_elapsed=0;
          flag2=1;
        }
       }
       if(flag2==1){
         if(mseg_elapsed>t_avance_espera_buscar){
          flag1=0;
          flag2=0;
         }
       }
     }
     else{
      setSpeeds(pwm_buscar,pwm_buscar);     
     }
    }
    else
     setSpeeds(0,0);
      
    estado=1;
  break;
//----------------------------------------------------------

case 21:        //Piso QTR IZQ delantero y Trasero, Gire Derecha
   estado_anterior=21;
   if(flag1==0){
    setSpeeds(pwm_piso_giro,-pwm_piso_giro);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag1==1){
    estado=3;
    if(mseg_elapsed>t_giro_borde_doble_sensor_piso/* && (sensorValues[0]>sensibilidad_P0_P2 || sensorValues[3]>sensibilidad_P3_P5)*/){
      setSpeeds(0,0);
      flag1=0;
      estado=1;
    }
   }
  break;

case 22:        //Piso QTR DER delantero y Trasero, Gire IZQ
   estado_anterior=22;
   if(flag1==0){
    setSpeeds(-pwm_piso_giro,pwm_piso_giro);
    mseg_elapsed=0;
    flag1=1;
   }
   if(flag1==1){
    estado=3;
    if(mseg_elapsed>t_giro_borde_doble_sensor_piso/* && (sensorValues[0]>sensibilidad_P0_P2 || sensorValues[3]>sensibilidad_P3_P5)*/){
      setSpeeds(0,0);
      flag1=0;
      estado=1;
    }
   }
  break;



//-------------------------------------------------------
  case 30: // Arranque Zig Zag
     estado_anterior=30;
     if(flag1==0){
      if(direccion_arranque==0){
       setSpeeds(-pmw_giro_arranque_1,pmw_giro_arranque_1);
      }
      else{
       setSpeeds(pmw_giro_arranque_1,-pmw_giro_arranque_1);
      }
       mseg_elapsed=0;
       flag1=1;
    }
    if(flag2==0 && flag1==1){
      if(mseg_elapsed>t_giro_diagonal){
       setSpeeds(pmw_avance_arranque_1,pmw_avance_arranque_1);
       mseg_elapsed=0;
       flag2=1;
      }
    }
    if(flag3==0 && flag2==1){
      estado=3; //leemos sensores piso  
      if(mseg_elapsed>t_avance_arranque || sensorValues[0]<sensibilidad_P0_P2 || sensorValues[2]<sensibilidad_P0_P2){  
        if(direccion_arranque==0){
         setSpeeds(pmw_giro_arranque_1,-pmw_giro_arranque_1);
        }
        else{
         setSpeeds(-pmw_giro_arranque_1,pmw_giro_arranque_1);
        }
       mseg_elapsed=0;
       flag3=1;
      }  
    }
    if(flag3==1){
      if(mseg_elapsed>t_giro_centrar){
       setSpeeds(pwm_buscar,pwm_buscar);
       flag1=0;
       flag2=0;
       flag3=0;
       estado=1;
      }  
    }
    
  break;

  case 31: // Arranque Retroceso a Linea
   estado_anterior=31;
    if(flag1==0){
      setSpeeds(-pmw_arranque_retroceso,-pmw_arranque_retroceso);
      mseg_elapsed=0;
      flag1=1;
     }
    if(flag2==0 && flag1==1){
      estado=3;
      if(mseg_elapsed>t_arranque_retroceso || sensorValues[3]<sensibilidad_P3_P5 || sensorValues[4]<sensibilidad_P4 || sensorValues[5]<sensibilidad_P3_P5){
       setSpeeds(pwm_arranque_retroceso_recuperacion,pwm_arranque_retroceso_recuperacion);
       mseg_elapsed=0;
       flag2=1;
      }
    }
    if(flag2==1){
      estado=3; //leemos sensores piso  
      if(mseg_elapsed>t_arranque_recuperacion && sensorValues[3]>sensibilidad_P3_P5 && sensorValues[4]>sensibilidad_P4 && sensorValues[5]>sensibilidad_P3_P5){  
       setSpeeds(0,0);
       flag1=0;
       flag2=0;
       estado=1;
      }
    }      
  break;

//--------------------------------------------------------

  case 50:      //Detener Robot por Sensores de Piso QTR
    setSpeeds(0,0);
    estado=1;
  break;

  case 51:      //Detener Robot por control
    estado_anterior=51;
    setSpeeds(0,0);
  break;    

//-----------------------------------------------------
  default: //Error al seleccionar el siguiente estado
  while(true){
   setSpeeds(0,0);
   led_on();
   mseg_elapsed=0;
   while(mseg_elapsed<100);
   led_off();
   mseg_elapsed=0;
   while(mseg_elapsed<100);
  }
    //estado=1; //vuelva al estado inicial si no se seleccionó algun estado
  break;

 }
}






















//----------------------------------------------------------------------------//
//void led_on()
//----------------------------------------------------------------------------//
void led_on(){
 digitalWrite(LED_PIN, HIGH);
}

//----------------------------------------------------------------------------//
//void led_off()
//----------------------------------------------------------------------------//
void led_off(){
 digitalWrite(LED_PIN, LOW);
}

//----------------------------------------------------------------------------//
//void setM1Speed(int speed)
//----------------------------------------------------------------------------//
void setM1Speed(int Speed){
unsigned int reverse = 0;
      
   if (Speed < 0)
   {
      Speed = -Speed;   // make speed a positive quantity
      reverse = 1;   // preserve the direction
   }
   
   if (Speed > 100)   
      Speed = 100;

   Speed=(Speed*127)/100;
   
   
   if (Speed == 0)
   {
     roboclaw.ForwardM1(address,0);     
   }
   else if (reverse)
   {
      roboclaw.BackwardM1(address,Speed);
   }
   else   // forward
   {
      roboclaw.ForwardM1(address,Speed);
   }
}
//----------------------------------------------------------------------------//
//void setM2Speed(int speed)
//----------------------------------------------------------------------------//
void setM2Speed(int Speed){
unsigned int reverse = 0;
      
   if (Speed < 0)
   {
      Speed = -Speed;   // make speed a positive quantity
      reverse = 1;   // preserve the direction
   }
   
   if (Speed > 100)   
      Speed = 100;

   Speed=(Speed*127)/100;
   
   
   if (Speed == 0)
   {
     roboclaw.ForwardM2(address,0);     
   }
   else if (reverse)
   {
      roboclaw.BackwardM2(address,Speed);
   }
   else   // forward
   {
      roboclaw.ForwardM2(address,Speed);
   }
}
//----------------------------------------------------------------------------//
//void setSpeeds(int speed)
//----------------------------------------------------------------------------//
void setSpeeds(int m1Speed, int m2Speed)
{
   setM1Speed(m1Speed);
   setM2Speed(m2Speed);
}


