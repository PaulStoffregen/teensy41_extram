/*
   This test uses the optional quad spi flash on Teensy 4.1
   https://github.com/pellepl/spiffs/wiki/Using-spiffs
   https://github.com/pellepl/spiffs/wiki/FAQ

   ATTENTION: Flash needs to be empty before first use of SPIFFS


   Frank B, 2020
*/


#include <spiffs.h>
#include <extRAM_t4.h>
extRAM_t4 eRAM;
uint8_t config = 2; //0 - init eram only, 1-init flash only, 2-init both

static spiffs fs; //filesystem

char buf[512] = "Hello World! What a wonderful World :) Hello World! What a wonderful World :) Hello World! What a wonderful World :) Hello World! What a wonderful World :)  Hello World! What a wonderful World :) Hello World! What a wonderful World :)";
int szLen = strlen( buf );
elapsedMicros my_us;

//random address to write from
uint16_t writeaddress = 0x00;
uint8_t valERAM;
uint8_t *ptrERAM = (uint8_t *)0x70000000;  // Set to ERAM
const uint32_t  sizeofERAM = 0x7FFFFE / sizeof( valERAM ); // sizeof free RAM in

void test_spiffs_write() {
  // Surely, I've mounted spiffs before entering here
  spiffs_file fd = SPIFFS_open(&fs, "my_file", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  if (SPIFFS_write(&fs, fd, (u8_t *)buf, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
  SPIFFS_fflush(&fs, fd);
}

static void test_spiffs_read() {
  // Surely, I've mounted spiffs before entering here
  spiffs_file  fd = SPIFFS_open(&fs, "my_file", SPIFFS_RDWR, 0);
  if (SPIFFS_read(&fs, fd, (u8_t *)buf, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
}

static void test_spiffs_listDir() {
  spiffs_DIR d;
  struct spiffs_dirent e;
  struct spiffs_dirent *pe = &e;

  SPIFFS_opendir(&fs, "/", &d);
  while ((pe = SPIFFS_readdir(&d, pe))) {
    Serial.printf("%s [%04x] size:%i\n", pe->name, pe->obj_id, pe->size);
  }
  SPIFFS_closedir(&d);
}

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
    eRAM.begin(config);
    eraseFlashChip();
  }
#endif

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.begin(config);
  my_spiffs_mount();

#if 1
  Serial.println("Write file:");
  Serial.println(buf);
  test_spiffs_write();
#endif

  Serial.println();
  Serial.println("Directory contents:");
  test_spiffs_listDir();

  memset(buf, 0, sizeof(buf)); //emtpy buffer
  Serial.println("Read file:");
  test_spiffs_read();
  Serial.println(buf);

  Serial.println();
  //eRAM.eramBegin();
  delay(100);
  check42();
  check24();

  Serial.println();
  Serial.println("Mount SPIFFS:");
  my_spiffs_mount();
  Serial.println("Directory contents:");
  test_spiffs_listDir();
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
    loopTest2();

    Serial.println("Loop Test");
    loopTest();

  }
}

void loopTest2() {
  int szLen;
  char xData[12048], xData1[12048], xData2[26];

  spiffs_file fd = SPIFFS_open(&fs, "loopTest2", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);

  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    if (SPIFFS_write(&fs, fd, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  }
  SPIFFS_close(&fs, fd);
  SPIFFS_fflush(&fs, fd);

  Serial.println("Directory contents:");
  test_spiffs_listDir();

  spiffs_file  fd1 = SPIFFS_open(&fs, "loopTest2", SPIFFS_RDWR, 0);
  if (SPIFFS_read(&fs, fd1, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);

  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    eRAM.writeArrayDMA(ii * 26, 26, (uint8_t*)xData);
  }

#ifdef DO_DEBUG
  Serial.println();
  for ( int ii = 0; ii < 100; ii++) {
    eRAM.readArrayDMA(ii * 26, 26, (uint8_t*)xData2);
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
  char xData[12048];
  static char fIdx = 'A';
  sprintf(erName, "%s%c", "F_" __TIME__, fIdx++);
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
  spiffs_file fd = SPIFFS_open(&fs, erName, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  if (SPIFFS_write(&fs, fd, &erData[0], 2600 ) < 0) Serial.printf("loopTest() errno %i\n", SPIFFS_errno(&fs));
  if ( fIdx > 'C') {
    Serial.println("\t first 2600 bytes");
    if (SPIFFS_write(&fs, fd, &erData[2600], kk - 2600 ) < 0) Serial.printf("loopTest() errno %i\n", SPIFFS_errno(&fs));
  }
  SPIFFS_close(&fs, fd);
  Serial.printf( "\t loopTest write took %lu elapsed us\n", (uint32_t)my_us );
  Serial.println("\t loopTest Directory contents:");
  test_spiffs_listDir();
#ifdef DO_DEBUG
  Serial.println( erName );
  Serial.printf( "%s\n", erData );
#endif
  Serial.println();
}





//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************
/*
   QSPI Flash Interface
*/

#define LUT0(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)))
#define LUT1(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define CMD_SDR         FLEXSPI_LUT_OPCODE_CMD_SDR
#define ADDR_SDR        FLEXSPI_LUT_OPCODE_RADDR_SDR
#define READ_SDR        FLEXSPI_LUT_OPCODE_READ_SDR
#define WRITE_SDR       FLEXSPI_LUT_OPCODE_WRITE_SDR
#define DUMMY_SDR       FLEXSPI_LUT_OPCODE_DUMMY_SDR
#define PINS1           FLEXSPI_LUT_NUM_PADS_1
#define PINS4           FLEXSPI_LUT_NUM_PADS_4

#define FLASH_MEMMAP 1 //Use memory-mapped access

static const void* extBase = (void*)0x70000000u;
//static const uint32_t flashBaseAddr = 0x01000000u;
static uint32_t flashCapacity = 16u * 1024u * 1024u;
//static char flashID[8];
/*
   Waits for busy bit = 0 (statusregister #1 )
   Timeout is optional
*/
bool waitFlash(uint32_t timeout = 0) {
  uint8_t val;
  uint32_t t = millis();
  FLEXSPI_IPRXFCR = FLEXSPI_IPRXFCR_CLRIPRXF; // clear rx fifo
  do {
    eRAM.flexspi_ip_read(8, flashBaseAddr, &val, 1 );
    if (timeout && (millis() - t > timeout)) return 1;
  } while  ((val & 0x01) == 1);
  return 0;
}


void eraseFlashChip() {
  //setupFlexSPI2();
  //setupFlexSPI2Flash();
  eRAM.flexspi_ip_command(11, flashBaseAddr);

  Serial.println("Erasing... (may take some time)");
  uint32_t t = millis();
  FLEXSPI2_LUT60 = LUT0(CMD_SDR, PINS4, 0x60); //Chip erase
  eRAM.flexspi_ip_command(15, flashBaseAddr);
#ifdef FLASH_MEMMAP
  arm_dcache_delete((void*)((uint32_t)extBase + flashBaseAddr), flashCapacity);
#endif
  while (waitFlash(500)) {
    Serial.print(".");
  }
  t = millis() - t;
  Serial.printf("\nChip erased in %d seconds.\n", t / 1000);
}

//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************
/*
   SPIFFS interface
*/

#define LOG_PAGE_SIZE       256

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * 4];
static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];

//********************************************************************************************************
static const u32_t blocksize = 4096; //or 32k or 64k (set correct flash commands above)

static s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t * dst) {
  uint8_t *p;
  p = (uint8_t *)extBase;
  p += addr;
#ifdef FLASH_MEMMAP
  memcpy(dst, p, size);
#else
  eRAM.flexspi_ip_read(5, addr, dst, size);
#endif
  return SPIFFS_OK;
}

static s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t * src) {
  eRAM.flexspi_ip_command(11, flashBaseAddr);  //write enable
  eRAM.flexspi_ip_write(13, addr, src, size);
#ifdef FLASH_MEMMAP
  arm_dcache_delete((void*)((uint32_t)extBase + addr), size);
#endif
  waitFlash(); //TODO: Can we wait at the beginning instead?
  return SPIFFS_OK;
}

static s32_t my_spiffs_erase(u32_t addr, u32_t size) {
  int s = size;
  while (s > 0) { //TODO: Is this loop needed, or is size max 4096?
    eRAM.flexspi_ip_command(11, flashBaseAddr);  //write enable
    eRAM.flexspi_ip_command(12, addr);
#ifdef FLASH_MEMMAP
    arm_dcache_delete((void*)((uint32_t)extBase + addr), size);
#endif
    addr += blocksize;
    s -= blocksize;
    waitFlash(); //TODO: Can we wait at the beginning intead?
  }
  return SPIFFS_OK;
}

//********************************************************************************************************

void my_spiffs_mount() {

  //setupFlexSPI2();
  //setupFlexSPI2Flash();

  spiffs_config cfg;

  cfg.phys_size = flashCapacity; // use 16 MB flash TODO use ID to get capacity
  cfg.phys_addr = /* 0x70000000 + */flashBaseAddr; // start spiffs here (physical adress)
  cfg.phys_erase_block = blocksize; //4K sectors
  cfg.log_block_size = cfg.phys_erase_block; // let us not complicate things
  cfg.log_page_size = LOG_PAGE_SIZE; // as we said

  cfg.hal_read_f = my_spiffs_read;
  cfg.hal_write_f = my_spiffs_write;
  cfg.hal_erase_f = my_spiffs_erase;

  int res = SPIFFS_mount(&fs,
                         &cfg,
                         spiffs_work_buf,
                         spiffs_fds,
                         sizeof(spiffs_fds),
                         spiffs_cache_buf,
                         sizeof(spiffs_cache_buf),
                         0);
  Serial.printf("Mount ADDR 0x%X with res: %i\n", cfg.phys_addr, res);
}

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
