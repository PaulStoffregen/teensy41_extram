// Buddhabrot
// j.tarbell   January, 2004
// Albuquerque, New Mexico
// complexification.net
//  http://www.complexification.net/gallery/machines/buddhabrot/appletl/index.html
// based on code by Paul Bourke
// astronomy.swin.edu.au/~pbourke/
// j.tarbell   April, 2005
//
/*
WRITE BMP TO SD CARD
Jeff Thompson
Summer 2012
Writes pixel data to an SD card, saved as a BMP file.  Lots of code
via the following...
BMP header and pixel format:
   http://stackoverflow.com/a/2654860
SD save:
   http://arduino.cc/forum/index.php?topic=112733 (lots of thanks!)
... and the SdFat example files too
www.jeffreythompson.org
*/
// Modified to run on Teensy
// MJS     March, 2019

#include <ili9488_t3_font_ArialBold.h>
#include <ili9488_t3_font_Arial.h>
#include <ILI9488_t3.h>


//#define ENABLE_EXT_DMA_UPDATES in the ILI9488_t3.h file
#define TRY_EXTMEM
//#define UPDATE_HALF_FRAME

#define ROTATION 3

#define TFT_DC  9  // only CS pin 
#define TFT_CS 10   // using standard pin
#define TFT_RST 8
ILI9488_t3 tft = ILI9488_t3(&SPI, TFT_CS, TFT_DC, TFT_RST);

EXTMEM RAFB extmem_frame_buffer[ILI9488_TFTWIDTH * ILI9488_TFTHEIGHT];

uint16_t our_pallet[] = {
  ILI9488_BLACK,  ILI9488_RED, ILI9488_GREEN,  ILI9488_BLUE,
  ILI9488_YELLOW, ILI9488_ORANGE, ILI9488_CYAN, ILI9488_PINK
};

#define COUNT_SHUTDOWN_FRAMES 16
volatile uint8_t shutdown_cont_update_count = 0xff;

//==================================
const int dim = 320;             // screen dimensions (square window)
int bailout = 200;         // number of iterations before bail
int plots = 10000;        // number of plots to execute per frame (x30 = plots per second)

// 2D array to hold exposure values
byte exposure[dim*dim];
int maxexposure;           // maximum exposure value
int time = 0;
int exposures = 0;
int timeBMP = 0;

boolean drawing;

//bmp save
char name[] = "9px_0000.bmp";       // filename convention (will auto-increment)
const int w = dim;                   // image width in pixels
const int h = dim;                    // " height
const boolean debugPrint = true;    // print details of process over serial?

const int imgSize = w*h;
// set fileSize (used in bmp header)
int rowSize = 4 * ((3*w + 3)/4);      // how many bytes in the row (used to create padding)
int fileSize = 54 + h*rowSize;        // headers (54 bytes) + pixel data

unsigned char *img = NULL;

#include <extRAM_t4.h>
#include <spiffs.h>
spiffs_file file1;

extRAM_t4 eRAM;
//uint8_t config = 2; //0 - init eram only, 1-init flash only, 2-init both
//uint8_t spiffs_region = 0;  //0 - flash, 1 - eram
//2 - 2 4meg eram pseudo partitions
//These have been replaced with defines for:
//INIT_PSRAM_ONLY
//INIT_FLASH_ONLY
//INIT_PSRM_FLASH
uint8_t config = INIT_PSRAM_ONLY;

elapsedMicros _dt;
#define dtSTART {_dt=0;}
#define dtEND(a) { Serial.printf( "\n%s()_%s : dt %ul us", __func__, a, (uint32_t)_dt);}
// do dtSTART; and dtEND( "what") to show time

void setup() {
  while (!Serial && (millis() < 4000)) ;
  Serial.begin(115200);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  Serial.printf("Begin: CS:%d, DC:%dRST: %d\n", TFT_CS, TFT_DC, TFT_RST);
  Serial.printf("  Size of RAFB: %d\n", sizeof(RAFB));
  tft.begin(26000000);
  tft.setFrameBuffer(extmem_frame_buffer);
  tft.setRotation(ROTATION);
  
  eRAM.begin(config);

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

  
  tft.useFrameBuffer(true);
  
  tft.setFont(Arial_9);
  tft.fillScreen(ILI9488_BLACK);
  tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);

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

  tft.setTextDatum(TL_DATUM);

  if ( ARM_DWT_CYCCNT == ARM_DWT_CYCCNT ) {
    ARM_DEMCR |= ARM_DEMCR_TRCENA; // T_3.x only needs this
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
    Serial.println("CycleCnt Started.");
  }

   tft.fillScreen(our_pallet[0]);
}

uint32_t CCdiff;
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
    if (1)
#endif
    {
    plotPlots();
    static int Dexposures = 0;
    time++;
    if (time % 30 == 0) {
      timeBMP += 1;
      // show progress every 2 seconds or so...
      CCdiff = ARM_DWT_CYCCNT;
      findMaxExposure();
      CCdiff = ARM_DWT_CYCCNT - CCdiff;
      renderBrot();

      //if(timeBMP % 4 == 0)
                //saveBMP();
      
      // show exposure value
      tft.setFont(Arial_10);
      tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
      tft.drawString("bailout:  ", 0, 0);
      tft.drawNumber(bailout, 0, 25);
      tft.drawString("exposures: ", 0, 40);
      tft.drawNumber(exposures, 0, 60);
      tft.drawNumber(exposures - Dexposures, 0, 80);
      tft.drawString("Cycles: ", 0, 100);
      tft.drawNumber(CCdiff, 0, 120);
      Dexposures = exposures;
    }
  
    if ( exposures > 10000000 ) {
      exposures = 0;
      memset(exposure, 0, sizeof( exposure ));
    }

      arm_dcache_flush(extmem_frame_buffer, sizeof(extmem_frame_buffer));
      tft.setClipRect();
    }
  }

}

void loop() {
  // See if any text entered
  int ich;
  if ((ich = Serial.read()) != -1) {
    while (Serial.read() != -1) ;
    switch (ich) {
      case 't':
        toggleOnOffDisplay();
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

void plotPlots() {
  float x, y;
  // iterate through some plots
  for (int n=0;n<plots;n++) {
    // Choose a random point in same range
    x = randomFloat(-2.0,1.0);
    y = randomFloat(-1.5,1.5);
    if (iterate(x,y,false)) {
      iterate(x,y,true);
      exposures++;
    }
  }
}


//   Iterate the Mandelbrot and return TRUE if the point exits
//   Also handle the drawing of the exit points
boolean iterate(float x0, float y0, boolean drawIt) {
  float x = 0;
  float y = 0;
  float xnew, ynew;
  int ix,iy;

  for (int i=0;i<bailout;i++) {
    xnew = x * x - y * y + x0;
    ynew = 2 * x * y + y0;
    if (drawIt && (i > 3)) {
      ix = int(dim * (xnew + 2.0) / 3.0);
      iy = int(dim * (ynew + 1.5) / 3.0);
      if (ix >= 0 && iy >= 0 && ix < dim && iy < dim) {
        // rotate and expose point
        exposure[ix*dim+iy]++;
      }
    }
    if ((xnew*xnew + ynew*ynew) > 4) {
      // escapes
      return true;
    }
    x = xnew;
    y = ynew;
  }
  // does not escape
  return false;
}

void findMaxExposure() {
  // assume no exposure
  maxexposure=0;
  // find the largest density value
  for (int i=0;i<dim;i++) {
    for (int j=0;j<dim;j++) {
      maxexposure = max(maxexposure,exposure[i*dim+j]);
    }
  }
}


void renderBrot() {
  // draw to screen
  for (int i=0;i<dim;i++) {
    for (int j=0;j<dim;j++) {
      float ramp = exposure[i*dim+j] / (maxexposure / 2.5);
      
      // blow out ultra bright regions
      if (ramp > 3)  {
        ramp = 1;
      }
      uint16_t color = tft.color565(int(ramp*128), int(ramp*128), int(ramp*255));
      tft.drawPixel(j+80, i, color);

      //color c = color(int(ramp*255), int(ramp*255), int(ramp*255));
      //set(j,i,c);
    }
  }
}

double randomFloat(float minf, float maxf)
{
  return minf + random(1UL << 31) * (maxf - minf) / (1UL << 31);  // use 1ULL<<63 for max double values)
}

void saveBMP(){
  // if name exists, create new filename
  for (int i=0; i<10000; i++) {
    name[4] = (i/1000)%10 + '0';    // thousands place
    name[5] = (i/100)%10 + '0';     // hundreds
    name[6] = (i/10)%10 + '0';      // tens
    name[7] = i%10 + '0';           // ones
    int file = eRAM.f_open(file1, name, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR | SPIFFS_EXCL);

    if (file !=  SPIFFS_ERR_FILE_EXISTS ) {
      break;
    } else {
      eRAM.f_close(file1);
    }
  }

  // set fileSize (used in bmp header)
  int rowSize = 4 * ((3*w + 3)/4);      // how many bytes in the row (used to create padding)
  int fileSize = 54 + h*rowSize;        // headers (54 bytes) + pixel data

  img = (unsigned char *)malloc(3*w*h);
  
  for (int i=0;i<dim;i++) {
    for (int j=0;j<dim;j++) {
      float ramp = exposure[i*dim+j] / (maxexposure / 2.5);
      
      // blow out ultra bright regions
      if (ramp > 3)  {
        ramp = 1;
      }
      //uint16_t color = tft.color565(int(ramp*128), int(ramp*128), int(ramp*255));
      //tft.drawPixel(j+80, i, color);
      img[(j*w + i)*3+0] = (unsigned char)(int(ramp*255));    // B
      img[(j*w + i)*3+1] = (unsigned char)(int(ramp*128));    // G
      img[(j*w + i)*3+2] = (unsigned char)(int(ramp*128));    // R
      
      // padding (the 4th byte) will be added later as needed...
      //color c = color(int(ramp*255), int(ramp*255), int(ramp*255));
      //set(j,i,c);
    }
  }

  // create padding (based on the number of pixels in a row
  unsigned char bmpPad[rowSize - 3*w];
  for (int i=0; i<sizeof(bmpPad); i++) {         // fill with 0s
    bmpPad[i] = 0;
  }

  // create file headers (also taken from StackOverflow example)
  unsigned char bmpFileHeader[14] = {            // file header (always starts with BM!)
    'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0   };
  unsigned char bmpInfoHeader[40] = {            // info about the file (size, etc)
    40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0   };

  bmpFileHeader[ 2] = (unsigned char)(fileSize      );
  bmpFileHeader[ 3] = (unsigned char)(fileSize >>  8);
  bmpFileHeader[ 4] = (unsigned char)(fileSize >> 16);
  bmpFileHeader[ 5] = (unsigned char)(fileSize >> 24);

  bmpInfoHeader[ 4] = (unsigned char)(       dim      );
  bmpInfoHeader[ 5] = (unsigned char)(       dim >>  8);
  bmpInfoHeader[ 6] = (unsigned char)(       dim >> 16);
  bmpInfoHeader[ 7] = (unsigned char)(       w >> 24);
  bmpInfoHeader[ 8] = (unsigned char)(       h      );
  bmpInfoHeader[ 9] = (unsigned char)(       h >>  8);
  bmpInfoHeader[10] = (unsigned char)(       h >> 16);
  bmpInfoHeader[11] = (unsigned char)(       h >> 24);

  // write the file (thanks forum!)
  dtSTART;
  eRAM.f_write(file1, bmpFileHeader, sizeof(bmpFileHeader));
  eRAM.f_write(file1, bmpInfoHeader, sizeof(bmpInfoHeader));
  for (int i=0; i<h; i++) {                            // iterate image array
    eRAM.f_write(file1, img+(w*(h-i-1)*3), 3*w);                // write px data
    eRAM.f_write(file1, bmpPad, (4-(w*3)%4)%4);                 // and padding as needed
  }
  dtEND( "0 :: Write Bitmap to SPIFF:");

  free(img);
  eRAM.f_close_write(file1);  // close file when done writing

  Serial.println("Directory contents:");
  eRAM.fs_listDir();
}