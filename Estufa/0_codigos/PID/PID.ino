#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "ArduPID.h"

// ******************** GLOBAL VARIABLES  ******************** //


// Temperature ranges
double T_segments[5] = {75.3571,116.1735, 156.9898, 197.8061,34.5408}; //
double t_segments[5] = {3, 3 ,3 ,3, 3 };        // minutes
double StartTime;

// SPI Thermocouple

#define MAXDO   6
#define MAXCS   7
#define MAXCLK  5
double setTemp = 0;                   // Target Temp for PID
double targetTemp = 0;                // Target Temp from loop
double currentTemp = 0;               // Current Temp
double elapsedTime = 0;        // Elapsed time since initialization
double elapsedTimeZero = millis();

Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// Relay

#define relay 12
int relay_status = 0;

// PID controll, the output is the time during which the relay will be HIGH

double p = 0.1;
double i = 0.0001666666666666667;
double d = 0.15;

int WindowSize = 10000;        // Lenght of the window used to analogize the output in milis
double WindowElapsedTime;;     // Milis elapsed in window
double WindowOnTime;           // Milis that the relay will be HIGH
double WindowElapsedTimeZero;


double setpoint=0, input, output; // Variables for PID control
ArduPID myController;

// For serial comunication

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

char messageFromPC[numChars] = {0};
int integerFromPC = 0;
float floatFromPC = 0.0;

boolean newData = false;

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

  // ***** Shut down realy ***** //
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  relay_status = 0;

  // ***** Initialize PID control ***** //
  myController.begin(&input, &output, &setpoint, p, i, d);

  // myController.reverse()               // Uncomment if controller output is "reversed"
  // myController.setSampleTime(10);      // OPTIONAL - will ensure at least 10ms have past between successful compute() calls
  myController.setOutputLimits(0, 1);
  //myController.setBias(255.0 / 2.0);
  myController.setWindUpLimits(-1000, 1000); // Groth bounds for the integral term to prevent integral wind-up

  myController.start();
  // myController.reset();               // Used for resetting the I and D terms - only use this if you know what you're doing
  // myController.stop();                // Turn off the PID controller (compute() will not do anything until start() is called)


  // ***** Initial time ***** //
  elapsedTime = 0;
  elapsedTimeZero = millis();
  WindowElapsedTime = 0;
}

// ******************** FUNCTIONS ******************** //

void check_thermomodule(){

    bool running = false;

    while (!running) {
      double internal_temp = thermocouple.readInternal();
      if (internal_temp >= 125.0){
        running = false;
        Serial.println("Thermocouple module is overheated");
        Serial.print("Internal Temperature = ");
        Serial.println(internal_temp);
        digitalWrite(relay, LOW);
        Serial.println("Relay turned off");
      }
      else if (internal_temp <= -40.0){
        running = false;
        Serial.println("Thermocouple module is overcooled");
        Serial.print("Internal Temperature = ");
        Serial.println(internal_temp);
        digitalWrite(relay, HIGH);
        Serial.println("Relay turned on");
      }
      else {
        running = true;
      }
    }
  };

void printVariables(){

  currentTemp = (thermocouple.readCelsius()-6.15)/0.98;
  elapsedTime = (millis() - elapsedTimeZero)/1000;

  Serial.print("Temp: ");
  Serial.print(currentTemp);
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

  setTemp = floatFromPC;

  Serial.print("Temperature set at:");
  Serial.print(setTemp);
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
}

void parseData() { // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC

    integerFromPC = atoi(messageFromPC);    // convert message to an integer

    floatFromPC = atof(messageFromPC);     // convert message to a float

}

void active_relay(){
  if (relay_status == 0) {
    Serial.println("Relay HIGH");
    digitalWrite(relay, HIGH);
    relay_status = 1;
  }
}

void desactive_relay(){
  if (relay_status == 1) {
    Serial.println("Relay LOW");
    digitalWrite(relay, LOW);
    relay_status = 0;
  }
}


// ******************** LOOP ******************** //

void PID(){


  // Check if temp module is ok
  check_thermomodule();

  // Read temperature and print
  printVariables();

  // Update elapsed time
  WindowElapsedTime = millis()-WindowElapsedTimeZero;

  if (WindowElapsedTime >= WindowSize){

    // ***** PID calculations ***** //

    Serial.print("PID -> ");

    input = (currentTemp);
    Serial.print("Input: ");
    Serial.print(input);

    setTemp = targetTemp;
    setpoint = (setTemp);
    Serial.print ("// Setpoint: ");
    Serial.print(setpoint);

    myController.compute();
    Serial.print(" // Output: ");
    Serial.println(output);

    WindowOnTime = output * WindowSize;   // Convert percentage to millis
    WindowElapsedTimeZero = millis();                // Start new window

  }
  else {
    if (WindowElapsedTime<=WindowOnTime){
      active_relay();       // HIGH if not
    }
    else {
      desactive_relay();    // LOW if not
    }
  }

};

void loop(){

  for (int i = 0; i < 5; i++) {
    targetTemp = T_segments[i];
    bool resetPID = true;
    Serial.print("Ramp from ");
    Serial.print(currentTemp);
    Serial.print(" to ");
    Serial.println(targetTemp);

    while (currentTemp <= targetTemp){
          if (currentTemp >= targetTemp -10.0 & resetPID){
            myController.reset(); // reset i and d therms
            resetPID = false;
            Serial.println("PID reseted");
          }
          PID();
          delay(1000);
        };

    if (currentTemp >= targetTemp){
        double segmentZeroTime = millis();
        Serial.print("Segment on");
        Serial.println(targetTemp);
        while(millis() - segmentZeroTime < t_segments[i] *60*1000){
          PID();
          delay(1000);
        };
      };
  };
};
