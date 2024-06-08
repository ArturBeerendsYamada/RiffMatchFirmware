#ifndef PIN_CONFIG_H
# define PIN_CONFIG_H

#include "Keypad_Matrix.h"

#define ROWS 5
#define COLS 5
#define OCT_PIN 36
#define SP_PIN 39
#define POT_PIN 35
#define MODE_PIN 34
#define DEBOUNCE_TIME  50 // the debounce time in millisecond, increase this time if it still chatters
#define START 1
#define STOP 0
#define NUMPIXELS   25
#define LEDPIN   13

enum Mode {
  APRENDIZADO,
  LIVRE,
  TREINO
};

const uint8_t keys[ROWS][COLS] = {
    {12, 13, 14, 15, 16},
    {17, 18, 19, 20, 21},
    {22, 23, 24, 25, 26},
    {27, 28, 29, 30, 31},
    {32, 33, 34, 35, 36}};


const byte rowPins[ROWS] = {17, 16, 4, 2, 15}; //connect to the row pinouts of the keypad
const byte colPins[COLS] = {27, 26, 25, 33, 32}; //connect to the column pinouts of the keypad

int volumeLevel;

#endif