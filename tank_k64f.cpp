#include "mbed.h"

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
#define PARITY_SUM(x) ((BIT0(x)>>0)+(BIT1(x)>>1)+(BIT2(x)>>2)+(BIT3(x)>>3)+(BIT4(x)>>4)+(BIT5(x)>>5)+(BIT6(x)>>6)+(BIT7(x)>>7))

Timer t, t_wifi;
Timeout t_dura;
char ret[15];
bool wifi_enable = 1;

Serial pc(USBTX, USBRX);
Serial HC06(D1, D0);
RawSerial ESP8266(D9, D7);

PwmOut wheel_left(D13), wheel_right(D12), cannon_base(D11), cannon_vert(D10);
DigitalIn encoder_left(D6), encoder_right(D5);
DigitalOut laser(D3), power(D2);

// Inline function to set pwm value with transformation
#define CENTER_LEFT     1500
#define CENTER_RIGHT    1500
#define CENTER_BASE     1500
#define CENTER_VERT     1500
inline void set_left( int value )        { wheel_left   = (CENTER_LEFT + value) / 20000.0f; }
inline void set_right( int value )       { wheel_right  = (CENTER_RIGHT - value) / 20000.0f; }
inline void set_ping_base( int value )   { cannon_base  = (CENTER_BASE + value) / 20000.0f; }
inline void set_cannon_vert( int value ) { cannon_vert  = (CENTER_VERT + value) / 20000.0f; }
int truncate( int a, int b, int radius ) {
    if (abs(a - b) < radius) return b;
    else return (a > b) ? (a - radius) : (a + radius);
}

// Cannon control, use a ticker to achieve continuous rotation
Ticker cannon_ticker;
int ramp_cannon_base = 0, ramp_cannon_vert = 0;
int value_cannon_vert = 0, value_cannon_base = 0;
void cannon_control() {
    value_cannon_vert = (ramp_cannon_vert < 0)
                        ? MAX(value_cannon_vert + ramp_cannon_vert, -500)
                        : MIN(value_cannon_vert + ramp_cannon_vert, 500);
    set_cannon_vert(value_cannon_vert);

    value_cannon_base = (ramp_cannon_base < 0)
                        ? MAX(value_cannon_base + ramp_cannon_base, -500)
                        : MIN(value_cannon_base + ramp_cannon_base, 500);
    set_ping_base(value_cannon_base);
}

// Servo control, use a ticker to achieve speed changing with finite acceleration
#define MAX_ACC 50
Ticker servo_control_ticker;
int servo_left = 0, servo_right = 0;
void servo_control() {
    static int last_servo_left;
    static int last_servo_right;
    last_servo_left = truncate(last_servo_left, servo_left, MAX_ACC);
    last_servo_right = truncate(last_servo_right, servo_right, MAX_ACC);
    set_left(last_servo_left);
    set_right(last_servo_right);
}

// Encoder control, use a ticker to count encoder value
Ticker encoder_ticker;
int encoder_counter_left = 0, encoder_counter_right = 0;
void encoder_control() {
    static int last_encoder_left = 0;
    static int last_encoder_right = 0;
    int curr_encoder_left = encoder_left;
    int curr_encoder_right = encoder_right;
    if (!last_encoder_left && curr_encoder_left) encoder_counter_left++;
    last_encoder_left = curr_encoder_left;
    if (!last_encoder_right && curr_encoder_right) encoder_counter_right++;
    last_encoder_right = curr_encoder_right;
}

// Router information
#define SSID "EECS822"
#define SSID_PASS "eecslab822"
// TCP server information
#define IP "192.168.0.197"
#define PORT 5000
void esp8266_sendCmd( char *str, int time_delay );
void esp8266_sendData( char *str );
void esp8266_recvCmd_blocking();

void disable_all(){
    power = 0;
    laser = 0;
    wifi_enable = 1;
}

#define WIFI_PING_PERIOD 1
int main()
{
    int i = 0, pin_base = 0;
    char cmd, esp_cmd[100];
    power = 0;
    laser = 0;

    // Initialize HC06, ESP8266, pc
    pc.baud(57600);
    HC06.baud(9600);
    ESP8266.baud(9600);
    wait(2);
    pc.printf("Start\n");

    // ESP8266 connecting wifi
    esp8266_sendCmd("AT\r\n",.5);
    sprintf( esp_cmd, "AT+CIPMUX=0\r\n");
    esp8266_sendCmd( esp_cmd, 1);
    sprintf( esp_cmd, "ATE0\r\n");
    esp8266_sendCmd( esp_cmd, 1);
    sprintf( esp_cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, SSID_PASS );
    esp8266_sendCmd( esp_cmd, 5);
    sprintf( esp_cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", IP, PORT );
    esp8266_sendCmd( esp_cmd, 5);
    esp8266_sendData("2");

    // Define ISR for servo, encoder, cannon
    servo_control_ticker.attach(&servo_control, .3);
    encoder_ticker.attach(&encoder_control, .01);
    cannon_ticker.attach(&cannon_control, .1);

    // Define PWM period
    cannon_vert.period(.02);
    cannon_base.period(.02);
    wheel_left.period(.02);
    wheel_right.period(.02);

    set_ping_base(-450);    wait(.5);
    set_ping_base(0);       wait(.5);
    set_ping_base(450);     wait(.5);
    set_cannon_vert(-450);  wait(.5);
    set_cannon_vert(0);     wait(.5);
    set_cannon_vert(450);   wait(.5);

    t_wifi.start();
    while (1) {
        if (HC06.readable()) {

            // Protocol
            // Bit[7:6]: (2:Forward, 1:Backward, 0:Stop)
            // Bit[4]: Mode select (1:Moving mode, 0:Cannon mode)
            // Bit[3]
            //  In moving mode: Rotate direction (1:left, 0:right)
            // `In cannon mode: Key pressed (1:Horizontal, 0:Vertical)
            // Bit[2:1]
            //  In moving mode: Rotating level (3:Most straight)
            //  In cannon mode: Bit2 representing (0:Horizontal, 1:Vertical)
            //                  Bit1 (0:ccw/up, 1:cw/down)
            cmd = HC06.getc();
            if(PARITY_SUM(cmd)%2){
                HC06.putc(0);
                pc.printf("parity error at %x(%d)\r\n", cmd, PARITY_SUM(cmd));
                continue;
            }
            pc.printf("(%x)(moving%d)", cmd, cmd & 0x10);

            // Launch state
            if ( BIT5(cmd) ) {
                laser = 1;
                wait(3);
                laser = 0;
            }
            // Moving state
            else if ( BIT4(cmd) ) {

                int rot_level, rot_dir;
                rot_dir = (BIT3(cmd)) ? 1 : 0;
                rot_level = (rot_dir) ? (8 - (LAST_3BIT(cmd >> 1))) : ((LAST_3BIT(cmd >> 1)) + 1);

                switch ( LAST_2BIT(cmd >> 6) ) {
                case 2:
                    servo_left  = (rot_dir) ? 100 : 6 * rot_level * rot_level;
                    servo_right = (rot_dir) ? 6 * rot_level * rot_level : 100;
                    encoder_counter_left = 0; encoder_counter_right = 0;
                    HC06.putc(1);
                    break;
                case 1: servo_left = -200; servo_right = -200;
                    encoder_counter_left = 0; encoder_counter_right = 0; 
                    HC06.putc(1);
                    break;
                case 0: servo_left = 0; servo_right = 0;
                        pc.printf("encoder = %d/%d\n", encoder_counter_left, encoder_counter_right);
                    HC06.putc(1);
                        break;
                }
            }

            // Cannon base state
            else if ( BIT3(cmd) ) {
                switch ( LAST_2BIT(cmd >> 6) ) {
                case 2: ramp_cannon_base = 30;  break;
                case 1: ramp_cannon_base = -30; break;
                case 0: ramp_cannon_base = 0;   break;
                }
                    HC06.putc(1);
            }

            // Cannon vertical state
            else {
                switch ( LAST_2BIT(cmd >> 6) ) {
                case 2: ramp_cannon_vert = -30;  break;
                case 1: ramp_cannon_vert = 30; break;
                case 0: ramp_cannon_vert = 0;   break;
                }
                    HC06.putc(1);
            }
        }

        // Ping TCP server to get state of the tank
        else if( wifi_enable && t_wifi.read() > WIFI_PING_PERIOD ){
            
            // Send data to TCP server
            esp8266_recvCmd_blocking();
            // esp8266_sendData("3");

            // Parse data from received string
            for(i=0; ret[i]; i++){
                if(ret[i]=='P'){
                    i+=5; break;
                }
            }
            pc.printf("\r\nGet %c(%d) back\r\n", ret[i], ret[i]);


            // State controller
            switch(ret[i]){
                case '1':
                    t_dura.attach(&disable_all, 10);
                    pc.printf("Open power");
                    power = 1;
                    wifi_enable = 0;
                    break;
                case '2':
                    pc.printf("close power");
                    break;
                case '4':
                    // t_dura.attach(&disable_all, 2);
                    pc.printf("Open cannon");
                    laser = 1;
                    // wifi_enable = 0;
                    break;
                case '0':
                    disable_all();
                    break;

            }
            t_wifi.reset();
            sprintf( esp_cmd, "%c", ret[i]);
            esp8266_sendData( esp_cmd );
        }

        wait(.05);
    }
}

void esp8266_sendCmd( char *str, int time_delay ){

    int i=0, j=0;
    pc.printf("\r\nSend: %s\r\n", str);
    ESP8266.printf("%s",str);
    pc.printf("Return: ");
    
    // Get string from ESP8266
    if(time_delay){
        for( t.start(); t.read()<time_delay; ){
            if(ESP8266.readable()){
                char buf = ESP8266.getc();
                ret[i++] = buf;
                // if(buf!='\r'&&buf!='\n') pc.printf("%c(%d)", buf, buf);
                // else pc.printf("_(%d)", buf);
            }
            wait(.001);
        }
        t.stop();
        t.reset();
        ret[i] = 0;
    }

    // Print out the received string
    pc.printf("\r\nreturn(%d) = ", i);
    for(j=0; j<i; j++){
        if(ret[j]!='\r'&&ret[j]!='\n') pc.printf("%c",ret[j]);
        else pc.printf("_");
    }
    pc.printf("|\r\n");
}

#define WIFI_CIPS_INTV 3
#define WIFI_CIPS_DATA 3
void esp8266_sendData( char *str ){
    int i=0, j=0;
    char buf, cmd[30];

    // Send size about data
    sprintf(cmd, "AT+CIPSEND=%d\r\n", strlen(str));
    esp8266_sendCmd( cmd, WIFI_CIPS_INTV );

    // Send body about data
    pc.printf("Send char %c\r\n", str[0]);
    ESP8266.putc(str[0]);
    pc.printf("Return: ");

    // Get string from ESP8266
    for( t.start(); t.read()<WIFI_CIPS_DATA; ){
        if(ESP8266.readable()){
            buf = ESP8266.getc();
            ret[i++] = buf;
        }
        wait(.001);
    }
    ret[i] = 0;
    t.stop();
    t.reset();

    // Print out the received string
    pc.printf("\r\nreturn(%d) = ", i);
    for(j=0; j<i; j++){
        if(ret[j]!='\r'&&ret[j]!='\n') pc.printf("%c",ret[j]);
        else pc.printf("_");
    }
}

void esp8266_recvCmd_blocking(){
    int i;
    char buf;
    while(!ESP8266.readable());
    for( t.start(); t.read()<(double).2; ){
        if(ESP8266.readable()){
            buf = ESP8266.getc();
            ret[i++] = buf;
        }
        wait(.001);
    }
    ret[i] = 0;
    t.stop();
    t.reset();
    printf("Blocking receive (%d): %s\r\n", i, ret);
}
