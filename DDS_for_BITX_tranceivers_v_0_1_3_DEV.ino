  //***************************************************************************************************
  // 
  // File name: DDS_for_BITX_tranceivers_v_0.1.3_DEV.ino
  // K. Hough (M0KOH), 19/01/2016
  //
  // This program provides generation of VFO and BFO signals for BITX transceivers, or for a simple general purpose
  // wide range signal generator. The latter is recommended for accurate determination of the frequency response (and hence
  // of BFO setting) for the BITX IF filter.
  //
  // This version of the program provides for BITX20 and BITX40. It should also be modifyable to suit other variants such as BITX17
  // BITX20 mode provides for Upper Sideband operation with the VFO operating at low frequency
  // BITX40 mode provides for Lower Sideband operation with the VFO operating at high frequency
  //
  // PLEASE READ THE NOTES INCLUDED BELOW
  // THESE INCLUDE GUIDANCE ON TO HOW TO CALIBRATE THE Aduino/Si5351.
  //
  // Beginers in C/C++ coding note that:
  // All lines beginning with '//' are taken to be comments/instructions for the users benefit only and will be ignored during compilation.
  // Lines beginning with '#define' are instructions to the compiler as follows:
  //
  // #define statements which include ONLY a name or condition, can be used with the '#ifdef' statement to detect whether or not
  // that name has been defined, and then to execute code accordingly.
  //
  // or
  //
  // #define statements which include a name followed by some text, will cause the complier to replace every occurence of the name with the
  // associated text. This provides a very convenient shorthand way of globaly changing values asigned to variables.
  //
  // Some of the variable values are terminated by 'ULL'. This defines a variable as type 'unsigned long long' and is
  // applied because some of the library routines require it. C/C++ is quite pedantic wrt typing of variables.
  //
  // Program Background
  // ~~~~~~~~~~~~~~~~~~
  // This program/sketch has been based on the Arduino sketch file given at the following link:
  //<http://ak2b.blogspot.co.uk/2015/04/multi-featured-vfo.html>
  //
  // The Si5351 can generate three individually programmed output signals at up to 3 Volts pk to pk. This program uses only one of these (ie CLK0) in signal
  // generator mode, or two of them (ie CLK0 and CLK2) in BITX mode.
  // 
  // The modified Arduino Si5351 library referred to at the site given above is REQUIRED and should be installed into
  // the Arduino sketchbook/libraries directory. Download the .zip file from here: <https://github.com/etherkit/Si5351Arduino>,
  // then install via the Arduino IDE using Sketch>Import Library>Add Library
  // It is ESSENCIAL that this modified library is used. If you already have the standard Si5351 library
  // installed, you will need to remove it otherwise the compiler will get confused.
  // 
  // If you are using an early version of the Arduino IDE, it is recommended that you upgrade to at least
  // version 1.6.3. This will make installing libraries in .zip format much easier
  // 
  // Modifications/updates to original code
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // -- Some tidying up and re-arranging of code has been done.
  // -- F_MIN, F_MAX, and other frequency settings now refer to ACTUAL frequency values
  // -- In signal generator mode, upper and lower frequency limits have can be set to 8kHz and 100MHz, but this can be changed to suit personal preferences
  // -- In BITX mode frequency settings have been set to correspond with operating band limits
  // -- Provision has been made to use either a 16 col x 2 row LCD or a 20 col x 4 row LCD (Liquid Crystal Display)
  // -- Function 'display_frequency()' has been competely re-written and now remains correct/stable for all
  //    frequencies. This uses an additional function 'reverseString(char s[])'
  // -- Function 'set_frequency(short dir)' has been modified
  //
  // -- Included for two extra buttons on I/O lines 4 and 12 to provide for stepping up/down of frequency increments (added 05/10/2015)
  //    This (experimentally) includes ruitine for providing two operations via one button, based on period of button press.(added 07/10/2015)
  //
  //    Note that Arduino I/O line no 13 is not used because it is connected with an on board/built-in LED
  //
  //    The original method for changing frequency increment setting via the rotary encoder push switch has been temporarilly retained in this version
  //    to allow for comparision of the two methods of changing increment setting.
  //
  //*******************************************************************************************************
  // User settings/procedures
  // ~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // CALIBRATION OF THE SI5351
  // The Si5351 is crystal controlled so frequency stability will be good. Frequency accuracy will also be fairly good, but to obtain best frequencies accuracy, a calibration
  // correction should be appplied as follows:
  //  a) Set the program to operate as a signal generator by uncommenting the line '#define Signal_Generator' (at approx line no 144)
  //     Make sure that the the other lines such as '#define BITX20' are commented out.
  //  b) Comment out the entire line containiing '#define Calibration_correction'  (at approx line no 161)
  //  c) Compile/upload the resulting program to Arduino. This will result in the si5351 generating (uncalibrated) RF. At startup the LCD will show
  //     a frequency reading of exectly 10MHz, but this can be changed to suit available test conditions.
  //  d) Use an ACCURATELY calibrated frequency counter or receiver to determine the ACTUAL frequency generated by the Arduino/Si5351/LCD
  //  e) Calculate the difference between the frequency displayed by the LCD and the ACTUAL measured frequency as parts per billion (ie parts per 10^9)
  //  f) Uncomment the line containing '#define Calibration_correction', insert this correction value, then re-compile and re-check calibration.
  //
  // SPECIFY REQUIRED MODE OF OPERATION
  // This program can be set to operate as a simple DDS signal generator (for test purpose?), or to control the VFO and BFO of a BITX20 or BITX40 SSB transceiver
  // Select mode of operation by commenting/uncommenting the appropriate '#define' statements below (at approx line no 140).
  //
  // Other settings that might need to be set to suit individual cases relate to:
  // -- frequency range and startup frequency
  // -- and in the case of BITX20 operation, also BFO frequency. This must be defined ACCURATELY! See below
  //
  // By default the program will assume that a 16 column x 2 row LCD device is used with the Arduino/Si5351.
  // If a 20 col x 4 row LCD is to be used, uncomment the line containing the statement '#define display_20x4' (at approx line no 139)
  //
  // User defined settings are identifed under sections identified by "User defined setting......" (at approx line no 194)
  // Users should not attempt to modify any other parts of the program
  //
  // DETERMINATION OF IF FREQUENCY RESPONSE
  // For correct operation in BITX20 mode, the BFO must be set to correspond with the -20dB or lower on the low frequency side of the IF filter frequency.
  // BITX40 mode (ie LSB) operation will require that the BFO is set at the -20dB frquency on the HIGH side of the IF filter response.
  // This is to ensure addequate rejection of the BFO/carrier during transmission. The best way to do this is to produce a graphical plot of
  // dB response versus frequency, from which the -20dB frequency can be determined and the BFO frequency set accordingly. The signal generator mode
  // of this program provides a convenient means to do this. Use frequency increments of 100Hz.
  //
  //***************************************************************************************************
  // MY PRACTICAL CONSTRUCTION  --  K. Hough
  // The original AK2B project showed the use an Arduino Uno. My preference is for Arduino Nano which is physically smaller than
  // the Uno and like the Adafruit Si5351 device, fits into SIL(single in line) header sockets which were mounted onto a piece
  // of Veroboard (ie stripboard) that was cut to be slightly larger than the LCD display device that was used (a 20 col x 4 row device).
  // I then fastened the LCD device to sit over, but not in contact with the copper side of the Veroboard. This made a compact
  // assembly that could be easily fitted into a front panel.
  // 
  // The original and present designs do not use all of the digital and analogue I/O lines of the Arduino. My construction has
  // included wiring and connecions to provide for future convenient access to three of the spare analogue inputs (A0, A1, and A2) and to two of the spare
  // digital I/O lines (D12, and D4  --  not D13 because of on board LED attached to this line).
  // This program uses D12 and D4 to provide an alternative and more practical means of changing frequency increments. The analogue lines
  // might be used in future for S meter, etc --software to provide an S meter has been developed separately.
  //
  //##################################################################################################################################
  //
  // The usual Arduino 'setup()' and 'loop()' functions can be found towards the end of this source code, after all of the additional program functions
  //
  //######################
  //# PROGRAM BEGINS HERE
  //######################
  
  //define the program name and version
  #define program_name "DDS_for_BITX"
  #define program_version "0.1.2_DEV"

  #include <stdint.h>
  
  #include <Rotary.h>
  #include <si5351.h>
  #include <Wire.h>
  #include <LiquidCrystal.h>
  
  //###################### USER SETTING -- Set LCD display format ###########################
  
  // This program will default to use a 16 col x 2 row LCD display if the define statement
  // below is commented out by inserting //
  
  #define display_20x4
  
  //###################### USER SETTING -- Set mode of operation ###########################
  // Uncomment only ONE of these three #define statements
  
  #define Signal_Generator
  
  //#define BITX20
  
  //#define BITX40
  
  // For BITX operation refer also to "ADDITIONAL USER SETTNGS" below
  
  //############### USER SETTING -- Set frequency calibration correction #####################
  
  // Apply correction factor to si5351 Xtal frequency  -- this is in parts per billion --  refer si5351.ccp
  // Use an ACCURATELY calibrated radio or frequency counter to determine the ACTUAL frequency genetared by the Arduino/si5351 in signal_generator mode
  // Then calulate the difference between the frequency displayed on the arduino LCD and the radio as parts per billion
  // NOTE: this is an american billion. ie 10^9
  
  // If frequency on Arduino/LCD is lower than 10MHz then correction should be positive, and vice versa
  
  #define f_calibration_correction 14200
  // Note that this correction value applied to MY si5351. Your si5351 will no doubt require a different value.
  
  // After correction, the si5351 was found to hold accuracy over the frequency range from 1MHz to 100MHz
  
  //##########################################################################################
  
  #define ENCODER_A        3    // Digital port to connect with Rotary encoder pin A
  #define ENCODER_B        2    // Digital port to connect with Rotary encoder pin B
  
  #define ENCODER_BTN     11    //Digital port to connect with encoder switch
  
  //extra buttons for increasing/decreasing frequency increments (added 05/10/2015)
  #define INCR_UP          12    //increase frequency increment 
  #define INCR_DWN          4    //decrease frequency increment
  //could not use I/O line no 13 because this line includes built-in LED
  //for now, increment can be changed via either the extra switches or the switch associated with the encoder
  
  #define LCD_RS            5   //connections to LCD display
  #define LCD_E	            6
  #define LCD_D4            7
  #define LCD_D5            8
  #define LCD_D6            9
  #define LCD_D7            10
  
   //set pin assignments above to LCDcrystal object
  LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
  
  Si5351 si5351;
  
  Rotary r = Rotary(ENCODER_A, ENCODER_B);
  
  //################################### ADDITIONAL USER SETTING  ####################################
  //It might seem to be untity to put these sttings here, but they must be placed after all of the statements above
 
  #ifdef Signal_Generator
    volatile uint32_t Start_frequency = 10000000ULL;   
    volatile uint32_t F_MIN = 8000ULL;
    volatile uint32_t F_MAX = 100000000ULL;
  #endif  
  
  #ifdef BITX20  
    // These settings work for my BITX20 rig. You should check for your own rig
    volatile uint32_t Start_frequency = 14200000ULL;  
    volatile uint32_t F_MIN = 14000000ULL;
    volatile uint32_t F_MAX = 14350000ULL;
    volatile uint32_t BFO_freq = 10999400ULL;  //depends critically on IF filter frequency response  --  see notes above
  #endif
  
  #ifdef BITX40  
    // These settings should be OK for BITX40. You should check for your own rig
    volatile uint32_t Start_frequency = 7100000ULL;  
    volatile uint32_t F_MIN = 7000000ULL;
    volatile uint32_t F_MAX = 7200000ULL;
    volatile uint32_t BFO_freq = 10999400ULL;  //depends critically on IF filter frequency response  --  see notes above
  #endif 
  
  //#######################################################################################
  //#                      NO MORE USER SETTINGS BELOW THIS POINT                       
  //#######################################################################################
  
  volatile uint32_t radix = 100;
  volatile uint32_t Signal_frequency = Start_frequency;
  
  boolean changed_f = 0;
  
  //**********************************************************************************************
  //* Interrupt service routine for
  //* encoder frequency change
  //******************************
  ISR(PCINT2_vect) {
    unsigned char result = r.process();
    if (result == DIR_CW)
      set_frequency(1);
    else if (result == DIR_CCW)
      set_frequency(-1);
  }
  
  //**********************************************************************************************
  //* Change the frequency
  //**********************
  // dir = 1    Increment
  // dir = -1   Decrement
  //*********************
  void set_frequency(short dir)
  {
    if (dir == 1) {
      if ((Signal_frequency + radix) < F_MAX) Signal_frequency += radix;
      else Signal_frequency = F_MAX;
    }
    if (dir == -1) {
      if ((Signal_frequency - radix) >= F_MIN & Signal_frequency > radix) Signal_frequency -= radix;
    }
    changed_f = 1;
  }
  
  //**********************************************************************************************
  //* Read the button with debouncing and reset radix
  //*************************************************
  void get_button()
  {
    if (!digitalRead(ENCODER_BTN))
    {
      delay(20);
      if (!digitalRead(ENCODER_BTN))
      {
      while (!digitalRead(ENCODER_BTN));
        
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
  }
  //**********************************************************************************************
  //* New read buttons function with debouncing and reset radix
  //***************************************************************
   void read_buttons()
  {
   // This includes a test ruitine to demonstrate provision of two actions for each button
   // Action selected depends on period of button press
   
    if (!digitalRead(INCR_UP))
    {
      delay(20);
      if (!digitalRead(INCR_UP))
      {
      unsigned long t = millis();
      while (!digitalRead(INCR_UP));
      if((millis() - t) < 2000){
        if(radix < 1000000) radix = radix * 10;
      }
      else
      {
        lcd.setCursor(0, 0); 
        lcd.print("#");
      }
      display_radix();     
      }
    }
    
    if (!digitalRead(INCR_DWN))
    {
      delay(20);
      if (!digitalRead(INCR_DWN))
      {
      unsigned long t = millis();  
      while (!digitalRead(INCR_DWN));
      if((millis() - t) < 2000){ 
        if(radix > 1) radix = radix / 10;
      }
      else
      {
        lcd.setCursor(0, 0); 
        lcd.print(" ");
      }
      display_radix(); 
      }
    }
    
  }
  //**********************************************************************************************
  //* Display frequency
  //*******************
  // This is a complete re-write of the original function
  // It is necessary to produce a string version of the 'freq' variable. The resulting string length will
  // depend on frequency, but for simplicity a fixed length string is needed, so the process is not simple, and essentially comprises:
  // 1. using sprinf to generate a string version of 'freq' as buffer1
  // 2. reverse string in buffer1 and add zero characters to the end of this string to produce a defined string length
  // 3. build new string as buffer2 inserting '.' characters into string to separate Hz, kHz, and MHz
  // 4. reverse the completed string ready to send to LCD display
  // Somewhat complicated, but this ensures that all frequencies from a few kHz to over 100MHz will be displayed correctly
  //*************************
  void display_frequency()
  {
    char buffer1 [15];
    char buffer2 [15];
  
    int  l = 0,  m = 0, n = 0;
  
    //copy 'Signal_frequency' into a string equivalent as buffer1
    sprintf(buffer1, "%lu", Signal_frequency);
  
    //reverse character sequence of buffer1
    reverseString(buffer1);
  
    //add zero chatarters to produce string of defined length
    while (strlen(buffer1) < 7) strcat(buffer1, "0");
  
    //now need to build string as buffer2 to include '.' markers to separate MHz, kHz, and Hz
    l = strlen(buffer1); 
    m = 0;
    for (n = 0; n <= l; n++) {
      buffer2[n + m] = buffer1[n];
      if (n == 2 || n == 5) {
        m++;
        buffer2[n + m] = '.';
      }
    }
    //add string terminater to buffer2 array (normal C practice needed for array to be recognised as a string variable)
    buffer2[n + m] = '\0';
  
    //reverse buffer2 string to "normal" appearance
    reverseString(buffer2);
  
    //build another string as buffer1 adding required number of spaces to front of string to produce fixed length
    //then add buffer2 to this new buffer1
    //this is to ensure that final display will be shown at correct position on LCD
    l = 12 - strlen(buffer2);
    strcpy(buffer1, "");
    for (n = 0; n < l - 1; n++) {
      strcat(buffer1, " ");
    }
    strcat(buffer1, buffer2);
  
    //print formatted frequency value to display
  #ifdef display_20x4
    lcd.setCursor(3, 1); 
    lcd.print(buffer1);
    lcd.setCursor(14, 1); 
    lcd.print(" Hz");
  #else
    lcd.setCursor(1, 0); 
    lcd.print(buffer1);
    lcd.setCursor(12, 0); 
    lcd.print(" Hz");
  #endif
  }
  
  //**********************************************************************************************
  //* Displays the frequency change step (radix)
  //********************************************
  void display_radix()
  {
  #ifdef display_20x4
    lcd.setCursor(3,2); lcd.print("incr:");
    lcd.setCursor(9, 2);
  #else
    lcd.setCursor(2,1);lcd.print("incr:");
    lcd.setCursor(7, 1);
  #endif
  
    switch (radix)
    {
    case 1:
      lcd.print("   +/-1");
      break;
    case 10:
      lcd.print("  +/-10");
      break;
    case 100:
      lcd.print(" +/-100");
      break;
    case 1000:
      lcd.print("  +/-1k");
      break;
    case 10000:
      lcd.print(" +/-10k");
      break;
    case 100000:
      //lcd.setCursor(10, 1);
      lcd.print("+/-100k");
      break;
    case 1000000:
      lcd.print("  +/-1M"); //1MHz increments
      break;
    }
    lcd.print("Hz");
  }
  
  //**********************************************************************************************
  //* Reverse a string
  //******************
  // Taken from article by Mihalis Soukalos, Linux User magazine
  // C pointer code is not the easiest to understand, but it is fast  -- K. Hough
  //
  // This function is used by the display_frequency() function
  //*********************************************************
  void reverseString(char s[]) {
    int len = strlen(s);
    char *p = &s[len - 1];
    char temp;
    int i;
    for (i = 0; i < len / 2; i++) {
      temp = s[i];
      s[i] = *p;
      *p = temp;
      p--;
    }
  }
  
  //**********************************************************************************************
  void setup()
  {

  boolean sig_gen_flag;
  // can't use nested #ifdef statements, so need to set a flag
  
  #ifdef Signal_Generator
   sig_gen_flag = true;
  #else
   sig_gen_flag = false;
  #endif
  
  #ifdef display_20x4
   lcd.begin(20, 4); lcd.clear();
  #else
   lcd.begin(16, 2); lcd.clear();
  #endif
    
  if (sig_gen_flag == true){
   lcd.setCursor(0, 0); lcd.print("Sig Gen Mode");
   delay(2000);
   lcd.clear();
  }
    
  // Initialize LCD and show identifying title page   
  #ifdef display_20x4
     int l = (20 - strlen(program_name))/2;
     lcd.setCursor(l, 0); 
     lcd.print(program_name);
     lcd.setCursor(7, 1); lcd.print("v"); lcd.print(program_version);
     lcd.setCursor(0, 2); lcd.print("For Arduino + Si5351");
     lcd.setCursor(15, 3); lcd.print("M0KOH");
     //delay to show title page
     delay(3000);
  #else
     int l = (16 - strlen(program_name))/2;
     lcd.setCursor(l, 0); lcd.print(program_name);
     lcd.setCursor(0, 1); lcd.print("v"); lcd.print(program_version);
     lcd.setCursor(11, 1); lcd.print("M0KOH");
     //delay to show title page
     delay(2000);
  #endif  
 
  //clear line ready for usage
  #ifdef display_20x4
     if(sig_gen_flag == true){lcd.setCursor(0,0); lcd.print("  Signal Generator  ");}
     lcd.setCursor(0, 2); lcd.print("                    ");
     
  #else
     lcd.setCursor(0, 1); lcd.print("                ");
  #endif
  
  Wire.begin();
  
  //apply frequency correction factor to si5351 -- this is in parts per billion --  refer si5351.ccp
  //see notes above for details
  si5351.set_correction(f_calibration_correction);
  
  //initialize the Si5351
   si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0); //If you're using a 27Mhz crystal, put in 27000000 instead of 0
  //0 is the default crystal frequency of 25Mhz.
  
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
        
  #ifdef Signal_Generator
    //ensure that CLK2 is not running
    si5351.set_clock_pwr(SI5351_CLK2, 0);
    si5351.set_freq((Start_frequency * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK0);
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA);
  #endif
    
  #ifdef BITX20
    //enable CLK0 and set VFO
    si5351.set_clock_pwr(SI5351_CLK0, 1);
    si5351.set_freq(((Start_frequency - BFO_freq) * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK0);
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA);
    //enable CLK2 and set BFO
    si5351.set_clock_pwr(SI5351_CLK2, 1);
    si5351.set_freq( BFO_freq * SI5351_FREQ_MULT, 0, SI5351_CLK2);
    si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_6MA);
  #endif
  
  #ifdef BITX40
    //enable CLK0 and set VFO
    si5351.set_clock_pwr(SI5351_CLK0, 1);
    si5351.set_freq(((Start_frequency + BFO_freq) * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK0);
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA);
    //enable CLK2 and set BFO
    si5351.set_clock_pwr(SI5351_CLK2, 1);
    si5351.set_freq( BFO_freq * SI5351_FREQ_MULT, 0, SI5351_CLK2);
    si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_6MA);
  #endif

  //enable internal pull-up resistors on the following I/O lines
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(INCR_UP, INPUT_PULLUP);
  pinMode(INCR_DWN, INPUT_PULLUP);
    
  //enable interrupt for the encoder
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
    
  //update frequency displayed on LCD
  display_frequency();
    
  //update radix/frequency increment setting on LCD
  display_radix();      
  
  //this line causes function 'loop()' to update correctly without having to first operate the encoder
  changed_f = true;
  }
  
  //**********************************************************************************************
  void loop()
  {
  //if selected frequency has changed, update CLK0 and LCD display
  if (changed_f) {
      
    display_frequency();
      
    #ifdef Signal_Generator
      si5351.set_freq((Signal_frequency * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK0);
    #endif
      
    #ifdef BITX20
      si5351.set_freq(((Signal_frequency - BFO_freq) * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK0);
    #endif
    
    #ifdef BITX40
      si5351.set_freq(((Signal_frequency + BFO_freq) * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK0);
    #endif
      
    changed_f = 0;
    }
  
   //check for button press and update the radix/frequency increments as 1Hz, 10Hz, 100Hz, 1kHz, 10kHz, 100kHz, or 1MHz
   get_button();
   
   //new read buttons function
   read_buttons();

  }
  //**********************************************************************************************






