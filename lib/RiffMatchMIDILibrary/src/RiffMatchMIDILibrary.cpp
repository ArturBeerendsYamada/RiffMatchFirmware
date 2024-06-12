#include "RiffMatchMIDILibrary.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <vector>

// Function to print a character as hex
void print_hex(uint8_t ch) {
    printf("%02X ", ch);        // Print the byte as a hexadecimal value
}

//Function to fill and return a struct with header information from MIDI file.
//If no header flag ("MThd") is found, return all fields as -1.
//Also fills timeInformation with default values
struct MIDIHeader getMIDIHeader(std::vector<uint8_t> fileData, int fileSize, struct timeInformation* timeInfo) {
    struct MIDIHeader header;
    //If file size is smaller than a MIDI header, returns "error"
    if (fileSize < 14) {
        header.lenght = -1;
        header.format = -1;
        header.ntrack = -1;
        header.tickdiv = -1;
        header.firstTrackIndex = -1;
        return header;
    }

    int i=0;
    if (fileData[i] == 0x4D &&     //ascii hex 'M'
        fileData[++i] == 0x54 &&   //ascii hex 'T'
        fileData[++i] == 0x68 &&   //ascii hex 'h'
        fileData[++i] == 0x64      //ascii hex 'd'
    ) {
        //if found header flag ("MThd")
        //adds the next 4 bytes (big-endian) to get the chunck lenght
        int chunklen =  (fileData[++i] << 8*3) +
                        (fileData[++i] << 8*2) +
                        (fileData[++i] << 8) +
                        fileData[++i];
        //adds the next 2 bytes (big-endian) to get the MIDI format of the file (0 | 1 | 2)
        int formato =   (fileData[++i] << 8) +
                        fileData[++i];
        //adds the next 2 bytes (big-endian) to get the number of tracks
        int ntracks  =  (fileData[++i] << 8) +
                        fileData[++i];
        //adds the next 2 bytes (big-endian) to get the number of ticks per quarter note
        int tickdiv =   (fileData[++i] << 8) +
                        fileData[++i];
        header.lenght = chunklen;
        header.format = formato;
        header.ntrack = ntracks;
        header.tickdiv = tickdiv;
        header.firstTrackIndex = ++i;
    }
    else {
        header.lenght = -1;
        header.format = -1;
        header.ntrack = -1;
        header.tickdiv = -1;
        header.firstTrackIndex = -1;
    }

    timeInfo->tempoMIDI = DEFAULT_TEMPO;
    timeInfo->timeSigDen = DEFAULT_TIMESIG_DEN;
    timeInfo->bpm = calculateBPM(DEFAULT_TEMPO, DEFAULT_TIMESIG_DEN);
    timeInfo->usPerTick = calculateUsPerTick(DEFAULT_TEMPO, header.tickdiv);
    timeInfo->msPerTick = (timeInfo->usPerTick) / 1000;
    timeInfo->tickdiv = header.tickdiv;
    #if DEBUG
        printf("----------File Header----------\nheader lenght: %d\nMIDI format: %d\nnumber of tracks: %d\ntickdiv: %d\nTrackIndex: %d\n--------------------\n",
        header.lenght, header.format, header.ntrack, header.tickdiv, header.firstTrackIndex);
        printf("----------Time Info----------\ntempoMIDI: %d\ntimeSigDen: %d\nbpm: %.2f\nusPerTick: %.2f\nmsPerTick: %.2f\n--------------------\n",
        timeInfo->tempoMIDI, timeInfo->timeSigDen, timeInfo->bpm, timeInfo->usPerTick, timeInfo->msPerTick);
    #endif
    return header;
}

//Function to check the presence of a Track (flegged by "Mtrk" in the file) and return its length
//Returns -2 if not found or -1 if length left in the file is less than the Track Header
int getMtrkHeader(std::vector<uint8_t> fileData, int fileSize, int index) {
    int i=index;
    if (fileSize - index < MIDI_HEADER_LEN) {
        return -1;
    }
    if (fileData[i] == 0x4D &&     //ascii hex 'M'
        fileData[++i] == 0x54 &&   //ascii hex 'T'
        fileData[++i] == 0x72 &&   //ascii hex 'r'
        fileData[++i] == 0x6B      //ascii hex 'k'
    ) {
        //if found header flag ("MThd")
        //adds the next 4 bytes (big-endian) to get the chunck lenght
        int chunklen =  (fileData[++i] << 8*3) +
                        (fileData[++i] << 8*2) +
                        (fileData[++i] << 8) +
                        fileData[++i];
        return chunklen;
    }
    else 
        return -2;
}

//Function to calculate BPM from tempo (us per quarter note) and time signature denomiator (which note represents a beat*)
//* X/4 is 1 beat per quarter note, X/2 is 1 beat ber half-note
float calculateBPM(int tempo, int timeSigDen) {
    float quarterNotePerSec = 1000000.0/tempo;
    float beatPerQuarterNote = pow(2, timeSigDen)/4.0;
    float beatPerSec = quarterNotePerSec * beatPerQuarterNote;
    float bpm = beatPerSec * 60;
    return bpm;
}

//Function to calculate length of a MIDI tick in us from tempo (us per quarter note) and tickdiv (ticks per quarter note)
float calculateUsPerTick(int tempo, int tickdiv) {
    float length = (float)tempo/(float)tickdiv;
    return length;
}

//Function to update relevant fields when there is a change in the tempo
void updateTempo(int tempo, struct timeInformation* timeInfo) {
    timeInfo->tempoMIDI = tempo;
    timeInfo->bpm = calculateBPM(timeInfo->tempoMIDI, timeInfo->timeSigDen);
    timeInfo->usPerTick = calculateUsPerTick(timeInfo->tempoMIDI, timeInfo->tickdiv);
    timeInfo->msPerTick = (timeInfo->usPerTick) / 1000;
    #if DEBUG
        printf("----------Update Tempo----------\ntempoMIDI: %d\ntimeSigDen: %d\nbpm: %.2f\nusPerTick: %.2f\nmsPerTick: %.2f\n--------------------\n",
        timeInfo->tempoMIDI, timeInfo->timeSigDen, timeInfo->bpm, timeInfo->usPerTick, timeInfo->msPerTick);
    #endif
}

//Function to update relevant fields when there is a change in the time signature
void updateTimeSig(int timeSigNum, int timeSigDen, struct timeInformation* timeInfo) {
    timeInfo->timeSigNum = timeSigNum;
    timeInfo->timeSigDen = timeSigDen;
    timeInfo->bpm = calculateBPM(timeInfo->tempoMIDI, timeInfo->timeSigDen);
    #if DEBUG
        printf("----------Update Time Signature----------\ntempoMIDI: %d\ntimeSigDen: %d\nbpm: %.2f\nusPerTick: %.2f\nmsPerTick: %.2f\n--------------------\n",
        timeInfo->tempoMIDI, timeInfo->timeSigDen, timeInfo->bpm, timeInfo->usPerTick, timeInfo->msPerTick);
    #endif
}

// Function to read a Variable Lenght Quantity from a byte stream and convert it to an integer
uint32_t readVLQ(std::vector<uint8_t> stream, int* index) {
    int value = 0;
    int shift = 0;
    uint8_t byte;

    do {
        byte = stream[(*index)++];
        value = value << shift;
        value = value + (byte & 0x7F);
        shift += 7;
    } while (byte & 0x80);

    return value;
}

//function to read an event starting on given index of byte array "fileData".
//Assumes that the given index is pointing to the beginning of a delta time.
//Returns the first index after the end of the event
//If the read event is type MIDI, stores it in the address given by parameter "event"
//Returns the beginning of first track if the file already ended and fills in -1 in all fields of "event"
int readEvent(std::vector<uint8_t> fileData, int index, int fileSize, struct timeInformation* timeInfo, MIDIHeader header, MidiEvent* event) {
    if(index >= fileSize)
    {
        #if DEBUG
            printf("File already ended. Looping back to beginning of First Track\n");
        #endif
        event->deltaTime = -1;
        event->status = -1;
        event->data1 =  -1;;
        event->data2 =  -1;;
        return header.firstTrackIndex+MIDI_HEADER_LEN;
    }
    int deltaTime = readVLQ(fileData, &index);
    float deltaTimeuSec = deltaTime*timeInfo->usPerTick;
    float deltaTimemSec = deltaTime*timeInfo->msPerTick;
    event->deltaTime = (int)deltaTimemSec;

    #if DEBUG
        printf("----------Event----------\nDeltaTime[0x]: %X\nDeltaTime[0d]: %d\nDeltaTime[us]: %.2f\nDeltaTime[ms]: %.2f\n",
        deltaTime, deltaTime, deltaTimeuSec, deltaTimemSec);
    #endif

    //SysEx events are unsupported, so just get the lenght and jump the index
    if (fileData[index] == SYSEX_EVENT) {
        #if DEBUG
                printf("SysEx Event unsuported\n");
        #endif
        index++;
        int length = readVLQ(fileData, &index); 
        index+=length;
    }
    else if (fileData[index] == META_EVENT) {
        //index out of bounds should be checked inside metaEvent function due to variable lenght
        //index should be updated inside metaEvent function due to variable lenght
        index++;
        metaEvent(fileData, &index, fileSize, timeInfo);
    }
    //If it is neither a SysEx or a Meta Event, it should be a MIDI event
    else {
        switch (fileData[index] & 0xF0) {
            case META_EVENT:
                //index out of bounds should be checked inside metaEvent function due to variable lenght
                //index should be updated inside metaEvent function due to variable lenght
                index++;
                metaEvent(fileData, &index, fileSize, timeInfo);
            break;

            //following three unsuported events have the same amount of bytes (3)
            case NOTE_ON_EVENT:
            case NOTE_OFF_EVENT:
            case POLYPHONIC_PRESSURE_EVENT:
            case CONTROLLER_EVENT:
            case PITCH_BEND_EVENT:
                #if DEBUG
                    printf("MIDI Event 3 bytes\n");
                #endif
                event->status = fileData[index];
                event->data1 = fileData[index+1];
                event->data2 = fileData[index+2];
                index+=CONTROLLER_EVENT_LEN;
            break;

            //following two unsuported events have the same amount of bytes (2)
            case PROGRAM_CHANGE_EVENT:
            case CHANNEL_PRESSURE_EVENT:
                #if DEBUG
                    printf("MIDI Event 2 bytes\n");
                #endif
                event->status = fileData[index];
                event->data1 = fileData[index+1];
                event->data2 = 0;
                index+=PROGRAM_CHANGE_EVENT_LEN;
            break;

            //SysEx events are unsupported, so just get the lenght and jump the index
            case SYSEX_EVENT:
                {
                    #if DEBUG
                    printf("SysEx Event unsuported\n");
                    #endif
                    index++;
                    int length = readVLQ(fileData, &index); 
                    index+=length;
                }
            break;

            default:
                #if DEBUG
                    printf("MIDI event unknown (%X)\n", fileData[index] & 0xF0);
                #endif
        }
    }
    
    #if DEBUG
        printf("--------------------\n");
    #endif
    return index;
}

//Function to work on Note Off Event
void noteOnEvent(uint8_t note, uint8_t velocity) {
    #if DEBUG
        printf("----------Note On----------\nNote[dec]: %d\nVelocity[dec]: %d\n--------------------\n", note, velocity);
    #endif
}

//Function to work on Note Off Event
void noteOffEvent(uint8_t note, uint8_t velocity) {
    #if DEBUG
        printf("----------Note Off----------\nNote[dec]: %d\nVelocity[dec]: %d\n--------------------\n", note, velocity);
    #endif
}

//Function to work on Meta Event
//Assumes that the given index is pointing to the type of Meta event, followed by beginning of a VLQ length.
void metaEvent(std::vector<uint8_t> fileData, int* index, int fileSize, struct timeInformation* timeInfo) {
    uint8_t metaEventType = fileData[(*index)++];
    int length = readVLQ(fileData, index);
    switch (metaEventType) {
        case META_TEMPO: {
            int tempo;
            tempo = 0;
            //adds the next 3 bytes (big-endian) to get the MIDITempo (us per quarter note)
            tempo += fileData[(*index)++] << 8*2;
            tempo += fileData[(*index)++] << 8;
            tempo += fileData[(*index)++] << 0;
            updateTempo(tempo, timeInfo);
        break;}

        case META_TIME_SIGNATURE: {
            int timeSigNum = fileData[(*index)++];
            int timeSigDen = fileData[(*index)++];
            int cc = fileData[(*index)++];//unused
            int bb = fileData[(*index)++];//unused
            updateTimeSig(timeSigNum, timeSigDen, timeInfo);
        break;}

        case META_ENDOFTRACK:
            #if DEBUG
                printf("----------End of Track----------\nlen: %d\n--------------------\n", length);
            #endif
        break;

        default:
            #if DEBUG
                printf("----------Unsupported Meta Event----------\nlen: %d\n--------------------\n", length);
            #endif
            (*index)+=length;
        break;
    }
    return;
}