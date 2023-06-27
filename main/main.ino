#include <Servo.h>
#include <SoftwareSerial.h>

// Bits setup for TIMER2 interruptions
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// Interruptions counter
#define howManyInterrupts  31 * 10

// -------------------- Sensors' state --------------------
#define OK_STATE_SENSOR                            108
#define ERROR_STATE_SENSOR                         666
// ----------------------------------------------

// -------------------- Thresholds --------------------
#define THRESHOLD_TIMEOUT                   10

#define THRESHOLD_LOW_HUMIDITY                          1000
#define THRESHOLD_HIGH_HUMIDITY                         400

#define LOW_LIGHT                                       1
 
#define THRESHOLD_NO_WATER                              900
#define THRESHOLD_LOW_WATER                             500
#define THRESHOLD_HIGH_WATER                            380

#define RAIN_SENSOR_HIGH                                1

// -------------------- Servomotors' spin angle --------------------
#define SERVO_STARTING_ANGLE                            0
#define OPENED_TANK_DOOR_ANGLE                          91
#define CLOSED_TANK_DOOR_ANGLE                          -90
#define OPENED_DRAINAGE_VALVE_ANGLE                     91
#define CLOSED_DRAINAGE_VALVE_ANGLE                     -90  
#define WATER_MOVEMENT_ANGLE                            90
#define WATER_MOVEMENT_MAX_ANGLE                        90

//-------------------- Commands --------------------

#define ACTIONS_DRAIN                                   "ACD"
#define SENSORS_VALUE_LIGHT                             "SVL"
#define SENSORS_VALUE_WATER_LEVEL                       "SVW"


// -------------------- Sensors --------------------
#define MAX_SENSORS_AMOUNT                              5
#define SENSOR_HUMIDITY                                 0
#define SENSOR_LIGHT                                    1
#define SENSOR_WATER_LEVEL                              2
#define SENSOR_RAIN                                     3
#define SENSOR_DRAINAGE                                 4

// -------------------- Pins --------------------
#define PIN_HUMIDITY_SENSOR                             A0
#define PIN_WATER_LEVEL_SENSOR                          A1
#define PIN_WATER_PUMP                                  4
#define PIN_DRAINAGE_VALVE_SERVO                        5
#define PIN_TX_BLUETOOTH                                6
#define PIN_RX_BLUETOOTH                                7
#define PIN_RAIN_SENSOR                                 8
#define PIN_WATER_MOVEMENT_SERVO                        9
#define PIN_TANK_DOOR_SERVO                             11
#define PIN_LIGHT_SENSOR                                13 
            


// -------------------- Sensor structure --------------------
struct stSensor
{
  int pin;
  int state;
  long currentValue;
  long prevValue;
};
stSensor sensors[MAX_SENSORS_AMOUNT];

// -------------------- States & events --------------------
enum states          { ST_INIT  , ST_IDLE  , ST_LOW_HUMIDITY  , ST_LOW_LIGHT  , ST_WATERING  , ST_RAINING  , ST_DOOR_OPEN  , ST_DRAINING  , ST_NO_WATER  , ST_ERROR  } currentState;
String states_s [] = { "ST_INIT", "ST_IDLE", "ST_LOW_HUMIDITY", "ST_LOW_LIGHT", "ST_WATERING", "ST_RAINING", "ST_DOOR_OPEN", "ST_DRAINING", "ST_NO_WATER", "ST_ERROR"};

enum events          { EV_CONT  ,  EV_LOW_HUMIDITY  , EV_MEDIUM_HUMIDITY  , EV_HIGH_HUMIDITY  , EV_NIGHTFALL  , EV_MORNING  , EV_NO_WATER  , EV_LOW_WATER  , EV_MEDIUM_WATER  , EV_HIGH_WATER  , EV_RAINING  , EV_NOT_RAINING  , EV_DRAINAGE  , EV_STOP_DRAINAGE  , EV_TIMEOUT  , EV_UNKNOWN  } newEvent;
String events_s [] = { "EV_CONT",  "EV_LOW_HUMIDITY", "EV_MEDIUM_HUMIDITY", "EV_HIGH_HUMIDITY", "EV_NIGHTFALL", "EV_MORNING", "EV_NO_WATER", "EV_LOW_WATER", "EV_MEDIUM_WATER", "EV_HIGH_WATER", "EV_RAINING", "EV_NOT_RAINING", "EV_DRAINAGE", "EV_STOP_DRAINAGE", "EV_TIMEOUT", "EV_UNKNOWN"};

#define MAX_STATES 10
#define MAX_EVENTS 16

typedef void (*transition)();



// -------------------- Global variables --------------------
bool timeout;
long lct;
bool moveWater = false;
int rotatingOffset = 45;
Servo tankDoorServo;
Servo waterMovementServo;
Servo drainageValveServo;
SoftwareSerial BTSerial(PIN_RX_BLUETOOTH, PIN_TX_BLUETOOTH);

// -------------------- Setup functions --------------------
void setPinModes()
{
  pinMode(PIN_WATER_PUMP, OUTPUT);
  pinMode(PIN_RAIN_SENSOR, INPUT);
  pinMode(PIN_TANK_DOOR_SERVO, OUTPUT);
  pinMode(PIN_DRAINAGE_VALVE_SERVO, OUTPUT);
  pinMode(PIN_WATER_MOVEMENT_SERVO, OUTPUT);
}

void setSensors()
{
  sensors[SENSOR_HUMIDITY].pin      = PIN_HUMIDITY_SENSOR;
  sensors[SENSOR_HUMIDITY].state    = OK_STATE_SENSOR;
  
  sensors[SENSOR_LIGHT].pin         = PIN_LIGHT_SENSOR;
  sensors[SENSOR_LIGHT].state       = OK_STATE_SENSOR;

  sensors[SENSOR_WATER_LEVEL].pin   = PIN_WATER_LEVEL_SENSOR;
  sensors[SENSOR_WATER_LEVEL].state = OK_STATE_SENSOR;

  sensors[SENSOR_RAIN].pin          = PIN_RAIN_SENSOR;
  sensors[SENSOR_RAIN].state        = OK_STATE_SENSOR;
}

void attachServos()
{
  // Initial positions
  tankDoorServo.write(SERVO_STARTING_ANGLE);
  drainageValveServo.write(SERVO_STARTING_ANGLE);
  waterMovementServo.write(SERVO_STARTING_ANGLE);

  tankDoorServo.attach(PIN_TANK_DOOR_SERVO);
  drainageValveServo.attach(PIN_DRAINAGE_VALVE_SERVO);
  waterMovementServo.attach(PIN_WATER_MOVEMENT_SERVO);
}

void configHWTimerInterruptions()
{
  // Setup TIMER2
  TIMSK2 = 0;
  cbi(ASSR, AS2);

  // Clean registers
  TCCR2B = 0;  
  TCCR2A = 0;
  TCNT2 = 0;

  sbi(TCCR2A, WGM20);

  sbi(TCCR2B, WGM22); 

  // Set timer bits
  sbi(TCCR2B,CS22);
  sbi(TCCR2B,CS21);
  sbi(TCCR2B,CS20);

  OCR2A = 252;

  // Allow timer
  sbi(TIMSK2,TOIE2);
}

void configSWTimerInterruptions()
{
  timeout = false;
  lct     = millis();
}

void initialSetup()
{
  Serial.begin(9600);
  BTSerial.begin(9600);

  setPinModes();
  setSensors();
  attachServos();
  configHWTimerInterruptions();
  configSWTimerInterruptions();

  // Initialize starting state
  currentState = ST_INIT;
}

// -------------------- Reading functions --------------------
long readHumiditySensor()
{
  return analogRead(PIN_HUMIDITY_SENSOR);
}

long readLightSensor()
{
  return digitalRead(PIN_LIGHT_SENSOR);
}

long readWaterLevelSensor()
{
  return analogRead(PIN_WATER_LEVEL_SENSOR);
}

int readRainSensor()
{
  return digitalRead(PIN_RAIN_SENSOR);
}

bool compareString(const String& receivedString, const String& targetString) {
  return receivedString.equals(targetString);
}

String readCommand(){
  String cmd = BTSerial.readStringUntil('\n');
  cmd.trim();
  return cmd;
}

void sendBTInformation(String value){
  BTSerial.println(value);
}



// -------------------- Servo movement functions --------------------
void setTankDoorServo(int angle)
{
  tankDoorServo.write(angle);
}

void setDrainageValveServo(int angle)
{
  drainageValveServo.write(angle);
}

void setWaterMovementServo(int angle)
{
  waterMovementServo.write(WATER_MOVEMENT_ANGLE + angle);
}

// -------------------- Sensor checking functions --------------------
bool checkHumiditySensorState()
{
  sensors[SENSOR_HUMIDITY].currentValue = readHumiditySensor();
  
  int currentValue = sensors[SENSOR_HUMIDITY].currentValue;
  int prevValue = sensors[SENSOR_HUMIDITY].prevValue;
  
  if (currentValue != prevValue)
  {
    sensors[SENSOR_HUMIDITY].prevValue = currentValue;
    
    if (currentValue >= THRESHOLD_LOW_HUMIDITY)
    {
      newEvent = EV_LOW_HUMIDITY;
    }
    else if (currentValue > THRESHOLD_HIGH_HUMIDITY && currentValue <= THRESHOLD_LOW_HUMIDITY )
    {
      newEvent = EV_MEDIUM_HUMIDITY;
    }
    else if (currentValue <= THRESHOLD_HIGH_HUMIDITY)
    {
      newEvent = EV_HIGH_HUMIDITY;
    }
    
    return true;
  }
  
  return false;
}

bool checkRainSensorState()
{
  sensors[SENSOR_RAIN].currentValue = readRainSensor();

  int currentValue = sensors[SENSOR_RAIN].currentValue;
  int prevValue = sensors[SENSOR_RAIN].prevValue;

  if (currentValue != prevValue)
  {
    sensors[SENSOR_RAIN].prevValue = currentValue;

    if (currentValue == RAIN_SENSOR_HIGH)
    {
      newEvent = EV_RAINING;
    } 
    else
    {
      newEvent = EV_NOT_RAINING; 
    }

    return true;
  }

  return false;
}

bool checkLightSensorState()
{
  sensors[SENSOR_LIGHT].currentValue = readLightSensor();
  
  int currentValue = sensors[SENSOR_LIGHT].currentValue;
  int prevValue = sensors[SENSOR_LIGHT].prevValue;
  
  if (currentValue != prevValue)
  {
    sensors[SENSOR_LIGHT].prevValue = currentValue;
    
    if (currentValue == LOW_LIGHT)
    {
      newEvent = EV_NIGHTFALL;
    }
    else
    {
      newEvent = EV_MORNING;
    }
    
    return true;
  }
  
  return false;
}

bool checkWaterLevelSensorState()
{
  sensors[SENSOR_WATER_LEVEL].currentValue = readWaterLevelSensor();
  
  int currentValue = sensors[SENSOR_WATER_LEVEL].currentValue;
  int prevValue = sensors[SENSOR_WATER_LEVEL].prevValue;
  
  if (currentValue != prevValue)
  {
    sensors[SENSOR_WATER_LEVEL].prevValue = currentValue;
    
    if (currentValue >= THRESHOLD_NO_WATER) 
    {
      newEvent = EV_NO_WATER;
      moveWater = false;

    }
    else if (currentValue < THRESHOLD_NO_WATER && currentValue >= THRESHOLD_LOW_WATER)
    {
      newEvent = EV_LOW_WATER;
      moveWater = false;
    }
    else if ((currentValue < THRESHOLD_LOW_WATER) && (currentValue > THRESHOLD_HIGH_WATER))
    {
      newEvent = EV_MEDIUM_WATER;
      moveWater = true;
    }
    else if (currentValue <= THRESHOLD_HIGH_WATER)
    {
      newEvent = EV_HIGH_WATER;
      moveWater = true;
    }
    
    return true;
  }
  
  return false;
}

bool checkBluetoothCommands()
{
  String cmd;
  BTSerial.flush();

    if(BTSerial.available()){
      cmd = readCommand();
      Serial.print(cmd);
      if(compareString(cmd, ACTIONS_DRAIN)){
        newEvent = EV_DRAINAGE;
        return true;
      }else if(compareString(cmd,SENSORS_VALUE_LIGHT)){
        newEvent = EV_CONT;
        long value = readLightSensor();
        String info = "L@" + (String)value;
        sendBTInformation(info);
        return true;
      }else if(compareString(cmd, SENSORS_VALUE_WATER_LEVEL)){
        newEvent = EV_CONT;
        Serial.println("Leyendo sensor agua");
        long value = readWaterLevelSensor();
        String info = "W@" + (String)value;
        sendBTInformation(info);
        return true;
      }  
    }

  return false;
}

bool checkAllSensors()
{
  return (checkBluetoothCommands() == true) || (checkHumiditySensorState() == true) || (checkLightSensorState() == true) || (checkWaterLevelSensorState() == true) || (checkRainSensorState() == true);
}


// -------------------- New event functions --------------------
void getNewEvent()
{
  long ct = millis();
  int  difference = (ct - lct);
  timeout = (difference > THRESHOLD_TIMEOUT)? (true):(false);

  if (timeout)
  {
    timeout = false;
    lct     = ct;
    
    if (checkAllSensors())
    {
      return;
    }
  }
  
  // Dummy event
  newEvent = EV_CONT;
}

// -------------------- Error & void functions --------------------
void error()
{
  Serial.println("There was an error. Please reset Arduino.");
}

void none()
{
}

void initConfig()
{
  currentState = ST_IDLE;
}

// -------------------- State change functions --------------------
void lowSunlight()
{
  currentState = ST_LOW_LIGHT;
}

void highSunlight()
{
  digitalWrite(PIN_WATER_PUMP, LOW);
  currentState = ST_IDLE;
}

void lowHumidity() 
{
  currentState = ST_LOW_HUMIDITY;
}

void mediumHumidity() 
{
  currentState = ST_IDLE;
}

void highHumidity() 
{
  digitalWrite(PIN_WATER_PUMP, LOW);
  currentState = ST_IDLE;
}

void lowWater()
{
  digitalWrite(PIN_WATER_PUMP, LOW);
  currentState = ST_IDLE;
}

void mediumWater()
{
  currentState = ST_IDLE;
}

void highWater()
{
  currentState = ST_WATERING;
}

void raining()
{
  digitalWrite(PIN_WATER_PUMP, LOW);
  currentState = ST_RAINING;
}

void notRaining()
{
  setTankDoorServo(CLOSED_TANK_DOOR_ANGLE);
  currentState = ST_IDLE;
}

void openTankDoor()
{
  setTankDoorServo(OPENED_TANK_DOOR_ANGLE);
  currentState = ST_DOOR_OPEN;
}

void closeTankDoor()
{
  setTankDoorServo(CLOSED_TANK_DOOR_ANGLE);
  currentState = ST_RAINING;
}

void closeTankDoorRainStopped() 
{
  setTankDoorServo(CLOSED_TANK_DOOR_ANGLE);
  currentState = ST_IDLE;
}

void watering()
{
  digitalWrite(PIN_WATER_PUMP, HIGH);
  currentState = ST_WATERING;
}

void draining()
{
  setDrainageValveServo(OPENED_DRAINAGE_VALVE_ANGLE);
  currentState = ST_DRAINING;
}

void stopDraining()
{
  setDrainageValveServo(CLOSED_DRAINAGE_VALVE_ANGLE);
  currentState = ST_IDLE;
}

void noWater()
{
  digitalWrite(PIN_WATER_PUMP, LOW);
  currentState = ST_NO_WATER;
}


transition stateTable[MAX_STATES][MAX_EVENTS] =
{
      {initConfig , error           , error               , error             , error         , error         , none          , error         , error           , error         , none        , none                      , error       , error             , none        , none        } , // state ST_INIT
      {none       , lowHumidity     , mediumHumidity      , highHumidity      , lowSunlight   , highSunlight  , noWater       , highSunlight  , none            , none          , raining     , none                      , draining    , none              , none        , none        } , // state ST_IDLE
      {none       , lowHumidity     , mediumHumidity      , highHumidity      , lowSunlight   , highSunlight  , noWater       , none          , none            , none          , raining     , none                      , draining    , none              , none        , none        } , // state ST_LOW_HUMIDITY
      {none       , none            , mediumHumidity      , highHumidity      , none          , highSunlight  , noWater       , none          , none            , highWater     , raining     , none                      , draining    , none              , none        , none        } , // state ST_LOW_LIGHT 
      {none       , watering        , none                , highHumidity      , watering      , highSunlight  , noWater       , lowWater      , none            , watering      , raining     , none                      , none        , none              , none        , none        } , // state ST_WATERING
      {none       , none            , none                , none              , none          , none          , openTankDoor  , openTankDoor  , openTankDoor    , closeTankDoor , raining     , notRaining                , none        , none              , none        , none        } , // state ST_RAINING
      {none       , none            , none                , none              , none          , none          , none          , none          , none            , closeTankDoor , none        , closeTankDoorRainStopped  , none        , none              , none        , none        } , // state ST_DOOR_OPEN
      {none       , none            , none                , none              , none          , none          , stopDraining  , none          , none            , none          , none        , none                      , draining    , stopDraining      , none        , none        } , // state ST_DRAINING
      {none       , none            , none                , none              , none          , none          , noWater       , lowWater      , mediumWater     , highWater     , raining     , none                      , none        , none              , none        , none        } , // state ST_NO_WATER
      {error      , error           , error               , error             , error         , error         , error         , error         , error           , error         , error       , error                     , error       , error             , error       , error       }   // state ST_ERROR
     //EV_CONT    , EV_LOW_HUMIDITY , EV_MEDIUM_HUMIDITY  , EV_HIGH_HUMIDITY  , EV_NIGHTFALL  , EV_MORNING    , EV_NO_WATER   , EV_LOW_WATER  , EV_MEDIUM_WATER , EV_HIGH_WATER , EV_RAINING  , EV_NOT_RAINING            , EV_DRAINAGE , EV_STOP_DRAINAGE  , EV_TIMEOUT  , EV_UNKNOWN  
};

// -------------------- State machine --------------------
void automaticWateringStateMachine()
{
  getNewEvent();

  Serial.print("ESTADO");
  Serial.println(states_s[currentState]);

  Serial.print("EVENTO");
  Serial.println(events_s[newEvent]);

  if ((newEvent >= 0) && (newEvent < MAX_EVENTS) && (currentState >= 0) && (currentState < MAX_STATES))
  {
    stateTable[currentState][newEvent]();
  }
  
  newEvent = EV_CONT;
}

// -------------------- TIMER2 interruption handler --------------------
int counterICQ = 0;
int offset = 0;
ISR(TIMER2_OVF_vect)
{
  counterICQ++;
  if(counterICQ == howManyInterrupts) {
    counterICQ = 0;
    if(moveWater)
    {
      setWaterMovementServo(offset);
      offset += rotatingOffset;
      if(offset == WATER_MOVEMENT_MAX_ANGLE)
      {
        offset = -offset;
      }
    }
  }
}


// -------------------- Arduino functions --------------------
void setup()
{
  initialSetup();
}


void loop()
{
  automaticWateringStateMachine();
}
