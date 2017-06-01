#include <SoftwareSerial.h>
#include "Arduino.h"

// Router information
#define SSID "EECS822"
#define SSID_PASS "eecslab822"

// TCP server information
#define IP "192.168.0.206"
#define PORT 5000

#define LED1 7
#define LED2 4
// Pin out information
SoftwareSerial ESP8266(12,11); // RX, TX

// Function declaration
void esp8266_sendCmd( char *str, int time_delay );
void esp8266_sendData( char *str );
void esp8266_getData();

char ret[100];
int prev, led2 = 0;
void setup() {

    char cmd[50];

    // Open serial communications and wait for port to open:
    Serial.begin(115200);
    ESP8266.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    esp8266_sendCmd("ATE0\r\n",500);
    esp8266_sendCmd("AT\r\n",500);

    sprintf( cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, SSID_PASS );
    esp8266_sendCmd( cmd, 10000);

    sprintf( cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", IP, PORT );
    esp8266_sendCmd( cmd, 10000);

    esp8266_sendData("3\n");

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    digitalWrite(LED1,HIGH);
    digitalWrite(LED2,HIGH);
    prev  = analogRead(0);
}

#define HIT 0
#define IDLE 1
bool wifi_enable = 1, being_hit = 0;;
void loop() { // run over and over
    
    static int state, counter;
    static int sensing_counter = 0;
    char cmd[50];
    int i, curr = analogRead(0);

    // Blocking receive, waiting starting signal from Wifi.
    if(wifi_enable){
        esp8266_getData();
        // esp8266_sendData("p\n");

        // Parse data from received string
        for(i=0; ret[i]; i++){
            if(ret[i]=='P'){
                i+=5; break;
            }
        }
        Serial.print("Get (");
        Serial.print(i);
        Serial.println(ret[i]);
        // if(ret[i]=='0') 
        if(ret[i]!='3') return;
        else{
            wifi_enable = 0;
            being_hit = 0;
            sensing_counter = 0;
            prev  = analogRead(0);
            digitalWrite(LED1,HIGH);
        }
        // Here should latch one data
    }

    // There is data input from Wifi. Send the result and switch to waiting mode.
    else if(ESP8266.available()){
        sprintf( cmd, "%d", being_hit);
        esp8266_sendData(cmd);

        // Parse data from received string
        for(i=0; ret[i]; i++){
            if(ret[i]=='P'){
                i+=5; break;
            }
        }
        Serial.print("Get (");
        Serial.print(i);
        Serial.println(ret[i]);
        // if(ret[i]=='0') 
        wifi_enable = 1;
        return;
    }

    Serial.print(prev);
    Serial.print(",");
    Serial.println(curr);
    // Difference over threshold, enable counter.
    if(curr-prev>30){
        if(counter>3){
            state = HIT;
Serial.println("Change to hit");
            prev = curr;
            being_hit = 1;
            // esp8266_sendData("I'm hit");
            // wifi_enable = 1;
            digitalWrite(LED1,LOW);
        }
        else{
Serial.println("Count");
            counter++;
        }
        // digitalWrite(LED1,LOW);
    }
    else if( prev-curr>30 ){
        if(counter>3){
            state = IDLE;
Serial.println("Change to idle");
            prev = curr;
            digitalWrite(LED1,HIGH);
        }
        else{
Serial.println("Count");
            counter++;
        }
    }
    // Difference under threshold, reset the counter.
    else if (counter){
        prev = curr;
        counter = 0;
    }
    else prev = curr;

    // Toggle the sensing counter
    sensing_counter++;
    if(sensing_counter%5==0){
        led2 = !led2;
        digitalWrite(LED2,led2);
    }

    delay(50);
}

void esp8266_sendCmd( char *str, int time_delay ){
    
    int i;
    unsigned long start;
    Serial.print("\nSend: ");
    Serial.println(str);
    ESP8266.print(str);
    Serial.print("Return: ");
    // delay(time_delay);
    for( i=0, start = millis(); millis()<start+time_delay; ){
        if( ESP8266.available() ){
            char tmp = ESP8266.read();
            Serial.print(tmp);
            ret[i++] = tmp;
        }
    }
    ret[i] = 0;
    Serial.println();
}

void esp8266_sendData( char *str ){
    char cmd[30];
    sprintf(cmd, "AT+CIPSEND=%d\r\n", strlen(str));
    esp8266_sendCmd(cmd,2000);
    esp8266_sendCmd(str,2000);
}

void esp8266_getData(){
    int i;
    unsigned long start;
    while( ESP8266.available()==0 );
    Serial.print("Return: ");
    for( i=0, start = millis(); millis()<start+100; ){
        if( ESP8266.available() ){
            char tmp = ESP8266.read();
            Serial.print(tmp);
            ret[i++] = tmp;
        }
    }
    Serial.println();
}
