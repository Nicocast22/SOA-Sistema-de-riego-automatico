#include <Servo.h>

// Bits setup for TIMER2 interruptions
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// Interruptions counter
#define howManyInterrupts  31 * 5

// -------------------- Sensors' state --------------------
#define OK_STATE_SENSOR                            108
#define ERROR_STATE_SENSOR                         666
// ----------------------------------------------

// -------------------- Thresholds --------------------
#define THRESHOLD_TIMEOUT                   10

#define THRESHOLD_LOW_HUMIDITY                      		250
#define THRESHOLD_MEDIUM_HUMIDITY                      	500
#define THRESHOLD_HIGH_HUMIDITY                      		700

#define THRESHOLD_LOW_LIGHT                   	    		200
#define THRESHOLD_HIGH_LIGHT                   	    		400
 
#define THRESHOLD_NO_WATER                              40
#define THRESHOLD_LOW_WATER                   	    		100
#define THRESHOLD_MEDIUM_WATER                       		500
#define THRESHOLD_HIGH_WATER                   	    		1000

#define RAIN_SENSOR_HIGH                                1
#define DRAINAGE_SENSOR_HIGH                            1

// -------------------- Servomotors' spin angle --------------------
#define OPENED_TANK_DOOR_ANGLE                          91
#define CLOSED_TANK_DOOR_ANGLE                          -90 


// -------------------- Sensors --------------------
#define MAX_SENSORS_AMOUNT                              5
#define SENSOR_HUMIDITY                           		  0
#define SENSOR_LIGHT                            			  1
#define SENSOR_WATER_LEVEL                              2
#define SENSOR_RAIN                                     3
#define	SENSOR_DRAINAGE								                  4

// -------------------- Pins --------------------
#define PIN_HUMIDITY_SENSOR                      	      A0
#define PIN_LIGHT_SENSOR                    			      A1               
#define PIN_WATER_LEVEL_SENSOR						              A2
#define PIN_ORANGE_LED 	                                2
#define PIN_WATER_PUMP                                  4
#define PIN_GREEN_LED                                   6
#define PIN_BLUE_LED	                                  7
#define PIN_RAIN_SENSOR                                 8
#define PIN_WATER_MOVEMENT_SERVO 	                      9
#define PIN_TANK_DOOR_SERVO                             10
#define	PIN_DRAINAGE_SENSOR							                12
#define PIN_YELLOW_LED							                    13

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

enum events          { EV_CONT  ,  EV_LOW_HUMIDITY  , EV_MEDIUM_HUMIDITY  , EV_HIGH_HUMIDITY  , EV_NIGHTFALL  , EV_MORNING  , EV_NO_WATER  , EV_LOW_WATER  , EV_MEDIUM_WATER  , EV_HIGH_WATER  , EV_RAINING  , EV_NOT_RAINING  , EV_DRAINAGE  , EV_STOP_DRAINAGE  , EV_TIMEOUT  , EV_UNKNOWN  } new_event;
String events_s [] = { "EV_CONT",  "EV_LOW_HUMIDITY", "EV_MEDIUM_HUMIDITY", "EV_HIGH_HUMIDITY", "EV_NIGHTFALL", "EV_MORNING", "EV_NO_WATER", "EV_LOW_WATER", "EV_MEDIUM_WATER", "EV_HIGH_WATER", "EV_RAINING", "EV_NOT_RAINING", "EV_DRAINAGE", "EV_STOP_DRAINAGE", "EV_TIMEOUT", "EV_UNKNOWN"};

#define MAX_STATES 10
#define MAX_EVENTS 16

typedef void (*transition)();

transition stateTable[MAX_STATES][MAX_EVENTS] =
{
      {none       , error           , error               , error		          , error         , error		      , none          , error         , error           , error         , none        , none                      , error       , error             , none        , none        } , // state ST_INIT
      {none       , lowHumidity     , mediumHumidity      , highHumidity	    , lowSunlight   , highSunlight  , noWater       , highSunlight  , none            , none          , raining     , none                      , draining    , none              , none        , none        } , // state ST_IDLE
      {none       , lowHumidity     , mediumHumidity      , highHumidity      , lowSunlight   , highSunlight  , noWater       , none          , none            , none          , raining     , none                      , draining    , none              , none        , none        } , // state ST_LOW_HUMIDITY
      {none       , none            , mediumHumidity      , highHumidity      , none          , highSunlight  , noWater       , none          , none            , highWater     , raining     , none                      , draining    , none              , none        , none        } , // state ST_LOW_LIGHT 
      {none       , watering        , none                , highHumidity	    , watering      , highSunlight  , noWater       , lowWater      , none            , watering      , raining     , none                      , none        , none              , none        , none        } , // state ST_WATERING
      {none       , none            , none                , none         	    , none          , none          , openTankDoor  , openTankDoor  , openTankDoor    , closeTankDoor , raining     , notRaining                , none        , none              , none        , none        } , // state ST_RAINING
      {none       , none            , none                , none         	    , none          , none          , none          , none          , none            , closeTankDoor , none        , closeTankDoorRainStopped  , none        , none              , none        , none        } , // state ST_DOOR_OPEN
      {none       , none            , none                , none         	    , none          , none          , noWater       , lowWater      , none            , none          , none        , none                      , draining    , stopDraining      , none        , none        } , // state ST_DRAINING
      {none       , none            , none                , none         	    , none          , none          , noWater       , lowWater      , mediumWater     , highWater     , raining     , none                      , none        , none              , none        , none        } , // state ST_NO_WATER
      {error      , error           , error               , error       	    , error         , error         , none          , none          , error 	        , none          , none        , none                      , none        , none              , none        , none        }   // state ST_ERROR
     //EV_CONT    , EV_LOW_HUMIDITY , EV_MEDIUM_HUMIDITY  , EV_HIGH_HUMIDITY  , EV_NIGHTFALL  , EV_MORNING	  , EV_NO_WATER   , EV_LOW_WATER  , EV_MEDIUM_WATER , EV_HIGH_WATER , EV_RAINING  , EV_NOT_RAINING            , EV_DRAINAGE , EV_STOP_DRAINAGE  , EV_TIMEOUT  , EV_UNKNOWN  
};

// -------------------- Global variables --------------------
bool timeout;
long lct;
bool moveWater = false;
bool rotatingLeft = true;
Servo tankDoorServo;
Servo waterMovementServo;

void printState()
{

}

// -------------------- Setup functions --------------------
void setPinModes()
{
  pinMode(PIN_GREEN_LED, OUTPUT);
  pinMode(PIN_BLUE_LED , OUTPUT);
  pinMode(PIN_ORANGE_LED, OUTPUT);
  pinMode(PIN_YELLOW_LED, OUTPUT);
  pinMode(PIN_WATER_PUMP, OUTPUT);
  pinMode(PIN_RAIN_SENSOR, INPUT);
  pinMode(PIN_TANK_DOOR_SERVO, OUTPUT);
  pinMode(PIN_WATER_MOVEMENT_SERVO, OUTPUT);
  pinMode(PIN_DRAINAGE_SENSOR, INPUT);
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

  sensors[SENSOR_DRAINAGE].pin      = PIN_DRAINAGE_SENSOR;
  sensors[SENSOR_DRAINAGE].state    = OK_STATE_SENSOR;
}

void attachServos()
{
  tankDoorServo.attach(PIN_TANK_DOOR_SERVO);
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

  setPinModes();
  setSensors();
  attachServos();
  configHWTimerInterruptions();
  configSWTimerInterruptions();

  turnLedsOff();

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
  return analogRead(PIN_LIGHT_SENSOR);
}

long readWaterLevelSensor()
{
  return analogRead(PIN_WATER_LEVEL_SENSOR);
}

int readRainSensor()
{
  return digitalRead(PIN_RAIN_SENSOR);
}

long readDrainageSensor()
{
  return digitalRead(PIN_DRAINAGE_SENSOR);
}


// -------------------- Leds functions --------------------
void turnLedsOff()
{
  digitalWrite(PIN_GREEN_LED, false);
  digitalWrite(PIN_BLUE_LED , false);
  digitalWrite(PIN_ORANGE_LED, false);
  digitalWrite(PIN_YELLOW_LED, false);
}

void updateBlueLed()
{
  digitalWrite(PIN_GREEN_LED, false);
  digitalWrite(PIN_BLUE_LED , true);
  digitalWrite(PIN_ORANGE_LED , false);
  digitalWrite(PIN_YELLOW_LED, false);
}

void updateGreenLed()
{
  digitalWrite(PIN_GREEN_LED, true);
  digitalWrite(PIN_BLUE_LED , false);
  digitalWrite(PIN_ORANGE_LED , false);
  digitalWrite(PIN_YELLOW_LED, false);
}

void updateOrangeLed()
{
  digitalWrite(PIN_BLUE_LED, false);
  digitalWrite(PIN_ORANGE_LED , true);
  digitalWrite(PIN_GREEN_LED , false);
  digitalWrite(PIN_YELLOW_LED, false);
}

void updateYellowLed()
{
  digitalWrite(PIN_YELLOW_LED, true);
  digitalWrite(PIN_ORANGE_LED , false);
  digitalWrite(PIN_GREEN_LED , false);
  digitalWrite(PIN_BLUE_LED , false);
}

// -------------------- Tank door functions --------------------
void setTankDoor(int angle){
  tankDoorServo.write(angle);
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
    
    if (currentValue < THRESHOLD_LOW_HUMIDITY)
    {
      new_event = EV_LOW_HUMIDITY;
    }
    else if (currentValue >= THRESHOLD_LOW_HUMIDITY && currentValue < THRESHOLD_MEDIUM_HUMIDITY)
    {
      new_event = EV_MEDIUM_HUMIDITY;
    }
    else if (currentValue >= THRESHOLD_HIGH_HUMIDITY)
    {
      new_event = EV_HIGH_HUMIDITY;
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
      new_event = EV_RAINING;
    } 
    else
    {
      new_event = EV_NOT_RAINING; 
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
    
    if (currentValue <= THRESHOLD_LOW_LIGHT)
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

bool checkWaterLevelSensorState()
{
  sensors[SENSOR_WATER_LEVEL].currentValue = readWaterLevelSensor();
  
  int currentValue = sensors[SENSOR_WATER_LEVEL].currentValue;
  int prevValue = sensors[SENSOR_WATER_LEVEL].prevValue;
  
  if (currentValue != prevValue)
  {
    sensors[SENSOR_WATER_LEVEL].prevValue = currentValue;
    
    if (currentValue <= THRESHOLD_NO_WATER) 
    {
      new_event = EV_NO_WATER;
    }
    else if (currentValue > THRESHOLD_NO_WATER && currentValue <= THRESHOLD_LOW_WATER)
    {
      new_event = EV_LOW_WATER;
    }
    else if ((currentValue > THRESHOLD_LOW_WATER) && (currentValue < THRESHOLD_HIGH_WATER))
    {
      new_event = EV_MEDIUM_WATER;
    }
    else if (currentValue >= THRESHOLD_HIGH_WATER)
    {
      new_event = EV_HIGH_WATER;
    }
    
    return true;
  }
  
  return false;
}

bool checkDrainageSensorState()
{
  sensors[SENSOR_DRAINAGE].currentValue = readDrainageSensor();
  
  int currentValue = sensors[SENSOR_DRAINAGE].currentValue;
  int prevValue = sensors[SENSOR_DRAINAGE].prevValue;
  
  if (currentValue != prevValue)
  {
    sensors[SENSOR_DRAINAGE].prevValue = currentValue;

    if (currentValue == DRAINAGE_SENSOR_HIGH)
    {
      new_event = EV_DRAINAGE;
    } 
    else 
    {
      new_event = EV_STOP_DRAINAGE; 
    }    

    return true;
  }

  return false;
}

void checkAllSensors()
{
  return (checkHumiditySensorState() == true) || (checkLightSensorState() == true) || (checkWaterLevelSensorState() == true) || (checkRainSensorState() == true) || (checkDrainageSensorState() == true);
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
  new_event = EV_CONT;
}

// -------------------- Error & void functions --------------------
void error()
{
  // print error
}

void none()
{
}

// -------------------- State change functions --------------------
void lowSunlight()
{
  updateBlueLed();
  currentState = ST_LOW_LIGHT;
}

void highSunlight()
{
  updateBlueLed();
  digitalWrite(PIN_WATER_PUMP, LOW);
  currentState = ST_IDLE;
}

void lowHumidity() 
{
  updateBlueLed();
  currentState = ST_LOW_HUMIDITY;
}

void mediumHumidity() 
{
  updateBlueLed();
  currentState = ST_IDLE;
}

void highHumidity() 
{
  updateBlueLed();
  digitalWrite(PIN_WATER_PUMP, LOW);
  currentState = ST_IDLE;
}

void lowWater()
{
  updateBlueLed();
  digitalWrite(PIN_WATER_PUMP, LOW);
  moveWater = false;
  currentState = ST_IDLE;
}

void mediumWater()
{
  updateBlueLed();
  moveWater = true;
  currentState = ST_IDLE;
}

void highWater()
{
  updateGreenLed();
  moveWater = true;
  currentState = ST_WATERING;
}

void raining()
{
  updateOrangeLed();
  digitalWrite(PIN_WATER_PUMP, LOW);
  currentState = ST_RAINING;
}

void notRaining()
{
  updateBlueLed();
  setTankDoor(CLOSED_TANK_DOOR_ANGLE);
  currentState = ST_IDLE;
}

void openTankDoor()
{
  setTankDoor(OPENED_TANK_DOOR_ANGLE);
  currentState = ST_DOOR_OPEN;
}

void closeTankDoor()
{
  updateOrangeLed();
  setTankDoor(CLOSED_TANK_DOOR_ANGLE);
  currentState = ST_RAINING;
}

void closeTankDoorRainStopped() 
{
  updateBlueLed();
  setTankDoor(CLOSED_TANK_DOOR_ANGLE);
  currentState = ST_IDLE;
}

void watering()
{
  updateGreenLed();
  digitalWrite(PIN_WATER_PUMP, HIGH);
  currentState = ST_WATERING;
}

void draining()
{
  updateYellowLed();
  currentState = ST_DRAINING;
}

void stopDraining()
{
  updateBlueLed();
  currentState = ST_IDLE;
}

void noWater()
{
  updateBlueLed();
  digitalWrite(PIN_WATER_PUMP, LOW);
  moveWater = false;
  currentState = ST_NO_WATER;
}

// -------------------- State machine --------------------
void automaticWateringStateMachine()
{
  getNewEvent();

  if ((new_event >= 0) && (new_event < MAX_EVENTS) && (currentState >= 0) && (currentState < MAX_STATES))
  {
    if (new_event != EV_CONT)
    {
      printState(states_s[currentState], events_s[new_event]);
    }
    
    stateTable[currentState][new_event]();
  }
  else
  {
    printState(states_s[ST_ERROR], events_s[EV_UNKNOWN]);
  }
  
  new_event = EV_CONT;
}

// -------------------- TIMER2 interruption handler --------------------
int counterICQ = 0;
ISR(TIMER2_OVF_vect)
{
  counterICQ++;
  if(moveWater && counterICQ == howManyInterrupts) {
    counterICQ = 0;
    waterMovementServo.write(rotatingLeft ? 0 : 179);
    rotatingLeft = !rotatingLeft;
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
