#include "LCD.h"
#include "I2C_IMU.h"
#include <math.h>
#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro

// DEVCFG0
#pragma config DEBUG = OFF // no debugging
#pragma config JTAGEN = OFF // no jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // no write protect
#pragma config BWP = OFF // no boot write protect
#pragma config CP = OFF // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF  // turn off secondary oscillator
#pragma config IESO = OFF // no switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // free up secondary osc pins
#pragma config FPBDIV = DIV_1 // divide CPU freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // do not enable clock switch
#pragma config WDTPS = PS1              /////// slowest wdt?? PS1048576 
#pragma config WINDIS = OFF // no wdt window
#pragma config FWDTEN = OFF // wdt off by default
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the CPU clock to 48MHz
#pragma config FPLLIDIV = DIV_2 // divide input clock (8MHz) to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV = DIV_2 // divider for the 8MHz input clock, then multiply by 12 to get 48MHz for USB
#pragma config UPLLEN = ON // USB clock on

// DEVCFG3
#pragma config USERID = 0x0101      // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations
#pragma config FUSBIDIO = ON // USB pins controlled by USB module
#pragma config FVBUSONIO = ON // USB BUSON controlled by USB module

// Constants
#define NUM_SAMPS 1000
#define PI 3.14159
#define IODIR 0x00
#define IOCON 0x05
#define GPIO  0x09
#define OLAT 0x0A
#define GYRO 0b1101011
#define OUT_TEMP_L 0x20

int main() {
	 __builtin_disable_interrupts();

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);
    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;
    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;
    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0; 
    // do your TRIS and LAT commands here 
    TRISAbits.TRISA4 = 0;        // Pin A4 is the GREEN LED, 0 for output
    LATAbits.LATA4 = 1;          // make GREEN LED pin "high" (1) = "on"
    TRISBbits.TRISB4 = 1;        // B4 (reset button) is an input pin
    TRISBbits.TRISB2 = 0;        // B2 and B3 set to output for LCD 
    TRISBbits.TRISB3 = 0;
    TRISBbits.TRISB15 = 0;       // B15 set to output 
    // initialize peripherals and chips
   SPI1_init();
   initI2C();
   LCD_init();
   init_IMU();
   
    __builtin_enable_interrupts();
    
     _CP0_SET_COUNT(0);
     LCD_clearScreen(WHITE);
    
    char textbuffer[20];
    
    //// PWM init. //////
    RPA0Rbits.RPA0R = 0b0101; // OC1
    RPB8Rbits.RPB8R = 0b0101; // OC2
    OC1CONbits.OCTSEL = 0; // select timer 2 for both
    OC2CONbits.OCTSEL = 0;
    OC1CONbits.OCM = 0b110; // set to PWM mode
    OC2CONbits.OCM = 0b110;
    T2CONbits.TCKPS = 6; //timer 2 prescaler = 1:4, N=4
    PR2 = 1999; //period 2 = (PR2+1) * N * 12.5 ns = 100 us, 10 kHz
    TMR2 = 0;
    OC1RS = 1000;
    OC1R = 1000;
    OC2RS = 1000;
    OC2R = 1000;
    T2CONbits.ON = 1;
    OC1CONbits.ON = 1;
    OC2CONbits.ON = 1;

//    sprintf(textbuffer,"Hello World");
//    LCD_type(28,45,textbuffer, BLACK);
    
    ///////////WHOAMI TEST//////////// 
//    unsigned char read = WHO_AM_I();
//    if(read == 0b01101001){
//        sprintf(textbuffer,"TRUE");
//    }
//    else{
//        sprintf(textbuffer,"False");
//    }  
//    LCD_type(28,32,textbuffer, BLACK);
    /////////////////////////////////////
    
    int bytes = 14;
    unsigned char IMU_data[bytes];
    short temp_L = 0;
    short temp_H = 0;
    short temp = 0; 
    short accelX_L = 0;
    short accelX_H = 0;
    short accelX = 0;
    short accelY_L = 0;
    short accelY_H = 0;
    short accelY = 0;
    short accelZ_L = 0;
    short accelZ_H = 0;
    short accelZ = 0;
    short gyroX_L = 0;
    short gyroX_H = 0;
    short gyroX = 0;
    short gyroY_L = 0;
    short gyroY_H = 0;
    short gyroY = 0;  
    short gyroZ_L = 0;
    short gyroZ_H = 0;
    short gyroZ = 0;
    char x = 10;
    char y = 10;
    float ax = 0;
    float ay = 0;
    float az = 0;
    
  
    while(1) {
       i2c_master_multiread(GYRO,OUT_TEMP_L,bytes,IMU_data);
       
       ///////////////PRINTING TO LCD//////////////
       unsigned short temp2;
       temp_L = IMU_data[0];
       temp_H = IMU_data[1];
       temp = (temp_H<<8)|temp_L;
       temp = (unsigned short)temp;
       temp2 = temp - 65400;
       sprintf(textbuffer,"Temp: %u     ",temp2);
       LCD_type(x,y,textbuffer,BLACK);
       y = y+20;
       
       gyroX_L = IMU_data[2];
       gyroX_H = IMU_data[3];
       gyroX = (signed short)((gyroX_H<<8)|gyroX_L);
       float gx;
       gx = (float)(gyroX/320.00);
       sprintf(textbuffer,"Gyro X: %4.3f",gx);
       LCD_type(x,y,textbuffer,BLACK);
       sprintf(textbuffer,"dps");
       LCD_type(x+80,y,textbuffer,BLACK);
       y = y+10;
       
       gyroY_L = IMU_data[4];
       gyroY_H = IMU_data[5];
       gyroY = (signed short)((gyroY_H<<8)|gyroY_L);
       float gy;
       gy = (float)(gyroY/320.00);
       sprintf(textbuffer,"Gyro Y: %4.3f",gy);
       LCD_type(x,y,textbuffer,BLACK);
       sprintf(textbuffer,"dps");
       LCD_type(x+80,y,textbuffer,BLACK);
       y = y+10;
       
       gyroZ_L = IMU_data[6];
       gyroZ_H = IMU_data[7];
       gyroZ = (signed short)((gyroZ_H<<8)|gyroZ_L);
       float gz;
       gz = (float)(gyroZ/320.00);
       sprintf(textbuffer,"Gyro Z: %4.3f",gz);
       LCD_type(x,y,textbuffer,BLACK);
       sprintf(textbuffer,"dps");
       LCD_type(x+80,y,textbuffer,BLACK);
       y = y+20;
       
       accelX_L = IMU_data[8];
       accelX_H = IMU_data[9];
       accelX = (signed short)((accelX_H<<8)|accelX_L);
       ax = (float)(accelX/32000.00)*9.8;
       sprintf(textbuffer,"Accel X: %4.3f",ax);
       LCD_type(x,y,textbuffer,BLACK);
       sprintf(textbuffer,"g");
       LCD_type(x+80,y,textbuffer,BLACK);
       y = y+10; 
       
       accelY_L = IMU_data[10];
       accelY_H = IMU_data[11];
       accelY = (signed short)((accelY_H<<8)|accelY_L);
       ay = (float)(accelY/32000.00)*9.8;
       //range is 0 - 3200, divide by this # then multiply by 9.8
       sprintf(textbuffer,"Accel Y: %4.3f",ay);
       LCD_type(x,y,textbuffer,BLACK);
       sprintf(textbuffer,"g");
       LCD_type(x+80,y,textbuffer,BLACK);
       y = y+10;

       accelZ_L = IMU_data[12];
       accelZ_H = IMU_data[13];
       accelZ = (signed short)((accelZ_H<<8)|accelZ_L);
       //accelZ = (signed short)accelZ;
       az = (float)accelZ +0.00;
       float az2; 
       az2= (az/32000.00)*9.8;
       sprintf(textbuffer,"Accel Z: %4.3f",az2);
       LCD_type(x,y,textbuffer,BLACK);
       sprintf(textbuffer,"g");
       LCD_type(x+80,y,textbuffer,BLACK);
       y = 10;
       
       ///////////////PWM//////////////////
       if (ax>=0){
       OC1RS = floor((ax*1000))+1000; //duty cycle = OC1RS/2000, range is 0,2g
       }
       else if (ax<0){
       OC1RS = floor((ax*-1000)); //duty cycle = OC1RS/2000, range is -2,0g
       }
       if (ay>=0){
       OC2RS = floor((ay*1000))+1000; //duty cycle = OC1RS/2000, range is 0,2g
       }
       else if (ay<0){
       OC2RS = floor((ay*-1000)); //duty cycle = OC1RS/2000, range is -2,0g
       }

       if(OC1RS>2000){
           OC1RS = 2000;
            }
       else if(OC1RS<0){
           OC1RS = 0;
       }
       
       if(OC2RS>2000){
           OC2RS = 2000;
            }
       else if(OC2RS<0){
           OC2RS = 0;
       }
       
       delay(100000);                      // controls frequency
       
    }
     
}
