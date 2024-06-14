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
#include "BluetoothSerial.h"
#include <RiffMatchMIDILibrary.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <vector>

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

#define LCD_DIM_X 16
#define LCD_DIM Y 2
#define LCD_NOTE0 0
#define LCD_NOTE1 4
#define LCD_NOTE2 8
#define LCD_NOTE3 12
#define LCD_MODE 0
#define LCD_LOWER_OCT 7
#define LCD_FIRTS_OCT 8
#define LCD_SECND_OCT 9
#define LCD_UPPER_OCT 10
#define LCD_STOPPLAY 12

#define LED_NOTE_ON_TIME 500 //in ms
#define LED_ON_TIME_LIMIT 1000 //in ms
#define NUM_NOTES 128

#define PRESS_TIME_TOLERANCE 500 // in ms, should be less than LED_ON_TIME_LIMIT

uint8_t channel = 0;
uint8_t instrument = 2;

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ, UNDEFINED, SPI);
Keypad_Matrix kpd = Keypad_Matrix( makeKeymap (keys), rowPins, colPins, ROWS, COLS );
Adafruit_NeoPixel pixels(NUM_PIXELS, LEDPIN, NEO_GRB + NEO_KHZ800); 
LiquidCrystal_I2C lcd(0x27, 16, 2);

// All notes vectors needed for all modes
unsigned long int notesStartTime[NUM_NOTES] = {};
unsigned long int notesOnFile[NUM_NOTES] = {};
unsigned long int notesOffFile[NUM_NOTES] = {};
unsigned long int notesPressed[NUM_NOTES] = {};
unsigned long int notesReleased[NUM_NOTES] = {};

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
char lcdLine1[17] = "                "; // 16 blanks + \0

String device_name = "ESP32-BT-Riffmatch";
BluetoothSerial SerialBT;
std::vector<uint8_t> MIDIfileData;
int MIDIfileLen = 0;
int currentMIDIfileIndex = 0;
struct MIDIHeader MIDIfileMIDIHeader;
struct timeInformation MIDIfileTimeInformation;
int fileOk = 0;
long unsigned int timestampLastEventms = 0;
MidiEvent currentMIDIEvent;

void newNoteLCD(uint8_t note);

// noteOn and noteOff based on MP3_Shield_RealtimeMIDI demo 
// by Matthias Neeracher, Nathan Seidle's Sparkfun Electronics example respectively

//Send a MIDI note-on message.  Like pressing a piano key
void noteOn(uint8_t channel, uint8_t note, uint8_t attack_velocity)
{
  player.sendMidiMessage( (0x90 | channel), note, attack_velocity);
}

//Send a MIDI note-off message.  Like releasing a piano key
void noteOff(uint8_t channel, uint8_t note, uint8_t release_velocity)
{
  player.sendMidiMessage( (0x80 | channel), note, release_velocity);
}

void keyDownLivre (const uint8_t which)
{
  Serial.print (F("Key down: "));
  newNoteLCD((which + 12 * octave));
  Serial.println (which);
  noteOn(channel, (which + 12 * octave), 45);
  pixels.setPixelColor((uint16_t)(which - 12), pixels.Color(150, 150, 150));
  pixels.show();
  Serial.print (F("LED: "));
  Serial.println ((uint16_t)(which - 12));
}

void keyUpLivre (const uint8_t which)
{
  Serial.print (F("Key up: "));
  Serial.println (which);
  noteOff(channel, (which + 12 * octave), 45);
  pixels.setPixelColor((uint16_t)(which - 12), pixels.Color(0, 0, 0));
  pixels.show();
  Serial.print (F("LED: "));
  Serial.println ((uint16_t)(which - 12));
}

void keyDownTreino (const uint8_t which)
{
  Serial.print (F("Key down: "));
  newNoteLCD((which + 12 * octave));
  Serial.println (which);
  noteOn(channel, (which + 12 * octave), 45);
  notesPressed[which + 12 * octave]=millis();
}

void keyUpTreino (const uint8_t which)
{
  Serial.print (F("Key up: "));
  Serial.println (which);
  noteOff(channel, (which + 12 * octave), 45);
  notesReleased[which + 12 * octave]=millis();
}

void trainingCheck()
{
  unsigned long int currentTime = millis();
  int ledIndex = 0;
  pixels.fill(0x000000);
  for (int i = 0; i < NUM_NOTES; i++)
  {
    ledIndex = i - (octave+1) * 12; //nota 60 na oitava 4 tem que mapear para led 0
    if((currentTime - notesStartTime[i] < LED_ON_TIME_LIMIT) || (currentTime - notesPressed[i] < LED_ON_TIME_LIMIT)) //If pressed or expected is recent
    {
      if ((notesPressed[i] > notesStartTime[i]) && (ledIndex >= 0 && ledIndex <= 24)) //If pressed is more recent than expected
      {
        if ((notesPressed[i] - notesStartTime[i] < PRESS_TIME_TOLERANCE) && (ledIndex >= 0 && ledIndex <= 24)) //If pressed is near expected
        {
          pixels.setPixelColor(ledIndex, pixels.Color(0, 255, 0)); // Green for pressed expected within time
        }
        else
        {
          pixels.setPixelColor(ledIndex, pixels.Color(255, 100, 0)); // Orange for unexpected press
        }
      }
      else if ((currentTime - notesStartTime[i] <= PRESS_TIME_TOLERANCE) && (ledIndex >= 0 && ledIndex <= 24)) //If expected but still within tolerance
      {
        pixels.setPixelColor(ledIndex, pixels.Color(0, 0, 255)); // Blue for should press
      }
      else if ((currentTime - notesStartTime[i] > PRESS_TIME_TOLERANCE) && (ledIndex >= 0 && ledIndex <= 24)) //If tolerance time from expected elapsed
      {
        pixels.setPixelColor(ledIndex, pixels.Color(255, 0, 0)); // Red for unpressed expectation
      }
    }
  }
  pixels.show();
}

void octaveReading()
{
  static int last_reading = 0; // Para armazenar a última leitura válida
  unsigned long currentMillis = millis(); // Obtém o tempo atual em milissegundos

  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis; // Atualiza o tempo anterior

    int current_reading = analogRead(OCT_PIN); // Lê o valor analógico do pino
    // Se a diferença entre a leitura atual e a última leitura válida for menor que o threshold, consideramos a leitura estável
    if (abs(current_reading - last_reading) > threshold)
    {
      octave_read = current_reading;
      // Serial.println(octave_read);
      if (octave_read < 10)
      {
        if (octave == 6)
          octave = 6;
        else
          octave++; // Incrementa a variável octave
        Serial.print("Increment: ");
        Serial.println(octave); // Imprime o valor lido
      }
      else if (octave_read < 3000)
      {
        if (octave == 0)
          octave = 0;
        else 
          octave--; // Decrementa a variável octave
        Serial.print("Decrement: ");
        Serial.println(octave); // Imprime o valor lido
      }
      
      lcd.setCursor(LCD_LOWER_OCT, 1);
      lcd.print(' ');
      lcd.setCursor(LCD_FIRTS_OCT, 1);
      lcd.print(octave);
      lcd.setCursor(LCD_SECND_OCT, 1);
      lcd.print(octave+1);
      lcd.setCursor(LCD_UPPER_OCT, 1);
      lcd.print(' ');

      last_reading = octave_read; // Atualiza a última leitura válida
    }
  }
}

//Adapted from https://esp32io.com/tutorials/esp32-button-debounce
void checkStartStopButton()
{
  // read the state of the switch/button:
  currentState = digitalRead(SP_PIN);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch/button changed, due to noise or pressing:
  if (currentState != lastFlickerableState)
  {
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
        lcd.setCursor(LCD_STOPPLAY, 1);
        lcd.print("Stop");
        startStopState = STOP;
      }
      else if (!startStopState) 
      {
        Serial.println("Started");
        lcd.setCursor(LCD_STOPPLAY, 1);
        lcd.print("Play");
        startStopState = START;
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
  {
    mode = LIVRE;
    //Serial.println("Modo Livre");
  }
  else if (modeReading < 4000 && modeReading > 1000)
  {
    mode = APRENDIZADO;
    //Serial.println("Modo Aprendizado");
  }
  else
  {
    mode = TREINO;
    //Serial.println("Modo Treino");
  }
  // Serial.println(mode);
  if (last_mode != mode)
  {
    currentMIDIfileIndex = MIDIfileMIDIHeader.firstTrackIndex + MIDI_HEADER_LEN;
    pixels.fill(0x000000);
    for(int i=0; i<16; i++)
      lcdLine1[i] = ' ';
    lcd.setCursor(0, 0);
    lcd.print(lcdLine1);
    lcd.setCursor(LCD_LOWER_OCT, 1);
    lcd.print(' ');
    lcd.setCursor(LCD_UPPER_OCT, 1);
    lcd.print(' ');
    pixels.show();
    if (mode == TREINO)
    {
      Serial.println("Modo Treino");
      kpd.setKeyDownHandler(keyDownTreino);
      kpd.setKeyUpHandler(keyUpTreino);
      lcd.setCursor(LCD_MODE, 1);
      lcd.print('T');
    }
    else
    {
      if (mode == LIVRE)
      {
        Serial.println("Modo Livre");
        lcd.setCursor(LCD_MODE, 1);
        lcd.print('L');
      }
      else
      {
        Serial.println("Modo Aprendizado");
        lcd.setCursor(LCD_MODE, 1);
        lcd.print('A');
      }
      kpd.setKeyDownHandler(keyDownLivre);
      kpd.setKeyUpHandler(keyUpLivre);
    }
  }
  last_mode = mode;
}

void carregarArquivo()
{
  MIDIfileData.clear();
  pixels.fill(0x0000FF);
  pixels.show();
  delay(200); //give time to make sure the file is fully sent
  while (SerialBT.available())
  {
    MIDIfileData.push_back(SerialBT.read());
  }
  MIDIfileLen = MIDIfileData.size();
  // for(int i=0; i<MIDIfileLen; i++){
  //   print_hex(MIDIfileData[i]);
  // }
  // printf("\n");
  MIDIfileMIDIHeader = getMIDIHeader(MIDIfileData, MIDIfileLen, &MIDIfileTimeInformation);
  printf("MTrkChunkLen: %d\n", getMtrkHeader(MIDIfileData, MIDIfileLen, MIDIfileMIDIHeader.firstTrackIndex));
  currentMIDIfileIndex = MIDIfileMIDIHeader.firstTrackIndex + MIDI_HEADER_LEN;

  fileOk = 1;
  pixels.fill(0x000000);
  pixels.show();
}

void MIDIEventTreatment()
{
  if(currentMIDIEvent.status <= 0xE0) //if it is a MIDI Event with 3 bytes
  {
    if(mode==APRENDIZADO || (currentMIDIEvent.data1 < (octave+1) * 12 || currentMIDIEvent.data1 > (octave+3) * 12)) //only send MIDI message from file if it is Learning mode or outside of current octaves
    {
      player.sendMidiMessage(currentMIDIEvent.status, currentMIDIEvent.data1, currentMIDIEvent.data2);
    }
    if((uint8_t)(currentMIDIEvent.status & (uint8_t)(0xF0)) == NOTE_ON_EVENT && currentMIDIEvent.data2 >= 5)
    {
      notesStartTime[currentMIDIEvent.data1] = millis();
      if(mode == APRENDIZADO)
        newNoteLCD(currentMIDIEvent.data1);
    }
  }
}

void updateLEDsFromMIDIFile()
{
  int currentTime = millis();
  int ledIndex = 0;
  pixels.fill(0x00000000);
  for (int i=0;i<NUM_NOTES;i++)
  {
    ledIndex = i - (octave+1) * 12; //nota 60 na oitava 4 tem que mapear para led 0
    if(currentTime - notesStartTime[i] <= LED_NOTE_ON_TIME)
    {
      if(ledIndex>24)
      {
        //acende canto direito por estar em oitava muita pra baixo
        pixels.setPixelColor((uint16_t)(24), pixels.Color(255, 255, 0));
        lcd.setCursor(LCD_UPPER_OCT, 1);
        lcd.print('>');
      }
      else if (ledIndex<0)
      {
        //acende canto esquerdo por estar em oitava muito pra cima
        pixels.setPixelColor((uint16_t)(0), pixels.Color(255, 255, 0));
        lcd.setCursor(LCD_LOWER_OCT, 1);
        lcd.print('<');
      }
      else
      {
        //acende
        pixels.setPixelColor((uint16_t)(ledIndex), pixels.Color(255, 0, 255));
      }
    }
  }
  pixels.show();
}

void newNoteLCD(uint8_t midiNote)
{
  char text[3];
  int octave, note;
  octave = midiNote/12 - 1;
  note = midiNote%12;
  switch (note)
  {
    case 0:
      text[0] = 'C';
      text[1] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      text[2] = ' ';
      break;
    
    case 1:
      text[0] = 'C';
      text[1] = '#';
      text[2] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      break;
    
    case 2:
      text[0] = 'D';
      text[1] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      text[2] = ' ';
      break;
    
    case 3:
      text[0] = 'D';
      text[1] = '#';
      text[2] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      break;
    
    case 4:
      text[0] = 'E';
      text[1] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      text[2] = ' ';
      break;
    
    case 5:
      text[0] = 'F';
      text[1] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      text[2] = ' ';
      break;
    
    case 6:
      text[0] = 'F';
      text[1] = '#';
      text[2] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      break;
    
    case 7:
      text[0] = 'G';
      text[1] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      text[2] = ' ';
      break;
    
    case 8:
      text[0] = 'G';
      text[1] = '#';
      text[2] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      break;
    
    case 9:
      text[0] = 'A';
      text[1] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      text[2] = ' ';
      break;
    
    case 10:
      text[0] = 'A';
      text[1] = '#';
      text[2] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      break;
    
    case 11:
      text[0] = 'B';
      text[1] = '0'+octave; //ascii '0' + deslocamento para pegar o ascii do número
      text[2] = ' ';
      break;
  
    default:
      break;
  }
  
  for(int i=15; i > 3; i--) // desloca todas as outras notas 4 casa pro lado
  {
    lcdLine1[i] = lcdLine1[i-4];
  }
  lcdLine1[0] = text[0];
  lcdLine1[1] = text[1];
  lcdLine1[2] = text[2];

  lcd.setCursor(0, 0);
  lcd.print(lcdLine1);
}

void setup()
{
  Serial.begin(115200);
  SerialBT.begin(device_name); //Bluetooth device name
  pinMode(SP_PIN, INPUT);
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(10); // not so bright
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

  lcd.begin();
  lcd.backlight();
  lcd.print("NT0 NT1 NT2 NT3 ");
  lcd.setCursor(0, 1);
  lcd.print("L OCT:  45  Stop");

  pixels.fill(0x00FF00);
  pixels.show();
  delay(500);
  // turn off
  pixels.fill(0x000000);
  pixels.show();
}

void loop()
{
  modeFunction();
  switch(mode)
  {
    case LIVRE:
      
    break;
    case APRENDIZADO:
      checkStartStopButton();
      if (SerialBT.available())
      {
        carregarArquivo();
      }
      if(fileOk && startStopState == START)
      {
        updateLEDsFromMIDIFile();
        long unsigned int currentTime =  millis();
        if(currentTime-timestampLastEventms > currentMIDIEvent.deltaTime)
        {
          timestampLastEventms = currentTime;
          MIDIEventTreatment();
          currentMIDIfileIndex = readEvent(MIDIfileData, currentMIDIfileIndex, MIDIfileLen, &MIDIfileTimeInformation, MIDIfileMIDIHeader, &currentMIDIEvent);
          if(currentMIDIEvent.deltaTime == -1) //if this happens, means readEvent reached the end of the file
          {
            //reads the first event to have valid values again
            currentMIDIfileIndex = readEvent(MIDIfileData, currentMIDIfileIndex, MIDIfileLen, &MIDIfileTimeInformation, MIDIfileMIDIHeader, &currentMIDIEvent);
            if(mode==TREINO) //On mode "Treino" the end of the song does not loop
            {
              startStopState = STOP;
            }
          }
          // Serial.print("New Index: ");
          // Serial.println(currentMIDIfileIndex);
        }
      }
    break;
    case TREINO:
      checkStartStopButton();
      if (SerialBT.available())
      {
        carregarArquivo();
      }
      if(fileOk && startStopState == START)
      {
        long unsigned int currentTime =  millis();
        if(currentTime-timestampLastEventms > currentMIDIEvent.deltaTime)
        {
          timestampLastEventms = currentTime;
          MIDIEventTreatment();
          currentMIDIfileIndex = readEvent(MIDIfileData, currentMIDIfileIndex, MIDIfileLen, &MIDIfileTimeInformation, MIDIfileMIDIHeader, &currentMIDIEvent);
          if(currentMIDIEvent.deltaTime == -1) //if this happens, means readEvent reached the end of the file
          {
            //reads the first event to have valid values again
            currentMIDIfileIndex = readEvent(MIDIfileData, currentMIDIfileIndex, MIDIfileLen, &MIDIfileTimeInformation, MIDIfileMIDIHeader, &currentMIDIEvent);
            if(mode==TREINO) //On mode "Treino" the end of the song does not loop
            {
              startStopState = STOP;
            }
          }
          // Serial.print("New Index: ");
          // Serial.println(currentMIDIfileIndex);
      }}
      trainingCheck();
    break;
  }
  kpd.scan();
  octaveReading();
  volumeLevel = analogRead(POT_PIN) / 32;
  player.sendMidiMessage(0xB0| channel, 0x07, volumeLevel); 
}