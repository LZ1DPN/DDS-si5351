#include <Rotary.h>

/*
 * MV5_C3
 * Works but left out coditional so continuously rewries LCD
 * this updates the USB/LSB display immediately so maybe wastes resources.
 * Simplified the sideband switching routine.
This entire program is taken from Jason Mildrum, NT7S and Przemek Sadowski, SQ9NJE.
There is not enough original code written by me to make it worth mentioning.
http://nt7s.com/
http://sq9nje.pl/
http://ak2b.blogspot.com/

Modified 10 Jan 2017 GM3VMB & G4IIB to allow use on BITX40
*/

#include <Rotary.h>
#include <si5351.h>
#include <Wire.h>
#include <LiquidCrystal.h>


#define F_MIN        100000000UL               // Lower frequency limit 1MHz
#define F_MAX        5000000000UL               //Upper limit 50MHz
#define cal          100000UL                    //frequency calibration not in use

#define ENCODER_A    3                      // Encoder pin A
#define ENCODER_B    2                      // Encoder pin B
#define ENCODER_BTN  4
#define LCD_RS    8
#define LCD_E     9
#define LCD_D4    10
#define LCD_D5    11
#define LCD_D6    12
#define LCD_D7    13
#define CAL_BUTTON (A2)


LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);       // LCD - pin assignment in
Si5351 si5351;
Rotary r = Rotary(ENCODER_A, ENCODER_B);
volatile uint32_t LSB = 1199435000ULL;// for high side vfo
volatile uint32_t USB = 1199665000ULL;
volatile uint32_t bfo = 1199435000ULL;//start in lsb

//These USB/LSB frequencies are added to or subtracted from the vfo frequency in the "Loop()"
//In this example my start frequency will be 7.10000 plus 11.999500 or clk0 = 19.9995Mhz
volatile uint32_t vfo = 710000000ULL / SI5351_FREQ_MULT; //start freq - change to suit
volatile uint32_t radix = 100;  //start step size - change to suit
boolean changed_f = 0;
String tbfo = "";
uint32_t duration = 0;
uint32_t t1 = 0;
bool sideband = 0;
bool mode_f = 0;

//------------------------------- Set Optional Features here --------------------------------------
//Remove comment (//) from the option you want to use. Pick only one
#define IF_Offset //Output is the display plus or minus the bfo frequency
//#define Direct_conversion //What you see on display is what you get
//#define FreqX4  //output is four times the display frequency
//--------------------------------------------------------------------------------------------------


/**************************************/
/* Interrupt service routine for      */
/* encoder frequency change           */
/**************************************/
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_CW)
    set_frequency(-1);
  else if (result == DIR_CCW)
    set_frequency(1);
}
/**************************************/
/* Change the frequency               */
/* dir = 1    Increase               */
/* dir = -1   Decrease               */
/**************************************/
void set_frequency(short dir)
{
  if (digitalRead(CAL_BUTTON) == HIGH){  
    if (dir == 1)
      vfo += radix;
    if (dir == -1)
      vfo -= radix;

    if(vfo > F_MAX/100)
      vfo = F_MAX/100;
    if(vfo < F_MIN/100)
      vfo = F_MIN/100;

  } else {
    if(sideband){
     if (dir == 1)
        USB += radix;
      if (dir == -1)
        USB -= radix;
    }else{
     if (dir == 1)
        LSB += radix;
      if (dir == -1)
        LSB -= radix;
    }
    delay(2000);
  }
  changed_f = 1;
}
/**************************************/
/* Read the button with debouncing    */
/**************************************/
boolean get_button()
{
  duration = 0;
  t1 = 0;
  if (!digitalRead(ENCODER_BTN))     
  delay(50);
 
  while(!digitalRead(ENCODER_BTN) && t1 <100){
    delay (100);
    t1++;
    } 
  duration = t1;
  
 //Decides here whether it has been a long press and
 //if so toggles sideband marker.
   
  if(duration >= 5){
    mode_f = 1;
    delay (50);  
     if(sideband)
     {
      sideband = 0;
     }else{
      sideband = 1; }
  //  Serial.println(sideband);
    
    delay(50);
   }
//decide here that it has been a short press so change radix.   
   if((duration >0) && (duration <5))
   {
      return 1;
    }
  return 0;
 }


/**************************************/
/* Displays the frequency             */
/**************************************/
void display_frequency()
{
  uint16_t f, g;

  lcd.setCursor(0, 0);
  f = vfo / 1000000;  //variable is now vfo instead of 'frequency'
  if (f < 10)
    lcd.print(' ');
  lcd.print(f);
  lcd.print('.');
  f = (vfo % 1000000) / 1000;
  if (f < 100)
    lcd.print('0');
  if (f < 10)
    lcd.print('0');
  lcd.print(f);
  lcd.print('.');
  f = vfo % 1000;
  if (f < 100)
    lcd.print('0');
  if (f < 10)
    lcd.print('0');
  lcd.print(f);
  lcd.print("Hz ");
  
  lcd.print(tbfo);
  //Serial.println(vfo + bfo);
  //Serial.println(tbfo);
  display_radix();

  if (digitalRead(CAL_BUTTON) == LOW){
    lcd.setCursor(0, 1);
    lcd.print(bfo);
  }  
}

/**************************************/
/* Displays the frequency change step */
/**************************************/
void display_radix()
{
  // set up blinking cursor...
    switch (radix)
   
 {  
    case 1:      
    lcd.setCursor(9, 0); 
     lcd.blink(); 
      break;
      
    case 10:
    lcd.setCursor(8,0);
      lcd.blink();      
      break;
      
    case 100:
     lcd.setCursor(7,0);
     lcd.blink(); 
      break;
      
    case 1000:
     lcd.setCursor(5,0);
      lcd.blink(); 
      break;
      
    case 10000:
     lcd.setCursor(4,0);
    lcd.blink(); 
      break;
      
    case 100000:     
    lcd.setCursor(3,0);
    lcd.blink(); 
      break;
      
    case 1000000:
     lcd.setCursor(1,0);
     lcd.blink(); 
     break;
 }
  delay(100);
}

void setup()
{
    
  Serial.begin(19200);
  lcd.begin(16, 2);                                                    // Initialize and clear the LCD
  lcd.clear();
  Wire.begin();

  pinMode(CAL_BUTTON, INPUT);
  digitalWrite(CAL_BUTTON, HIGH);

 // si5351.set_correction(1000); //**mine. There is a calibration sketch in File/Examples/si5351Arduino-Jason
  //where you can determine the correction by using the serial monitor.

  //initialize the Si5351
 si5351.init(SI5351_CRYSTAL_LOAD_8PF,0, 0); //If you're using a 27Mhz crystal, put in 27000000 instead of 0
  // 0 is the default crystal frequency of 25Mhz.  // 0 is the default crystal frequency of 25Mhz.

  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  // Set CLK0 to output the starting "vfo" frequency as set above by vfo = ?

#ifdef IF_Offset
 /*Superhet mode.  This calculates the CLK2 (vfo in this case) output freq by adding the dial freq (x100 for 1Hz) The idea is  
 to have the local oscillator above the IF and to subtract the IF from it.  
 This causes a sideband inversion so the IF generates USB but the output is LSB. 
 However it generally makes filtering easier for bands below 10 MHz.  You can have (BFO-VFO) or (VFO-BFO)
 as alternatives but the result MUST be a positive number. See again below inside Loop.
 */
  si5351.set_freq(((vfo * SI5351_FREQ_MULT) + bfo) -145000, SI5351_CLK2); //for sum 19Mhz -1450hz correction
  volatile uint32_t vfoT = (vfo * SI5351_FREQ_MULT) + bfo ; 
  // si5351.set_freq((vfo * SI5351_FREQ_MULT) + bfo, SI5351_CLK2); //for sum 19Mhz without correction use this to obtain calibratino difference on Khz
  //volatile uint32_t vfoT = (vfo * SI5351_FREQ_MULT) + bfo ;                                                                                                               
//  si5351.set_freq(bfo - (vfo * SI5351_FREQ_MULT) + 145000, SI5351_CLK2); //for difference 4.8Mhz + 1450hz correction 
//  volatile uint32_t vfoT = bfo - (vfo * SI5351_FREQ_MULT) ;
  tbfo = "LSB";
  // Set CLK0 to output bfo frequency
  si5351.set_freq( bfo,SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_4MA); //you can set this to 2MA, 4MA, 6MA or 8MA
  //si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_2MA); //be careful though - measure into 50ohms
  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_4MA); //

  
#endif

#ifdef Direct_conversion
 //Generates output frequency directly - signal generator mode
  si5351.set_freq((vfo * SI5351_FREQ_MULT),  SI5351_CLK2);
#endif

#ifdef FreqX4
//Can be set to multiply by 2 if required.  Need to increase max frequency  to allow over 12.5MHz!
  si5351.set_freq((vfo * SI5351_FREQ_MULT) * 4,  SI5351_CLK2);
#endif
 
  PCICR |= (1 << PCIE2);           // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  PCICR |= (1 << PCIE2);           // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
  display_frequency();  // Update the display
 // display_radix();

 

  
}


void loop()
{
    // Update the display if the frequency has been changed
    if ((changed_f) or (mode_f))
  {  
     display_frequency();
  //   display_radix();
#ifdef IF_Offset

    
  // si5351.set_freq(bfo - (vfo * SI5351_FREQ_MULT) + 145000, SI5351_CLK2);// low side 4.8Mhz 145000 being the corection factor for 1.45Khz
    //you can also add the bfo to suit your needs
   // si5351.set_freq((vfo * SI5351_FREQ_MULT) + bfo, SI5351_CLK2); //for sum 19Mhz no correction
si5351.set_freq(((vfo * SI5351_FREQ_MULT) + bfo) -145000, SI5351_CLK2); //for sum 19Mhz -1450hz correction

// Auto switch sidebands

 //  Serial.println(mode_f);
//Serial.println(tbfo);
//Serial.println(bfo);
   if(sideband)
   {
       bfo = USB;
      tbfo = "USB      ";
      si5351.set_freq( bfo,SI5351_CLK0);
        }
    else
    {
     bfo = LSB;
     tbfo = "LSB      ";
     si5351.set_freq( bfo,SI5351_CLK0);
     }

    display_frequency(); //**********
   
   mode_f = 0; 
  }     
#endif

 
#ifdef Direct_conversion
    si5351.set_freq((vfo * SI5351_FREQ_MULT), SI5351_CLK2);
    tbfo = "";
#endif

#ifdef FreqX4
    si5351.set_freq((vfo * SI5351_FREQ_MULT) * 4, SI5351_CLK2);
    tbfo = "";
#endif
 changed_f = 0;
 
  

  // Button press changes the frequency change step size
  if (get_button())
  {
    switch (radix)
    {
      case 1:
        radix = 10;
        break;
      case 10:
        radix = 100;
        break;
      case 100:
        radix = 1000;
        break;
      case 1000:
        radix = 10000;
        break;
      case 10000:
        radix = 100000;
        break;
      case 100000:
        radix = 1000000;
        break;
      case 1000000:
        radix = 1;
        break;
    }
    display_radix();
  }
}
