#include <Servo.h>

Servo servo;

int drain = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  servo.attach(5);
  attachInterrupt(digitalPinToInterrupt(2), startDrain, CHANGE);
}

void loop() {
  if(drain == 1){
    Serial.println("draining");
    servo.write(180);
  }

}

void startDrain(){
  Serial.println("Button interrupt");
  drain = 1;
}