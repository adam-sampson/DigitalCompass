#include <Wire.h>
#include <LSM303.h>
#include <LiquidCrystal.h>
#include <Bounce.h>
#include <EEPROM.h>
#include <EEPROManything.h>

#define filterSamples 5  //filterSamples should be an odd number, no smaller than 3

int headingOffset = 0;
int pitchOffset = 0;
int rollOffset = 0;

LSM303 compass;
LSM303::vector running_min_m = {2047, 2047, 2047}, running_max_m = {-2048, -2048, -2048}, running_min_a = {2047, 2047, 2047}, running_max_a = {-2048, -2048, -2048};
LSM303::vector min_m = {2047, 2047, 2047}, max_m = {-2048, -2048, -2048}, min_a = {2047, 2047, 2047}, max_a = {-2048, -2048, -2048};

int mx_smoothArray [filterSamples];  //Array for holding raw sensor values
int my_smoothArray [filterSamples];  //Array for holding raw sensor values
int mz_smoothArray [filterSamples];  //Array for holding raw sensor values
int ax_smoothArray [filterSamples];  //Array for holding raw sensor values
int ay_smoothArray [filterSamples];  //Array for holding raw sensor values
int az_smoothArray [filterSamples];  //Array for holding raw sensor values

//LiquidCrystal lcd(12, 11, 10, 9, 8, 6);
LiquidCrystal lcd(6, 5, 8, 10, 9, 11);

//Set up the structure for using EEPROM
struct config_t
{
    char verify[8];
    int EEmx_max;
    int EEmx_min;
    int EEmy_max;
    int EEmy_min;
    int EEmz_max;
    int EEmz_min;
    int EEax_max;
    int EEax_min;
    int EEay_max;
    int EEay_min;
    int EEaz_max;
    int EEaz_min;
} configuration;

//Connect my buttons to pins
const int leftButtonPin = A3;
const int centerButtonPin = A4;
const int rightButtonPin = A2;

long sensorRate = 100;  // set the delay between sensor samples in millis
long previousMillis = 0;  //count the millis since the last loop

float myHeading = 0;
float myBackshot = 180;
float myPitch = 0;
float myRoll = 0;

int Action; int menu=0; int submenu=0; bool laserStatus=0;

void calc_heading(float &myHeading, float &myBackshot);
void calc_pitch_roll(float &myPitch, float &myRoll);
void myMenu(int Action);

// Set a bounce object with a 15 milli debounce time
Bounce bounceLeft = Bounce(leftButtonPin, 15);
Bounce bounceCenter = Bounce(centerButtonPin, 15);
Bounce bounceRight = Bounce(rightButtonPin, 15);

void setup() {
  //Initialize serial output to computer
  Serial.begin(9600);
  Wire.begin();
  
  pinMode(leftButtonPin, INPUT);
  pinMode(centerButtonPin, INPUT);
  pinMode(rightButtonPin, INPUT);
  
  //Initialize the compass
  compass.init();
  compass.enableDefault();
  
  //Set the laser to keep pin drift
  pinMode(4, OUTPUT);
  digitalWrite(4,laserStatus);
  
  //Get the calibration values from memory or defaults
  readCalMemory();
  
  //Set up the LCD's number of columns and rows: 
  lcd.begin(8, 2);
  // Print a message to the LCD.
  lcd.print("..Init..");
}

void loop() {
  unsigned long currentMillis = millis();
  
  //Update debouncer
  boolean leftButtonChanged = bounceLeft.update();
  boolean centerButtonChanged = bounceCenter.update();
  boolean rightButtonChanged = bounceRight.update();
  
  //Check for button presses
  if ( bounceCenter.fallingEdge() ) {
    myMenu(2);
  }
  else if ( bounceLeft.fallingEdge() ) {
    myMenu(1);
  }
  else if ( bounceRight.fallingEdge() ) {
    myMenu(3);
  }
  
  //actions here are only done on intervals over the sensorRate set above
  if(currentMillis - previousMillis > sensorRate) {
    //calc_heading(myHeading, myBackshot);
    //calc_pitch_roll(myPitch, myRoll);
    myMenu(0); //no input
    //userInterface(myHeading, myBackshot, myPitch, myRoll);
    previousMillis = currentMillis;  //reset the timer
  }
}

void readCalMemory() {
  //Read the EEPROM
  EEPROM_readAnything(0, configuration);
  
  //Check whether this contains my verify bit
  if ( configuration.verify == "AdamSet" ) {
    min_m.x = configuration.EEmx_min;
    max_m.x = configuration.EEmx_max;
    min_m.y = configuration.EEmy_min;
    max_m.y = configuration.EEmy_max;
    min_m.z = configuration.EEmz_min;
    max_m.z = configuration.EEmz_max;
    min_a.x = configuration.EEax_min;
    max_a.x = configuration.EEax_max;
    min_a.y = configuration.EEay_min;
    max_a.y = configuration.EEay_max;
    min_a.z = configuration.EEaz_min;
    max_a.z = configuration.EEaz_max;
  }
  else {
  //If we didn't get a reading then we set the mins to the defaults
  min_m.x = -572; min_m.y = -809; min_m.z = -416;
  max_m.x = +738; max_m.y = +303; max_m.z = +605;
  min_a.x = -1081; min_a.y = -1048; min_a.z = -1028;
  max_a.x = +1008; max_a.y = +1022; max_a.z = +1043;
  
  } //end default settings
  
  //using code from http://playground.arduino.cc/Code/EEPROMWriteAnything
}

void myMenu(int Action) {
//Pick our menu location
  
//////Heading
  if (menu == 0) { //Heading
    //Pick our actions and/or submenu
    if (submenu == 0) { //run
      //right and left do different if running versus frozen
      if (Action == 1) { //left
        menu = 4;
        submenu = 0;
      }
      else if (Action == 3) { //right
        menu = 1;
        submenu = 0;
      }
      else if (Action == 2) { //center
        submenu = 1; //freeze
      }
      else {
        //Calculate new values
        calc_heading(myHeading, myBackshot);
        calc_pitch_roll(myPitch, myRoll);
        
        //Display the Heading and Backshot
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("Hd:");
        lcd.print((float)myHeading,1);
        lcd.setCursor(0, 1);
        lcd.print("Bk:");
        lcd.print((float)myBackshot,1);
      }
    }
    else if (submenu == 1) { //freeze
      //right and left do different if running versus frozen
      if (Action == 1) { //left
        menu = 1;
        submenu = 1;
      }
      else if (Action == 3) { //right
        menu = 1;
        submenu = 1;
      }
      else if (Action == 2) { //center
        submenu = 0; //unfreeze
      }
      else {
        //Do not recacluate values
        //Display the Heading and Backshot
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("Hd:");
        lcd.print((float)myHeading,1);
        lcd.setCursor(0, 1);
        lcd.print("Bk:");
        lcd.print((float)myBackshot,1);
      }
    }
  }
  
//////Pitch
  else if (menu == 1) {
    //Pick our actions and/or submenu
    if (submenu == 0) { //run
      //right and left do different if running versus frozen
      if (Action == 1) { //left
        menu = 0;
        submenu = 0;
      }
      else if (Action == 3) { //right
        menu = 2;
        submenu = 0;
      }
      else if (Action == 2) { //center
        submenu = 1; //freeze
      }
      else {
        //Calculate new values
        calc_heading(myHeading, myBackshot);
        calc_pitch_roll(myPitch, myRoll);
        
        //Display the Heading and Backshot
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("Pt:");
        lcd.print((float)myPitch,1);
        lcd.setCursor(0, 1);
        lcd.print("Rl:");
        lcd.print((float)myRoll,1);
      }
    }
    else if (submenu == 1) { //freeze
      //right and left do different if running versus frozen
      if (Action == 1) { //left
        menu = 0;
        submenu = 1;
      }
      else if (Action == 3) { //right
        menu = 0;
        submenu = 1;
      }
      else if (Action == 2) { //center
        submenu = 0; //unfreeze
      }
      else {
        //Do not recacluate values
        //Display the Heading and Backshot
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("Pt:");
        lcd.print((float)myPitch,1);
        lcd.setCursor(0, 1);
        lcd.print("Rl:");
        lcd.print((float)myRoll,1);
      }
    }
  }
    
//////Laser Toggle
  else if (menu == 2) {
    //Pick our actions and/or submenu
    if (Action == 1) { //left
      menu = 1;
      submenu = 0;
    }
    else if (Action == 3) { //right
      menu = 3;
      submenu = 0;
    }
    else if (Action == 2) { //center
      submenu = 1;
    }
    else if (submenu == 0) {
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Toggle ");
        lcd.setCursor(0, 1);
        lcd.print(" Laser? ");
    }
    else if (submenu == 1) {
      laserStatus = !laserStatus; //toggle the variable
      digitalWrite(4,laserStatus);
      submenu = 0; //return to menu item automatically
    }
  }
    
//////Calibrate Magnetometer
  else if (menu == 3) {
    //Pick our actions and/or submenu
    
    if (submenu == 0) {
      if (Action == 2) { //center
        submenu = 1;
      }
      else if (Action == 1) { //left
        menu = 2;
        submenu = 0;
      }
      else if (Action == 3) { //right
        menu = 4;
        submenu = 0;
      }
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("  Cal.  ");
        lcd.setCursor(0, 1);
        lcd.print("  Mag?  ");
    }
    else if (submenu == 1) { //Mx
      if (Action == 1) { //left
        submenu = 5;
      }
      else if (Action == 3) { //right
        submenu = 2;
      }
      else if (Action == 2) { //center
        //reset this variable
        running_min_m.x = 0;
        running_max_m.x = 0;
      }
      else {
        compass.read();
        running_min_m.x = min(running_min_m.x, compass.m.x);
        running_max_m.x = max(running_max_m.x, compass.m.x);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("Xmax");
        lcd.print(running_max_m.x);
        lcd.setCursor(0, 1);
        lcd.print("Xmin");
        lcd.print(running_min_m.x);
      }
    }
    else if (submenu == 2) { //My
      if (Action == 1) { //left
        submenu = 1;
      }
      else if (Action == 3) { //right
        submenu = 3;
      }
      else if (Action == 2) { //center
        //reset this variable
        running_min_m.y = 0;
        running_max_m.y = 0;
      }
      else {
        compass.read();
        running_min_m.y = min(running_min_m.y, compass.m.y);
        running_max_m.y = max(running_max_m.y, compass.m.y);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("Ymax");
        lcd.print(running_max_m.y);
        lcd.setCursor(0, 1);
        lcd.print("Ymin");
        lcd.print(running_min_m.y);
      }
    }
    else if (submenu == 3) { //Mz
      if (Action == 1) { //left
        submenu = 2;
      }
      else if (Action == 3) { //right
        submenu = 4;
      }
      else if (Action == 2) { //center
        //reset this variable
        running_min_m.z = 0;
        running_max_m.z = 0;
      }
      else {
        compass.read();
        running_min_m.z = min(running_min_m.z, compass.m.z);
        running_max_m.z = max(running_max_m.z, compass.m.z);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("Zmax");
        lcd.print(running_max_m.z);
        lcd.setCursor(0, 1);
        lcd.print("Zmin");
        lcd.print(running_min_m.z);
      }
    }
    else if (submenu == 4) { //cancel
      if (Action == 1) { //left
        submenu = 3;
      }
      else if (Action == 3) { //right
        submenu = 5;
      }
      else if (Action == 2) { //center
        submenu = 0;
      }
      else {
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Cancel ");
        lcd.setCursor(0, 1);
        lcd.print("   ??   ");
      }
    }
    else if (submenu == 5) {  //accept
      if (Action == 1) { //left
        submenu = 4;
      }
      else if (Action == 3) { //right
        submenu = 1;
      }
      else if (Action == 2) { //center to accept
        configuration = (config_t){ "Adamset" , running_min_m.x , running_max_m.x , running_min_m.y , running_max_m.y , running_min_m.z , running_max_m.z , min_a.x , max_a.x , min_a.y , max_a.y , min_a.z , max_a.z };
        EEPROM_writeAnything(0, configuration); //write to EEPROM starting at address 0
        min_m.x = running_min_m.x; 
        max_m.x = running_max_m.x;
        min_m.y = running_min_m.y;
        max_m.y = running_max_m.y; 
        min_m.z = running_min_m.z;
        max_m.z = running_max_m.z;
        submenu = 0;
      }
      else {
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Accept ");
        lcd.setCursor(0, 1);
        lcd.print("   ??   ");
      }
    }
  }
    
//////Calibrate Accelerometer
  else if (menu == 4) {
    //Pick our actions and/or submenu
    if (submenu == 0) {
      if (Action == 1) { //left
        menu = 3;
        submenu = 0;
      }
      else if (Action == 3) { //right
        menu = 0;
        submenu = 0;
      }
      else if (Action == 2) { //center
        submenu = 1;  
      }
      else{
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print("  Cal.  ");
        lcd.setCursor(0, 1);
        lcd.print(" Accel? ");
      }
    }
    else if (submenu == 1) { //ax min
      if (Action == 1) { //left
        submenu = 8;
      }
      else if (Action == 3) { //right
        submenu = 2;
      }
      else if (Action == 2) { //center
        running_min_a.x = 0;
      }
      else {
        compass.read();
        running_min_a.x = min(running_min_a.x, compass.a.x);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Ax min  ");
        lcd.setCursor(0, 1);
        lcd.print("  ");
        lcd.print(running_min_a.x);
      }
    }
    else if (submenu == 2) { //ax max
      if (Action == 1) { //left
        submenu = 1;
      }
      else if (Action == 3) { //right
        submenu = 3;
      }
      else if (Action == 2) { //center
        running_max_a.x = 0;
      }
      else {
        compass.read();
        running_max_a.x = max(running_max_a.x, compass.a.x);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Ax max  ");
        lcd.setCursor(0, 1);
        lcd.print("  ");
        lcd.print(running_max_a.x);
      }
    }
    else if (submenu == 3) { //ay min
      if (Action == 1) { //left
        submenu = 2;
      }
      else if (Action == 3) { //right
        submenu = 4;
      }
      else if (Action == 2) { //center
        running_min_a.y = 0;
      }
      else {
        compass.read();
        running_min_a.y = min(running_min_a.y, compass.a.y);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Ay min  ");
        lcd.setCursor(0, 1);
        lcd.print("  ");
        lcd.print(running_min_a.y);
      }
    }
    else if (submenu == 4) { //ay max
      if (Action == 1) { //left
        submenu = 3;
      }
      else if (Action == 3) { //right
        submenu = 5;
      }
      else if (Action == 2) { //center
        running_max_a.y = 0;
      }
      else {
        compass.read();
        running_max_a.y = max(running_max_a.y, compass.a.y);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Ay max  ");
        lcd.setCursor(0, 1);
        lcd.print("  ");
        lcd.print(running_max_a.y);
      }
    }
    else if (submenu == 5) {  //az min
      if (Action == 1) { //left
        submenu = 4;
      }
      else if (Action == 3) { //right
        submenu = 6;
      }
      else if (Action == 2) { //center
        running_min_a.z = 0;
      }
      else {
        compass.read();
        running_min_a.z = min(running_min_a.z, compass.a.z);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Az min  ");
        lcd.setCursor(0, 1);
        lcd.print("  ");
        lcd.print(running_min_a.z);
      }
    }
    else if (submenu == 6) {  //az max
      if (Action == 1) { //left
        submenu = 5;
      }
      else if (Action == 3) { //right
        submenu = 7;
      }
      else if (Action == 2) { //center
        running_max_a.z = 0;
      }
      else {
        compass.read();
        running_max_a.z = max(running_max_a.z, compass.a.z);
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Az max  ");
        lcd.setCursor(0, 1);
        lcd.print("  ");
        lcd.print(running_max_a.z);
      }
    }
    else if (submenu == 7) {  //cancel
      if (Action == 1) { //left
        submenu = 6;
      }
      else if (Action == 3) { //right
        submenu = 8;
      }
      else if (Action == 2) { //center
        submenu = 0;
      }
      else {
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Cancel ");
        lcd.setCursor(0, 1);
        lcd.print("   ??   ");
      }
    }
    else if (submenu == 8) {  //accept
      if (Action == 1) { //left
        submenu = 7;
      }
      else if (Action == 3) { //right
        submenu = 1;
      }
      else if (Action == 2) { //center to accept
        configuration = (config_t){ "Adamset" , min_m.x , max_m.x , min_m.y , max_m.y , min_m.z , max_m.z , running_min_a.x , running_max_a.x , running_min_a.y , running_max_a.y , running_min_a.z , running_max_a.z };
        EEPROM_writeAnything(0, configuration); //write to EEPROM starting at address 0
        min_a.x = running_min_a.x; 
        max_a.x = running_max_a.x;
        min_a.y = running_min_a.y;
        max_a.y = running_max_a.y; 
        min_a.z = running_min_a.z;
        max_a.z = running_max_a.z;
        submenu = 0;
      }
      else {
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Accept ");
        lcd.setCursor(0, 1);
        lcd.print("   ??   ");
      }
    } 
  }
  else if (menu == 5) {
    //Pick our actions and/or submenu
    if (Action == 1) { //left
      menu = 4;
      submenu = 0;
    }
    else if (Action == 3) { //right
      menu = 0;
      submenu = 0;
    }
    else if (submenu == 0) {
        lcd.setCursor(0, 0);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 1);
        lcd.print("        "); //Clear only this line
        lcd.setCursor(0, 0);
        lcd.print(" Test:  ");
        lcd.setCursor(0, 1);
        lcd.print(configuration.verify);
    }
  }
}

//digitalSmooth was taken from http://playground.arduino.cc/Main/DigitalSmooth
int digitalSmooth(int rawIn, int *sensSmoothArray){     // "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
  int j, k, temp, top, bottom;
  long total;
  static int i;
 // static int raw[filterSamples];
  static int sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples;    // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;                 // input new data into the oldest slot

  // Serial.print("raw = ");

  for (j=0; j<filterSamples; j++){     // transfer data array into anther array for sorting and averaging
    sorted[j] = sensSmoothArray[j];
  }

  done = 0;                // flag to know when we're done sorting              
  while(done != 1){        // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++){
      if (sorted[j] > sorted[j + 1]){     // numbers are out of order - swap
        temp = sorted[j + 1];
        sorted [j+1] =  sorted[j] ;
        sorted [j] = temp;
        done = 0;
      }
    }
  }

/*
  for (j = 0; j < (filterSamples); j++){    // print the array to debug
    Serial.print(sorted[j]); 
    Serial.print("   "); 
  }
  Serial.println();
*/

  // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
  bottom = max(((filterSamples * 15)  / 100), 1); 
  top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for ( j = bottom; j< top; j++){
    total += sorted[j];  // total remaining indices
    k++; 
    // Serial.print(sorted[j]); 
    // Serial.print("   "); 
  }

//  Serial.println();
//  Serial.print("average = ");
//  Serial.println(total/k);
  return total / k;    // divide by number of samples
}

void calc_heading(float &myHeading, float &myBackshot) {
  //algorithyms modified from LSM303 library to include digital filter
  LSM303::vector from = {0,-1,0};
  compass.read(); //We only read this once per cycle. Either heading or pitch calc.
  
  // smooth our raw data before calculating our output
    compass.m.x = digitalSmooth(compass.m.x, mx_smoothArray);
    compass.m.y = digitalSmooth(compass.m.y, my_smoothArray);
    compass.m.z = digitalSmooth(compass.m.z, mz_smoothArray);
    compass.a.x = digitalSmooth(compass.a.x, ax_smoothArray);
    compass.a.y = digitalSmooth(compass.a.y, ay_smoothArray);
    compass.a.z = digitalSmooth(compass.a.z, az_smoothArray);
  
  // shift and scale to avoid offset and skew
    compass.m.x = (compass.m.x - min_m.x) / (max_m.x - min_m.x) * 2 - 1.0;
    compass.m.y = (compass.m.y - min_m.y) / (max_m.y - min_m.y) * 2 - 1.0;
    compass.m.z = (compass.m.z - min_m.z) / (max_m.z - min_m.z) * 2 - 1.0;
    
    compass.a.x = compass.a.x + ((-1)*(max_a.x+min_a.x)/2);
    compass.a.y = compass.a.y + ((-1)*(max_a.y+min_a.y)/2);
    compass.a.z = compass.a.z + ((-1)*(max_a.z+min_a.z)/2);

    LSM303::vector temp_a = compass.a;
    // normalize
    LSM303::vector_normalize(&temp_a);
    //vector_normalize(&m);

    // compute E and N
    LSM303::vector E;
    LSM303::vector N;
    LSM303::vector_cross(&compass.m, &temp_a, &E);
    LSM303::vector_normalize(&E);
    LSM303::vector_cross(&temp_a, &E, &N);

    // compute heading
    float heading = atan2(LSM303::vector_dot(&E, &from), LSM303::vector_dot(&N, &from)) * 180 / M_PI;
    if (heading < 0) heading += 360;
  
  //Account for orientation of chip relative to laser
  if(heading < 270) {
    heading = heading + 90;
  }
  else {
    heading = heading + 90 - 360;
  }
  
  myHeading = heading + headingOffset;  //Offset is for mechanical laser offset from chip
  
  if(myHeading < 180) {
    myBackshot = myHeading + 180;
  }
  else {
    myBackshot = myHeading - 180;
  }
}

void calc_pitch_roll(float &myPitch, float &myRoll) {
  float ax_val, ay_val, az_val, pitch, roll;
  float center_a_x, center_a_y, center_a_z;
  
  //Set our deviation from center from experimental data
  center_a_x = -120;
  center_a_y = 30;
  center_a_z = 37;
  
  //compass.read(); //We only read this once per cycle. Either heading or pitch calc.
  
  // Lets get the deviations from our baseline
   ax_val = (float)compass.a.x-(float)center_a_x;
   ay_val = (float)compass.a.y-(float)center_a_y;
   az_val = (float)compass.a.z-(float)center_a_z;
   
  // lets scale the factors so that they are even
   //Need to fill in code here
  
  pitch = -(atan2((ax_val),sqrt((ay_val*ay_val)+(az_val*az_val))))*180/(3.14159265);
  roll = -atan2((ay_val),sqrt((ax_val*ax_val)+(az_val*az_val)))*180/(3.14159265);
  myPitch = pitch + pitchOffset;  //Offset is for mechanical laser offset from chip
  myRoll = roll + rollOffset;  //Offset is for mechanical laser offset from chip
}
