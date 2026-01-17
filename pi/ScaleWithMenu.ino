#include <Arduino.h>
#include "HX711.h"

const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

HX711 scale;
String InputString;
bool InputComplete = false;

void RunScale() {
  Serial.println("Waiting for user command 'c'...");
 
  // This loop will run indefinitely until serial data is available.
  while (!Serial.available()) {
    long reading = scale.get_units(10);
    Serial.println("Raw Reading: ");
    Serial.println(reading);
    Serial.println(" wait for 'c'");
  }
  
  // A command has been sent, now we read it.
  char command = Serial.read();

  if (command == 'c' || command == 'C') {
    Serial.println("Command 'c' received. Proceeding...");
  } else {
    Serial.print("Received '");
    Serial.print(command);
    Serial.println("'. Incorrect command. Please restart the program.");
    // An optional delay can be added to prevent spamming the serial monitor.
    delay(100); 
  }
}


void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(1);
  scale.tare();

  // Initial prompt, only printed once in setup
  Serial.println("Scale initialized. Place known weight on scale.\n");
  Serial.println("t for tare, c for calibrate, r for reset scale, e for run scale. Press enter:\n");
}



void loop() {
  // Check for available serial data
  if (Serial.available() > 0) {
    char inChar = Serial.read();

    if (inChar != '\n' && inChar != '\r') {
      InputString += inChar;
    } else {
      InputComplete = true;
    }
  }

  // Process the command only when a full line of input has been received
  if (InputComplete) {
    char command = InputString.charAt(0);

    switch(command){
      case 't':
        Serial.println("Doing the Tare.");
        scale.tare();
        break;
      case 'c':
        Serial.println("Doing the Calibrate.");
        break;
      case 'r':
        Serial.println("Doing the Scale Reset.");
        scale.tare();
        scale.set_scale(1);
        break;
      case 'e':
        Serial.println("Running Scale...");
        RunScale();
        // This is a good place to start the continuous scale reading loop.
        // For example, you could have a separate function to read the scale.
        break;
      default:
        Serial.println("Invalid command. Please enter 't', 'c', 'r', or 'e'.");
        break;
    }

    // Reset variables for the next command
    InputComplete = false;
    InputString = "";

    // Print the prompt again after a command is processed
    Serial.println("Waiting For Input,t for tare, c for calibrate, r for reset scale, e for run scale. Press enter:");
  }
}
