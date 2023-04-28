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
#define UMBRAL_AGUA_BAJA                   	    		100
#define UMBRAL_AGUA_MEDIA                       		500
#define UMBRAL_AGUA_ALTA                   	    		1000
//Lluvia
#define RAIN_TRUE                                   1
//Tapa
#define OPEN_DOOR_ANGLE                             91
#define CLOSED_DOOR_ANGLE                           0 


// Sensores
#define MAX_CANT_SENSORES                           3

#define SENSOR_HUMEDAD                           		0
#define SENSOR_LUZ                            			1
#define SENSOR_NIVEL_AGUA                           2
#define SENSOR_LLUVIA                               3

// Pines
#define PIN_SENSOR_HUMEDAD                      	  A0
#define PIN_SENSOR_LUZ                    			    A1               
#define PIN_SENSOR_NIVEL_AGUA						            A2
#define PIN_BOMBA_AGUA                              4
#define PIN_LED_VERDE                               6
#define PIN_LED_AZUL	                              7
#define PIN_SENSOR_LLUVIA                           8
#define PIN_SERVO_TAPA                              10


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

enum states          { ST_INIT,  ST_IDLE , ST_LOW_HUMIDITY, ST_LOW_LIGHT, ST_WATERING, ST_RANNING, ST_DOOR_OPEN, ST_DOOR_CLOSED, ST_ERROR } current_state;
String states_s [] = { "ST_INIT", "ST_IDLE" , "ST_LOW_HUMIDITY", "ST_LOW_LIGHT", "ST_WATERING", "ST_RANNING", "ST_DOOR_OPEN", "ST_DOOR_CLOSED", "ST_ERROR"};

enum events          { EV_CONT,  EV_LOW_MOISTURE, EV_MEDIUM_MOISTURE, EV_HIGH_MOISTURE, EV_NIGHTFALL, EV_MORNING, EV_LOW_WATER, EV_MEDIUM_WATER, EV_HIGH_WATER, EV_TIMEOUT, EV_UNKNOWN, EV_RAINNING, EV_NOT_RAINING} new_event;
String events_s [] = { "EV_CONT",  "EV_LOW_MOISTURE", "EV_MEDIUM_MOISTURE", "EV_HIGH_MOISTURE", "EV_NIGHTFALL", "EV_MORNING", "EV_LOW_WATER", "EV_MEDIUM_WATER", "EV_HIGH_WATER", "EV_TIMEOUT" , "EV_UNKNOWN", "EV_RAINNING", "EV_NOT_RAINING"};

#define MAX_STATES 9
#define MAX_EVENTS 13

typedef void (*transition)();

transition state_table[MAX_STATES][MAX_EVENTS] =
{
      {initConfig   , error             , error             , error		        , error       , error		      , error        , error           , error         , none      , none       , none            , none          } , // state ST_INIT
      {none         , low_moisture      , medium_moisture   , high_moisture	  , low_sunlight, high_sunlight , low_water    , none            , none          , none      , none       , set_rain        , none          } , // state ST_IDLE
      {none         , low_moisture      , medium_moisture   , high_moisture   , low_sunlight, high_sunlight , none         , none            , none          , none      , none       , set_rain        , none          } , // state ST_LOW_HUMIDITY
      {none         , none              , medium_moisture   , high_moisture   , none        , high_sunlight , none         , none            , high_water    , none      , none       , set_rain        , none          } , // state ST_LOW_LIGHT 
      {none         , watering          , none              , high_moisture	  , watering    , high_sunlight , low_water    , medium_water    , watering      , none      , none       , none            , none          } , // state ST_WATERING
      {none         , none              , none              , none         	  , none        , none          , open_door    , open_door       , close_door    , none      , none       , none            , set_not_rain  } , // state ST_RAINNING
      {none         , none              , none              , none         	  , none        , none          , none         , none            , close_door    , none      , none       , none            , close_door    } , // state ST_DOOR_OPEN
      {none         , none              , none              , none         	  , none        , none          , none         , none            , none          , none      , none       , set_rain        , none          } , // state ST_DOOR_CLOSED
      {error        , error             , error             , error       	  , error       , error         , none         , error 	         , none          , none      , none       , none            , none          }   // state ST_ERROR
     //EV_CONT      , EV_LOW_MOISTURE	  , EV_MEDIUM_MOISTURE, EV_HIGH_MOISTURE, EV_NIGHTFALL, EV_MORNING	  , EV_LOW_WATER , EV_MEDIUM_WATER , EV_HIGH_WATER , EV_TIMEOUT, EV_UNKNOWN , EV_RAINNING     , EV_NOT_RAINING
};

bool timeout;
long lct;
bool temperatura;
bool humedad;
bool luz;
Servo servo;

//----------------------------------------------
void do_init()
{
  Serial.begin(9600);
  
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AZUL , OUTPUT);
  pinMode(PIN_BOMBA_AGUA, OUTPUT);
  pinMode(PIN_SENSOR_LLUVIA, INPUT);
  pinMode(PIN_SERVO_TAPA, OUTPUT);

  sensores[SENSOR_HUMEDAD].pin        = PIN_SENSOR_HUMEDAD;
  sensores[SENSOR_HUMEDAD].estado     = ESTADO_SENSOR_OK;
  
  sensores[SENSOR_LUZ].pin            = PIN_SENSOR_LUZ;
  sensores[SENSOR_LUZ].estado         = ESTADO_SENSOR_OK;

  sensores[SENSOR_NIVEL_AGUA].pin     = PIN_SENSOR_NIVEL_AGUA;
  sensores[SENSOR_NIVEL_AGUA].estado  = ESTADO_SENSOR_OK;

  sensores[SENSOR_LLUVIA].pin         = PIN_SENSOR_LLUVIA;
  sensores[SENSOR_LLUVIA].estado      = ESTADO_SENSOR_OK;

  servo.attach(PIN_SERVO_TAPA);

  
  // Inicializo el evento inicial
  current_state = ST_INIT;
  
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
//----------------------------------------------

//----------------------------------------------
void apagar_leds( )
{
  digitalWrite(PIN_LED_VERDE, false);
  digitalWrite(PIN_LED_AZUL , false);
}
//----------------------------------------------

//----------------------------------------------
void actualizar_indicador_led_azul( )
{
  digitalWrite(PIN_LED_VERDE, false);
  digitalWrite(PIN_LED_AZUL , true );
}
//----------------------------------------------

//----------------------------------------------
void actualizar_indicador_led_verde( )
{
  digitalWrite(PIN_LED_VERDE, true );
  digitalWrite(PIN_LED_AZUL , false);
}

void set_door(int angle){
  servo.write(angle);
  delay(1000);
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
    else if( (valor_actual >= UMBRAL_HUMEDAD_BAJA) && (valor_actual < UMBRAL_HUMEDAD_MEDIA) )
    {
      new_event = EV_MEDIUM_MOISTURE;
    }
    if( valor_actual >= UMBRAL_HUMEDAD_ALTA )
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
      new_event = EV_RAINNING;  
      return true;
    }else{
      new_event = EV_NOT_RAINING; 
      return true;
    }    
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
    
    if( valor_actual <= UMBRAL_AGUA_BAJA )
    {
      new_event = EV_LOW_WATER;
    }
    else if( (valor_actual > UMBRAL_AGUA_BAJA) && (valor_actual < UMBRAL_AGUA_ALTA) )
    {
      new_event = EV_MEDIUM_WATER;
    }
    if( valor_actual >= UMBRAL_AGUA_ALTA )
    {
      new_event = EV_HIGH_WATER;
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
    
    if( (verificarEstadoSensorHumedad() == true) || (verificarEstadoSensorLuz() == true) || (verificarEstadoSensorNivelAgua() == true) || (verificarSensorLluvia() == true))
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
  current_state = ST_IDLE;
}

void low_water()
{
  actualizar_indicador_led_azul();
  current_state = ST_IDLE;
}

void medium_water()
{
  actualizar_indicador_led_azul();
  current_state = ST_IDLE;
}

void high_water()
{
  actualizar_indicador_led_verde();
  current_state = ST_WATERING;
}

void set_rain()
{
  current_state = ST_RANNING;
}

void set_not_rain(){
  current_state = ST_IDLE;
}

void open_door(){
  set_door(OPEN_DOOR_ANGLE);
  current_state = ST_DOOR_OPEN;
}

void close_door(){
  set_door(CLOSED_DOOR_ANGLE);
  current_state = ST_DOOR_CLOSED;
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


//----------------------------------------------
void maquinaEstadosRegadorAutomatico( )
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

//----------------------------------------------
void loop()
{
  Serial.println(current_state);
  maquinaEstadosRegadorAutomatico();
}
//----------------------------------------------
