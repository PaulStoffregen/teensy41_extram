/*
   This test uses the optional quad spi flash on Teensy 4.1
   https://github.com/pellepl/spiffs/wiki/Using-spiffs
   https://github.com/pellepl/spiffs/wiki/FAQ

   ATTENTION: Flash needs to be empty before first use of SPIFFS


   Frank B, 2020
*/


#include <extRAM_t4.h>
#include <spiffs.h>

extRAM_t4 eRAM;
//uint8_t config = 2; //0 - init eram only, 1-init flash only, 2-init both
//These have been replaced with defines for:
//INIT_PSRAM_ONLY
//INIT_FLASH_ONLY
//INIT_PSRM_FLASH
uint8_t config = INIT_PSRM_FLASH;

//Setup files IO
spiffs_file file1;

//#define DO_DEBUG 1

char buf[512] = "Hello World! What a wonderful World :)";
char fname[32] = "my_file1";
int szLen = strlen( buf );
elapsedMicros my_us;

//random address to write from
uint16_t writeaddress = 0x00;
uint8_t valERAM;
uint8_t *ptrERAM = (uint8_t *)0x70000000;  // Set to ERAM
const uint32_t  sizeofERAM = 0x7FFFFE / sizeof( valERAM ); // sizeof free RAM in


void setup() {
  while (!Serial);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
#if 1
  Serial.println("\n Enter 'y' in 6 seconds to format FlashChip - other to skip");
  uint32_t pauseS = millis();
  char chIn = 9;
  while ( pauseS + 6000 > millis() && 9 == chIn ) {
    if ( Serial.available() ) {
      do {
        if ( chIn != 'y' )
          chIn = Serial.read();
        else
          Serial.read();
      }
      while ( Serial.available() );
    }
  }
  if ( chIn == 'y' ) {
    int8_t result = eRAM.begin(config);
    if(result == 0){
      eRAM.eraseFlashChip();
    } else {
      eRAM.eraseDevice();
    }
  }
#endif

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.begin(config);
  eRAM.fs_mount();

#if 1
  Serial.println("Write file:");
  Serial.println(buf);
  eRAM.f_writeFile(fname, buf, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
#endif

  Serial.println();
  Serial.println("Directory contents:");
  eRAM.fs_listDir();

  memset(buf, 0, sizeof(buf)); //emtpy buffer
  Serial.println("Read file:");
  eRAM.f_readFile(fname, buf, sizeof(buf)/sizeof(char), SPIFFS_RDONLY);
  Serial.println(buf);

  Serial.println();
  //eRAM.eramBegin();
  delay(100);
  check42();
  check24();

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.fs_mount();
  Serial.println("Directory contents:");
  eRAM.fs_listDir();
}

void loop() {
  char chIn = 9;
  if ( Serial.available() ) {
    do {
      if ( chIn != 'y' )
        chIn = Serial.read();
      else
        Serial.read();
    }
    while ( Serial.available() );
  }
  if ( chIn == 'y' ) {
    Serial.println();
    Serial.println("Loop Test2");
    loopTest2();
    Serial.println();
    Serial.println("Loop Test");
    loopTest();

  }
}

void loopTest2() {
  int szLen;
  char xData[12048], xData1[12048], xData2[26];

  char fname1[32] = "loopTest2";
  //spiffs_file fd1 = SPIFFS_open(&fs, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  eRAM.f_open(file1, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    //if (SPIFFS_write(&fs, fd1, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
    eRAM.f_write(file1, xData, 26);
  }
  //SPIFFS_close(&fs, fd1);
  //SPIFFS_fflush(&fs, fd1);
  eRAM.f_close(file1);
  

  Serial.println("Directory contents:");
  eRAM.fs_listDir();

  eRAM.f_open(file1, fname1, SPIFFS_RDONLY);
  //if (SPIFFS_read(&fs, fd, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  eRAM.f_read(file1, xData1,2600);
  //SPIFFS_close(&fs, fd);
  eRAM.f_close(file1);

  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    eRAM.writeArray(ii * 26, 26, (uint8_t*)xData);
  }

#ifdef DO_DEBUG
  Serial.println();
  for ( int ii = 0; ii < 100; ii++) {
    eRAM.readArray(ii * 26, 26, (uint8_t*)xData2);
    for ( int jj = 0; jj < 26; jj++) {
      Serial.print( xData2[jj]);
    }
    Serial.println("  <<< eRAM");
    for ( int jj = 0; jj < 26; jj++) {
      Serial.print( xData1[jj + (ii * 26)]);
    }
    Serial.println("  <<< FLASH");
  }
#endif
}

void loopTest() {
  char * erName = (char *)0x70007000;
  char * erData = (char *)0x70007200;
  
  char fName[52];
  char xData[12048];
  static char fIdx = 'A';
  sprintf(erName, "%s%c", "F_" ,__TIME__, fIdx++);
  snprintf(fName, 52, "F_%s%c", __TIME__, fIdx++);
  
  if ( fIdx > 'Z' ) fIdx = 'A';
  int kk = 0;
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[kk] = erData[kk] = 'A' + jj;
      else
        xData[kk] = erData[kk] = 'a' + jj;
      if ( memcmp( xData, erData, kk)) {
        Serial.printf( "\n\n%d :: FAILED memcmp !\n%s\n", kk, erData );
        xData[kk] = erData[kk] = 0;
        ii = jj = 200000;
      }
      kk++;
    }
    xData[kk] = erData[kk] = '0' + ((ii / 10) % 10);
    kk++;
    xData[kk] = erData[kk] = '0' + ii % 10;
    kk++;
    xData[kk] = erData[kk] = '\n';
    kk++;
  }
#ifdef DO_DEBUG
  xData[kk] = erData[kk] = 0;
  Serial.printf( "%d :: RAM \n%s\n", kk, xData );
  Serial.printf( "%d :: ERAM \n%s\n", kk, erData );
#endif
  erData[kk] = 0;
  if ( memcmp( xData, erData, kk)) {
    Serial.printf( "\n\n%d :: FAILED memcmp !\n%s\n", kk, erData );
    xData[kk] = erData[kk] = 0;
    Serial.printf( "%d :: RAM \n%s\n", kk, xData );
    Serial.printf( "%d :: ERAM \n%s\n", kk, erData );
  }

  my_us = 0;
  eRAM.fs_mount();
  //spiffs_file fd = SPIFFS_open(&fs, "test", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  eRAM.f_open(file1, fname, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  //if (SPIFFS_write(&fs, fd, (u8_t *)xData, kk ) < 0) Serial.printf("loopTest() errno %i\n", SPIFFS_errno(&fs));
    eRAM.f_write(file1, xData, kk);
  if ( fIdx > 'C') {
    Serial.println("\t first 2600 bytes");
    //if (SPIFFS_write(&fs, fd, (u8_t *)xData, kk - 2600 ) < 0) Serial.printf("loopTest() errno %i\n", SPIFFS_errno(&fs));
    eRAM.f_write(file1, xData, kk - 2600);
  }
  eRAM.f_close(file1);
  
  Serial.printf( "\t loopTest write took %lu elapsed us\n", (uint32_t)my_us );
  Serial.println("\t loopTest Directory contents:");
  eRAM.fs_listDir();
#ifdef DO_DEBUG
  Serial.println( erName );
  Serial.printf( "%s\n", erData );
#endif
  Serial.println();
}





//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************





uint32_t errCnt = 0;
void check42() {
  byte value;
  uint32_t ii;
  uint32_t jj = 0, kk = 0;
  Serial.print("\n    ERAM ========== memory map ====== eRAM class ====== check42() : WRITE !!!!\n");
  Serial.printf("\t\tERAM length 0x%X element size of %d\n", sizeofERAM, sizeof( valERAM ));
  my_us = 0;
  for ( ii = 0; ii < sizeofERAM; ii++ ) {
    //if ( !( ii/1024) )  Serial.printf("ERAM @ 0x%X\n", ii*1024);
    eRAM.writeByte(ii, 42);
  }
  Serial.printf( "\t took %lu elapsed us\n", (uint32_t)my_us );
  Serial.print("    ERAM ============================ check42() : COMPARE !!!!\n");
  my_us = 0;
  for ( ii = 0; ii < sizeofERAM; ii++ ) {
    eRAM.readByte(ii, &value);
    if ( 42 != value ) {
      if ( kk != 0 ) {
        Serial.printf( "\t+++ NOT 42 Good Run of %u {bad @ %u}\n", kk, ii );
        kk = 0;
      }
      if ( jj < 100 ) Serial.printf( "%3u=%8u\n", ii, ptrERAM[ii] );
      jj++;
    }
    else kk++;
  }
  Serial.printf( "\t took %lu elapsed us\n", (uint32_t)my_us );
  if ( 0 == jj )
    Serial.printf( "Good, " );
  else
    Serial.printf( "Failed to find 42 in ERAM %d Times", jj );
  errCnt += jj;
  Serial.printf( "\tFound 42 in ERAM 0x%X Times\n", sizeofERAM - jj );
}

#if 0
uint32_t *ptrERAM_32 = (uint32_t *)0x70000001l;  // Set to ERAM
const uint32_t  sizeofERAM_32 = 0x7FFFFF / sizeof( ptrERAM_32 ); // sizeof free RAM in uint32_t units.
#else
uint8_t *ptrERAM_32 = (uint8_t *)0x70000001l;  // Set to ERAM
const uint32_t  sizeofERAM_32 = 0x7FFFFE / sizeof( ptrERAM_32[0] ); // sizeof free RAM in uint32_t units.
#endif
void check24() {
  uint32_t ii;
  uint32_t jj = 0, kk = 0;
  Serial.print("\n    ERAM ========== memory map ===== array* ======== check24() : WRITE !!!!\n");
  Serial.printf("\t\tERAM length 0x%X element size of %d\n", sizeofERAM_32, sizeof( ptrERAM_32[0] ));
  my_us = 0;
  for ( ii = 0; ii < sizeofERAM_32; ii++ ) {
    ptrERAM_32[ii] = 24;
  }
  Serial.printf( "\t took %lu elapsed us\n", (uint32_t)my_us );
  Serial.print("    ERAM ============================ check24() : COMPARE !!!!\n");
  my_us = 0;
  for ( ii = 0; ii < sizeofERAM_32; ii++ ) {
    if ( 24 != ptrERAM_32[ii] ) {
      if ( kk != 0 ) {
        Serial.printf( "\t+++ Good Run of %u {bad @ %u}\n", kk, ii );
        kk = 0;
      }
      if ( jj < 100 )
        Serial.printf( "%3u=%8u\n", ii, ptrERAM_32[ii] );
      jj++;
    }
    else {
      kk++;
    }
  }
  Serial.printf( "\t took %lu elapsed us\n", (uint32_t)my_us );
  if ( 0 == jj )
    Serial.printf( "Good, " );
  else
    Serial.printf( "Failed to find 24 in ERAM %d Times", jj );
  Serial.printf( "\tFound 24 in ERAM %X Times\n", sizeofERAM_32 - jj );
}

