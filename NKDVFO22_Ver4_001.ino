

/*
 *    Multi Band DDS VFO Ver4.001         
 *                         JA2NKD 2016.11.19     
 *     Aruduino IDE 1.6.5 & 1.6.12
 *     Arduino mega2560 only
 *     
 * <Rotary2560.h> https://sites.google.com/site/ja2gqp/home/Rotary2560.h?attredirects=0&d=1
 * <EF_AD9850.h>  http://elecfreaks.com/store/download/datasheet/breakout/DDS/EF_AD9850_library.zip
 * "Ucglib.h"     https://github.com/olikraus/ucglib
 * <Keypad.h>     http://playground.arduino.cc/Code/Keypad
 */

// Library include
#include <SPI.h>
#include <EEPROM.h>
#include "Rotary2560.h"
#include <EF_AD9850.h>
#include "Ucglib.h"
#include <Keypad.h>

// Encorder Pins
#define ENCODER_A 2
#define ENCODER_B 3

/*
 * Hardware SPI Pins:
 * Arduino mega2560 sclk(SCK)=52, data(MOSI)=51 
*/

// I/O setting
  const byte __CS=47;
  const byte __RST=48;
  const byte __DC=49;
  const byte ddsreset=40;
  const byte ddsdata=41;
  const byte ddsfqup=42;
  const byte ddsclock=43;
  const byte modeout1=38;
  const byte modeout2=39;
  const byte txsw=44;
  const byte txout=45; 
  const byte meterin=A15;
  const int encorderPin_A=8;
  const int encorderPin_B=9;

// LCD SET
  EF_AD9850 AD9850(ddsclock,ddsfqup,ddsreset,ddsdata);
  Ucglib_ILI9341_18x240x320_HWSPI ucg(__DC, __CS, __RST);
  
// Keypad set
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','S'},
  {'4','5','6','M'},
  {'7','8','9','E'},
  {'C','0','N','R'}
};
byte rowPins[ROWS] = {22,23,24,25}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {26,27,28,29}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

// Variable setting
  unsigned long romf[4];
  unsigned long freq =7100000;
  unsigned long freqmax=7200000;
  unsigned long freqmin=7000000;
  unsigned long freqold=0;
  long freqrit=0;
  String freqt=String(freq);
  unsigned long ifshift = 0;
  unsigned long ifshiftLSB =8998500; // VFO>frequency USB/LSB is turn over
  unsigned long ifshiftUSB =9001500;
  unsigned long ifshiftCW  =8999200;
  unsigned long ifshiftAM  =9000000;
  unsigned long txshiftLSB =8998500;
  unsigned long txshiftUSB =9001500;
  unsigned long txshiftCW  =9000000;
  unsigned long txshiftAM  =9000000;
  unsigned long ddsfreq = 0;
  char f100m,f10m,fmega,f100k,f10k,f1k,f100,f10,f1;
  int ddsstep=2;
  int rit=0;
  int fstep=100;
  int steprom=1;
  int fmode=1;
  int fmodeold=1;
  int flagrit=0;
  int fritold=0;
  int flagmode=0;
  int meterval1=0;
  int tmeterval=0;
  int romadd=0x100;
  int analogdata=0;
  int band=0;
  int bandmax=10;
//  int counter=0;  
//  int counte=0;
  int fin=0;
  unsigned long freqin=0;
  int finflg=0;
  int txflg=0;
    
void setup() {
  delay(100);
  ucg.begin(UCG_FONT_MODE_TRANSPARENT);
 //ucg.begin(UCG_FONT_MODE_SOLID);
  ucg.clearScreen();
  ucg.setRotate270();
  pinMode(txsw,INPUT_PULLUP);
  pinMode(modeout1,OUTPUT);
  pinMode(modeout2,OUTPUT);
  pinMode(txout,OUTPUT);
  DDRC = B11111111;
  PORTC = 0;
  
//  encoder_begin(encorderPin_A,encorderPin_B);
  Fnc_Rotary_Init(ENCODER_A,ENCODER_B);
  digitalWrite(txout,LOW);
  AD9850.init();
  AD9850.reset();

  band=EEPROM.read(0x001);
  romadd=0x010+(band*0x10);
  for (int i=0; i<3;i++){
  romf[i]=eepread((romadd+4*i));
  }
  freq = romf[0];
  freqmin = romf[1];
  freqmax = romf[2];
  fmode = EEPROM.read(romadd+12);
  steprom = EEPROM.read(romadd+13);
  if (steprom==1){fstep=10;}
  if (steprom==2){fstep=100;}
  if (steprom==3){fstep=1000;}
  if (steprom==4){fstep=10000;}
  banddataout();
  screen01();
  chlcd();
  if (fmode==0){fmode=3;}
  else{fmode=fmode-1;}
  modeset();
  steplcd();
  freqt=String(freq);
  freqlcd();

}
//Main Routine

void loop() {

  char customKey = customKeypad.getKey();
  if (finflg==0){
  if ( customKey =='S'){setstep();}
  if ( customKey =='M'){modeset();}
  if ( customKey =='E'){bandwrite();}
  if ( customKey =='R'){setrit();}
  if ( customKey =='N'){freqinputsub();} 
  if ( customKey =='1'){bandwrite(); band = 0; bandcall();}
  if ( customKey =='2'){bandwrite(); band = 1; bandcall();}
  if ( customKey =='3'){bandwrite(); band = 2; bandcall();}
  if ( customKey =='4'){bandwrite(); band = 3; bandcall();}
  if ( customKey =='5'){bandwrite(); band = 4; bandcall();}
  if ( customKey =='6'){bandwrite(); band = 5; bandcall();}
  if ( customKey =='7'){bandwrite(); band = 6; bandcall();}
  if ( customKey =='8'){bandwrite(); band = 7; bandcall();}
  if ( customKey =='9'){bandwrite(); band = 8; bandcall();}
  if ( customKey =='0'){bandwrite(); band = 9; bandcall();}  
  }
  else{
    if ( customKey =='1'){fin=1;freqinput();}
    if ( customKey =='2'){fin=2;freqinput();}
    if ( customKey =='3'){fin=3;freqinput();}
    if ( customKey =='4'){fin=4;freqinput();}
    if ( customKey =='5'){fin=5;freqinput();}
    if ( customKey =='6'){fin=6;freqinput();}
    if ( customKey =='7'){fin=7;freqinput();}
    if ( customKey =='8'){fin=8;freqinput();}
    if ( customKey =='9'){fin=9;freqinput();}
    if ( customKey =='0'){fin=0;freqinput();}
    if ( customKey =='N'){freqinputend();}
    if ( customKey =='C'){fin=10;freqinput();}   
  }
  if (digitalRead(txsw)==LOW){txset();}
  if (flagrit==1){
    if (freqrit == fritold){
      meter();
    }    
    if (freqrit!=fritold){
      ddswrite();
      ritlcd();
      fritold=freqrit;  
    }
  }
  else{
    if (freq == freqold){
        meter();
    }
  ddswrite();
  freqt=String(freq); 
  freqlcd();
  freqold=freq;
  }
}

// meter

void meter(){
 meterval1=analogRead(meterin);
 meterval1=meterval1/50;
 if (meterval1>15){meterval1=15;}
  int sx1=sx1+(meterval1*17);
  sx1=sx1+41;
  int sx2=0;
  sx2=sx2+(40+((15-meterval1)*17));
  ucg.setFont(ucg_font_fub35_tr);
  ucg.setColor(0,0,0);
  ucg.drawBox(sx1,180,sx2,16);
  ucg.setPrintPos(40,200);
  for(int i=1;i<=meterval1;i++){
    if (i<=9){
      ucg.setColor(0,255,255);
      ucg.print("-");  
    }
    else{
      ucg.setColor(255,0,0);
      ucg.print("-");
    }
  }
}

// Encoder Interrupt
void Fnc_Rotary(){
  pinstate= (digitalRead(pin2) << 1) | digitalRead(pin1);
  state = ttable[state & 0xf][pinstate];
  sts = state &0x30;
   if (flagrit ==1 ){
    if(sts){
      if(sts == DIR_CW){
        freqrit=freqrit+fstep;
        if (freqrit>=10000){
         freqrit=10000;
         }
    }
    else{
        freqrit=freqrit-fstep;
        if (freqrit<=-10000){
          freqrit=-10000;
          }
         }
       }
  }
 else{
  if(sts){
    if(sts == DIR_CW){
     freq=freq+fstep;
     if (freq>=freqmax){freq=freqmax;}
     }
     else{
      freq=freq-fstep;
      if (freq<=freqmin){freq=freqmin;}
    }
   }
  }
}
// On Air
void txset(){
  noInterrupts();
  if (txflg==0){
    digitalWrite(txout,HIGH);
    if (fmode==0){ddsfreq=freq+txshiftLSB;}
    if (fmode==1){ddsfreq=freq+txshiftUSB;}
    if (fmode==2){ddsfreq=freq+txshiftCW;}
    if (fmode==3){ddsfreq=freq+txshiftAM;}
    AD9850.wr_serial(0x00,ddsfreq);
    ucg.setPrintPos(110,140);
    ucg.setFont(ucg_font_fub17_tr);
    ucg.setColor(255,0,0);
    ucg.print("ON AIR");
    while(digitalRead(txsw) == LOW){meter();}
  }
  else{
    ucg.setPrintPos(110,140);
    ucg.setFont(ucg_font_fub17_tr);
    ucg.setColor(255,0,0);
    ucg.print("Don't Transmit");
    delay(1000);
  }
    digitalWrite(txout,LOW);
    ucg.setFont(ucg_font_fub17_tr);
    ucg.setColor(0,0,0);
    ucg.drawBox(100,120,250,30);  //45
    ddswrite();  
    interrupts();
  }

// Mode change(LSB-USB-CW-AM)
void modeset(){
  ucg.setFont(ucg_font_fub17_tr);
    ucg.setPrintPos(82,82);
    ucg.setColor(0,0,0);
    ucg.print("USB");
    ucg.setPrintPos(12,82);
    ucg.print("LSB"); 
    ucg.setPrintPos(82,112);
    ucg.print("A M");
    ucg.setPrintPos(12,112);
    ucg.print("C W");    
  if (fmode==0){
    ifshift=ifshiftUSB;
    ucg.setColor(255,255,0);
    ucg.setPrintPos(82,82);
    ucg.print("USB");
    digitalWrite(modeout1,HIGH);
    digitalWrite(modeout2,LOW);    
  }
  if(fmode==1){
    ifshift=ifshiftCW;
    ucg.setPrintPos(12,112);
    ucg.setColor(255,255,0);
    ucg.print("C W");
    digitalWrite(modeout1,LOW);
    digitalWrite(modeout2,HIGH);
  }
  if (fmode==2){
    ifshift=ifshiftAM;
    ucg.setPrintPos(82,112);
    ucg.setColor(255,255,0); 
    ucg.print("A M");
    digitalWrite(modeout1,HIGH);
    digitalWrite(modeout2,HIGH);
    }
  if (fmode==3){
    ifshift=ifshiftLSB;
    ucg.setPrintPos(12,82);
    ucg.setColor(255,255,0);
    ucg.print("LSB");
    digitalWrite(modeout1,LOW);
    digitalWrite(modeout2,LOW);
  }
  fmode=fmode+1;
  if (fmode==4){fmode=0;}
  ddswrite();
}

//Rit SET
void setrit(){
  if(flagrit==0){
    flagrit=1;
    ucg.setFont(ucg_font_fub11_tr);
    ucg.setPrintPos(190,110);
    ucg.setColor(255,0,0);
    ucg.print("RIT");
    ritlcd();
  }
  else {
    flagrit=0;
    ddsfreq=freq+ifshift;
//    AD9850.wr_serial(0x00,ddsfreq);
    freqt=String(freq); 
    ucg.setFont(ucg_font_fub11_tr);
    ucg.setPrintPos(190,110);
    ucg.setColor(255,255,255);
    ucg.print("RIT");
    ucg.setColor(0,0,0);  
    ucg.drawRBox(222,92,91,21,3);
    freqrit=0;
  }
}

//Rit screen
void ritlcd(){
  noInterrupts();
  ucg.setColor(0,0,0);  
  ucg.drawBox(222,92,91,21);
  ucg.setFont(ucg_font_fub17_tr);
  ucg.setColor(255,255,255); 
  ucg.setPrintPos(230,110);
  ucg.print(freqrit);
  interrupts();
}

//encorder frequency step set
void setstep(){
  noInterrupts();
  if (fstep==10000){
    fstep=10;
  }
  else{
    fstep=fstep * 10;
  } 
 steplcd(); 
// while(analogRead(analogsw) < 1000);
 interrupts();
}

//Step Screen
void steplcd(){
  ucg.setColor(0,0,0);
  ucg.drawRBox(221,61,93,23,3);
  ucg.setFont(ucg_font_fub17_tr);
  ucg.setColor(255,255,255);
  ucg.setPrintPos(220,80);
  if (fstep==10){ucg.print("    10Hz");}
  if (fstep==100){ucg.print("   100Hz");}
  if (fstep==1000){ucg.print("    1KHz");}
  if (fstep==10000){ucg.print("  10KHz");} 
}

//Main frequency screen
void freqlcd(){
  ucg.setFont(ucg_font_fub35_tn); 
  int mojisuu=(freqt.length());
   if(freq<100){
    ucg.setColor(0,0,0);
    ucg.drawBox(217,9,28,36);     
  }
  if(f10 !=(freqt.charAt(mojisuu-2))){
    ucg.setColor(0,0,0);
    ucg.drawBox(245,9,28,36);
    ucg.setPrintPos(245,45);
    ucg.setColor(0,255,0);
    ucg.print(freqt.charAt(mojisuu-2)); 
    f10 = (freqt.charAt(mojisuu-2));
  }
/*  
  if(freq<10){
    ucg.setColor(0,0,0);
    ucg.drawBox(245,9,28,36);    
     }
  if(f1 !=(freqt.charAt(mojisuu-1))){
    ucg.setColor(0,0,0);
    ucg.drawBox(273,9,28,36);
    ucg.setPrintPos(273,45);
    ucg.setColor(0,255,0);
    ucg.print(freqt.charAt(mojisuu-1));     
    f1  = (freqt.charAt(mojisuu-1));   
  }
*/
  if(freq<1000){
    ucg.setColor(0,0,0);
    ucg.drawBox(202,9,15,36);        
    }
  if(f100 !=(freqt.charAt(mojisuu-3))){
    ucg.setColor(0,0,0);
    ucg.drawBox(217,9,28,36);
    ucg.setPrintPos(217,45);
    ucg.setColor(0,255,0);
    ucg.print(freqt.charAt(mojisuu-3)); 
    f100 = (freqt.charAt(mojisuu-3));
  }
  if(freq>=1000){
    ucg.setPrintPos(202,45);
    ucg.setColor(0,255,0);
    ucg.print(".");
  }
  if(freq<10000){
    ucg.setColor(0,0,0);
    ucg.drawBox(146,9,28,36);     
    }
  if(f1k !=(freqt.charAt(mojisuu-4))){
    ucg.setColor(0,0,0);
    ucg.drawBox(174,9,28,36);
    ucg.setPrintPos(174,45);
    ucg.setColor(0,255,0);
    ucg.print(freqt.charAt(mojisuu-4));       
    f1k  = (freqt.charAt(mojisuu-4));
  }
  if(freq<100000){
    ucg.setColor(0,0,0);
    ucg.drawBox(118,9,28,36); 
  }
  if(f10k !=(freqt.charAt(mojisuu-5))){
    ucg.setColor(0,0,0);
    ucg.drawBox(146,9,28,36);
    ucg.setPrintPos(146,45);
    ucg.setColor(0,255,0);
    ucg.print(freqt.charAt(mojisuu-5));   
    f10k = (freqt.charAt(mojisuu-5));
  }
   if(freq<1000000){
    ucg.setColor(0,0,0);
    ucg.drawBox(103,9,15,36); 
    }
  if(f100k !=(freqt.charAt(mojisuu-6))){
    ucg.setColor(0,0,0);
    ucg.drawBox(118,9,28,36);
    ucg.setPrintPos(118,45);
    ucg.setColor(0,255,0);
    ucg.print(freqt.charAt(mojisuu-6));   
    f100k = (freqt.charAt(mojisuu-6));
  }
       
   if(freq>=1000000){
    ucg.setPrintPos(103,45);
    ucg.setColor(0,255,0);
    ucg.print(".");
  }
   if(freq<10000000){
     ucg.setColor(0,0,0);
    ucg.drawBox(47,9,28,36);
     }
   if(fmega !=(freqt.charAt(mojisuu-7))){
    ucg.setColor(0,0,0);
    ucg.drawBox(75,9,28,36);
    ucg .setPrintPos(75,45);
    ucg.setColor(0,255,0);
    ucg.print(freqt.charAt(mojisuu-7));   
    fmega  = (freqt.charAt(mojisuu-7));
   }

   if(freq<100000000){
    ucg.setColor(0,0,0);
    ucg.drawBox(19,9,28,36);
       }
   if (f10m !=(freqt.charAt(mojisuu-8))){
    ucg.setColor(0,0,0);
    ucg.drawBox(47,9,28,36);
    ucg .setPrintPos(47,45);
    ucg.setColor(0,255,0);
    ucg.print(freqt.charAt(mojisuu-8));
    f10m = (freqt.charAt(mojisuu-8));
   }

/*
  ucg.setPrintPos(19,45);
  if(freq>=100000000){
    if(f100m !=(freqt.charAt(0))){
      ucg.setColor(0,0,0);
      ucg.drawBox(19,9,28,36);
      ucg .setPrintPos(19,45);
      ucg.setColor(0,255,0);
      ucg.print(freqt.charAt(0));
      f100m = (freqt.charAt(0));
    }
  }
*/

}

//Basic Screen draw
void screen01(){
  ucg.setColor(255,255,255);
  ucg.drawRFrame(1,1,314,55,5);
  ucg.drawRFrame(2,2,312,53,5);
  ucg.setColor(50,50,50);
  ucg.drawRBox(5,60,60,25,3);
  ucg.drawRBox(75,60,60,25,3);
  ucg.drawRBox(5,90,60,25,3);
  ucg.drawRBox(75,90,60,25,3);
  ucg.setFont(ucg_font_fub17_tr);
  ucg.setPrintPos(12,82);
  ucg.setColor(0,0,0);
  ucg.print("LSB");
  ucg.setPrintPos(82,82);
  ucg.print("USB");
  ucg.setPrintPos(12,112);
  ucg.print("C W");
  ucg.setPrintPos(82,112);
  ucg.print("A M"); 
  ucg.setColor(255,255,255);
  ucg.drawRFrame(220,60,95,25,3);
  ucg.drawRFrame(220,90,95,25,3);
  ucg.setFont(ucg_font_fub11_tr);
  ucg.setColor(255,255,255);
  ucg.setPrintPos(175,80);
  ucg.print("STEP");
  ucg.setPrintPos(190,110);
  ucg.setColor(255,255,255);
  ucg.print("RIT");
  ucg.setColor(100,100,100);
  ucg.setPrintPos(10,210);
  ucg.print(" S:  1-----3-------6-------9---Over--------");
  ucg.setPrintPos(10,177);
  ucg.print(" P:  1-----3-----5-----------10--------------");  
  ucg.setPrintPos(10,230);
  ucg.setColor(235,0,200);
  ucg.print(" Home Blew DDS-VFO Ver4.0 JA2NKD"); 
  ucg.setFont(ucg_font_fub35_tr);
    ucg.setColor(0,255,0);
//    ucg.setPrintPos(19,45);
//    ucg.print("4");
//    ucg.setPrintPos(47,45);
//    ucg.print("3");
//    ucg.setPrintPos(75,45);
//    ucg.print("7");   
//    ucg.setPrintPos(103,45);
//    ucg.print(".");
    ucg.setPrintPos(273,45);
    ucg.print("0");    
}

//DDS Frequency write
void ddswrite(){
  if (flagrit==0){
    ddsfreq=freq+ifshift;
    AD9850.wr_serial(0x00,ddsfreq);
  }
  if(flagrit==1){
    ddsfreq=freq+ifshift+freqrit;
    AD9850.wr_serial(0x00,ddsfreq);
  }
}

//Band data call from eeprom
void bandcall(){
//  band=band+1;
//  if (band>(bandmax-1)){band=0;}
  romadd=0x010+(band*0x010);
 for (int i=0; i<3;i++){
   romf[i]=eepread((romadd+4*i));
  }
  freq = romf[0];
  freqmin = romf[1];
  freqmax = romf[2];
  fmode = EEPROM.read(romadd+12);
  steprom = EEPROM.read(romadd+13);
  if (fmode >=1 ){fmode=fmode-1;}
  else if (fmode==0){fmode=3;}
  if (steprom==1){fstep=10;}
  if (steprom==2){fstep=100;}
  if (steprom==3){fstep=1000;}
  if (steprom==4){fstep=10000;}
  modeset();
  steplcd();
  freqt=String(freq);
  freqlcd();  
  banddataout();
  chlcd();
  if (band == 9){txflg=1;}
  else{txflg = 0;}
}

//Band data write to eeprom
 void bandwrite(){
  romadd=0x010+(band*0x010);
    eepwrite(freq,romadd);
  EEPROM.write(0x001,band);
  EEPROM.write(romadd+12,fmode);
  if (fstep==10){steprom=1;}
  if (fstep==100){steprom=2;}
  if (fstep==1000){steprom=3;}
  if (fstep==10000){steprom=4;}
  EEPROM.write(romadd+13,steprom);
  ucg.setPrintPos(110,140);
  ucg.setFont(ucg_font_fub17_tr);
  ucg.setColor(240,180,40);
  ucg.print("Memorized !!");
  delay (500);
  ucg.setColor(0,0,0);
  ucg.drawBox(100,120,250,30);
  //ucg.drawBox(41,145,270,16); 
 }

 // EEPROM Write subroutine
 
void eepwrite(unsigned long f0,int epadd){
  for (int i=0; i<=3;i++){
  EEPROM.write(epadd+i,f0%256);
  f0=f0/256;
  }
}

// EEPROM Read subroutin

unsigned long eepread(int epadd){
  unsigned long f0=0;
  for (int i=3; i >=0 ;i--){
    f0=f0*256+EEPROM.read(epadd+i);
    }
    return f0;
}

//Band data output I/O
void banddataout(){
  PORTC =band; 
}

// Band No.(Chanel No.) write to LCD
void chlcd(){
ucg.setColor(0,0,0);
ucg.drawBox(5,120,80,20);
ucg.setFont(ucg_font_fub11_tr);
ucg.setPrintPos(12,137);
ucg.setColor(255,255,255);
ucg.print("CH: ");
ucg.print(band+1); 
}

// Direct frequency input
void freqinput(){
    if (fin==10){
      fin=0;
      freqin=0;
      ucg.setColor(0,0,0);
      ucg.drawBox(110,120,200,20);   
    }
    else{
      ucg.setPrintPos(110,140);
      ucg.setFont(ucg_font_fub17_tr);
      ucg.setColor(255,255,255);
      freqin=(freqin*10)+fin;
      ucg.print(freqin);
    }
}

void freqinputsub(){
      finflg=1;
      ucg.setPrintPos(70,140);
      ucg.setFont(ucg_font_fub17_tr);
      ucg.setColor(255,0,0);
      ucg.print("F= ");
}

void freqinputend(){
   freqcheck();   
   finflg=0;
   freqin=0;
   ucg.setColor(0,0,0);
   ucg.drawBox(70,120,200,20);   
}

void freqcheck(){
  if (freqin > freqmax){freqin = freq;}
  if (freqin < freqmin){freqin = freq;}
    freq=freqin; 
    ddswrite();
    freqt=String(freq); 
    freqlcd();
    freqold=freq; 
}

