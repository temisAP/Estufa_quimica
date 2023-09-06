#include <SPI.h>
#include "Adafruit_MAX31855.h"


// ******************** GLOBAL VARIABLES  ******************** //

// SPI Thermocouple

#define MAXDO   6
#define MAXCS   7
#define MAXCLK  5

double currentTemp = 0;    // current temperature
double targetTemp  = 0;     // target temperature

Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// Relay

#define relay 12
int relay_state = 0;

// Temperature ranges
double T_segments[9] = {40, 60, 80, 100, 120, 140, 160, 180, 200}; // degrees
double t_segments[9] = {10, 10 ,10 ,10,  10,  10,  10,  10,  10 };   // minutes
double StartTime;
double elapsed_time = 0;


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

  // ***** Initialize PID control ***** //
  StartTime = millis();

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

  currentTemp = thermocouple.readCelsius();
  Serial.print("Temp: ");
  Serial.print(currentTemp);
  Serial.print("C // ");

  elapsed_time = (millis()-StartTime)/1000.0; // In seconds
  Serial.print("Elapsed time: ");
  Serial.print(elapsed_time);
  Serial.println("s");
};

void relay_activation(){
     if (currentTemp <= targetTemp & relay_state == 0){
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

// ******************** LOOP ******************** //

void loop(){

  for (int i = 0; i < 9; i++) {
    targetTemp = T_segments[i];
    Serial.print("Ramp from ");
    Serial.print(currentTemp);
    Serial.print(" to ");
    Serial.println(targetTemp);

    while (currentTemp <= targetTemp){
          check_thermomodule();
          printVariables();
          relay_activation();
          delay(1000);
        };

    if (currentTemp >= targetTemp){
        double segmentZeroTime = millis();
        Serial.print("Segment on");
        Serial.println(targetTemp);
        while(millis() - segmentZeroTime < t_segments[i] *60*1000){
          check_thermomodule();
          printVariables();
          relay_activation();
          delay(1000);
        };
      };
  };
};
