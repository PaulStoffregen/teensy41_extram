/**************************************************************************/
/*!
    @file     FRAM_MB85RC_I2C.cpp
    @author   SOSAndroid (E. Ha.)
    @license  BSD (see license.txt)

    Driver for the MB85RC I2C FRAM from Fujitsu.

    @section  HISTORY

    v1.0 - First release

	
*/
/**************************************************************************/

#include <stdlib.h>

#include "spiffs_t4.h"
#include "spiffs.h"

extern "C" {
  extern uint8_t external_psram_size;
}

/*========================================================================*/
/*                            CONSTRUCTORS                                */
/*========================================================================*/

/**************************************************************************/
/*!
    Constructor
*/
/**************************************************************************/

	static spiffs fs; //filesystem
	spiffs_file fd1;
	
	uint8_t _spiffs_region;
	static const uint32_t flashBaseAddr[3] = { 0x800000u,  0x800000u};
	static const uint32_t eramBaseAddr = 0x07000000u;
	static char flashID[8];
	static const void* extBase = (void*)0x70000000u;
	//4meg = 4,194,304‬bytes
	static uint32_t flashCapacity[3] = {16u * 1024u * 1024u,  8u * 1024u * 1024u};
	
spiffs_t4::spiffs_t4()
{

}

/*========================================================================*/
/*                           PUBLIC FUNCTIONS                             */
/*========================================================================*/

//int8_t spiffs_t4::begin(uint8_t config, uint8_t spiffs_region) {
int8_t spiffs_t4::begin( ) {
	memset(flashID, 0, sizeof(flashID));
	int8_t result = -1;
	
	_spiffs_region = 0;
	
	 // _spiffs_region = spiffs_region;
	if(external_psram_size == 16){
		Serial.println("You have 2 PSRAM Chips No Flash Installed Exiting!");
		Serial.println("2nd PSRAM chip will be used for SPIFFS");
		_spiffs_region = 1;
		result = 1;
	} else {
	  FLEXSPI2_FLSHA2CR0 = 0x4000;
	  FLEXSPI2_FLSHA2CR1 = FLEXSPI_FLSHCR1_CSINTERVAL(2)
		| FLEXSPI_FLSHCR1_TCSH(3) | FLEXSPI_FLSHCR1_TCSS(3);
	  FLEXSPI2_FLSHA2CR2 = FLEXSPI_FLSHCR2_AWRSEQID(6) | FLEXSPI_FLSHCR2_AWRSEQNUM(0)
		| FLEXSPI_FLSHCR2_ARDSEQID(5) | FLEXSPI_FLSHCR2_ARDSEQNUM(0);

		
	  // cmd index 7 = read ID bytes
	  FLEXSPI2_LUT28 = LUT0(CMD_SDR, PINS1, 0x9F) | LUT1(READ_SDR, PINS1, 1);

	  printStatusRegs();

	  // ----------------- FLASH only ----------------------------------------------

	  // cmd index 8 = read Status register #1 SPI
	  FLEXSPI2_LUT32 = LUT0(CMD_SDR, PINS1, 0x05) | LUT1(READ_SDR, PINS1, 1);

	  // cmd index 9 = read Status register #2 SPI
	  FLEXSPI2_LUT36 = LUT0(CMD_SDR, PINS1, 0x35) | LUT1(READ_SDR, PINS1, 1);

	  //cmd index 10 = exit QPI mode
	  FLEXSPI2_LUT40 = LUT0(CMD_SDR, PINS4, 0xFF);

	  //cmd index 11 = write enable QPI
	  FLEXSPI2_LUT44 = LUT0(CMD_SDR, PINS4, 0x06);

	  //cmd index 12 = sector erase
	  FLEXSPI2_LUT48 = LUT0(CMD_SDR, PINS4, 0x20) | LUT1(ADDR_SDR, PINS4, 24);

	  //cmd index 13 = page program
	  FLEXSPI2_LUT52 = LUT0(CMD_SDR, PINS4, 0x02) | LUT1(ADDR_SDR, PINS4, 24);
	  FLEXSPI2_LUT53 = LUT0(WRITE_SDR, PINS4, 1);

	  //cmd index 14 = set read parameters
	  FLEXSPI2_LUT56 = LUT0(CMD_SDR, PINS4, 0xc0) | LUT1(CMD_SDR, PINS4, 0x20);

	  //cmd index 15 = enter QPI mode
	  FLEXSPI2_LUT60 = LUT0(CMD_SDR, PINS1, 0x38);
	  
	  // reset the chip
	  flexspi_ip_command(10,flashBaseAddr[_spiffs_region]); //exit QPI
	  flexspi_ip_command(1, flashBaseAddr[_spiffs_region]); //reset enable
	  flexspi_ip_command(2, flashBaseAddr[_spiffs_region]); //reset
	  delayMicroseconds(50);

	  flexspi_ip_read(7, flashBaseAddr[_spiffs_region], flashID, sizeof(flashID) ); // flash begins at offset 0x01000000

	#if 0
	  Serial.print("FLASH ID:");
	  for (unsigned i = 0; i < sizeof(flashID); i++) Serial.printf(" %02X", flashID[i]);
	  Serial.printf("\n");
	  Serial.printf("at 0x %x\n", flashBaseAddr[_spiffs_region]);
	#endif
	
	

	  if(flashID[0] != 0xEF && (flashID[1] != 0x40 || flashID[1] != 0x70)) {
		  Serial.println("No FLASH INSTALLED!!!!");
		  exit(1);
	  }
	  //TODO!!!!! set QPI enable bit in status reg #2 if not factory set!!!!!
	  //if( flashID[1] == 0x70){
		uint8_t val[0];
		flexspi_ip_read(9, flashBaseAddr[_spiffs_region], &val, 1 );	

		val[0] |= 1<<1;

		FLEXSPI2_LUT28 = LUT0(CMD_SDR, PINS1, 0x50);
		flexspi_ip_command(7, flashBaseAddr[_spiffs_region]);

		  FLEXSPI2_LUT28 = LUT0(CMD_SDR, PINS1, 0x31) | LUT1(WRITE_SDR, PINS1, 1);
		  flexspi_ip_write(7, flashBaseAddr[_spiffs_region], val, 1);
	  //}
	  
	  //  Serial.println("ENTER QPI MODE");
	  flexspi_ip_command(15, flashBaseAddr[_spiffs_region]);

	  //patch LUT for QPI:
	  // cmd index 8 = read Status register #1
	  FLEXSPI2_LUT32 = LUT0(CMD_SDR, PINS4, 0x05) | LUT1(READ_SDR, PINS4, 1);
	  // cmd index 9 = read Status register #2
	  FLEXSPI2_LUT36 = LUT0(CMD_SDR, PINS4, 0x35) | LUT1(READ_SDR, PINS4, 1);
	  
	  flexspi_ip_command(14, flashBaseAddr[_spiffs_region]);
	  spiffs_t4::printStatusRegs();

	  result = 0;
	}
	
	  printStatusRegs();
	  
	  /* Table of clock settings reference only:
		SEL: 0  DIV: 7 = 49.50000 MHz ok
		SEL: 0  DIV: 6 = 56.57143 MHz ok
		SEL: 0  DIV: 5 = 66.00000 MHz ok
		SEL: 0  DIV: 4 = 79.20000 MHz ok
		SEL: 0  DIV: 3 = 99.00000 MHz ok
		SEL: 0  DIV: 2 = 132.00000 MHz
		SEL: 0  DIV: 1 = 198.00000 MHz
		SEL: 0  DIV: 0 = 396.00000 MHz
		SEL: 1  DIV: 7 = 90.00000 MHz ok
		SEL: 1  DIV: 6 = 102.85714 MHz ok
		SEL: 1  DIV: 5 = 120.00000 MHz
		SEL: 1  DIV: 4 = 144.00000 MHz
		SEL: 1  DIV: 3 = 180.00000 MHz
		SEL: 1  DIV: 2 = 240.00000 MHz
		SEL: 1  DIV: 1 = 360.00000 MHz
		SEL: 1  DIV: 0 = 720.00000 MHz
		SEL: 2  DIV: 7 = 83.07750 MHz ok
		SEL: 2  DIV: 6 = 94.94572 MHz ok
		SEL: 2  DIV: 5 = 110.77000 MHz ok
		SEL: 2  DIV: 4 = 132.92400 MHz
		SEL: 2  DIV: 3 = 166.15500 MHz
		SEL: 2  DIV: 2 = 221.53999 MHz
		SEL: 2  DIV: 1 = 332.31000 MHz
		SEL: 2  DIV: 0 = 664.62000 MHz
		SEL: 3  DIV: 7 = 66.00000 MHz ok
		SEL: 3  DIV: 6 = 75.42857 MHz ok
		SEL: 3  DIV: 5 = 88.00000 MHz ok
		SEL: 3  DIV: 4 = 105.60000 MHz ok
		SEL: 3  DIV: 3 = 132.00000 MHz
		SEL: 3  DIV: 2 = 176.00000 MHz
		SEL: 3  DIV: 1 = 264.00000 MHz
		SEL: 3  DIV: 0 = 528.00000 MHz
	  */
	  
	  //Reset clock to 132.9 Mhz
	  // turn on clock  (TODO: increase clock speed later, slow & cautious for first release)
	  CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_OFF);
	  CCM_CBCMR = (CCM_CBCMR & ~(CCM_CBCMR_FLEXSPI2_PODF_MASK | CCM_CBCMR_FLEXSPI2_CLK_SEL_MASK))
		  | CCM_CBCMR_FLEXSPI2_PODF(4) | CCM_CBCMR_FLEXSPI2_CLK_SEL(2); // 528/5 = 132 MHz
	  CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_ON);
	  
	return result;

}

void spiffs_t4::printStatusRegs() {
#if 0
  uint8_t val;

  flexspi_ip_read(8, flashBaseAddr[_spiffs_region], &val, 1 );
  Serial.print("Status 1:");
  //Serial.printf(" %02X", val);
  //Serial.printf("\n");
  Serial.println(val, BIN);

  // cmd index 9 = read Status register #2 SPI
  flexspi_ip_read(9, flashBaseAddr[_spiffs_region], &val, 1 );
  Serial.print("Status 2:");
  //Serial.printf(" %02X", val);
  //Serial.printf("\n");
  Serial.println(val, BIN);
#endif
}

void spiffs_t4::flexspi_ip_command(uint32_t index, uint32_t addr)
{
  uint32_t n;
  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(index);
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
  while (!((n = FLEXSPI2_INTR) & FLEXSPI_INTR_IPCMDDONE)); // wait
  if (n & FLEXSPI_INTR_IPCMDERR) {
    Serial.printf("Error: FLEXSPI2_IPRXFSTS=%08lX\n", FLEXSPI2_IPRXFSTS);
  }
  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
}

void spiffs_t4::flexspi_ip_read(uint32_t index, uint32_t addr, void *data, uint32_t length)
{
  uint32_t n;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *src;

  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(index) | FLEXSPI_IPCR1_IDATSZ(length);
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
  while (!((n = FLEXSPI2_INTR) & FLEXSPI_INTR_IPCMDDONE)) {
    if (n & FLEXSPI_INTR_IPRXWA) {
      //Serial.print("*");
      if (length >= 8) {
        length -= 8;
        *(uint32_t *)(p+0) = FLEXSPI2_RFDR0;
        *(uint32_t *)(p+4) = FLEXSPI2_RFDR1;
        p += 8;
      } else {
        src = (const uint8_t *)&FLEXSPI2_RFDR0;
        while (length > 0) {
          length--;
          *p++ = *src++;
        }
      }
      FLEXSPI2_INTR = FLEXSPI_INTR_IPRXWA;
    }
  }
  if (n & FLEXSPI_INTR_IPCMDERR) {
    Serial.printf("Error: FLEXSPI2_IPRXFSTS=%08lX\r\n", FLEXSPI2_IPRXFSTS);
  }
  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
  //printf(" FLEXSPI2_RFDR0=%08lX\r\n", FLEXSPI2_RFDR0);
  //if (length > 4) Serial.printf(" FLEXSPI2_RFDR1=%08lX\n", FLEXSPI2_RFDR1);
  //if (length > 8) Serial.printf(" FLEXSPI2_RFDR1=%08lX\n", FLEXSPI2_RFDR2);
  //if (length > 16) Serial.printf(" FLEXSPI2_RFDR1=%08lX\n", FLEXSPI2_RFDR3);
  src = (const uint8_t *)&FLEXSPI2_RFDR0;
  while (length > 0) {
    *p++ = *src++;
    length--;
  }
  if (FLEXSPI2_INTR & FLEXSPI_INTR_IPRXWA) FLEXSPI2_INTR = FLEXSPI_INTR_IPRXWA;
}

void spiffs_t4::flexspi_ip_write(uint32_t index, uint32_t addr, const void *data, uint32_t length)
{
  const uint8_t *src;
  uint32_t n, wrlen;

  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(index) | FLEXSPI_IPCR1_IDATSZ(length);
  src = (const uint8_t *)data;
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
  
  while (!((n = FLEXSPI2_INTR) & FLEXSPI_INTR_IPCMDDONE)) {

    if (n & FLEXSPI_INTR_IPTXWE) {
      wrlen = length;
      if (wrlen > 8) wrlen = 8;
      if (wrlen > 0) {
  
        //memcpy((void *)&FLEXSPI2_TFDR0, src, wrlen); !crashes sometimes!
        uint8_t *p = (uint8_t *) &FLEXSPI2_TFDR0;
        for (unsigned i = 0; i < wrlen; i++) *p++ = *src++;
  
        //src += wrlen;
        length -= wrlen;
        FLEXSPI2_INTR = FLEXSPI_INTR_IPTXWE;
      }
    }
    
  }
  
  if (n & FLEXSPI_INTR_IPCMDERR) {
    Serial.printf("Error: FLEXSPI2_IPRXFSTS=%08lX\r\n", FLEXSPI2_IPRXFSTS);
  }
  
  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
}


//***************************************************
/*
   This test uses the optional quad spi flash on Teensy 4.1
   https://github.com/pellepl/spiffs/wiki/Using-spiffs
   https://github.com/pellepl/spiffs/wiki/FAQ

   ATTENTION: Flash needs to be empty before first use of SPIFFS


   Frank B, 2020
*/


void spiffs_t4::fs_space(uint32_t * total1, uint32_t *used1)
{
  u32_t total, used;
  SPIFFS_info(&fs, &total, &used);
  *total1 = total;
  *used1 = used;
}

dir spiffs_t4::fs_getDir(uint16_t * numrecs) {
	
   dir entries;
   uint16_t i = 0;
   char buffer[32];
   int retVal, buf_size = 32;
	
   spiffs_DIR d;
   struct spiffs_dirent e;
   struct spiffs_dirent *pe = &e;
	
  SPIFFS_opendir(&fs, "/", &d);
  while ((pe = SPIFFS_readdir(&d, pe))) {
    //Serial.printf("%d %s [%04x] size:%i\n", i, pe->name, pe->obj_id, pe->size);
    retVal = snprintf(buffer, buf_size, "%s", pe->name);
    if (retVal > 0 && retVal < buf_size)
    {
		entries.fnamelen[i] = retVal;
	}
	for(uint8_t j=0; j<retVal; j++)
		entries.filename[i][j] = buffer[j];
	entries.fsize[i] = pe->size;
	entries.fid[i] = pe->obj_id;
	i += 1;
  }
  SPIFFS_closedir(&d);
  
  *numrecs = i;
  
  return entries;
}


void spiffs_t4::fs_listDir() {
	
	spiffs_DIR d;
	struct spiffs_dirent e;
	struct spiffs_dirent *pe = &e;
	
  SPIFFS_opendir(&fs, "/", &d);
  while ((pe = SPIFFS_readdir(&d, pe))) {
    Serial.printf("%s [%04x] size:%i\n", pe->name, pe->obj_id, pe->size);
  }
  SPIFFS_closedir(&d);
}

//Functions to Read/Write all buffered data and close file
int spiffs_t4::f_writeFile(const char* fname, const char *dst, spiffs_flags flags) {
	int szLen = strlen( dst );

  // Surely, I've mounted spiffs before entering here
  spiffs_file fd = SPIFFS_open(&fs, fname, flags, 0);
  if (SPIFFS_write(&fs, fd, (u8_t *)dst, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
  SPIFFS_fflush(&fs, fd);
  if(SPIFFS_close(&fs, fd) < 0) {
	  return SPIFFS_errno(&fs);
  } else {
	  return fd;
  }
}

int spiffs_t4::f_readFile(const char* fname, const char *dst, int szLen, spiffs_flags flags) {
  // Surely, I've mounted spiffs before entering here
  spiffs_file  fd = SPIFFS_open(&fs, fname, flags, 0);
  if (SPIFFS_read(&fs, fd, (u8_t *)dst, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  if(SPIFFS_close(&fs, fd) < 0) {
	  return SPIFFS_errno(&fs);
  } else {
	  return fd;
  }
}

//Basic wrapper functions to sppiff commands.
/**
 * Opens/creates a file.
 * @param fs            the file system struct
 * @param path          the path of the new file
 * @param flags         the flags for the open command, can be combinations of
 *                      SPIFFS_APPEND, SPIFFS_O_TRUNC, SPIFFS_CREAT, SPIFFS_RDONLY,
 *                      SPIFFS_WRONLY, SPIFFS_RDWR, SPIFFS_DIRECT, SPIFFS_EXCL
**/
int spiffs_t4::f_open(spiffs_file &fd, const char* fname, spiffs_flags flags){
	fd = SPIFFS_open(&fs, fname, flags, 0);
	//fd1 = fd;
	if(fd < 0) {
		return SPIFFS_errno(&fs);
	} else {
		return fd;
	}
}

int spiffs_t4::f_write(spiffs_file fd, const char *dst, int szLen) {
	int res;
	//if (SPIFFS_write(&fs, fd1, (u8_t *)dst, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
	res = SPIFFS_write(&fs, fd, (u8_t *)dst, szLen);
	return res;
	
}

int spiffs_t4::f_close_write(spiffs_file fd){
	int res;
	res = SPIFFS_close(&fs, fd);
	res = SPIFFS_fflush(&fs, fd);
	return res;
}

int spiffs_t4::f_read(spiffs_file fd, const char *dst, int szLen) {
	int res;
	res = SPIFFS_read(&fs, fd, (u8_t *)dst, szLen);
	//if (SPIFFS_read(&fs, fd1, (u8_t *)dst, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
	return res;
}

void spiffs_t4::f_close(spiffs_file fd ){
	SPIFFS_close(&fs, fd);

}

/**
 * Moves the read/write file offset. Resulting offset is returned or negative if error.
 * lseek(fs, fd, 0, SPIFFS_SEEK_CUR) will thus return current offset.
 * @param fs            the file system struct
 * @param fh            the filehandle
 * @param offs          how much/where to move the offset
 * @param whence        if SPIFFS_SEEK_SET, the file offset shall be set to offset bytes
 *                      if SPIFFS_SEEK_CUR, the file offset shall be set to its current location plus offset
 *                      if SPIFFS_SEEK_END, the file offset shall be set to the size of the file plus offset, which should be negative
 */

int spiffs_t4::f_seek(spiffs_file fd, int32_t offset, int start){
	int res;
	res = SPIFFS_lseek(&fs, fd, offset, start);
	return res;
}

int spiffs_t4::f_rename(const char* fname_old, const char* fname_new) {
	int res;
	res = SPIFFS_rename(&fs, fname_old, fname_new);
	return res;
}

int spiffs_t4::f_remove(const char* fname) {
	int res;
	res = SPIFFS_remove(&fs, fname);
	return res;
}

int32_t spiffs_t4::f_position(spiffs_file fd ) { 
	int res;
	res = SPIFFS_tell(&fs, fd);
	return res;
}

int spiffs_t4::f_eof(spiffs_file fd ) {
	int res;
	res = SPIFFS_eof(&fs, fd);
	return res;
}

/**
 * Gets file status by path
 * @param fs            the file system struct
 * @param path          the path of the file to stat
 * @param s             the stat struct to populate
 */
void spiffs_t4::f_info(const char* fname, spiffs_stat *s){
	SPIFFS_stat(&fs, fname, s);
}

/*
   Waits for busy bit = 0 (statusregister #1 )
   Timeout is optional
*/
bool spiffs_t4::waitFlash(uint32_t timeout) {
  uint8_t val;
  uint32_t t = millis();
  FLEXSPI_IPRXFCR = FLEXSPI_IPRXFCR_CLRIPRXF; // clear rx fifo
  do {
	if(_spiffs_region == 0) {
		flexspi_ip_read(8, flashBaseAddr[_spiffs_region], &val, 1 );
	} else {
		flexspi_ip_read(5, flashBaseAddr[_spiffs_region], &val, 1 );
	}
    if (timeout && (millis() - t > timeout)) {
	return 1; }
  } while  ((val & 0x01) == 1);
  return 0;
}

void spiffs_t4::eraseFlashChip() {
  flexspi_ip_command(11, flashBaseAddr[_spiffs_region]);
  Serial.println("Erasing... (may take some time)");
  uint32_t t = millis();
  FLEXSPI2_LUT60 = LUT0(CMD_SDR, PINS4, 0x60); //Chip erase
  flexspi_ip_command(15, flashBaseAddr[_spiffs_region]);
#ifdef FLASH_MEMMAP
  arm_dcache_delete((void*)((uint32_t)extBase + flashBaseAddr[_spiffs_region]), flashCapacity[_spiffs_region]);
#endif
  while (waitFlash(500)) {
    Serial.print(".");
  }
  t = millis() - t;
  Serial.printf("\nChip erased in %d seconds.\n", t / 1000);
}

/**************************************************************************/
/*!
    @brief  Erase device by overwriting it to 0x00

*/
/**************************************************************************/
void spiffs_t4::eraseDevice(void) {
		uint32_t i=0;
		uint32_t jj = 0;
		uint8_t *ptrERAM = (uint8_t *)(0x70000000 + flashBaseAddr[1]);	
		
		Serial.println("Start erasing device");
		Serial.flush();
		
		while(i < 8388608){
		  ptrERAM[i] = 0xFF;
		  if(i % 100000 == 0){
			  Serial.print(".");
			  jj++;
			  if(jj % 50 == 0) Serial.println();
		  }
		  i++;
		}
		Serial.println();

		Serial.println("device erased");
		Serial.println("...... ...... ......");
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

 s32_t spiffs_t4::spiffs_read(u32_t addr, u32_t size, u8_t * dst) {
  uint8_t *p;
  p = (uint8_t *)extBase;
  p += addr;
#ifdef FLASH_MEMMAP
  memcpy(dst, p, size);
#else
  flexspi_ip_read(5, addr, dst, size);
#endif
  return SPIFFS_OK;
}

 s32_t spiffs_t4::spiffs_write(u32_t addr, u32_t size, u8_t * src) {
  if(_spiffs_region == 0) {
	flexspi_ip_command(11, flashBaseAddr[_spiffs_region]);  //write enable
	flexspi_ip_write(13, addr, src, size);
  } else {
	  flexspi_ip_write(6, addr, src, size);
  }
#ifdef FLASH_MEMMAP
  arm_dcache_delete((void*)((uint32_t)extBase + addr), size);
#endif
  if(_spiffs_region == 0) 
	waitFlash(); //TODO: Can we wait at the beginning instead?
  return SPIFFS_OK;
}

 s32_t spiffs_t4::fs_erase(u32_t addr, u32_t size) {
  int s = size;
  while (s > 0) { //TODO: Is this loop needed, or is size max 4096?
    flexspi_ip_command(11, flashBaseAddr[_spiffs_region]);  //write enable
    flexspi_ip_command(12, addr);
#ifdef FLASH_MEMMAP
    arm_dcache_delete((void*)((uint32_t)extBase + addr), size);
#endif
    addr += blocksize;
    s -= blocksize;
    waitFlash(); //TODO: Can we wait at the beginning intead?
  }
  return SPIFFS_OK;
}

// overwrite functions from class Print:
void spiffs_t4::printTo(spiffs_file fd){
	fd1 = fd;
}

size_t spiffs_t4::write(uint8_t c) {
	return write(&c, 1);
}

size_t spiffs_t4::write(const uint8_t *buffer, size_t size)
{
	f_write(fd1, buffer, size);
	return 1;
}


//********************************************************************************************************
void spiffs_t4::fs_unmount(){
	SPIFFS_unmount(&fs);
}


void spiffs_t4::fs_mount() {

  spiffs_config cfg;

  cfg.phys_size = flashCapacity[_spiffs_region]; // use 16 MB flash TODO use ID to get capacity
  cfg.phys_addr = /* 0x70000000 + */flashBaseAddr[_spiffs_region]; // start spiffs here (physical adress)
  cfg.phys_erase_block = blocksize; //4K sectors
  cfg.log_block_size = cfg.phys_erase_block; // let us not complicate things
  cfg.log_page_size = LOG_PAGE_SIZE; // as we said

  cfg.hal_read_f = spiffs_read;
  cfg.hal_write_f = spiffs_write;
  cfg.hal_erase_f = fs_erase;

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
