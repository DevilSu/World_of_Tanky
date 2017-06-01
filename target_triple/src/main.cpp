#include <SoftwareSerial.h>
#include "Arduino.h"

// Router information
#define SSID "EECS822"
#define SSID_PASS "eecslab822"

// TCP server information
#define IP "192.168.0.206"
#define PORT 5000

#define LED1 13
#define LED2 4
// Pin out information
SoftwareSerial ESP8266(10,11); // RX, TX

// Function declaration
void esp8266_sendCmd( char *str, int time_delay, char *needle = NULL );
void esp8266_sendData( char *str );
void esp8266_getData();

char ret[100];
int prev[3], led2 = 0;
int pin[3] = {0,2,3};
void setup() {

    char cmd[50];

    // Open serial communications and wait for port to open:
    Serial.begin(115200);
    ESP8266.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    esp8266_sendCmd("ATE0\r\n", 500, "OK");
    esp8266_sendCmd("AT\r\n", 500, "OK");

    sprintf( cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, SSID_PASS );
    esp8266_sendCmd( cmd, 10000, "OK" );

    sprintf( cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", IP, PORT );
    esp8266_sendCmd( cmd, 10000, "OK" );

    esp8266_sendData("3\n");

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    digitalWrite(LED1,HIGH);
    digitalWrite(LED2,HIGH);
    for( int i=0; i<3; i++ ){
        prev[i]  = analogRead(pin[i]);
    }
}

#define HIT 0
#define IDLE 1
bool wifi_enable = 1, being_hit[3] = {0,0,0};
void loop() { // run over and over
    
    static int state[3], counter[3];
    static int sensing_counter = 0;
    char cmd[50];
    int i, curr[3];
    for( i=0; i<3; i++ ){
        curr[i]  = analogRead(pin[i]);
    }

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
            for( i=0; i<3; i++ ){
                being_hit[i] = 0;
                prev[i]  = analogRead(pin[i]);
            }
            sensing_counter = 0;
            digitalWrite(LED1,HIGH);
        }
        // Here should latch one data
    }

    // There is data input from Wifi. Send the result and switch to waiting mode.
    else if(ESP8266.available()){
        int sum = 0;
        for( i=0; i<3; i++ ) sum+=being_hit[i];
        sprintf( cmd, "%d", sum);
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

    for( i=0; i<3; i++ ){
        Serial.print("(");
        Serial.print(prev[i]);
        Serial.print(",");
        Serial.print(curr[i]);
        Serial.print(")\t");
    }
    Serial.println();

    // Difference over threshold, enable counter.
    for( i=0; i<3; i++ ){
        if(curr[i]-prev[i]>30){
            if(counter[i]>3){
                state[i] = HIT;
    Serial.println("Change to hit");
                prev[i] = curr[i];
                being_hit[i] = 1;
                // esp8266_sendData("I'm hit");
                // wifi_enable = 1;
                digitalWrite(LED1,LOW);
            }
            else{
    Serial.println("Count");
                counter[i]++;
            }
            // digitalWrite(LED1,LOW);
        }
        else if( prev[i]-curr[i]>30 ){
            if(counter[i]>3){
                state[i] = IDLE;
    Serial.println("Change to idle");
                prev[i] = curr[i];
                digitalWrite(LED1,HIGH);
            }
            else{
    Serial.println("Count");
                counter[i]++;
            }
        }
        // Difference under threshold, reset the counter[i].
        else if (counter[i]){
            prev[i] = curr[i];
            counter[i] = 0;
        }
        else prev[i] = curr[i];
    }
    // Toggle the sensing counter
    sensing_counter++;
    if(sensing_counter%5==0){
        led2 = !led2;
        digitalWrite(LED2,led2);
    }

    delay(50);
}

void esp8266_sendCmd( char *str, int time_delay, char *needle = NULL ){
    
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
            ret[i] = 0;
            if( needle!=NULL && strstr(ret, needle)!=NULL ){
                while( ESP8266.available()!=0 );
                break;
            }
        }
    }
    ret[i] = 0;
    Serial.println();
}

void esp8266_sendData( char *str ){
    char cmd[30];
    sprintf(cmd, "AT+CIPSEND=%d\r\n", strlen(str));
    esp8266_sendCmd(cmd,2000, ">");
    esp8266_sendCmd(str,2000);
}

void esp8266_getData(){
    int i;
    unsigned long start;
    while( ESP8266.available()==0 );
    Serial.print("Return; ");
    for( i=0, start = millis(); millis()<start+100; ){
        if( ESP8266.available() ){
            char tmp = ESP8266.read();
            Serial.print(tmp);
            ret[i++] = tmp;
        }
    }
    Serial.println();
}
