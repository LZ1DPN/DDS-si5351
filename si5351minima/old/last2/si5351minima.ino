/*
Revision 1.0 - Main code by Richard Visokey AD7C - www.ad7c.com
Revision 2.0 - November 6th, 2013...  ever so slight revision by  VK8BN for AD9851 chip Feb 24 2014
Revision 3.0 - April, 2016 - AD9851 + ARDUINO PRO NANO + integrate cw decoder (by LZ1DPN) (uncontinued version)
Revision 4.0 - May 31, 2016  - deintegrate cw decoder and add button for band change (by LZ1DPN)
Revision 5.0 - July 20, 2016  - change LCD with OLED display + IF --> ready to control transceiver RFT SEG-100 (by LZ1DPN)
Revision 6.0 - August 16, 2016  - serial control buttons from computer with USB serial (by LZ1DPN) (1 up freq, 2 down freq, 3 step increment change, 4 print state)
									for no_display work with DDS generator
Revision 7.0 - November 30, 2016  - added some things from Ashhar Farhan's Minima TRX sketch to control transceiver, keyer, relays and other ...									
Revision 7.5 - December 12, 2016  - for Minima and Bingo Transceivers (LZ1DPN mod).
Revision 8.0 - February 15, 2017  - for Minima, BitX and Bingo Transceivers (LZ1DPN mod).
Revision 9.0 - March 06, 2017  - Si5351 + Arduino + OLED - for Minima and Bingo Transceivers (LZ1DPN mod).
				Parts of this program is taken from Jason Mildrum, NT7S and Przemek Sadowski, SQ9NJE.
				http://nt7s.com/
				http://sq9nje.pl/
				http://ak2b.blogspot.com/
				+ example from:
				si5351_vcxo.ino - Example for using the Si5351B VCXO functions
				with Si5351Arduino library Copyright (C) 2016 Jason Milldrum <milldrum@gmail.com>

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SPI.h>
#include <Wire.h>

// new
#include <si5351.h>
Si5351 si5351;

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// Include the library code
#include <rotary.h>
#include <avr/io.h>
#define CW_TIMEOUT (600l) // in milliseconds, this is the parameter that determines how long the tx will hold between cw key downs
unsigned long cwTimeout = 0;     //keyer var - dead operator control
#define TX_RX (12)   // (2 sided 2 possition relay) - for Farhan minima +5V to Receive 0V to Transmit !!! (see Your schema and change if need)
#define TX_ON (7)   // this is for microphone PTT in SSB transceivers (not need for EK1A)
#define CW_KEY (4)   // KEY output pin - in Q7 transistor colector (+5V when keyer down for RF signal modulation) (in Minima to enable sidetone generator on)
#define BAND_HI (6)  // relay for RF output LPF  - (0) < 10 MHz , (1) > 10 MHz (see LPF in EK1A schematic)  
#define USB (5)
#define ANALOG_KEYER (A1)  // KEYER input - for analog straight key
char inTx = 0;     // trx in transmit mode temp var
char keyDown = 0;   // keyer down temp vat
int var_start = 1;

#define BTNDEC (A2)  // BAND CHANGE BUTTON from 1,8 to 29 MHz - 11 bands
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }
Rotary r = Rotary(2,3); // sets the pins for rotary encoder uses.  Must be interrupt pins.
  
int_fast32_t rx=7000000/SI5351_FREQ_MULT; // Starting frequency of VFO
int_fast32_t rx2=1; // temp variable to hold the updated frequency
int_fast32_t rxif=19996500; // IF freq, will be summed with vfo freq - rx variable
int_fast32_t rxifLSB=19996500;  // - 3000
int_fast32_t rxifUSB=20002500;   // + 3000 need more tests for USB mode
String tbfo = "";

int_fast32_t increment = 100; // starting VFO update increment in HZ. tuning step
int buttonstate = 0;   // temp var
String hertz = "100 Hz";
int  hertzPosition = 0;

byte ones,tens,hundreds,thousands,tenthousands,hundredthousands,millions ;  //Placeholders
String freq; // string to hold the frequency
int_fast32_t timepassed = millis(); // int to hold the arduino miilis since startup
int memstatus = 1;  // value to notify if memory is current or old. 0=old, 1=current.
int ForceFreq = 1;  // Change this to 0 after you upload and run a working sketch to activate the EEPROM memory.  YOU MUST PUT THIS BACK TO 0 AND UPLOAD THE SKETCH AGAIN AFTER STARTING FREQUENCY IS SET!
int byteRead = 0;
const int colums = 10; /// have to be 16 or 20 - in LCD 16x2 - 16, or other , see LCD spec.
const int rows = 2;  /// have to be 2 or 4 - in LCD 16x2 - 2, or other , see LCD spec.
int lcdindex = 0;
int line1[colums];
int line2[colums];

// buttons temp var
int BTNdecodeON = 0;   
int BTNlaststate = 0;
int BTNcheck = 0;
int BTNcheck2 = 0;
int BTNinc = 3; // set number of default band minus 1 ---> (for 7MHz = 3)

void checkTX(){   // this is stopped now, but if you need to use mike for SSB PTT button, start in main loop function - not fully tested after last changes
  //we don't check for ptt when transmitting cw
  if (cwTimeout > 0)
    return;

  if (digitalRead(TX_ON) == 0 && inTx == 0){
      //put the  TX_RX line to transmit
      digitalWrite(TX_RX, 1);
	  inTx = 1;
  }

  if (digitalRead(TX_ON) == 1 && inTx == 1){
      //put the  TX_RX line to RX
      digitalWrite(TX_RX, 0);
      inTx = 0;
  }
  //give a few ms to settle the T/R relays
  delay(50);
}

/*CW is generated by keying the bias of a side-tone oscillator.
nonzero cwTimeout denotes that we are in cw transmit mode.
*/

void checkCW(){
  if (keyDown == 0 && analogRead(ANALOG_KEYER) < 50){
    //switch to transmit mode if we are not already in it
    if (inTx == 0){
        //put the  TX_RX line to transmit
          digitalWrite(TX_RX, 0);
        //give the relays a few ms to settle the T/R relays
        delay(50);
    }
    inTx = 1;
    keyDown = 1;
    digitalWrite(CW_KEY, 1); //start the side-tone
  }

  //reset the timer as long as the key is down
  if (keyDown == 1){
     cwTimeout = CW_TIMEOUT + millis();
  }

  //if we have a keyup
  if (keyDown == 1 && analogRead(ANALOG_KEYER) > 150){
    keyDown = 0;
    digitalWrite(CW_KEY, 0);  // stop the side-tone
    cwTimeout = millis() + CW_TIMEOUT;
  }

  //if we have keyuup for a longish time while in cw rx mode
  if (inTx == 1 && cwTimeout < millis()){
    //move the radio back to receive
    digitalWrite(TX_RX, 1);
    digitalWrite(CW_KEY, 0);
    inTx = 0;
    cwTimeout = 0;
  }
}

// start variable setup

void setup() {

Wire.begin();
 
// Start serial and initialize the Si5351
 Serial.begin(115200);
 
// rotary 
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
  
// new
 
  si5351.set_correction(140); //**mine. There is a calibration sketch in File/Examples/si5351Arduino-Jason
  //where you can determine the correction by using the serial monitor.

  //initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0); //If you're using a 27Mhz crystal, put in 27000000 instead of 0
  // 0 is the default crystal frequency of 25Mhz.

  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  // Set CLK0 to output the starting "vfo" frequency as set above by vfo = ?

  // CLK0 - VFO
  si5351.set_freq((rx * SI5351_FREQ_MULT) + rxif, SI5351_PLL_FIXED, SI5351_CLK0);
  volatile uint32_t vfoT = (rx * SI5351_FREQ_MULT) + rxif;
  tbfo = "LSB";
  // Set CLK2 - output BFO frequency
  si5351.set_freq( rxif, 0, SI5351_CLK2);
  //si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_2MA); //you can set this to 2MA, 4MA, 6MA or 8MA
  //si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_2MA); //be careful though - measure into 50ohms
  //si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_2MA); //

// new
  
//set up the pins in/out and logic levels
pinMode(TX_RX, OUTPUT);
digitalWrite(TX_RX, LOW);  
digitalWrite(TX_RX, HIGH); 

pinMode(BAND_HI, OUTPUT);  
digitalWrite(BAND_HI, LOW);

pinMode(USB, OUTPUT); 
digitalWrite(USB, LOW);

//pinMode(FBUTTON, INPUT);  
//digitalWrite(FBUTTON, 1);
  
pinMode(TX_ON, INPUT);    // need pullup resistor see Minima schematic
digitalWrite(TX_ON, LOW);
  
pinMode(CW_KEY, OUTPUT);
digitalWrite(CW_KEY, HIGH);
digitalWrite(CW_KEY, LOW);
  digitalWrite(CW_KEY, 1);
  digitalWrite(CW_KEY, 0);

// Initialize the Serial port so that we can use it for debugging
//  Serial.begin(115200);
  Serial.println("Start VFO ver 9.0 minima 3");

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C address 0x3C (for oled 128x32)
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();

  // Clear the buffer.
  display.clearDisplay();
  
  // text display tests
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(rx);
  display.display();
  
  pinMode(BTNDEC,INPUT);		// band change button
  digitalWrite(BTNDEC,HIGH);    // level
  pinMode(A0,INPUT); // Connect to a button that goes to GND on push - rotary encoder push button - for FREQ STEP change
  digitalWrite(A0,HIGH);  //level

  display.clearDisplay();	
  display.setCursor(0,0);
  display.println(rx);
  display.setCursor(0,18);
  display.println(hertz);
  display.display();
  
	for (int index = 0; index < colums; index++){
		line1[index] = 32;
		line2[index] = 32;
	}
}

///// START LOOP - MAIN LOOP

void loop() {
        if (var_start == 1) {
            var_start=0;
            digitalWrite(TX_RX, 0);
            digitalWrite(CW_KEY, 1);
            digitalWrite(CW_KEY, 0);
            digitalWrite(TX_RX, 1);
            delay(50);
        }
            
	checkCW();   // when pres keyer
	checkTX();   // microphone PTT
	checkBTNdecode();  // BAND change

	
// freq change 
  if (rx != rx2){		
		showFreq();    		
        sendFrequency(rx);
        rx2 = rx;
      }

//  step freq change     
  buttonstate = digitalRead(A0);
  if(buttonstate == LOW) {
        setincrement();        
    };

// LPF band switch relay	  
	  
	if(rx <= 14999999){
		digitalWrite(BAND_HI, 0);
	    }
	if(rx > 14999999){
		digitalWrite(BAND_HI, 1);
		}
		
	if(rx < 10000000){
		rxif=rxifLSB;
                digitalWrite(USB, 0);
	    }
	if(rx >= 10000000){
		rxif=rxifUSB;
                digitalWrite(USB, 1);
		}
}	  
/// END of main loop ///
/// ===================================================== END ============================================


/// START EXTERNAL FUNCTIONS

ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result) {    
    if (result == DIR_CW){rx=rx+increment;}
    else {rx=rx-increment;};       
      if (rx >=200000000){rx=rx2;}; // UPPER VFO LIMIT 
      if (rx <=100000){rx=rx2;}; // LOWER VFO LIMIT
  }
}

// new
void sendFrequency(double frequency) { 
if (rx >= 10000000ULL & tbfo != "USB")
    {
	  si5351.set_freq((rx * SI5351_FREQ_MULT) - rxif  , SI5351_PLL_FIXED, SI5351_CLK0);
      rxif = rxifUSB;
      tbfo = "USB";
      si5351.set_freq( rxif, 0, SI5351_CLK2);
      Serial.println("We've switched from LSB to USB");
    }
else if (rx < 10000000ULL & tbfo != "LSB")
    {
	  si5351.set_freq((rx * SI5351_FREQ_MULT) + rxif, SI5351_PLL_FIXED, SI5351_CLK0);
      rxif = rxifLSB;
      tbfo = "LSB";
      si5351.set_freq( rxif, 0, SI5351_CLK2);
      Serial.println("We've switched from USB to LSB");
    }
// Serial.println(frequency);   // for serial console debuging
// Serial.println(frequency + rxif);
}
// new

// step increments for rotary encoder button
void setincrement(){
  if(increment == 1){increment = 10; hertz = "10 Hz"; hertzPosition=0;} 
  else if(increment == 10){increment = 50; hertz = "50 Hz"; hertzPosition=0;}
  else if (increment == 50){increment = 100;  hertz = "100 Hz"; hertzPosition=0;}
  else if (increment == 100){increment = 500; hertz="500 Hz"; hertzPosition=0;}
  else if (increment == 500){increment = 1000; hertz="1 Khz"; hertzPosition=0;}
  else if (increment == 1000){increment = 2500; hertz="2.5 Khz"; hertzPosition=0;}
  else if (increment == 2500){increment = 5000; hertz="5 Khz"; hertzPosition=0;}
  else if (increment == 5000){increment = 10000; hertz="10 Khz"; hertzPosition=0;}
  else if (increment == 10000){increment = 100000; hertz="100 Khz"; hertzPosition=0;}
  else if (increment == 100000){increment = 1000000; hertz="1 Mhz"; hertzPosition=0;} 
  else{increment = 1; hertz = "1 Hz"; hertzPosition=0;};  
  showFreq()
  delay(250); // Adjust this delay to speed up/slow down the button menu scroll speed.
};

// oled display functions
void showFreq(){
    millions = int(rx/1000000);
    hundredthousands = ((rx/100000)%10);
    tenthousands = ((rx/10000)%10);
    thousands = ((rx/1000)%10);
    hundreds = ((rx/100)%10);
    tens = ((rx/10)%10);
    ones = ((rx/1)%10);

	display.clearDisplay();	
	display.setCursor(0,0);
	display.println(rx);
	display.setCursor(0,18);
	display.println(hertz);
	display.display();

	timepassed = millis();
    memstatus = 0; // Trigger memory write
};

void checkBTNdecode(){
//  BAND CHANGE !!! band plan - change if need
BTNdecodeON = digitalRead(BTNDEC);
if(BTNdecodeON != BTNlaststate){
    if(BTNdecodeON == HIGH){
    delay(200);
    BTNcheck2 = 1;
    BTNinc = BTNinc + 1;
switch (BTNinc) {
    case 1:
      rx=1810000;
      break;
    case 2:
      rx=3500000;
      break;
    case 3:
      rx=5250000;
      break;
    case 4:
      rx=7000000;
      break;
    case 5:
      rx=10100000;
      break;
    case 6:
      rx=14000000;
      break;
    case 7:
      rx=18068000;
      break;    
    case 8:
      rx=21000000;
      break;    
    case 9:
      rx=24890000;
      break;    
    case 10:
      rx=28000000;
      break;
    case 11:
      rx=29100000;
      break;    	  
    default:
      if(BTNinc > 11){
         BTNinc = 0;
      }
    break;
  }
}

if(BTNdecodeON == LOW){
    BTNcheck2 = 0;
	}
    BTNlaststate = BTNcheck2;
  }
}

//// OK END OF PROGRAM
