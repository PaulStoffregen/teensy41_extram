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
uint8_t config = 0; //0 - init eram only, 1-init flash only, 2-init both
uint8_t spiffs_region = 1; //0 - flash, 1 - eram

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
    eRAM.begin(config, spiffs_region);
    if(spiffs_region == 0){
      eRAM.eraseFlashChip();
    } else {
      eRAM.eraseDevice();
    }
  }
#endif

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.begin(config, spiffs_region);
  eRAM.fs_mount();

#if 1
  Serial.println("Write file:");
  Serial.println(buf);
  eRAM.fs_write(fname, buf);
#endif

  Serial.println();
  Serial.println("Directory contents:");
  eRAM.fs_listDir();

  memset(buf, 0, sizeof(buf)); //emtpy buffer
  Serial.println("Read file:");
  eRAM.fs_read(fname, buf, sizeof(buf)/sizeof(char));
  Serial.println(buf);

  Serial.println();

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.fs_mount();
  Serial.println("Directory contents:");
  eRAM.fs_listDir();
}

void loop() {
  char chIn = 255;
  if ( Serial.available() ) {
    do {
      if ( chIn != '2' && chIn != '1' )
        chIn = Serial.read();
      else
        Serial.read();
    }
    while ( Serial.available() );
  }
  if ( chIn == '1' ) {
    Serial.println();
    Serial.println("Loop Test1 - New Files");
    loopTest1();
    Serial.println();
  }
  if ( chIn == '2' ) {
    Serial.println();
    Serial.println("Loop Test2 - Overwrite");
    loopTest2();
    Serial.println();
  }
}

void loopTest2() {
  char xData[12048], xData1[12048];

  char fname1[52] = "loopTest2";
  
  //spiffs_file fd1 = SPIFFS_open(&fs, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  eRAM.fs_open_write(fname1);
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    //if (SPIFFS_write(&fs, fd1, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
    eRAM.write_fs(xData,26);
  }
  //SPIFFS_close(&fs, fd1);
  //SPIFFS_fflush(&fs, fd1);
  eRAM.fs_close_write();
  

  Serial.println("Directory contents:");
  eRAM.fs_listDir();

  eRAM.fs_open_read(fname1);
  //if (SPIFFS_read(&fs, fd, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  eRAM.read_fs(xData1,2600);
  //SPIFFS_close(&fs, fd);
  eRAM.fs_close();


#ifdef DO_DEBUG
  Serial.println();
  for ( int ii = 0; ii < 100; ii++) {
    Serial.println("  <<< eRAM");
    for ( int jj = 0; jj < 26; jj++) {
      Serial.print( xData1[jj + (ii * 26)]);
    }
    Serial.println("  <<< FLASH");
  }
#endif
}

void loopTest1() {
  char xData[12048], xData1[12048];

  char fname1[52];
  static char fIdx = 'A';
  snprintf(fname1, 52, "lt1_%s_%c", __TIME__, fIdx++);
  if ( fIdx > 'Z' ) fIdx = 'A';

  //spiffs_file fd1 = SPIFFS_open(&fs, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  eRAM.fs_open_write(fname1);
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    //if (SPIFFS_write(&fs, fd1, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
    eRAM.write_fs(xData,26);
  }
  //SPIFFS_close(&fs, fd1);
  //SPIFFS_fflush(&fs, fd1);
  eRAM.fs_close_write();
  

  Serial.println("Directory contents:");
  eRAM.fs_listDir();

  eRAM.fs_open_read(fname1);
  //if (SPIFFS_read(&fs, fd, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  eRAM.read_fs(xData1,2600);
  //SPIFFS_close(&fs, fd);
  eRAM.fs_close();


#ifdef DO_DEBUG
  Serial.println();
  for ( int ii = 0; ii < 100; ii++) {
    Serial.println("  <<< eRAM");
    for ( int jj = 0; jj < 26; jj++) {
      Serial.print( xData1[jj + (ii * 26)]);
    }
    Serial.println("  <<< FLASH");
  }
#endif
}