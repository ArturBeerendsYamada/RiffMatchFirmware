#include "BluetoothSerial.h"

//#define USE_PIN // Uncomment this to use PIN during pairing. The pin is specified on the line below
const char *pin = "1234"; // Change this to more secure PIN.

String device_name = "ESP32-BT-Riffmatch";

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

void printBytesAsHex(BluetoothSerial& btSerial);

void setup() {
  Serial.begin(115200);
  SerialBT.begin(device_name); //Bluetooth device name
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());
  //Serial.printf("The device with name \"%s\" and MAC address %s is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str(), SerialBT.getMacString()); // Use this after the MAC method is implemented
  #ifdef USE_PIN
    SerialBT.setPin(pin);
    Serial.println("Using PIN");
  #endif

  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_BT);
  Serial.print("Bluetooth MAC: ");
  for (int i = 0; i < 5; i++) {
    Serial.printf("%02X:", baseMac[i]);
  }
  Serial.printf("%02X\n", baseMac[5]);
}

void loop() {
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    printBytesAsHex(SerialBT);
  }
  delay(20);
}

void printBytesAsHex(BluetoothSerial& btSerial) {
  int i=0, line_size = 16;
  while (btSerial.available()) {
    uint8_t byteRead = btSerial.read(); // Read a byte from Bluetooth Serial
    if (byteRead < 0x10) {
      Serial.print("0");                // Add a leading zero for single digit hex values
    }
    Serial.print(byteRead, HEX);        // Print the byte as a hexadecimal value
    Serial.print(" ");                  // Print a space between hex values
    i++;
    if(i==line_size){
      Serial.println();
      i=0;
    }
  }
  Serial.println();                     // Newline after all bytes are printed
}