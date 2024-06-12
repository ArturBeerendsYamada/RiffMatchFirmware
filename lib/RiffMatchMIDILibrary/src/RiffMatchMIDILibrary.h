//MIDI file references:
//http://www.somascape.org/midi/tech/mfile.html
//http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html#BMA1_2
//https://www.recordingblogs.com/wiki/musical-instrument-digital-interface-midi

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <vector>

#define DEBUG 1

#define DEFAULT_TEMPO 500000
#define DEFAULT_TIMESIG_DEN 1
#define MIDI_HEADER_LEN 8

//all events here are considered to be going to channel 0, pay attention during implementation
#define NOTE_OFF_EVENT 0x80
#define NOTE_ON_EVENT 0x90
#define POLYPHONIC_PRESSURE_EVENT 0xA0
#define CONTROLLER_EVENT 0xB0
#define PROGRAM_CHANGE_EVENT 0xC0
#define CHANNEL_PRESSURE_EVENT 0xD0
#define PITCH_BEND_EVENT 0xE0
#define SYSEX_EVENT 0xF0
#define META_EVENT 0xFF

#define NOTE_ON_EVENT_LEN 3
#define NOTE_OFF_EVENT_LEN 3
#define POLYPHONIC_PRESSURE_EVENT_LEN 3
#define CONTROLLER_EVENT_LEN 3
#define PROGRAM_CHANGE_EVENT_LEN 2
#define CHANNEL_PRESSURE_EVENT_LEN 2
#define PITCH_BEND_EVENT_LEN 3

#define META_SEQUENCE_NUMBER 0x00
#define META_TEXT 0x01
#define META_COPYRIGHT 0x02
#define META_TRACKNAME 0x03
#define META_INSTRUMENT_NAME 0x04
#define META_LYRIC 0x05
#define META_MARKER 0x06
#define META_CUEPOINT 0x07
#define META_PROGRAM_NAME 0x08
#define META_DEVICENAME 0x09
#define META_MIDICHANNEL 0x20
#define META_MIDIPORT 0x21
#define META_ENDOFTRACK 0x2F
#define META_TEMPO 0x51
#define META_SMTPE_OFFSET 0x54
#define META_TIME_SIGNATURE 0x58
#define META_KEY_SIGNATURE 0x59


struct MIDIHeader {
    int lenght;
    int format;
    int ntrack;
    int tickdiv;
    int firstTrackIndex;
};

struct timeInformation {
    int tickdiv;    //ticks per quarter note
    int timeSigNum; //numerator of time signature
    int timeSigDen; //denominator of time signature
    int tempoMIDI;  //us per quarter note
    float bpm;      //bpm = 1000000.0/tempo * TimeSigDen/4.0 * 60
    float usPerTick;//microseconds per tick(from tickdiv) usPerTick = tempo/tickdiv
    float msPerTick;//miliseconds per tick(from tickdiv) msPerTick = usPerTick/1000
};

struct MidiEvent {
    int deltaTime;
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
};

//Function to print a character as hex
void print_hex(uint8_t ch);

//Function to fill and return a struct with header information from MIDI file.
//If no header flag ("MThd") is found, return all fields as -1.
//Also fills timeInformation with default values
struct MIDIHeader getMIDIHeader(std::vector<uint8_t> fileData, int fileSize, struct timeInformation* timeInfo);

//Function to check the presence of a Track Header int the given index (flegged by "Mtrk" in the file) and return its length
//Returns -1 if not found
int getMtrkHeader(std::vector<uint8_t> fileData, int fileSize, int index);

//Function to calculate BPM from tempo (us per quarter note) and time signature denomiator (which note represents a beat*)
//* X/4 is 1 beat per quarter note, X/2 is 1 beat ber half-note
float calculateBPM(int tempo, int timeSigDen);

//Function to calculate length of a MIDI tick from tempo (us per quarter note) and tickdiv (ticks per quarter note)
float calculateUsPerTick(int tempo, int tickdiv);

//Function to update relevant fields when there is a change in the tempo
void updateTempo(int tempo, struct timeInformation* timeInfo);

//Function to update relevant fields when there is a change in the time signature
void updateTimeSig(int timeSigNum, int timeSigDen, struct timeInformation* timeInfo);

//Function to read a Variable Lenght Quantity (VLQ) from a byte stream and convert it to an integer
uint32_t readVLQ(std::vector<uint8_t> stream, int* index);

//function to read an event starting on given index of byte array "fileData".
//Assumes that the given index is pointing to the beginning of a delta time.
//Returns the first index after the end of the event
//If the read event is type MIDI, stores it in the address given by parameter "event"
//Returns the beginning of first track if the file already ended and fills in -1 in all fields of "event"
int readEvent(std::vector<uint8_t> fileData, int index, int fileSize, struct timeInformation* timeInfo, MIDIHeader header, MidiEvent* event);

//Function to work on Note Off Event
void noteOnEvent(uint8_t note, uint8_t velocity);

//Function to work on Note Off Event
void noteOffEvent(uint8_t note, uint8_t velocity);

//Function to work on Meta Event
//Assumes that the given index is pointing to the type of Meta event, followed by beginning of a VLQ length.
void metaEvent(std::vector<uint8_t> fileData, int* index, int fileSize, struct timeInformation* timeInfo);
