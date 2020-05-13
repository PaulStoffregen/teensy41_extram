# teensy41_extram

**WARNING: Both the SPIFFS_t4 and the extRAM_SPIFFS_t4 libraries need to be installed into your libraries folder to use the external PSRAM or FLASH!**

Implemation of SPIFFS using Peter Andersson (pelleplutt1976 at gmail.com) SPIFFS library as the base: https://github.com/pellepl/spiffs.  Wrapper functions created to make function calls similar to SDCard card functions.  Makes it easier to convert.

User Frank B did the all effort to implement SPIFFS on the T4.1. While others have done the wrapper functions and the examples.

This supports using the external flash and/or the PSRAM chip on the T4.1.

Notes:
1. Begining of sketch:
```c++
#include <extRAM_t4.h>   //Libraries to include for 
#include <spiffs.h>

//Setup 3 files IO
spiffs_file file1;
spiffs_file file2;
spiffs_file file3;

extRAM_t4 eRAM;
//uint8_t config = 0; //0 - init eram only, 1-init flash only, 2-init both
//uint8_t spiffs_region = 1; //0 - flash, 1 - eram
                           //2 - 2 4meg eram pseudo partitions```
//These have been replaced with defines for:
//INIT_PSRAM_ONLY
//INIT_FLASH_ONLY
//INIT_FLASH_PSRAM                           

2. In setup
```c++
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
#endif```
On first use of the flash or the PSRAM the chips need to be initialized.  This allows the user to select if they want to initialize or not.

3.   after that you call begin with the selected configuration and mount the file system.
```c++
  eRAM.begin(config, spiffs_region);
  eRAM.fs_mount();
  ```
4. Following wrapper functions are available for user.
```c++
f_open(FileHandle, fileName, flags)         //fileHandle is created when you call this function and
                                            //and specified by spiffs_file FileHandle after the include statements
                                            //more on flags later
f_write(FileHandle, buffer, bufferSize);    //write buffer to file
f_read(FileHandle, buffer, bufferSize);     //write buffer to file
f_close(FileHandle);                        //close file
```
5.  Additional wrapper functions:
```c++
	f_position(FileHandle );                       //gets current position in file
	f_eof(FileHandle);                             //tests for end of file 
	f_seek(FileHandle, offset, whence);            //seek position in file
	f_rename(fname_old, fname_new);                //renames a file
	f_remove(fname);                               //removes a file by filename from the File System
```

6.  Flags:
```
  the flags for the open command, can be combinations of
      SPIFFS_APPEND, SPIFFS_O_TRUNC, SPIFFS_CREAT, SPIFFS_RDONLY,
      SPIFFS_WRONLY, SPIFFS_RDWR, SPIFFS_DIRECT, SPIFFS_EXCL
 ```
 
 7. whence for f_seek:
 ```
    if SPIFFS_SEEK_SET, the file offset shall be set to offset bytes
    if SPIFFS_SEEK_CUR, the file offset shall be set to its current location plus offset
    if SPIFFS_SEEK_END, the file offset shall be set to the size of the file plus offset, which should be negative
  ```
