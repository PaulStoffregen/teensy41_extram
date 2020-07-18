#include <w25n01g_t4.h>

#define DHprint( a ) { Serial.print( #a); Serial.print(": ");Serial.println ( (uint32_t)a,HEX ); }
#define DTprint( a ) { Serial.println( #a); }
#define Dprint(a) Serial.print( a );

w25n01g_t4 myNAND;

uint8_t buffer[2112];
uint16_t flashBufferSize = (2048 + 64) ;  //2,112 bytes, 64 ecc + 2048 data buffer

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Begin Init");

  myNAND.begin();

  // A “Bad Block Marker” is a non-FFh data byte stored at Byte 0 of
  // Page 0 for each bad block. An additional marker is also stored in the first byte 
  // of the 64-Byte spare area.
  Serial.println("Reading Data in byte0 of the ECC Spare area:");
  memset(buffer, 0, 2112);
  for(uint16_t j = 0; j < 65000; j++){
    myNAND.readECC(j*2112, buffer, 2112); 
      if(j % 1000 == 0) {
        Serial.print(".");
      }
      if(255 != buffer[2048]){
        Serial.printf("\nBad Block Marker Found in ECC Block in Page %d\n", j);
      }
  }
  Serial.println("\nScan Finished!");

  //READ BB_LUT
  myNAND.readBBLUT();
 
}

void loop() {}

