#include <SoftwareSerial.h>
#include "Arduino.h"

#define BIT0(x) (x&(1<<0))
#define BIT1(x) (x&(1<<1))
#define BIT2(x) (x&(1<<2))
#define BIT3(x) (x&(1<<3))
#define BIT4(x) (x&(1<<4))
#define BIT5(x) (x&(1<<5))
#define BIT6(x) (x&(1<<6))
#define BIT7(x) (x&(1<<7))
#define LAST_2BIT(x) ((x)&0x03)
#define LAST_3BIT(x) ((x)&0x07)
#define ADD_PARITY(x) (x|(((BIT1(x)>>1)+(BIT2(x)>>2)+(BIT3(x)>>3)+(BIT4(x)>>4)+(BIT5(x)>>5)+(BIT6(x)>>6)+(BIT7(x)>>7))%2))

#define BUTTON_UP   5
#define BUTTON_DOWN 3
#define BUTTON_LEFT 4
#define BUTTON_RIGHT 2
#define HC05_TX     6
#define HC05_RX     7
#define LED1        8
#define LED2        9
#define LED3        10
#define BUTTON_B    11
#define BUTTON_A    12

class button{
    public:
    button( int input ){ pin = input; last_value = 0; value = 0; }
    operator int(){ return digitalRead(pin); }
    void update(){
        last_value = value;
        value = digitalRead(pin);
    }
    int posedge(){ return (last_value==0) && (value==1); }
    int negedge(){ return (last_value==1) && (value==0); }

private:
    int pin;
    int value, last_value;
};

button btn_up(BUTTON_UP), btn_down(BUTTON_DOWN), btn_left(BUTTON_LEFT), btn_right(BUTTON_RIGHT), btn_A(BUTTON_A), btn_B(BUTTON_B);

SoftwareSerial hc05(HC05_RX,HC05_TX); // RX, TX
int moving = 1;
int cannon = 0;
void setup() {
    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    pinMode(BUTTON_UP,      INPUT);
    pinMode(BUTTON_DOWN,    INPUT);
    pinMode(BUTTON_LEFT,    INPUT);
    pinMode(BUTTON_RIGHT,   INPUT);
    pinMode(BUTTON_A,       INPUT);
    pinMode(BUTTON_B,       INPUT);

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(13,OUTPUT);

    Serial.println("Goodnight moon!");

    // set the data rate for the SoftwareSerial port
    hc05.begin(9600);
    // hc05.println("Hello, world?");
}

// Bit[7:6]: (2:Forward, 1:Backward, 0:Stop)
// Bit[4]: Mode select (1:Moving mode, 0:Cannon mode)
// Bit[3]
//  In moving mode: Rotate direction (1:left, 0:right)
// `In cannon mode: Key pressed (1:pressed, 0:released)
// Bit[2:1]
//  In moving mode: Rotating level (3:Most straight)
//  In cannon mode: Bit2 representing (0:Horizontal, 1:Vertical)
//                  Bit1 (0:ccw/up, 1:cw/down)
int level = 0, pinNum;
void loop() { // run over and over
    unsigned char cmd = 0;
    // static int last_pin2 = 0, last_pin3 = 0, last_pin_right = 0, last_pin_left = 0;
    static unsigned long pin4_time=0, pinb_time=0, pin_left_time=0, pin_right_time=0;

    // int level = analogRead(0) / 128;
    pinNum = abs(level);
    if(level>0) cmd = (char)(level+4) << 1;
    else cmd = (char)(level+3) << 1;
    // cmd = (char)level << 1;
    // cmd = (char)0x20;

    #ifdef HANDSHAKE
    if (hc05.available()) {
        Serial.print("Receive:");
        Serial.print(hc05.read(), HEX);
    }
    else
    #endif

    if (Serial.available()) {
        char c = Serial.read();
        // hc05.write(ADD_PARITY(c));
        Serial.print("Receive command: "); Serial.println(c, BIN);
        Serial.println(analogRead(0));
    }

    switch(pinNum){
        case 0:
            digitalWrite(LED1,HIGH);
            digitalWrite(LED2,HIGH);
            digitalWrite(LED3,HIGH);
            break;
        case 1:
            digitalWrite(LED1,LOW);
            digitalWrite(LED2,HIGH);
            digitalWrite(LED3,HIGH);
            break;
        case 2:
            digitalWrite(LED1,LOW);
            digitalWrite(LED2,LOW);
            digitalWrite(LED3,HIGH);
            break;
        case 3:
            digitalWrite(LED1,LOW);
            digitalWrite(LED2,LOW);
            digitalWrite(LED3,LOW);
            break;
    }
// Serial.print(digitalRead(2));    
// Serial.println(digitalRead(3));
    btn_up.update();
    if( btn_up.posedge() ){
            cmd |= 0x80;
            if(moving) cmd |= 0x10;
            else{
                if(cannon==1) cmd |=0x08;
                else cmd &= 0xF7;
            }
            hc05.write(ADD_PARITY(cmd));
            Serial.print("cmd = "); Serial.println(cmd, BIN);
            delay(100);
            return;
    }
    else if( btn_up.negedge() ){
        cmd &= 0x3F;
        if(moving) cmd |= 0x10;
        else{
            if(cannon==1) cmd |=0x08;
            else cmd &= 0xF7;
        }
        hc05.write(ADD_PARITY(cmd));
        Serial.print("cmd = "); Serial.println(cmd, BIN);
        delay(100);
        return;
    }

    btn_down.update();
    if( btn_down.posedge() ){
            cmd |= 0x40;
            if(moving) cmd |= 0x10;
            else{
                if(cannon==1) cmd |=0x08;
                else cmd &= 0xF7;
            }
            hc05.write(ADD_PARITY(cmd));
            Serial.print("cmd = "); Serial.println(cmd, BIN);
            delay(100);
            return;
    }
    else if( btn_down.negedge() ){
        cmd &= 0x3F;
        if(moving) cmd |= 0x10;
        else{
            if(cannon==1) cmd |=0x08;
            else cmd &= 0xF7;
        }
        hc05.write(ADD_PARITY(cmd));
        Serial.print("cmd = "); Serial.println(cmd, BIN);
        delay(100);
        return;
    }


    if (btn_A == 1) {
        if(millis()>pin4_time){
            pin4_time = millis()+500;
            moving = !moving;
            digitalWrite(13,moving);
Serial.print("switch moving");
        }
    }

    if (btn_B == 1) {
        if(millis()>pinb_time){
            pinb_time = millis()+500;
            cmd |= 0x20;
            hc05.write(ADD_PARITY(cmd));
            return;
        }
    }

    btn_left.update();
    if(btn_left.posedge()){
        if(moving){
            pin_left_time = millis()+500;
            if(level!=-3) level--;
        }
        else{
            cmd |= 0x80;
            cmd |= 0x08;
            hc05.write(ADD_PARITY(cmd));
            Serial.print("cmd = "); Serial.println(cmd, BIN);
            delay(100);
            return;
        }
    }
    else if( btn_left.negedge() ){
        cmd &= 0x3F;
        cmd |= 0x08;
        hc05.write(ADD_PARITY(cmd));
        Serial.print("cmd = "); Serial.println(cmd, BIN);
        delay(100);
        return;
    }

    btn_right.update();
    if( btn_right.posedge() ){
        if(moving){
                    if(level!=3) level++;
        Serial.print("level = "); Serial.println(level);
        }
        else{
            cmd |= 0x40;
            cmd |= 0x08;
            hc05.write(ADD_PARITY(cmd));
            Serial.print("cmd = "); Serial.println(cmd, BIN);
            delay(100);
            return;
        }
    }
    else if( btn_right.negedge() ){
        cmd &= 0x3F;
        cmd |= 0x08;
        hc05.write(ADD_PARITY(cmd));
        Serial.print("cmd = "); Serial.println(cmd, BIN);
        delay(100);
        return;
    }
}
