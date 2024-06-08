/**
  A simple example to use ESP_VS1053_Library (plays a test sound every 3s)
  https://github.com/baldram/ESP_VS1053_Library
  If you like this project, please add a star.

  Copyright (C) 2018 Marcin Szalomski (github.com/baldram)
  Licensed under GNU GPL v3

  The circuit (example wiring for ESP8266 based board like eg. LoLin NodeMCU V3):
  --------------------------------
  | VS1053  | ESP8266 |  ESP32   |
  --------------------------------
  |   SCK   |   D5    |   IO18   |
  |   MISO  |   D6    |   IO19   |
  |   MOSI  |   D7    |   IO23   |
  |   XRST  |   RST   |   EN     |
  |   CS    |   D1    |   IO5    |
  |   DCS   |   D0    |   IO16   |
  |   DREQ  |   D3    |   IO4    |
  |   5V    |   5V    |   5V     |
  |   GND   |   GND   |   GND    |
  --------------------------------

  Note: It's just an example, you may use a different pins definition.
  For ESP32 example, please follow the link:
    https://github.com/baldram/ESP_VS1053_Library/issues/1#issuecomment-313907348

  To run this example define the platformio.ini as below.

  [env:nodemcuv2]
  platform = espressif8266
  board = nodemcuv2
  framework = arduino
  lib_deps =
    ESP_VS1053_Library

  [env:esp32dev]
  platform = espressif32
  board = esp32dev
  framework = arduino
  lib_deps =
    ESP_VS1053_Library


*/

// This ESP_VS1053_Library
// #include <VS1053.h>

// This ESP_VS1053_Library
#include <VS1053Driver.h>
#include "patches_midi/rtmidi1003b.h"
#include "patches_midi/rtmidi1053b.h"
#include "pinConfig.h"
#include <Adafruit_NeoPixel.h>

#define UNDEFINED    -1

// Wiring of VS1053 board (SPI connected in a standard way)
#ifdef ARDUINO_ARCH_ESP8266
#define VS1053_CS     D1
#define VS1053_DCS    D0
#define VS1053_DREQ   D3
#endif

#ifdef ARDUINO_ARCH_ESP32
#define VS1053_CS     5
#define VS1053_DCS    12
#define VS1053_DREQ   14
#endif

uint8_t channel = 0;
uint8_t instrument = 2;

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ, UNDEFINED, SPI);
Keypad_Matrix kpd = Keypad_Matrix( makeKeymap (keys), rowPins, colPins, ROWS, COLS );
Adafruit_NeoPixel pixels(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

int last_mode = LIVRE;
int octave_read = 0;
int octave = 4;
unsigned long previousMillis = 0;
const unsigned long interval = 200; 
const int threshold = 10;
int modeReading = 0;
int mode = LIVRE;
// Variables will change:
int lastSteadyState = LOW;       // the previous steady state from the input pin
int lastFlickerableState = LOW;  // the previous flickerable state from the input pin
int currentState;                // the current reading from the input pin
int startStopState = STOP;
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled

// noteOn and noteOff based on MP3_Shield_RealtimeMIDI demo 
// by Matthias Neeracher, Nathan Seidle's Sparkfun Electronics example respectively

//Send a MIDI note-on message.  Like pressing a piano key
void noteOn(uint8_t channel, uint8_t note, uint8_t attack_velocity) {
  player.sendMidiMessage( (0x90 | channel), note, attack_velocity);
}

//Send a MIDI note-off message.  Like releasing a piano key
void noteOff(uint8_t channel, uint8_t note, uint8_t release_velocity) {
  player.sendMidiMessage( (0x80 | channel), note, release_velocity);
}

void keyDownLivre (const uint8_t which)
{
  Serial.print (F("Key down: "));
  Serial.println (which);
  noteOn(channel, (which + 12 * octave), 45);
  pixels.setPixelColor((uint16_t)(which - 12), pixels.Color(0, 150, 0));
  pixels.show();
  Serial.println ((uint16_t)(which - 12));
}

void keyUpLivre (const uint8_t which)
{
  Serial.print (F("Key up: "));
  Serial.println (which);
  noteOff(channel, (which + 12 * octave), 45);
  pixels.setPixelColor((uint16_t)(which - 12), pixels.Color(0, 0, 0));
  pixels.show();
  Serial.println ((uint16_t)(which - 12));
}

void keyDownTreino (const uint8_t which)
{
  Serial.print (F("Key down: "));
  Serial.println (which);
  Serial.println("Waiting Features...");
}

void keyUpTreino (const uint8_t which)
{
  Serial.print (F("Key up: "));
  Serial.println (which);
  Serial.println("Waiting Features...");
}

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
  if (last_mode != mode)
  {
    if (mode == TREINO)
    {
      kpd.setKeyDownHandler(keyDownTreino);
      kpd.setKeyUpHandler(keyUpTreino);
    }
    else
    {
      kpd.setKeyDownHandler(keyDownLivre);
      kpd.setKeyUpHandler(keyUpLivre);
    }
  }
  last_mode = mode;
}

void setup() {
    Serial.begin(115200);
    pinMode(SP_PIN, INPUT);
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    pixels.setBrightness(50); // not so bright
    // initialize SPI
    SPI.begin();
    Serial.println("Hello VS1053!\n");
    player.beginMidi();  
    player.setVolume(100);  

    /**  MIDI messages, 0x80 to 0xEF Channel Messages,  0xF0 to 0xFF System Messages
 *   a MIDI message ranges from 1 byte to three bytes
 *   the first byte consists of 4 command bits and 4 channel bits, i.e. 16 channels
 *   
 *   0x80     Note Off
 *   0x90     Note On
 *   0xA0     Aftertouch
 *   0xB0     Continuous controller
 *   0xC0     Patch change
 *   0xD0     Channel Pressure
 *   0xE0     Pitch bend
 *   0xF0     (non-musical commands)
 */

/** 0xB0 Continuous controller commands, 0-127
 *  0 Bank Select (MSB)
 *  1 Modulation Wheel
 *  2 Breath controller
 *  3 Undefined
 *  4 Foot Pedal (MSB)
 *  5 Portamento Time (MSB)
 *  6 Data Entry (MSB)
 *  7 Volume (MSB)
 *  8 Balance (MSB)
 *  9 Undefined
 *  10 Pan position (MSB)
 *  11 Expression (MSB)
 *  ...
 */

 // Continuous controller 0, bank select: 0 gives you the default bank depending on the channel
 // 0x78 (percussion) for Channel 10, i.e. channel = 9 , 0x79 (melodic)  for other channels
  player.sendMidiMessage(0xB0| channel, 0, 0x00); //0x00 default bank 

  // Patch change, select instrument
  player.sendMidiMessage(0xC0| channel, instrument, 0);

  kpd.begin();
  kpd.setKeyDownHandler(keyDownLivre);
  kpd.setKeyUpHandler(keyUpLivre);
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();
  delay(500);
  // turn off
  pixels.fill(0x000000);
  pixels.show();
  delay(500); // wait half a second
}

void loop()
 { 
  modeFunction();
  switch(mode)
  {
    case LIVRE:
      
    break;
    case APRENDIZADO:
      Serial.println("Futuro");
    break;
    case TREINO:
      checkStartStopButton();
    break;
  }
  kpd.scan();
  octaveReading();
  volumeLevel = analogRead(POT_PIN) / 32;
  player.sendMidiMessage(0xB0| channel, 0x07, volumeLevel); 
}