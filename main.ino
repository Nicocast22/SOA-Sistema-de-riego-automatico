#include <Servo.h>


// Habilitacion de debug para la impresion por el puerto serial ...
//----------------------------------------------
#define SERIAL_DEBUG_ENABLED 0

#if SERIAL_DEBUG_ENABLED
  #define DebugPrint(str)\
      {\
        Serial.println(str);\
      }
#else
  #define DebugPrint(str)
#endif

#define DebugPrintEstado(estado,evento)\
      {\
        String est = estado;\
        String evt = evento;\
        String str;\
        str = "-----------------------------------------------------";\
        DebugPrint(str);\
        str = "EST-> [" + est + "]: " + "EVT-> [" + evt + "].";\
        DebugPrint(str);\
        str = "-----------------------------------------------------";\
        DebugPrint(str);\
      }
//----------------------------------------------

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
//Variables globales

// Contamos la cantidad de interrupciones
#define howManyInterrupts  31 * 5
bool moveWater = false;
bool rotatingLeft = true;

//----------------------------------------------
// Estado de un sensor ...
#define ESTADO_SENSOR_OK                            108
#define ESTADO_SENSOR_ERROR                         666
//----------------------------------------------

//----------------------------------------------
// Estado de un mensaje ...
#define MENSAJE_ENVIADO_OK                          10
#define MENSAJE_ENVIADO_ERROR                       666
//----------------------------------------------

// Otras constantes ....
//----------------------------------------------
#define UMBRAL_DIFERENCIA_TIMEOUT                   10
// Humedad
#define UMBRAL_HUMEDAD_BAJA                      		250
#define UMBRAL_HUMEDAD_MEDIA                      	500
#define UMBRAL_HUMEDAD_ALTA                      		700
// Luz
#define UMBRAL_LUZ_BAJA                   	    		200
#define UMBRAL_LUZ_ALTA                   	    		400
// Agua   
#define UMBRAL_SIN_AGUA                             40
#define UMBRAL_AGUA_BAJA                   	    		100
#define UMBRAL_AGUA_MEDIA                       		500
#define UMBRAL_AGUA_ALTA                   	    		1000
//Lluvia
#define RAIN_TRUE                                   1
//Tapa
#define OPEN_DOOR_ANGLE                             91
#define CLOSED_DOOR_ANGLE                           -90 
//Drenaje
#define DRAINAGE_TRUE                               1

// Sensores
#define MAX_CANT_SENSORES                           5

#define SENSOR_HUMEDAD                           		0
#define SENSOR_LUZ                            			1
#define SENSOR_NIVEL_AGUA                           2
#define SENSOR_LLUVIA                               3
#define	SENSOR_DRENADOR								              4

// Pines
#define PIN_SENSOR_HUMEDAD                      	  A0
#define PIN_SENSOR_LUZ                    			    A1               
#define PIN_SENSOR_NIVEL_AGUA						            A2
#define PIN_LED_NARANJA 	                          2
#define PIN_SERVO_MOVIMIENTO_AGUA 	                9
#define PIN_BOMBA_AGUA                              4
#define PIN_LED_VERDE                               6
#define PIN_LED_AZUL	                              7
#define PIN_SENSOR_LLUVIA                           8
#define PIN_SERVO_TAPA                              10
#define	PIN_SENSOR_DRENADOR							            12
#define PIN_LED_AMARILLO							              13

//----------------------------------------------

//----------------------------------------------
struct stSensor
{
  int  pin;
  int  estado;
  long valor_actual;
  long valor_previo;
};
stSensor sensores[MAX_CANT_SENSORES];
//----------------------------------------------

enum states          { ST_INIT,  ST_IDLE , ST_LOW_HUMIDITY, ST_LOW_LIGHT, ST_WATERING, ST_RAINING, ST_DOOR_OPEN, ST_DRAINING, ST_NO_WATER, ST_ERROR } current_state;
String states_s [] = { "ST_INIT", "ST_IDLE" , "ST_LOW_HUMIDITY", "ST_LOW_LIGHT", "ST_WATERING", "ST_RAINING", "ST_DOOR_OPEN", "ST_DRAINING", "ST_NO_WATER", "ST_ERROR"};

enum events          { EV_CONT,  EV_LOW_MOISTURE, EV_MEDIUM_MOISTURE, EV_HIGH_MOISTURE, EV_NIGHTFALL, EV_MORNING, EV_LOW_WATER, EV_MEDIUM_WATER, EV_HIGH_WATER, EV_TIMEOUT, EV_UNKNOWN, EV_RAINING, EV_NOT_RAINING, EV_DRAINAGE, EV_STOP_DRAINAGE, EV_NO_WATER} new_event;
String events_s [] = { "EV_CONT",  "EV_LOW_MOISTURE", "EV_MEDIUM_MOISTURE", "EV_HIGH_MOISTURE", "EV_NIGHTFALL", "EV_MORNING", "EV_LOW_WATER", "EV_MEDIUM_WATER", "EV_HIGH_WATER", "EV_TIMEOUT" , "EV_UNKNOWN", "EV_RAINING", "EV_NOT_RAINING", "EV_DRAINAGE", "EV_STOP_DRAINAGE", "EV_NO_WATER"};

#define MAX_STATES 10
#define MAX_EVENTS 16

typedef void (*transition)();

transition state_table[MAX_STATES][MAX_EVENTS] =
{
      {initConfig   , error             , error             , error		        , error       , error		      , error        , error           , error         , none      , none       , none            , none                  , error       , error          , none      } , // state ST_INIT
      {none         , low_moisture      , medium_moisture   , high_moisture	  , low_sunlight, high_sunlight , low_water    , none            , none          , none      , none       , set_rain        , none                  , draining    , none           , noWater   } , // state ST_IDLE
      {none         , low_moisture      , medium_moisture   , high_moisture   , low_sunlight, high_sunlight , none         , none            , none          , none      , none       , set_rain        , none                  , draining    , none           , noWater   } , // state ST_LOW_HUMIDITY
      {none         , none              , medium_moisture   , high_moisture   , none        , high_sunlight , none         , none            , high_water    , none      , none       , set_rain        , none                  , draining    , none           , noWater   } , // state ST_LOW_LIGHT 
      {none         , watering          , none              , high_moisture	  , watering    , high_sunlight , low_water    , none            , watering      , none      , none       , set_rain        , none                  , none        , none           , noWater   } , // state ST_WATERING
      {none         , none              , none              , none         	  , none        , none          , open_door    , open_door       , close_door    , none      , none       , set_rain        , set_not_rain          , none        , none           , open_door } , // state ST_RAINING
      {none         , none              , none              , none         	  , none        , none          , none         , none            , close_door    , none      , none       , none            , closeDoorRainStopped  , none        , none           , none      } , // state ST_DOOR_OPEN
      {none         , none              , none              , none         	  , none        , none          , low_water    , none            , none          , none      , none       , none            , none                  , draining    , stopDraining   , noWater   } , // state ST_DRAINING
      {none         , none              , none              , none         	  , none        , none          , low_water    , medium_water    , high_water    , none      , none       , set_rain        , none                  , none        , none           , noWater   } , // state ST_NO_WATER
      {error        , error             , error             , error       	  , error       , error         , none         , error 	         , none          , none      , none       , none            , none                  , none        , none           , none      }   // state ST_ERROR
     //EV_CONT      , EV_LOW_MOISTURE	  , EV_MEDIUM_MOISTURE, EV_HIGH_MOISTURE, EV_NIGHTFALL, EV_MORNING	  , EV_LOW_WATER , EV_MEDIUM_WATER , EV_HIGH_WATER , EV_TIMEOUT, EV_UNKNOWN , EV_RAINING      , EV_NOT_RAINING        , EV_DRAINAGE , EV_STOP_DRAINAGE, EV_NO_WATER
};

bool timeout;
long lct;
bool temperatura;
bool humedad;
bool luz;
Servo servo;
Servo waterMovementServo;

//----------------------------------------------
void do_init()
{
  Serial.begin(9600);
  
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AZUL , OUTPUT);
  pinMode(PIN_LED_NARANJA, OUTPUT);
  pinMode(PIN_LED_AMARILLO, OUTPUT);
  pinMode(PIN_BOMBA_AGUA, OUTPUT);
  pinMode(PIN_SENSOR_LLUVIA, INPUT);
  pinMode(PIN_SERVO_TAPA, OUTPUT);
  pinMode(PIN_SERVO_MOVIMIENTO_AGUA, OUTPUT);
  pinMode(PIN_SENSOR_DRENADOR, INPUT);

  sensores[SENSOR_HUMEDAD].pin        = PIN_SENSOR_HUMEDAD;
  sensores[SENSOR_HUMEDAD].estado     = ESTADO_SENSOR_OK;
  
  sensores[SENSOR_LUZ].pin            = PIN_SENSOR_LUZ;
  sensores[SENSOR_LUZ].estado         = ESTADO_SENSOR_OK;

  sensores[SENSOR_NIVEL_AGUA].pin     = PIN_SENSOR_NIVEL_AGUA;
  sensores[SENSOR_NIVEL_AGUA].estado  = ESTADO_SENSOR_OK;

  sensores[SENSOR_LLUVIA].pin         = PIN_SENSOR_LLUVIA;
  sensores[SENSOR_LLUVIA].estado      = ESTADO_SENSOR_OK;

  sensores[SENSOR_DRENADOR].pin         = PIN_SENSOR_DRENADOR;
  sensores[SENSOR_DRENADOR].estado      = ESTADO_SENSOR_OK;

  servo.attach(PIN_SERVO_TAPA);
  waterMovementServo.attach(PIN_SERVO_MOVIMIENTO_AGUA);
  
  // Inicializo el evento inicial
  current_state = ST_INIT;

  // Setup TIMER2
  TIMSK2 = 0;
  cbi(ASSR, AS2);

  // Limpiamos registros de timers
  TCCR2B = 0;  
  TCCR2A = 0;
  TCNT2 = 0;

  sbi(TCCR2A, WGM20);

  sbi(TCCR2B, WGM22); 

  // Settings timer
  sbi(TCCR2B,CS22); //set this bit
  sbi(TCCR2B,CS21); //set this bit
  sbi(TCCR2B,CS20); //set this bit

  OCR2A = 252;

  // Habilitamos el timer
  sbi(TIMSK2,TOIE2); 
  //sei();
  
  timeout = false;
  lct     = millis();
}
//----------------------------------------------

//----------------------------------------------
long leerSensorTemperatura( )
{
  return analogRead(PIN_SENSOR_HUMEDAD);
}

long leerSensorHumedad()
{
  return analogRead(PIN_SENSOR_HUMEDAD);
}

long leerSensorLuz()
{
  return analogRead(PIN_SENSOR_LUZ);
}

long leerSensorNivelAgua()
{
  return analogRead(PIN_SENSOR_NIVEL_AGUA);
}

int leerSensorLluvia(){
  return digitalRead(PIN_SENSOR_LLUVIA);
}

long leerSensorDrenador()
{
  return digitalRead(PIN_SENSOR_DRENADOR);
}
//----------------------------------------------

//----------------------------------------------
void apagar_leds( )
{
  digitalWrite(PIN_LED_VERDE, false);
  digitalWrite(PIN_LED_AZUL , false);
  digitalWrite(PIN_LED_NARANJA, false);
  digitalWrite(PIN_LED_AMARILLO, false);

}
//----------------------------------------------

//----------------------------------------------
void actualizar_indicador_led_azul( )
{
  digitalWrite(PIN_LED_VERDE, false);
  digitalWrite(PIN_LED_AZUL , true);
  digitalWrite(PIN_LED_NARANJA , false);
  digitalWrite(PIN_LED_AMARILLO, false);
}
//----------------------------------------------

//----------------------------------------------
void actualizar_indicador_led_verde( )
{
  digitalWrite(PIN_LED_VERDE, true);
  digitalWrite(PIN_LED_AZUL , false);
  digitalWrite(PIN_LED_NARANJA , false);
  digitalWrite(PIN_LED_AMARILLO, false);

}

void actualizar_indicador_led_naranja( )
{
  digitalWrite(PIN_LED_AZUL, false);
  digitalWrite(PIN_LED_NARANJA , true);
  digitalWrite(PIN_LED_VERDE , false);
  digitalWrite(PIN_LED_AMARILLO, false);

}

void actualizar_indicador_led_amarillo( )
{
  digitalWrite(PIN_LED_AMARILLO, true);
  digitalWrite(PIN_LED_NARANJA , false);
  digitalWrite(PIN_LED_VERDE , false);
  digitalWrite(PIN_LED_AZUL , false);
}

void set_door(int angle){
  servo.write(angle);
}
//----------------------------------------------

// ---------------------------------------------
bool verificarEstadoSensorHumedad()
{
  sensores[SENSOR_HUMEDAD].valor_actual = leerSensorHumedad();
  
  int valor_actual = sensores[SENSOR_HUMEDAD].valor_actual;
  int valor_previo = sensores[SENSOR_HUMEDAD].valor_previo;
  
  if( valor_actual != valor_previo )
  {
    sensores[SENSOR_HUMEDAD].valor_previo = valor_actual;
    
    if( valor_actual < UMBRAL_HUMEDAD_BAJA )
    {
      new_event = EV_LOW_MOISTURE;
    }
    else if( valor_actual >= UMBRAL_HUMEDAD_BAJA && valor_actual < UMBRAL_HUMEDAD_MEDIA )
    {
      new_event = EV_MEDIUM_MOISTURE;
    }
    else if( valor_actual >= UMBRAL_HUMEDAD_ALTA )
    {
      new_event = EV_HIGH_MOISTURE;
    }
    
    return true;
  }
  
  return false;
}


bool verificarSensorLluvia(){
  sensores[SENSOR_LLUVIA].valor_actual = leerSensorLluvia();

  int valor_actual = sensores[SENSOR_LLUVIA].valor_actual;
  int valor_previo = sensores[SENSOR_LLUVIA].valor_previo;

  if(valor_actual != valor_previo){
    sensores[SENSOR_LLUVIA].valor_previo = valor_actual;

    if(valor_actual == RAIN_TRUE){
      new_event = EV_RAINING;
    } else{
      new_event = EV_NOT_RAINING; 
    }    
    return true;
  }
  return false;
}

bool verificarEstadoSensorLuz()
{
  sensores[SENSOR_LUZ].valor_actual = leerSensorLuz();
  
  int valor_actual = sensores[SENSOR_LUZ].valor_actual;
  int valor_previo = sensores[SENSOR_LUZ].valor_previo;
  
  if( valor_actual != valor_previo )
  {
    sensores[SENSOR_LUZ].valor_previo = valor_actual;
    
    if( valor_actual <= UMBRAL_LUZ_BAJA )
    {
      new_event = EV_NIGHTFALL;
    }
    else
    {
      new_event = EV_MORNING;
    }
    
    return true;
  }
  
  return false;
}

bool verificarEstadoSensorNivelAgua()
{
  sensores[SENSOR_NIVEL_AGUA].valor_actual = leerSensorNivelAgua();
  
  int valor_actual = sensores[SENSOR_NIVEL_AGUA].valor_actual;
  int valor_previo = sensores[SENSOR_NIVEL_AGUA].valor_previo;
  
  if( valor_actual != valor_previo )
  {
    sensores[SENSOR_NIVEL_AGUA].valor_previo = valor_actual;
    
    if(valor_actual <= UMBRAL_SIN_AGUA) 
    {
      new_event = EV_NO_WATER;
    }
    else if( valor_actual > UMBRAL_SIN_AGUA && valor_actual <= UMBRAL_AGUA_BAJA )
    {
      new_event = EV_LOW_WATER;
    }
    else if( (valor_actual > UMBRAL_AGUA_BAJA) && (valor_actual < UMBRAL_AGUA_ALTA) )
    {
      new_event = EV_MEDIUM_WATER;
    }
    else if( valor_actual >= UMBRAL_AGUA_ALTA )
    {
      new_event = EV_HIGH_WATER;
    }
    
    return true;
  }
  
  return false;
}

bool verificarEstadoSensorDrenador()
{
  sensores[SENSOR_DRENADOR].valor_actual = leerSensorDrenador();
  
  int valor_actual = sensores[SENSOR_DRENADOR].valor_actual;
  int valor_previo = sensores[SENSOR_DRENADOR].valor_previo;
  
  if(valor_actual != valor_previo){
    sensores[SENSOR_DRENADOR].valor_previo = valor_actual;

    if(valor_actual == DRAINAGE_TRUE){
      new_event = EV_DRAINAGE;
    } 
    else {
      new_event = EV_STOP_DRAINAGE; 
    }    
    return true;
  }
  return false;
}
//----------------------------------------------

//----------------------------------------------
void get_new_event( )
{
  long ct = millis();
  int  diferencia = (ct - lct);
  timeout = (diferencia > UMBRAL_DIFERENCIA_TIMEOUT)? (true):(false);

  if( timeout )
  {
    // Doy acuse de la recepcion del timeout
    timeout = false;
    lct     = ct;
    
    if( (verificarEstadoSensorHumedad() == true) || (verificarEstadoSensorLuz() == true) || (verificarEstadoSensorNivelAgua() == true) || (verificarSensorLluvia() == true) || (verificarEstadoSensorDrenador() == true))
    {
      return;
    }
  }
  
  // Genero evento dummy ....
  new_event = EV_CONT;
}
//----------------------------------------------

void initConfig()
{
  DebugPrintEstado(states_s[current_state], events_s[new_event]);
  apagar_leds();
  current_state = ST_IDLE;
}

void error()
{
}

void none()
{
}

// IDLE
//----------------------------------------------
void low_sunlight()
{
  actualizar_indicador_led_azul();
  current_state = ST_LOW_LIGHT;
}

void high_sunlight()
{
  actualizar_indicador_led_azul();
  digitalWrite(PIN_BOMBA_AGUA, LOW);
  current_state = ST_IDLE;
}

void low_moisture() 
{
  actualizar_indicador_led_azul();
  current_state = ST_LOW_HUMIDITY;
}

void medium_moisture() 
{
  actualizar_indicador_led_azul();
  current_state = ST_IDLE;
}

void high_moisture() 
{
  actualizar_indicador_led_azul();
  digitalWrite(PIN_BOMBA_AGUA, LOW);
  current_state = ST_IDLE;
}

void low_water()
{
  actualizar_indicador_led_azul();
  digitalWrite(PIN_BOMBA_AGUA, LOW);
  moveWater = false;
  current_state = ST_IDLE;
}

void medium_water()
{
  actualizar_indicador_led_azul();
  moveWater = true;
  current_state = ST_IDLE;
}

void high_water()
{
  actualizar_indicador_led_verde();
  moveWater = true;
  current_state = ST_WATERING;
}

void set_rain()
{
  actualizar_indicador_led_naranja();
  digitalWrite(PIN_BOMBA_AGUA, LOW);
  current_state = ST_RAINING;
}

void set_not_rain()
{
  actualizar_indicador_led_azul();
  set_door(CLOSED_DOOR_ANGLE);
  current_state = ST_IDLE;
}

void open_door()
{
  set_door(OPEN_DOOR_ANGLE);
  current_state = ST_DOOR_OPEN;
}

void close_door()
{
  actualizar_indicador_led_naranja();
  set_door(CLOSED_DOOR_ANGLE);
  current_state = ST_RAINING;
}

void closeDoorRainStopped() 
{
  actualizar_indicador_led_azul();
  set_door(CLOSED_DOOR_ANGLE);
  current_state = ST_IDLE;
}

//----------------------------------------------


//WATERING
//----------------------------------------------
void watering()
{
  actualizar_indicador_led_verde();
  digitalWrite(PIN_BOMBA_AGUA, HIGH);
  current_state = ST_WATERING;
}
//----------------------------------------------

//DRAINING
//----------------------------------------------
void draining()
{
  actualizar_indicador_led_amarillo();
  current_state = ST_DRAINING;
}

void stopDraining()
{
  actualizar_indicador_led_azul();
  current_state = ST_IDLE;
}
//----------------------------------------------

void noWater()
{
  actualizar_indicador_led_azul();
  digitalWrite(PIN_BOMBA_AGUA, LOW);
  moveWater = false;
  current_state = ST_NO_WATER;
}

//----------------------------------------------
void maquinaEstadosRegadorAutomatico()
{
  get_new_event();

  if( (new_event >= 0) && (new_event < MAX_EVENTS) && (current_state >= 0) && (current_state < MAX_STATES) )
  {
    if( new_event != EV_CONT )
    {
      DebugPrintEstado(states_s[current_state], events_s[new_event]);
    }
    
    state_table[current_state][new_event]();
  }
  else
  {
    DebugPrintEstado(states_s[ST_ERROR], events_s[EV_UNKNOWN]);
  }
  
  // Consumo el evento...
  new_event   = EV_CONT;
}
//----------------------------------------------


// Funciones de arduino !. 
//----------------------------------------------
void setup()
{
  do_init();
}
//----------------------------------------------

int counterICQ = 0;

ISR(TIMER2_OVF_vect)
{
  counterICQ++;
  if(moveWater && counterICQ == howManyInterrupts) {
    counterICQ = 0;
    Serial.println("CAMBIANDO DIRECCION DE SERVO");
    waterMovementServo.write(rotatingLeft ? 0 : 179);
    rotatingLeft = !rotatingLeft;
  }
}

//----------------------------------------------
void loop()
{
  //Serial.println(current_state);
  Serial.println(moveWater);
  maquinaEstadosRegadorAutomatico();
}
//----------------------------------------------
