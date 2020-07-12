#include <algorithm> 

#include "defines.h"
#define DHprint( a ) { Serial.print( #a); Serial.print(": ");Serial.println ( (uint32_t)a,HEX ); }
#define DTprint( a ) { Serial.println( #a); }
#define Dprint(a) Serial.print( a );

char buf[512] = "Hello World! What a wonderful World :)";
uint8_t temp[512];
uint8_t xData[2048], buffer[2048];
uint8_t valERAM;
const uint32_t  sizeofNAND= 134217728;

//only used for check42 or check24
uint16_t arraySize = 4096;
uint8_t x42[4096];
uint16_t flashBufferSize = (2048 + 64) * 2;  //2,112 bytes, 64 ecc + 2048 data buffer



void setup(){
    Serial.begin(115200);
    delay(1000);
    Serial.println("Begin Init");

    configure_flash();
    w25n01g_init();

    delay(500);

    check42(false);

    //Lets try an erase
    //w25n01g_deviceErase();  //Erase chip

    memset(buffer, 0xFF, 2048);
    for(uint16_t i = 0; i < 2048; i++) buffer[i] = i;

    //Serial.println("Loading data");
    //w25n01g_programDataLoad(0, buffer, 16);
    //w25n01g_randomProgramDataLoad(0, buffer, 16);
    //w25n01g_programExecute(0);
    w25n01g_pageProgramDataLoad(4094, buffer, 16);

    //Serial.println("Reading Data");
    memset(buffer, 0, 2048);
    w25n01g_readBytes(4094, buffer, 16);

    for(uint16_t i = 0; i < 32; i++) {
      Serial.printf("0x%02x, ",buffer[i]);
    } Serial.println();


const uint8_t beefy[] = "DEADBEEFdeadbeef\n";
   //Serial.println("Loading data");
Dprint( (char *)beefy )
    memset(buffer, 0, 2048);
    for(uint8_t j = 0; j < 20; j++) buffer[j] = beefy[j];

    //w25n01g_programDataLoad(4000, buffer, 20);
    //w25n01g_randomProgramDataLoad(4000, buffer, 20);
    //w25n01g_programExecute(4000);
    w25n01g_pageProgramDataLoad(4000, buffer, sizeof(beefy));

   
    //Serial.println("Reading Data");
    memset(buffer, 0, 2048);
    w25n01g_readBytes(4000, buffer, 20);

    for(uint16_t i = 0; i < 96; i++) {
      Serial.printf("0x%02x[%c], ",buffer[i], buffer[i]);
    } Serial.println();  

    memset(x42, 42, arraySize);
    w25n01g_pageProgramDataLoad(0, x42, arraySize);
    memset(x42, 0, arraySize);
    w25n01g_readBytes(0, x42, arraySize);

    for(uint16_t i = 0; i < arraySize; i++) {
      Serial.printf("0x%02x, ",x42[i]);
      if(i % 100 == 0) Serial.println();
    } Serial.println();

    Serial.println();
    check42(true);
}

void loop(){}


uint32_t errCnt = 0;
elapsedMicros my_us;
void check42( bool doWrite ) {
  uint32_t test = sizeofNAND / flashBufferSize ;

  byte value;
  uint32_t ii;
  uint32_t jj = 0, kk = 0;
  if ( doWrite) {
    Serial.print("\n    NAND ========== memory map ======  ====== check42() : WRITE !!!!\n");
    Serial.printf("\t\tNAND length 0x%X element size of %d\n", sizeofNAND, sizeof(valERAM));
    my_us = 0;

    memset(x42, 42, arraySize);

    for ( ii = 0; ii < test; ii++ ) { //write pages
        w25n01g_pageProgramDataLoad(ii * arraySize, x42, arraySize);
    }

    Serial.printf( "\t took %lu elapsed us\n", (uint32_t)my_us );
  }  //end doWrite


  Serial.print("    NAND ============================ check42() : COMPARE !!!!\n");
  my_us = 0;
  w25n01g_writeEnable(false);

  for ( ii = 0; ii < test; ii++ ) {
    memset(x42, 0, arraySize);
    w25n01g_readBytes(ii * arraySize, x42, arraySize);
    for (uint16_t ik = 0; ik < arraySize; ik++) {
      if ( 42 != x42[ik] ) {
        if ( ik != 0 ) {
          Serial.printf( "\t+++ NOT 42 Good Run of %u {bad @ %u}\n", ik, ii );
          ik = 0;
          return;
        }
        if ( jj < 100 ) Serial.printf( "%3u=%8u\n", ik, x42[ik] );
        jj++;
      }
      else kk++;
    }
  }

  Serial.printf( "\t took %lu elapsed us\n", (uint32_t)my_us );
  if ( 0 == jj )
    Serial.printf( "Good, " );
  else
    Serial.printf( "Failed to find 42 in NAND %d Times", jj );
  errCnt += jj;
  Serial.printf( "\tFound 42 in NAND 0x%X Times\n", sizeofNAND - jj );
}



