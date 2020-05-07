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
uint8_t config = INIT_FLASH_ONLY;

//Setup files IO
spiffs_file file1;

//#define DO_DEBUG 1

char buf[512] = "Hello World! What a wonderful World :)";
char fname[32] = "my_file1";
int szLen = strlen( buf );
elapsedMicros my_us;


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

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.fs_mount();
  Serial.println("Directory contents:");
  eRAM.fs_listDir();
  Serial.println("'ENTER' '2' for Loop Test2");
}

void loop() {
  char chIn = 9;
  if ( Serial.available() ) {
    do {
      if ( chIn != '2' )
        chIn = Serial.read();
      else
        Serial.read();
    }
    while ( Serial.available() );
  }
  if ( chIn == '2' ) {
    Serial.println();
    Serial.println("Loop Test2");
    loopTest2();
  }
}


void loopTest2() {
  char xData[12048], xData1[12048];
  static char chInc='A';
  char fname1[32] = "?_loopTest2";
  //spiffs_file fd1 = SPIFFS_open(&fs, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  fname1[0]=chInc;
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
  chInc++;
  if ( chInc > 'Z' ) chInc = 'A';
}
