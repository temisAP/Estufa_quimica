#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Code created by Andres Pedraza & Daniel del Rio

// ******************** PIN MAP  ******************** //

//
// D1
// D2 Increase Button
// D3 Decrease Button
// D4
// D5 Thermocouple module
// D6 Thermocouple module
// D7 Thermocouple module
// D8
// D9
// D10
// D11
// D12 Relay 

// ******************** GLOBAL VARIABLES  ******************** //

// SPI Thermocouple
#define MAXDO   6
#define MAXCS   7
#define MAXCLK  5

double currentTemp = 0;    // current temperature
double targetTemp  = 0;    // target temperature 
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// Relay
#define relay 12
int relay_state = 0;

// Time
double elapsedTime;

// For serial comunication
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing
char messageFromPC[numChars] = {0};
int integerFromPC = 0;
float floatFromPC = 0.0;
boolean newData = false;

// Constants for LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address and dimensions

// Constants for buttons
const int increaseButtonPin = 2;     // Button to increase target temperature
const int decreaseButtonPin = 3;     // Button to decrease target temperature


// ******************** SETUP ******************** //

void setup() {

  // ***** Initialize thermocouple ***** //
  Serial.begin(9600);
  while (!Serial) delay(1); // wait for Serial on Leonardo/Zero, etc
  Serial.println("MAX31855 test");
  delay(500);   // wait for MAX chip to stabilize
  Serial.print("Initializing sensor...");
  if (!thermocouple.begin()) {
    Serial.println("ERROR.");
    while (1) delay(10);
  }
  Serial.println("DONE.");

  // ***** Initialize realy ***** //
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW); // Shut down
  relay_state = 0;

  // ***** Initialize the LCD display ***** //
  lcd.init();
  lcd.backlight();

  // Print initial display
  updateDisplay();

  // ***** Set up buttons ***** //

  // Initialize the buttons as inputs
  pinMode(increaseButtonPin, INPUT_PULLUP);
  pinMode(decreaseButtonPin, INPUT_PULLUP);
  
  // Attach interrupt service routines to the buttons
  attachInterrupt(digitalPinToInterrupt(increaseButtonPin), increaseTemperature, FALLING);
  attachInterrupt(digitalPinToInterrupt(decreaseButtonPin), decreaseTemperature, FALLING);
  

};
// ******************** FUNCTIONS ******************** //

void check_thermomodule(){
    bool running = false;
    while (!running) {
      double internal_temp = thermocouple.readInternal();
      if (internal_temp >= 125.0){
        digitalWrite(relay, LOW);
        running = false;
        
        Serial.println("Thermocouple module is overheated");
        Serial.print("Internal Temperature = ");
        Serial.println(internal_temp);
        Serial.println("Relay turned off");

        lcd.setCursor(0, 0);
        lcd.print("Module overheated");

        lcd.setCursor(0, 1);
        lcd.print("Tint=");
        lcd.print(internal_temp);
        lcd.print("C;  ");
        lcd.print(relay_state ? "ON" : "OFF");
        
        lcd.setCursor(15, 1);

      }
      else if (internal_temp <= -40.0){
        digitalWrite(relay, HIGH);
        running = false;
        
        Serial.println("Thermocouple module is overcooled");
        Serial.print("Internal Temperature = ");
        Serial.println(internal_temp);
        Serial.println("Relay turned on");

        lcd.setCursor(0, 0);
        lcd.print("Module overcooled");

        lcd.setCursor(0, 1);
        lcd.print("Tint=");
        lcd.print(internal_temp);
        lcd.print("C;  ");
        lcd.print(relay_state ? "ON" : "OFF");

      }
      else {
        running = true;
      }
    }
  };


  void printVariables(){
    currentTemp = thermocouple.readCelsius()-8.2; //(thermocouple.readCelsius()-6.15)/0.98;
    elapsedTime = elapsedTime + millis()/1000;
    Serial.print("Temp: ");
    Serial.print(currentTemp);
    Serial.print(" C ->");
    Serial.print("Target Temp: ");
    Serial.print(targetTemp);
    Serial.print(" C");
    Serial.print(" // ");
    Serial.print("Elapsed time: ");
    Serial.print(elapsedTime);
    Serial.println(" s");
  };

void receive(){
    recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        parseData();
        newData = false;
    }
    targetTemp = floatFromPC;
    Serial.print("Temperature set at:");
    Serial.print(targetTemp);
    Serial.println("C");
  };


  void recvWithStartEndMarkers() {
      static boolean recvInProgress = false;
      static byte ndx = 0;
      char startMarker = '<';
      char endMarker = '>';
      char rc;
      while (Serial.available() > 0 && newData == false) {
          rc = Serial.read();
          if (recvInProgress == true) {
              if (rc != endMarker) {
                  receivedChars[ndx] = rc;
                  ndx++;
                  if (ndx >= numChars) {
                      ndx = numChars - 1;
                  }
              }
              else {
                  receivedChars[ndx] = '\0'; // terminate the string
                  recvInProgress = false;
                  ndx = 0;
                  newData = true;
              }
          }
          else if (rc == startMarker) {
              recvInProgress = true;
          }
      }
  };


void parseData() { // split the data into its parts
    char * strtokIndx; // this is used by strtok() as an index
    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC
    integerFromPC = atoi(messageFromPC);    // convert message to an integer
    floatFromPC = atof(messageFromPC);     // convert message to a float
};

void active_relay(){
  if (relay_state == 0) {
    Serial.println("Relay HIGH");
    digitalWrite(relay, HIGH);
    relay_state = 1;
  }
};
void desactive_relay(){
  if (relay_state == 1) {
    Serial.println("Relay LOW");
    digitalWrite(relay, LOW);
    relay_state = 0;
  }
};
void relay_activation(){
     if (currentTemp <= targetTemp & relay_state == 0 & currentTemp < 300.00){
       Serial.println("Relay HIGH");
       digitalWrite(relay, HIGH);
       relay_state = 1;
     }
     else if (currentTemp >= targetTemp & relay_state == 1) {
         Serial.println("Relay LOW");
         digitalWrite(relay, LOW);
         relay_state = 0;
     }
};

void increaseTemperature() {
  targetTemp += 1.0;
}

void decreaseTemperature() {
  targetTemp -= 1.0;
}

void updateDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("T= ");
  lcd.print(currentTemp);
  lcd.print("C; ");
  lcd.print(relay_state ? "ON" : "OFF");

  lcd.setCursor(0, 1);
  lcd.print("Target: ");
  lcd.print(targetTemp);
  lcd.print("C  ");
  
  lcd.setCursor(15, 1);

}


// ******************** LOOP ******************** //
void loop(){
    check_thermomodule();
    printVariables();
    receive();
    relay_activation();
    updateDisplay();
    delay(1000); 
 };
