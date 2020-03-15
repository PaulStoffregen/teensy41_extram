#include <extRAM_t4.h>
#include <spiffs.h>
#include "uSDFS.h"

//Setup 2 files IO
spiffs_file sourceFile;

//Configure uSDS to USB
// for use of USB-HUBs
#include <USBHost_t36.h>

extern USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);

const char *Dev = "2:/";  // USB
FRESULT rc;       /* Result code */
FATFS fatfs;      /* File system object */
FIL fil;          /* File object */
DIR dir;          /* Directory object */
FILINFO fno;      /* File information object */
UINT bw, br, wr;

//set up buffer
int blocksize = 64;
int blockSize = blocksize - 1;
char buf[64] = "";
char buf1[64] = "";
int blocks, remainBytes;

extRAM_t4 eRAM;
//uint8_t config = 0; //0 - init eram only, 1-init flash only, 2-init both
//uint8_t spiffs_region = 1; //0 - flash, 1 - eram
                           //2 - 2 4meg eram pseudo partitions
//These have been replaced with defines for:
//INIT_PSRAM_ONLY
//INIT_FLASH_ONLY
//INIT_FLASH_PSRAM

void setup(){
  Serial.begin(115200);

  Serial.println("Test uSDFS");
  Serial.println(Dev);
  if((rc = f_mount (&fatfs, Dev, 1))) die("Mount",rc);      /* Mount/Unmount a logical drive */

  //-----------------------------------------------------------
  Serial.printf("\nChange drive\n");
  if((rc = f_chdrive(Dev))) die("chdrive",rc);


  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.begin(INIT_PSRAM_ONLY);
  eRAM.fs_mount();

  Serial.println();
  Serial.println("Initial Directory contents:");
  eRAM.fs_listDir();
  
  eRAM.f_open(sourceFile, "PRINTOUTPUT1", SPIFFS_RDONLY);

  spiffs_stat sourceFileInfo;
  eRAM.f_info("PRINTOUTPUT1", &sourceFileInfo);
  Serial.printf("Source File Size: %d\n", sourceFileInfo.size);

  //let's do some math
  blocks = sourceFileInfo.size/blockSize;
  remainBytes = sourceFileInfo.size - blocks*blockSize;
  Serial.printf("Blocks: %d, remainBytes: %d\n", blocks, remainBytes);
  
  if((rc = f_open(&fil, "PRINTOUTPUT1a", FA_WRITE | FA_CREATE_ALWAYS))) die("Open",rc);
  for(int i=0; i < blocks; i++) {
    Serial.println(eRAM.f_position(sourceFile));
    eRAM.f_read(sourceFile, buf, blockSize);
    rc = f_write(&fil, buf, blockSize, &wr);
  }
    eRAM.f_read(sourceFile, buf, remainBytes);
    rc = f_write(&fil, buf, remainBytes, &wr);

  if((rc = f_close(&fil))) die("Close",rc);
  eRAM.f_close(sourceFile);

  printDirectory();

  Serial.println();
  Serial.println("=== Dump File =======");
  if((rc = f_open(&fil, "PRINTOUTPUT1a", FA_READ))) die("Open",rc);
  for(int i=0; i < blocks; i++) {
    rc=f_read(&fil, buf1, blockSize, &wr);
    Serial.print(buf1);
  }
    rc=f_read(&fil, buf1, remainBytes, &wr);
    for(int i=0; i<remainBytes; i++){
      Serial.print(buf1[i]);
    }
    
  if((rc = f_close(&fil))) die("Close",rc);

  pinMode(13,OUTPUT);
}

void loop(){
  // put your main code here, to run repeatedly:
  digitalWriteFast(13,!digitalReadFast(13));
  delay(1000);
}

void printDirectory() {
  //-----------------------------------------------------------
  Serial.println("\nOpen root directory.");
  if((rc = f_opendir(&dir, Dev))) die("Dir",rc);

  Serial.println("Directory listing...");
  for (;;) 
  {
      if((rc = f_readdir(&dir, &fno))) die("Listing",rc);   /* Read a directory item */
      if (!fno.fname[0]) break; /* Error or end of dir */
      if (fno.fattrib & AM_DIR)
           Serial.printf("   <dir>  %s\r\n", fno.fname);
      else
           Serial.printf("%8d  %s\r\n", (int)fno.fsize, fno.fname);
    delay(10);
  }

}

void die(const char *text, FRESULT rc)
{ Serial.printf("%s: Failed with rc=%s.\r\n", text,FR_ERROR_STRING[rc]);  while(1) asm("wfi"); }
