#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "RiffMatchMIDILibrary.h"

void delay_us(unsigned int microseconds) {
    clock_t start = clock();
    while ((clock() - start) * 1000000 / CLOCKS_PER_SEC < microseconds);
}

// Function to read a file into a byte array
unsigned char* readFile(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize < 0) {
        perror("Error determining file size");
        fclose(file);
        return NULL;
    }

    // Allocate memory to hold the file contents
    unsigned char *buffer = (unsigned char *)malloc(fileSize);
    if (buffer == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead != fileSize) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Close the file
    fclose(file);

    // Set the size output parameter
    *size = fileSize;
    return buffer;
}

int main() {
    const char *filename = "MyTwinkle.mid"; // Replace with your file name
    size_t fileSize;
    uint8_t *fileData = readFile(filename, &fileSize);

    if (fileData == NULL) {
        fprintf(stderr, "Failed to read file\n");
        return EXIT_FAILURE;
    }

    // Print the bytes in hexadecimal format
    for (size_t i = 0; i < fileSize; i++) {
        if (i%16==0) printf("\n");
        print_hex(fileData[i]);
    }
    printf("\n\n");

    struct timeInformation timeinfo;
    struct MIDIHeader header = getMIDIHeader(fileData, fileSize, &timeinfo);

    int i, chunklen, flag = 1;
    for (i = 0; i < fileSize && flag; i++) {
        if (fileData[i] == 0x4D &&     //ascii hex 'M'
            fileData[++i] == 0x54 &&   //ascii hex 'T'
            fileData[++i] == 0x72 &&   //ascii hex 'r'
            fileData[++i] == 0x6B      //ascii hex 'k'
        ) {
            //if found header flag ("MTrk")
            //adds the next 4 bytes (big-endian) to get the chunck lenght
            chunklen =  (fileData[++i] << 8*3) +
                        (fileData[++i] << 8*2) +
                        (fileData[++i] << 8) +
                        fileData[++i];
            i++;
            break;
        }
    }
    int message = 0;
    chunklen += i;
    while(i<chunklen) {
        message++;
        printf("\n\nMessage %d index %X\n", message, i);
        i = readEvent(fileData, i, fileSize, &timeinfo);
    }
    printf("Final index: %X (%d)\n", i, i);
    // if (fileData[i] == 0x4D &&     //ascii hex 'M'
    //     fileData[++i] == 0x54 &&   //ascii hex 'T'
    //     fileData[++i] == 0x72 &&   //ascii hex 'r'
    //     fileData[++i] == 0x6B      //ascii hex 'k'
    // ) {
    //     //if found header flag ("MTrk")
    //     //adds the next 4 bytes (big-endian) to get the chunck lenght
    //     chunklen =  (fileData[++i] << 8*3) +
    //                 (fileData[++i] << 8*2) +
    //                 (fileData[++i] << 8) +
    //                 fileData[++i];
    //     i++;
    // }
    // else {
    //     chunklen = 0;
    //     printf("did not follow another track\n");
    // }

    // message = 0;
    // chunklen += i;
    // while(i<chunklen) {
    //     message++;
    //     printf("\n\nMessage %d index %X\n", message, i);
    //     i = readEvent(fileData, i, fileSize, &timeinfo);
    // }

    // Free the allocated memory
    free(fileData);

    return EXIT_SUCCESS;
}