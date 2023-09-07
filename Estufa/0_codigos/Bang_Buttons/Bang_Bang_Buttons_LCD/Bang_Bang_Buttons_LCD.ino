#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Constants for temperature control
const int temperatureSensorPin = A0; // Analog pin for the temperature sensor
const int relayPin = 8;              // Digital pin for the relay

// Constants for LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address and dimensions

// Constants for buttons
const int increaseButtonPin = 2;     // Button to increase target temperature
const int decreaseButtonPin = 3;     // Button to decrease target temperature

// Variables
float currentTemperature = 0.0;
float targetTemperature = 25.0;      // Default target temperature in Celsius
bool ovenOn = false;

void setup() {
  // Initialize the LCD display
  lcd.init();
  lcd.backlight();
  
  // Initialize the relay pin
  pinMode(relayPin, OUTPUT);
  
  // Initialize the buttons as inputs
  pinMode(increaseButtonPin, INPUT_PULLUP);
  pinMode(decreaseButtonPin, INPUT_PULLUP);
  
  // Attach interrupt service routines to the buttons
  attachInterrupt(digitalPinToInterrupt(increaseButtonPin), increaseTemperature, FALLING);
  attachInterrupt(digitalPinToInterrupt(decreaseButtonPin), decreaseTemperature, FALLING);
  
  // Print initial display
  updateDisplay();
}

void loop() {
  // Read the current temperature
  currentTemperature = readTemperature();
  
  // Control the oven using bang-bang control
  if (currentTemperature < targetTemperature) {
    digitalWrite(relayPin, HIGH); // Turn on the relay
    ovenOn = true;
  } else {
    digitalWrite(relayPin, LOW); // Turn off the relay
    ovenOn = false;
  }
  
  // Update the display
  updateDisplay();
  
  delay(1000); // Delay for one second
}

void increaseTemperature() {
  targetTemperature += 1.0;
}

void decreaseTemperature() {
  targetTemperature -= 1.0;
}

float readTemperature() {
  // Replace this with your temperature sensor reading code
  // Example: return analogRead(temperatureSensorPin) * 0.48876; // Convert analog value to Celsius
}

void updateDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Target: ");
  lcd.print(targetTemperature);
  lcd.print(" C  ");
  
  lcd.setCursor(0, 1);
  lcd.print("Current: ");
  lcd.print(currentTemperature);
  lcd.print(" C  ");
  
  lcd.setCursor(15, 1);
  lcd.print(ovenOn ? "ON" : "OFF");
}
