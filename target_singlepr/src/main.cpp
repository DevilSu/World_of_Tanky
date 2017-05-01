#include <SoftwareSerial.h>
#include "Arduino.h"

int prev;
void setup() {
    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    pinMode(13, OUTPUT);
    digitalWrite(13,LOW);
    prev  = analogRead(0);
}

#define HIT 0
#define IDLE 1
void loop() { // run over and over
    
    static int state, counter;
    int curr = analogRead(0);
    Serial.print(prev);
    Serial.print(",");
    Serial.println(curr);
    if(curr-prev>30){
        if(counter>3){
            state = HIT;
Serial.println("Change to hit");
            prev = curr;
            digitalWrite(13,HIGH);
        }
        else{
Serial.println("Count");
            counter++;
        }
        // digitalWrite(13,HIGH);
    }
    else if( prev-curr>30 ){
        if(counter>3){
            state = IDLE;
Serial.println("Change to idle");
            prev = curr;
            digitalWrite(13,LOW);
        }
        else{
Serial.println("Count");
            counter++;
        }
    }
    else if (counter){
        prev = curr;
        counter = 0;
    }
    else prev = curr;
    delay(50);

}
