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
  }
}

