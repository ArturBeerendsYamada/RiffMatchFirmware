
/*
 * 
 * Example of using Keypad_Matrix with a 5x5 keypad matrix.
 * 
 */
#include <Arduino.h>
#include "Keypad_Matrix.h"

#define ROWS 5
#define COLS 5

const uint8_t keys[ROWS][COLS] = {
    {12, 13, 14, 15, 16},
    {17, 18, 19, 20, 21},
    {22, 23, 24, 25, 26},
    {27, 28, 29, 30, 31},
    {32, 33, 34, 35, 36}};


const byte rowPins[ROWS] = {17, 16, 4, 2, 15}; //connect to the row pinouts of the keypad
const byte colPins[COLS] = {27, 26, 25, 33, 32}; //connect to the column pinouts of the keypad

  // Create the Keypad
Keypad_Matrix kpd = Keypad_Matrix( makeKeymap (keys), rowPins, colPins, ROWS, COLS );

void keyDown (const uint8_t which)
{
  Serial.print (F("Key down: "));
  Serial.println (which);
}

void keyUp (const uint8_t which)
{
  Serial.print (F("Key up: "));
  Serial.println (which);
}


void setup() 
{
  Serial.begin (115200);
  Serial.println ("Starting.");
  kpd.begin();
  kpd.setKeyDownHandler(keyDown);
  kpd.setKeyUpHandler(keyUp);
}

void loop() 
{
  kpd.scan();
}

