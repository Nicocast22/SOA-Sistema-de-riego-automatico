#include <SoftwareSerial.h>

#define PIN_TX_BLUETOOTH                                6
#define PIN_RX_BLUETOOTH                                7

SoftwareSerial BTSerial(PIN_RX_BLUETOOTH, PIN_TX_BLUETOOTH);

void setup() {
    Serial.begin(9600);
    BTSerial.begin(9600);
}

void loop() {
    if (BTSerial.available()) {
        Serial.write(BTSerial.read());
    }

    if(Serial.available()) {
        BTSerial.write(Serial.read());
    }
}
