
/*
 * 
 * Example of using Keypad_Matrix with a 5x5 keypad matrix.
 * 
 */
#include <Arduino.h>
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

// Create the Keypad
Keypad_Matrix kpd = Keypad_Matrix( makeKeymap (keys), rowPins, colPins, ROWS, COLS );

int volumeLevel = 100;
int modeReading = 0;
int mode = LIVRE;
int octave_read = 0;
int octave = 4;
unsigned long previousMillis = 0;
const unsigned long interval = 200; 
const int threshold = 10;
// Variables will change:
int lastSteadyState = LOW;       // the previous steady state from the input pin
int lastFlickerableState = LOW;  // the previous flickerable state from the input pin
int currentState;                // the current reading from the input pin
int startStopState = STOP;
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled


void octaveReading() {
  static int last_reading = 0; // Para armazenar a última leitura válida
  unsigned long currentMillis = millis(); // Obtém o tempo atual em milissegundos

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Atualiza o tempo anterior

    int current_reading = analogRead(OCT_PIN); // Lê o valor analógico do pino
    // Se a diferença entre a leitura atual e a última leitura válida for menor que o threshold, consideramos a leitura estável
    if (abs(current_reading - last_reading) > threshold) {
      octave_read = current_reading;
      // Serial.println(octave_read);
      if (octave_read < 10) {
        if (octave == 6)
          octave = 6;
        else
          octave++; // Incrementa a variável octave
        Serial.print("Increment: ");
        Serial.println(octave); // Imprime o valor lido
      } else if (octave_read < 3000) {
        if (octave == 0)
          octave = 0;
        else 
          octave--; // Decrementa a variável octave
        Serial.print("Decrement: ");
        Serial.println(octave); // Imprime o valor lido
      }

      last_reading = octave_read; // Atualiza a última leitura válida
    }
  }
}

void modeFunction()
{
  modeReading = analogRead(MODE_PIN);
  if (modeReading > 4000)
    mode = LIVRE;
  else if (modeReading < 4000 && modeReading > 1000)
    mode = APRENDIZADO;
  else
    mode = TREINO;
  // Serial.println(mode);
}

void checkStartStopButton()
{
  // read the state of the switch/button:
  currentState = digitalRead(SP_PIN);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch/button changed, due to noise or pressing:
  if (currentState != lastFlickerableState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
    // save the the last flickerable state
    lastFlickerableState = currentState;
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) 
  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (lastSteadyState == HIGH && currentState == LOW)
    {
      if(startStopState)
      {
        Serial.println("Stopped");
        startStopState = 0;
      } else if (!startStopState) 
      {
        Serial.println("Started");
        startStopState = 1;
      }
    }
    // save the the last steady state
    lastSteadyState = currentState;
  }
}
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
  pinMode(SP_PIN, INPUT);
}

void loop() 
{
  volumeLevel = analogRead(POT_PIN) / 32;
  // Serial.print("Volume: ");
  // Serial.println(volumeLevel);
  kpd.scan();
  modeFunction();
  octaveReading();
  checkStartStopButton();
}