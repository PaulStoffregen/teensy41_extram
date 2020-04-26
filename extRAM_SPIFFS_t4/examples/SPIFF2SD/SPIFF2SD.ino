/***************************************************
  This is our Bitmap drawing example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include <ILI9488_t3.h> // Hardware-specific library
#include <SPI.h>

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.

#define TFT_DC  9
#define TFT_CS 10
ILI9488_t3 tft = ILI9488_t3(&SPI, TFT_CS, TFT_DC, 8);

#include <extRAM_t4.h>
#include <spiffs.h>
spiffs_file file1;

extRAM_t4 eRAM;
//uint8_t config = 0; //0 - init eram only, 1-init flash only, 2-init both
//uint8_t spiffs_region = 1; //0 - flash, 1 - eram
                           //2 - 2 4meg eram pseudo partitions
//These have been replaced with defines for:
//INIT_PSRAM_ONLY
//INIT_FLASH_ONLY
//INIT_PSRM_FLASH

uint8_t config = INIT_PSRAM_ONLY;

void setup(void) {

  tft.begin();
  tft.fillScreen(ILI9488_BLUE);

  Serial.begin(9600);
  tft.setTextColor(ILI9488_WHITE);
  tft.setTextSize(2);
  tft.println(F("Waiting for Arduino Serial Monitor..."));
  while (!Serial) {
    if (millis() > 8000) break;
  }

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.begin(config);
  eRAM.fs_mount();
  Serial.println("OK!");

  tft.fillScreen(ILI9488_BLACK);
  bmpDraw("9px_0000.bmp", 0, 0);
  delay(5000);
  tft.fillScreen(ILI9488_BLACK);
  bmpDraw("9px_0001.bmp", 0, 0);
  delay(5000);
  tft.fillScreen(ILI9488_BLACK);
  bmpDraw("9px_0002.bmp", 0, 0);
  delay(5000);
  tft.fillScreen(ILI9488_BLACK);
  bmpDraw("9px_0003.bmp", 0, 0);
  delay(5000);
}

void loop() {

}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance for tiny AVR chips.

// Larger buffers are slightly more efficient, but if
// the buffer is too large, extra data is read unnecessarily.
// For example, if the image is 240 pixels wide, a 100
// pixel buffer will read 3 groups of 100 pixels.  The
// last 60 pixels from the 3rd read may not be used.

#define BUFFPIXEL 80


//===========================================================
// Try Draw using writeRect
void bmpDraw(const char *bmpFile, uint8_t x, uint16_t y) {

  eRAM.f_open(file1,bmpFile, SPIFFS_RDONLY);
  Serial.println(file1);
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint16_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();
  int32_t position;

  uint16_t awColors[320];  // hold colors for one row at a time...

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(bmpFile);
  Serial.println('\'');

  // Open requested file on SD card


  // Parse BMP header
  if(read16() == 0x4D42) { // BMP signature
    position += 2;
    Serial.print(F("File size: ")); Serial.println(read32());
    (void)read32(); // Read & ignore creator bytes
    position += 4;
    bmpImageoffset = read32(); // Start of image data
    position += 4;
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32());
    bmpWidth  = read32();
    bmpHeight = read32();
    position += 8;
    if(read16() == 1) { // # planes -- must be '1'
      bmpDepth = read16(); // bits per pixel
      position += 2;
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32() == 0)) { // 0 = uncompressed
        position += 4;
        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(eRAM.f_seek(file1, 0, SPIFFS_SEEK_CUR) != pos) { // Need seek?
            eRAM.f_seek(file1, pos, SPIFFS_SEEK_SET);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              eRAM.f_read(file1, sdbuffer, sizeof(sdbuffer));
              position += sizeof(sdbuffer);
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            awColors[col] = tft.color565(r,g,b);
          } // end pixel
          tft.writeRect(0, row, w, 1, awColors);
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  eRAM.f_close(file1);
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}



// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16() {
  uint16_t result;
  char buf[1];
  eRAM.f_read(file1, buf, 1); // LSB
  ((uint8_t *)&result)[0] = buf[0];
  eRAM.f_read(file1, buf, 1); // MSB
  ((uint8_t *)&result)[1] = buf[0];
  return result;
}

uint32_t read32() {
  uint32_t result;
  char buf[1];
  eRAM.f_read(file1,buf, 1); // LSB
  ((uint8_t *)&result)[0] = buf[0];
  eRAM.f_read(file1,buf, 1);
  ((uint8_t *)&result)[1] = buf[0];
  eRAM.f_read(file1,buf, 1);
  ((uint8_t *)&result)[2] = buf[0];
  eRAM.f_read(file1,buf, 1); // MSB
  ((uint8_t *)&result)[3] = buf[0];
  return result;
}