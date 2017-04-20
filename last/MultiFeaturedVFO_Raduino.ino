/*
This entire program is taken from Jason Mildrum, NT7S and Przemek Sadowski, SQ9NJE.
There is not enough original code written by me to make it worth mentioning.
http://nt7s.com/
http://sq9nje.pl/
http://ak2b.blogspot.com/

Referenced version: showBar Graph and alike parts reused from Pavel Milanes sketch. (CO7WT) <pavelmc@gmail.com> Who did it the cuban way :)

Version resulted by PA3FAT (Ron). Use as you like. Credits to above mentioned (not sure if i am complete in any way).
Defined my own MySMeter function with bars.

Also put more 'logic' in the smeter function, and removed it out of Loop for readability. The part of if freq
changed and part of the setup might also be moved into a single function. Doing the same over then it is time
for lazyness and put it in a function (tmho)
*/

#include <Rotary.h>
#include <si5351.h>
#include <Wire.h>
#include <LiquidCrystal.h>


#define F_MIN        100000000UL               // Lower frequency limit
#define F_MAX        5000000000UL

#define ENCODER_A    3                      // Encoder pin A
#define ENCODER_B    2                      // Encoder pin B
#define ENCODER_BTN  4
#define LCD_RS		8
#define LCD_E		  9
#define LCD_D4		10
#define LCD_D5		11
#define LCD_D6		12
#define LCD_D7		13


#define CALLSIGN "PA3FAT"
#define CALLSIGN_POS 5
#define BITX40   "BITX40 V3c"
#define BITX40_POS 3

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);       // LCD - pin assignement in

// defining the chars for the S-meter function

byte meter_s1[8] = {
B00000,
B00000,
B00000,
B00000,
B00000,
B00000,
B00000,
B11000
};

byte meter_s2[8] = {
B00000,
B00000,
B00000,
B00000,
B00000,
B00000,
B00000,
B11011
};

byte meter_s3[8] = {
B00000,
B00000,
B00000,
B00000,
B00000,
B00000,
B11000,
B11000
};

byte meter_s4[8] = {
B00000,
B00000,
B00000,
B00000,
B00000,
B00000,
B11011,
B11011
};

byte meter_s5[8] = {
B00000,
B00000,
B00000,
B00000,
B00000,
B11000,
B11000,
B11000,
};

byte meter_s6[8] = {
B00000,
B00000,
B00000,
B00000,
B00000,
B11011,
B11011,
B11011,
};
byte meter_s7[8] = {
B00000,
B00000,
B00000,
B00000,
B11000,
B11000,
B11000,
B11000,
};

byte meter_s8[8] = {
B00000,
B00000,
B00000,
B11011,
B11011,
B11011,
B11011,
B11011,
};
byte meter_s9[8] = {
B11000,
B00000,
B11000,
B11000,
B11000,
B11000,
B11000,
B11000,
};
byte meter_s10[8] = {
B11011,
B00000,
B11011,
B11011,
B11011,
B11011,
B11011,
B11011,
};
byte meter_s20[8] = {
B11000,
B00000,
B11000,
B11000,
B11000,
B11000,
B11000,
B11000,
};
byte meter_s30[8] = {
B11011,
B00000,
B11011,
B11011,
B11011,
B11011,
B11011,
B11011,
};


Si5351 si5351;
Rotary r = Rotary(ENCODER_A, ENCODER_B);
//volatile uint32_t LSB = 219850000ULL;
//volatile uint32_t USB = 220150000ULL;
volatile uint32_t LSB = 1199850000ULL;
volatile uint32_t USB = 1200150000ULL;
volatile uint32_t bfo = LSB; //start in lsb
//volatile uint32_t if_offset = 00003200UL; 
//These USB/LSB frequencies are added to or subtracted from the vfo frequency in the "Loop()"
//In this example my start frequency will be 7.100000 Mhz
volatile uint32_t vfo = 710000000ULL / SI5351_FREQ_MULT; //start freq - change to suit
volatile uint32_t radix = 100;	//start step size - change to suit
boolean changed_f = 0;
String tbfo = "";

boolean mustShowStep = false;     // var to show the step instead the bargraph
boolean tx =           false;
// sampling interval for the AGC, 33 milli averaged every 10 reads: gives 3
// updates per second and a good visual effect
#define SM_SAMPLING_INTERVAL  33
#define SM_SAMPLE_CNT 15
byte pep[SM_SAMPLE_CNT];                      // s-meter readings storage
long lastMilis = 0;       // to track the last sampled time

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
    set_frequency(1);
  else if (result == DIR_CCW)
    set_frequency(-1);
}
/**************************************/
/* Change the frequency               */
/* dir = 1    Increment               */
/* dir = -1   Decrement               */
/**************************************/
void set_frequency(short dir)
{
  if (dir == 1)
    vfo += radix;
  if (dir == -1)
    vfo -= radix;

  //    if(vfo > F_MAX)
  //      vfo = F_MAX;
  //    if(vfo < F_MIN)
  //      vfo = F_MIN;

  changed_f = 1;
}
/**************************************/
/* Read the button with debouncing    */
/**************************************/
boolean get_button()
{
  if (!digitalRead(ENCODER_BTN))
  {
    delay(20);
    if (!digitalRead(ENCODER_BTN))
    {
      while (!digitalRead(ENCODER_BTN));
      return 1;
    }
  }
  return 0;
}

/****************************************/
/* MySMeter bar alike display           */
/****************************************/
void mySMeter ( int level, char label ){
   static byte lastpos=0;
   byte i;

   // Write the S meter indicator 2n line first column
   lcd.setCursor(0,1);
   lcd.write( label );
   
   // Note the need to stick to createChar# per bar type written. 
   // You can't use single char creation e.g. 0 or 1 etc as it will show directly on already written chars elsewhere.
   // Resulting in strange looking s-meter graph.
   
   switch( level ){
     case 13: lcd.createChar(6, meter_s30); lastpos=7; lcd.setCursor(lastpos,1); lcd.write( 6 ); break;
     case 12: lcd.createChar(6, meter_s30); lastpos=6; lcd.setCursor(lastpos,1); lcd.write( 6 ); break; // lcd.write(" "); break;
     case 11: lcd.createChar(6, meter_s20); lastpos=6; lcd.setCursor(lastpos,1); lcd.write( 6 ); break; // lcd.write(" "); break;
     case 10: lcd.createChar(5, meter_s10); lastpos=5; lcd.setCursor(lastpos,1); lcd.write( 5 ); break; // lcd.write("  "); break;
     case 9:  lcd.createChar(5, meter_s9); lastpos=5; lcd.setCursor(lastpos,1); lcd.write( 5 );  break; // lcd.write("  "); break;
     case 8:  lcd.createChar(4, meter_s8); lastpos=4; lcd.setCursor(lastpos,1); lcd.write( 4 );  break; // lcd.write("   "); break;
     case 7:  lcd.createChar(4, meter_s7); lastpos=4; lcd.setCursor(lastpos,1); lcd.write( 4 );  break; // lcd.write("   "); break;
     case 6:  lcd.createChar(3, meter_s6); lastpos=3; lcd.setCursor(lastpos,1); lcd.write( 3 );  break; // lcd.write("    ");break;
     case 5:  lcd.createChar(3, meter_s5); lastpos=3; lcd.setCursor(lastpos,1); lcd.write( 3 );  break; // lcd.print("    "); break;
     case 4:  lcd.createChar(2, meter_s4); lastpos=2; lcd.setCursor(lastpos,1); lcd.write( 2 );  break; // lcd.write("     "); break;
     case 3:  lcd.createChar(2, meter_s3); lastpos=2; lcd.setCursor(lastpos,1); lcd.write( 2 );  break; // lcd.write("     "); break;
     case 2:  lcd.createChar(1, meter_s2); lastpos=1; lcd.setCursor(lastpos,1); lcd.write( 1 );  break; // lcd.write("     "); break;
     case 1:  lcd.createChar(1, meter_s1); lastpos=1; lcd.setCursor(lastpos,1); lcd.write( 1 );  break; // lcd.write("      "); break;
     case 0:                               lastpos=1; lcd.setCursor(lastpos,1); break; //lcd.write("       "); break;
     default: break;
   }
   // Clear the remaining bars
   for (i=lastpos;i<7;i++)
   {
     lcd.setCursor(i+1,1); lcd.write(' ');
   }
}

/**************************************/
/* Displays the frequency             */
/**************************************/
void display_frequency()
{
  uint16_t f, g;

  lcd.setCursor(4, 0);
  f = vfo / 1000000; 	//variable is now vfo instead of 'frequency'
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
  lcd.print("Hz");
  lcd.setCursor(0, 0);
  lcd.print(tbfo);
  //Serial.println(vfo + bfo);
  //Serial.println(tbfo);

}

/**************************************/
/* Displays the frequency change step */
/**************************************/
void display_radix()
{
  lcd.setCursor(9, 1);
  switch (radix)
  {
    case 1:
      lcd.print("    1");
      break;
    case 10:
      lcd.print("   10");
      break;
    case 100:
      lcd.print("  100");
      break;
    case 1000:
      lcd.print("   1k");
      break;
    case 10000:
      lcd.print("  10k");
      break;
    case 100000:
      //lcd.setCursor(10, 1);
      lcd.print(" 100k");
      break;
      //case 1000000:
      //lcd.setCursor(9, 1);
      //lcd.print("1000k"); //1MHz increments
      //break;
  }
  lcd.print("Hz");
}

// show the bar graph for the RX or TX modes
void showBarGraph() {
    static byte old_ave = 0;
    byte ave = 0, i;
    
    // find the average
    for (i=0; i<SM_SAMPLE_CNT; i++) {
        ave += pep[i];
    }
    ave /= SM_SAMPLE_CNT;
    
    // print the bars
    // Serial.print("Value averaged: ");Serial.println(ave);
    if (old_ave != ave)
    {
      mySMeter(ave,'S');
      old_ave = ave;
    }
}


// smeter reading, this take a sample of the smeter/txpower each time; an will
// rise a flag when they have rotated the array of measurements 2/3 times to
// have a moving average
void smeter() {
    // contador para el ciclo de lecturas en el array
    static byte smeterCount = 0; // Count the measurements done 
    word val = 0;                // Input value read from the input pin

    // sample and process the S-meter in RX & TX
    // Serial.print("milliseconds passed :"); Serial.println(millis()-lastMilis);
    if ((millis() - lastMilis) >= SM_SAMPLING_INTERVAL) {
      lastMilis = millis();
      // it has rotated already?
      if (smeterCount < SM_SAMPLE_CNT) {
         // take a measure and rotate the array

         // we are sensing a value that must move in the 0-1.1v so internal reference
         analogReference(INTERNAL);
         // read the value and map it for 13 chars (0-12) in the LCD bar
         if (tx) {
            // we are on TX, sensing via A1
            val = analogRead(A1);
         } else {
            // we are in RX, sensing via A0
            val = analogRead(A0);
         }
         // reset the reference for the buttons handling
         analogReference(DEFAULT);

         // watchout !!! map can out peaks, so smooth
         if (val > 1023) val = 1023;
         // Serial.print("Value unmapped: ");Serial.println(val);

         // scale it to 13 blocks (0-14)
         val = map(val, 0, 1023, 0, 14);

         // push it in the array
         for (byte i = 0; i < SM_SAMPLE_CNT-1; i++) {
             pep[i] = pep[i+1];
             // Serial.print("Value pep array: ");Serial.println(pep[i]);
         }
         pep[SM_SAMPLE_CNT-1] = val;
         // Serial.print("Value mapped: ");Serial.println(val);

         // increment counter
         smeterCount += 1;
         
      } else {
        // rise the flag about the need to show the bar graph and reset the count
        showBarGraph();
        smeterCount = 0;
      }
    }
}


void setup()
{
  Serial.begin(19200);
  Serial.println("Setting up sketch...");
  lcd.begin(16, 2);                                                    // Initialize and clear the LCD
  lcd.clear();
  Wire.begin();

  // Show the CALL SIGN and BITX40 version on the display. Settings in define section
  Serial.println(CALLSIGN);
  Serial.println(BITX40);
  lcd.setCursor(CALLSIGN_POS, 0);
  lcd.print(CALLSIGN);
  lcd.setCursor(BITX40_POS, 1); 
  lcd.print(BITX40); 
  delay(3000);
  lcd.clear();

  si5351.set_correction(140); //**mine. There is a calibration sketch in File/Examples/si5351Arduino-Jason
  //where you can determine the correction by using the serial monitor.

  //initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0); //If you're using a 27Mhz crystal, put in 27000000 instead of 0
  // 0 is the default crystal frequency of 25Mhz.

  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  // Set CLK0 to output the starting "vfo" frequency as set above by vfo = ?

#ifdef IF_Offset
  uint32_t vco =bfo-(vfo * SI5351_FREQ_MULT);
  si5351.set_freq(vco, SI5351_PLL_FIXED, SI5351_CLK2);
  // seems not used : volatile uint32_t vfoT = (vfo * SI5351_FREQ_MULT) - bfo;
  tbfo = "LSB";
  // Set CLK2 to output bfo frequency
  si5351.set_freq( bfo, 0, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_2MA); //you can set this to 2MA, 4MA, 6MA or 8MA
  //si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_2MA); //be careful though - measure into 50ohms
  //si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_2MA); //
#endif

#ifdef Direct_conversion
  si5351.set_freq((vfo * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK2);
#endif

#ifdef FreqX4
  si5351.set_freq((vfo * SI5351_FREQ_MULT) * 4, SI5351_PLL_FIXED, SI5351_CLK2);
#endif

  pinMode(ENCODER_BTN, INPUT_PULLUP);
  PCICR |= (1 << PCIE2);           // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
  display_frequency();  // Update the display
  display_radix();
  
  Serial.println("Setting up sketch...finished");
  
}


void loop(){

  smeter();
  showBarGraph();

  // Update the display if the frequency has been changed
  if (changed_f)
  {
    display_frequency();


#ifdef IF_Offset
    uint32_t vco =bfo-(vfo * SI5351_FREQ_MULT);
    si5351.set_freq(vco, SI5351_PLL_FIXED, SI5351_CLK2);
    //you can also subtract the bfo to suit your needs
    //si5351.set_freq((vfo * SI5351_FREQ_MULT) + bfo  , SI5351_PLL_FIXED, SI5351_CLK0);

    Serial.print("bfo : ");Serial.println(bfo);
    Serial.print("vfo : ");Serial.println(vfo);
    Serial.print("mult : ");Serial.println(int(SI5351_FREQ_MULT));
    Serial.print("vco : ");Serial.println(vco);
    
    if (vfo >= 10000000ULL & tbfo != "USB")
    {
      bfo = USB;
      tbfo = "USB";
      si5351.set_freq( bfo, 0, SI5351_CLK0);
      Serial.println("We've switched from LSB to USB");
    }
    else if (vfo < 10000000ULL & tbfo != "LSB")
    {
      bfo = LSB;
      tbfo = "LSB";
      si5351.set_freq( bfo, 0, SI5351_CLK0);
      Serial.println("We've switched from USB to LSB");
    }
#endif

#ifdef Direct_conversion
    si5351.set_freq((vfo * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK0);
    tbfo = "";
#endif

#ifdef FreqX4
    si5351.set_freq((vfo * SI5351_FREQ_MULT) * 4, SI5351_PLL_FIXED, SI5351_CLK0);
    tbfo = "";
#endif

    changed_f = 0;
  }

  // Button press changes the frequency change step for 1 Hz steps
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
        radix = 1;
        break;
    }
    display_radix();
  }
}


