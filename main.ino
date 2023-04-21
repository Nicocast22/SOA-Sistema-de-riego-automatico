
// Habilitacion de debug para la impresion por el puerto serial ...
//----------------------------------------------
#define SERIAL_DEBUG_ENABLED 1

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
#define UMBRAL_DIFERENCIA_TIMEOUT                   50
#define UMBRAL_TEMPERATURA_FRIO                     250
#define UMBRAL_TEMPERATURA_MEDIO                    700
#define MAX_CANT_SENSORES                           1
#define SENSOR_TEMPERATURA                          0
#define PIN_SENSOR_TEMPERATURA                      A0
#define PIN_LED_AZUL                                7
#define PIN_LED_VERDE                               6
#define PIN_LED_ROJO                                5
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

enum states          { ST_INIT,  ST_IDLE        , ST_WAITING_RESPONSE   , ST_ERROR                                                  } current_state;
String states_s [] = {"ST_INIT", "ST_IDLE"      , "ST_WAITING_RESPONSE" , "ST_ERROR"                                                };

enum events          { EV_CONT,  EV_TEMP_COLD   , EV_TEMP_NICE          , EV_TEMP_HOT   , EV_TIMEOUT  , EV_ACK_MSJ    , EV_UNKNOW   } new_event;
String events_s [] = {"EV_CONT", "EV_TEMP_COLD" , "EV_TEMP_NICE"        , "EV_TEMP_HOT", "EV_TIMEOUT" , "EV_ACK_MSJ"  , "EV_UNKNOW" };

#define MAX_STATES 4
#define MAX_EVENTS 6

typedef void (*transition)();

transition state_table[MAX_STATES][MAX_EVENTS] =
{
      {init_    , error         , error         , error       , error       , error       } , // state ST_INIT
      {none     , temp_cold_a   , temp_nice_a   , temp_hot_a  , none        , error       } , // state ST_IDLE
      {none     , temp_cold_a   , temp_nice     , temp_hot    , none        , error       } , // state ST_WAITING_RESPONSE
      {error    , error         , error         , error       , error       , error       }   // state ST_ERROR
      
     //EV_CONT  , EV_TEMP_COLD  , EV_TEMP_NICE  , EV_TEMP_HOT , EV_TIMEOUT  , EV_ACK_MSJ
};

bool timeout;
long lct;
bool temperatura;


//----------------------------------------------
void do_init()
{
  Serial.begin(9600);
  
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AZUL , OUTPUT);
  pinMode(PIN_LED_ROJO , OUTPUT);

  sensores[SENSOR_TEMPERATURA].pin    = PIN_SENSOR_TEMPERATURA;
  sensores[SENSOR_TEMPERATURA].estado = ESTADO_SENSOR_OK;
  
  // Inicializo el evento inicial
  current_state = ST_INIT;
  
  timeout = false;
  lct     = millis();
}
//----------------------------------------------

//----------------------------------------------
long leerSensorTemperatura( )
{
  return analogRead(PIN_SENSOR_TEMPERATURA);
}
//----------------------------------------------

//----------------------------------------------
void apagar_leds( )
{
  digitalWrite(PIN_LED_VERDE, false);
  digitalWrite(PIN_LED_AZUL , false);
  digitalWrite(PIN_LED_ROJO , false);
}
//----------------------------------------------

//----------------------------------------------
void actualizar_indicador_led_azul( )
{
  digitalWrite(PIN_LED_VERDE, false);
  digitalWrite(PIN_LED_AZUL , true );
  digitalWrite(PIN_LED_ROJO , false);
}
//----------------------------------------------

//----------------------------------------------
void actualizar_indicador_led_verde( )
{
  digitalWrite(PIN_LED_VERDE, true );
  digitalWrite(PIN_LED_AZUL , false);
  digitalWrite(PIN_LED_ROJO , false);
}
//----------------------------------------------

//----------------------------------------------
void actualizar_indicador_led_rojo( )
{
  digitalWrite(PIN_LED_VERDE, false);
  digitalWrite(PIN_LED_AZUL , false);
  digitalWrite(PIN_LED_ROJO , true );
}
//----------------------------------------------

//----------------------------------------------
void actualizar_display_temperatura()
{
  // temperatura
}
//----------------------------------------------

// ---------------------------------------------
bool verificarEstadoSensorTemperatura( )
{
  sensores[SENSOR_TEMPERATURA].valor_actual = leerSensorTemperatura( );
  
  int valor_actual = sensores[SENSOR_TEMPERATURA].valor_actual;
  int valor_previo = sensores[SENSOR_TEMPERATURA].valor_previo;
  
  if( valor_actual != valor_previo )
  {
    sensores[SENSOR_TEMPERATURA].valor_previo = valor_actual;
    
    if( valor_actual < UMBRAL_TEMPERATURA_FRIO )
    {
      new_event = EV_TEMP_COLD;
    }
    else if( (valor_actual >= UMBRAL_TEMPERATURA_FRIO)&& (valor_actual < UMBRAL_TEMPERATURA_MEDIO) )
    {
      new_event = EV_TEMP_NICE;
    }
    if( valor_actual >= UMBRAL_TEMPERATURA_MEDIO )
    {
      new_event = EV_TEMP_HOT;
    }
    
    return true;
  }
  
  return false;
}
//----------------------------------------------

//----------------------------------------------
bool verificarOtroSensor1()
{
  // Emulacion Sensor 1 ...
  return false;
}
//----------------------------------------------

//----------------------------------------------
bool verificarOtroSensor2()
{
  // Emulacion Sensor 2 ...
  return false;
}
//----------------------------------------------

//----------------------------------------------
bool enviar_mensaje()
{
}
//----------------------------------------------

//----------------------------------------------
bool verificarSiHayRespuestaServidor()
{
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
    
    if( (verificarEstadoSensorTemperatura() == true) || (verificarOtroSensor1() == true) || (verificarOtroSensor2() == true) ||
        (verificarSiHayRespuestaServidor() == true) )
    {
      return;
    }
  }
  
  // Genero evento dummy ....
  new_event = EV_CONT;
}
//----------------------------------------------

void init_()
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

void temp_cold_a()
{
  actualizar_indicador_led_azul( );
  current_state = ST_IDLE;
}

void temp_nice_a()
{
  actualizar_display_temperatura( );
  actualizar_indicador_led_verde( );
  current_state = ST_IDLE;
}

void temp_hot_a()
{
  actualizar_display_temperatura( );
  actualizar_indicador_led_rojo( );
  enviar_mensaje( );
  current_state = ST_WAITING_RESPONSE;
}

void temp_hot()
{
  actualizar_display_temperatura( );
  actualizar_indicador_led_rojo( );
  current_state = ST_WAITING_RESPONSE;
}

void temp_cold()
{
  actualizar_indicador_led_azul( );
  current_state = ST_WAITING_RESPONSE;
}

void temp_nice()
{
  actualizar_display_temperatura( );
  actualizar_indicador_led_verde( );
  current_state = ST_WAITING_RESPONSE;
}

//----------------------------------------------
void maquina_estados_climatizador_ambiente( )
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
    DebugPrintEstado(states_s[ST_ERROR], events_s[EV_UNKNOW]);
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
  maquina_estados_climatizador_ambiente();
}
//----------------------------------------------

