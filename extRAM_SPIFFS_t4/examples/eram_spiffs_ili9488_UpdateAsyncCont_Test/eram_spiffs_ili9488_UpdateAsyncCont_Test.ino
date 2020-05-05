/*
   This test uses the optional quad spi flash on Teensy 4.1
   https://github.com/pellepl/spiffs/wiki/Using-spiffs
   https://github.com/pellepl/spiffs/wiki/FAQ

   ATTENTION: Flash needs to be empty before first use of SPIFFS

   Frank B, 2020
*/
// This one is completely hacked up by KurtE to stress the heck out of
// memory by also doing continuous DMA updates to an ILI9488 display using EXTRAM

#include <extRAM_t4.h>
#include <spiffs.h>

elapsedMicros _dt;
#define dtSTART {_dt=0;}
#define dtEND(a) { Serial.printf( "\n%s()_%s : dt %ul us", __func__, a, (uint32_t)_dt);}
// do dtSTART; and dtEND( "what") to show time

extRAM_t4 eRAM;
spiffs_file file;

//uint8_t config = 2; //0 - init eram only, 1-init flash only, 2-init both
//uint8_t spiffs_region = 0;  //0 - flash, 1 - eram
//2 - 2 4meg eram pseudo partitions
//These have been replaced with defines for:
//INIT_PSRAM_ONLY
//INIT_FLASH_ONLY
//INIT_PSRM_FLASH


#include <ili9488_t3_font_ArialBold.h>
#include <ILI9488_t3.h>

#define TRY_EXTMEM
#define UPDATE_HALF_FRAME

#define ROTATION 3

#include "SPI.h"

//#define USE_SPI1
#if defined(USE_SPI1)
#ifdef ARDUINO_TEENSY41
#define TFT_DC 38
#define TFT_CS  37
#define TFT_RST 36

#define TFT_SCK 27
#define TFT_MISO 39
#define TFT_MOSI 26
#elif defined(ARDUINO_TEENSY40)
#define TFT_DC 2
#define TFT_CS  0
#define TFT_RST 3

#define TFT_SCK 27
#define TFT_MISO 1
#define TFT_MOSI 26

#else
#define TFT_DC 31
#define TFT_CS 10 // any pin will work not hardware
#define TFT_RST 8
#define TFT_SCK 32
#define TFT_MISO 5
#define TFT_MOSI 21
//#define DEBUG_PIN 13
#endif
ILI9488_t3 tft = ILI9488_t3(&SPI1, TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCK, TFT_MISO);

//------------------------------------
#else // default pins
#define TFT_DC  9  // only CS pin 
#define TFT_CS 10   // using standard pin
#define TFT_RST 8
ILI9488_t3 tft = ILI9488_t3(&SPI, TFT_CS, TFT_DC, TFT_RST);
//--------------------------------------
#endif

uint16_t our_pallet[] = {
  ILI9488_BLACK,  ILI9488_RED, ILI9488_GREEN,  ILI9488_BLUE,
  ILI9488_YELLOW, ILI9488_ORANGE, ILI9488_CYAN, ILI9488_PINK
};

#define COUNT_SHUTDOWN_FRAMES 16
volatile uint8_t shutdown_cont_update_count = 0xff;

//#include <extRAM_t4.h>
//extRAM_t4 ext_mem;
EXTMEM RAFB extmem_frame_buffer[ILI9488_TFTWIDTH * ILI9488_TFTHEIGHT];
//#define DO_DEBUG 1

char buf[512] = "Hello World! What a wonderful World :)";
char fname[32] = "my_file1";
int szLen = strlen( buf );
elapsedMicros my_us;


void setup() {
  while (!Serial && (millis() < 4000)) ;
  Serial.begin(115200);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  Serial.printf("Begin: CS:%d, DC:%dRST: %d\n", TFT_CS, TFT_DC, TFT_RST);
  Serial.printf("  Size of RAFB: %d\n", sizeof(RAFB));
  tft.begin(26000000);

  tft.setFrameBuffer(extmem_frame_buffer);
  tft.setRotation(ROTATION);
  eRAM.begin(INIT_PSRM_FLASH);
  tft.useFrameBuffer(true);
  tft.fillScreen(ILI9488_BLACK);
  tft.setCursor(ILI9488_t3::CENTER, ILI9488_t3::CENTER);
  tft.setTextColor(ILI9488_RED);
  tft.setFont(Arial_20_Bold);
  tft.println("*** Auto start ***");
  tft.updateScreen();
  delay(250);
  tft.setFrameCompleteCB(&frame_callback, true);
  // We are not running DMA currently so start it up.
  Serial.println("Starting up DMA Updates");
  shutdown_cont_update_count = 0xff;
  tft.updateScreenAsync(true);

  // Now lets start Spiffs test
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
    int8_t result = eRAM.begin(INIT_PSRAM_ONLY);
    if (result == 0) {
      eRAM.eraseFlashChip();
    } else {
      eRAM.eraseDevice();
    }
  }
#endif

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.begin(INIT_PSRAM_ONLY);
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
  eRAM.f_readFile(fname, buf, sizeof(buf) / sizeof(char), SPIFFS_RDWR);
  Serial.println(buf);

  Serial.println("==================================================");
  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.fs_mount();
  Serial.println("Directory contents:");
  eRAM.fs_listDir();

  //test of print
  Serial.println();
  Serial.println("Using println and printf to printoutput file");
  eRAM.f_open(file, "PRINTOUTPUT", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  eRAM.println("THIS IS A TEST");
  eRAM.printf("Float: %f, Int: %c\n", 26.4, 98);
  eRAM.f_close(file);

  Serial.println("Directory contents:");
  eRAM.fs_listDir();
  Serial.println();
  Serial.println("Test print output:");
  eRAM.f_open(file, "PRINTOUTPUT", SPIFFS_RDONLY);
  eRAM.f_read(file, buf, 512);
  eRAM.f_close(file);
  Serial.println(buf);
  Serial.println("==================================================");

  Serial.println("\n*** Commands ***");
  Serial.println("    0 - Directory contents");
  Serial.println("    1 - Loop Test1 - New Files");
  Serial.println("    2 - Loop Test2 - Overwrite");
  Serial.println("    4 - Loop Test4 - check24()");
  Serial.println("    t - Toggle display updates on or off");
}

void frame_callback() {
  //Serial.printf("FCB: %d %d\n", tft.frameCount(), tft.subFrameCount());
  uint32_t frameCount = tft.frameCount();
  // See if end of test signalled.
  if (shutdown_cont_update_count == COUNT_SHUTDOWN_FRAMES) {
    uint8_t color_index = (frameCount >> 4) & 0x7;
    tft.setCursor(ILI9488_t3::CENTER, ILI9488_t3::CENTER);
    tft.setTextColor(our_pallet[(color_index + 3) & 7]);
    tft.setFont(Arial_20_Bold);
    tft.println("Stop Signalled");
    shutdown_cont_update_count--;
    arm_dcache_flush(extmem_frame_buffer, sizeof(extmem_frame_buffer));
  } else if (shutdown_cont_update_count == 0) {
    tft.setCursor(ILI9488_t3::CENTER, tft.getCursorY());
    tft.println("endUpdateAsync");
    tft.endUpdateAsync();
    Serial.println("after endUpdateAsync");
    arm_dcache_flush(extmem_frame_buffer, sizeof(extmem_frame_buffer));
  } else if (shutdown_cont_update_count < COUNT_SHUTDOWN_FRAMES) {
    shutdown_cont_update_count--;
  } else {
#ifdef UPDATE_HALF_FRAME
    bool draw_frame = false;
    if (((frameCount & 0xf) == 0) && tft.subFrameCount()) {
      draw_frame = true;
      tft.setClipRect(0, 0, tft.width(), tft.height() / 2);
    } else if (((frameCount & 0xf) == 1) && !tft.subFrameCount()) {
      draw_frame = true;
      tft.setClipRect(0, tft.height() / 2, tft.width(), tft.height() / 2);
    }
    if (draw_frame)
#else
    if (tft.subFrameCount()) {
      // lets ignore these right now
      return;
    }
    if ((frameCount & 0xf) == 0)
#endif
    {
      // First pass ignore subframe...
      uint8_t color_index = (frameCount >> 4) & 0x7;
      tft.fillScreen(our_pallet[color_index]);
      tft.drawRect(5, 5, tft.width() - 10, tft.height() - 10, our_pallet[(color_index + 1) & 7]);
      tft.drawRect(25, 25, tft.width() - 50, tft.height() - 50, our_pallet[(color_index + 2) & 7]);

      static uint8_t display_other = 0;
      switch (display_other) {
        case 0:
          tft.fillRect(50, 50, tft.width() - 100, tft.height() - 100, our_pallet[(color_index + 1) & 7]);
          break;
        case 1:
          tft.fillCircle(tft.width() / 2, tft.height() / 2, 100, our_pallet[(color_index + 1) & 7]);
          break;
        case 2:
          tft.fillTriangle(50, 50, tft.width() - 50, 50, tft.width() / 2, tft.height() - 50, our_pallet[(color_index + 1) & 7]);
          break;
      }
      if (!tft.subFrameCount()) {
        display_other++;
        if (display_other > 2) display_other =  0 ;
      }

      arm_dcache_flush(extmem_frame_buffer, sizeof(extmem_frame_buffer));
      tft.setClipRect();
    }
  }

}

void loop(void) {
  // See if any text entered
  int ich;
  if ((ich = Serial.read()) != -1) {
    while (Serial.read() != -1) ;
    switch (ich) {
      case 't':
        toggleOnOffDisplay();
        break;
      case '0':
        dtSTART
        eRAM.fs_listDir();
        dtEND( "0 :: Directory contents:");
        Serial.println();
        break;
      case '1':
        Serial.println("\n1 :: Loop Test1 - New Files");
        loopTest1();
        Serial.println();
        break;
      case '2':
        Serial.println("\n2 :: Loop Test2 - Overwrite");
        loopTest2();
        Serial.println();
        break;
      case '3':
        break;
      case '4':
        Serial.println("\n4 :: Loop Test4 - check24()");
        check24();
        Serial.println();
        break;
    }
  }
}

void toggleOnOffDisplay() {
  if (!tft.asyncUpdateActive()) {
    // We are not running DMA currently so start it up.
    Serial.println("Starting up DMA Updates");
    shutdown_cont_update_count = 0xff;
    tft.updateScreenAsync(true);
  } else {
    shutdown_cont_update_count = COUNT_SHUTDOWN_FRAMES;
    while (shutdown_cont_update_count) ;
    tft.waitUpdateAsyncComplete();
    tft.setCursor(ILI9488_t3::CENTER, tft.getCursorY());
    tft.print("Finished Test\n");
    Serial.println("after waitUpdateAsyncComplete");
    Serial.println("Finished test");

    delay(2000);
    Serial.println("Do normal update to see if data is there");
    tft.updateScreen();
  }

}
void loopTest2() {
  char xData[12048], xData1[12048];

  char fname1[52] = "loopTest2";

  /**
     Opens/creates a file.
     @param fs      the file system struct
     @param path    the path of the new file
     @param flags   the flags for the open command, can be combinations of
                    SPIFFS_APPEND, SPIFFS_TRUNC, SPIFFS_CREAT, SPIFFS_RDONLY,
                    SPIFFS_WRONLY, SPIFFS_RDWR, SPIFFS_DIRECT, SPIFFS_EXCL
  **/
  dtSTART;
  eRAM.f_open(file, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    //if (SPIFFS_write(&fs, fd1, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
    eRAM.f_write(file, xData, 26);
  }
  //SPIFFS_close(&fs, fd1);
  //SPIFFS_fflush(&fs, fd1);
  eRAM.f_close(file);
  dtEND( "write:");

  Serial.println("\nDirectory contents:");
  eRAM.fs_listDir();

  dtSTART;
  eRAM.f_open(file, fname1, SPIFFS_RDWR);
  eRAM.f_read(file, xData1, 2600);
  eRAM.f_close(file);
  dtEND( "f_read:");

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
  dtSTART;
  eRAM.f_open(file, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    //if (SPIFFS_write(&fs, fd1, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
    eRAM.f_write(file, xData, 26);
  }
  //SPIFFS_close(&fs, fd1);
  //SPIFFS_fflush(&fs, fd1);
  eRAM.f_close(file);


  Serial.println("\nDirectory contents:");
  eRAM.fs_listDir();

  eRAM.f_open(file, fname1, SPIFFS_RDWR);
  //if (SPIFFS_read(&fs, fd, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  eRAM.f_read(file, xData1, 2600);
  //SPIFFS_close(&fs, fd);
  eRAM.f_close(file);
  dtEND( "write");


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

#if 0
uint32_t *ptrERAM_32 = (uint32_t *)0x70400012;  // Set to ERAM
const uint32_t  sizeofERAM_32 = 0x3FFF71 / sizeof( ptrERAM_32 ); // sizeof free RAM in uint32_t units.
#else
uint8_t *ptrERAM_32 = (uint8_t *)0x70400012;  // Set to ERAM
const uint32_t  sizeofERAM_32 = 0x3FFF71 / sizeof( ptrERAM_32[0] ); // sizeof free RAM in uint32_t units.
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