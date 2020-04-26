/**************************************************************************/
/*! 
    @file     extRAM_t4.h
	
    @section  HISTORY

    v1.0 - First release

    Driver for the 
	

*/
/**************************************************************************/
#ifndef _EXTRAM_T4_H_
#define _EXTRAM_T4_H_

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#include <Wire.h>

#include <spiffs.h>

// Enabling debug I2C - comment to disable / normal operations
#ifndef SERIAL_DEBUG
//  #define SERIAL_DEBUG 1
#endif

#define LUT0(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)))
#define LUT1(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define CMD_SDR         FLEXSPI_LUT_OPCODE_CMD_SDR
#define ADDR_SDR        FLEXSPI_LUT_OPCODE_RADDR_SDR
#define READ_SDR        FLEXSPI_LUT_OPCODE_READ_SDR
#define WRITE_SDR       FLEXSPI_LUT_OPCODE_WRITE_SDR
#define DUMMY_SDR       FLEXSPI_LUT_OPCODE_DUMMY_SDR
#define PINS1           FLEXSPI_LUT_NUM_PADS_1
#define PINS4           FLEXSPI_LUT_NUM_PADS_4

// IDs
//Manufacturers codes

// Error management
#define ERROR_0 0 // Success    
#define ERROR_1 1 // Data too long to fit the transmission buffer on Arduino
#define ERROR_2 2 // received NACK on transmit of address
#define ERROR_3 3 // received NACK on transmit of data
#define ERROR_4 4 // Serial seems not available
#define ERROR_5 5 // Not referenced device ID
#define ERROR_6 6 // Unused
#define ERROR_7 7 // Fram chip unidentified
#define ERROR_8 8 // Number of bytes asked to read null
#define ERROR_9 9 // Bit position out of range
#define ERROR_10 10 // Not permitted opération
#define ERROR_11 11 // Memory address out of range


#define INIT_PSRAM_ONLY		0
#define INIT_FLASH_ONLY		1
#define INIT_PSRM_FLASH		2
#define INIT_PSRAM_PART		3

#define FLASH_MEMMAP 1 //Use memory-mapped access


class extRAM_t4 : public Print
{
 public:
	extRAM_t4();
	//int8_t  begin(uint8_t config, uint8_t spiffs_region = 0);
	int8_t  begin(uint8_t _config);
	byte	readBit(uint32_t ramAddr, uint8_t bitNb, byte *bit);
	byte	setOneBit(uint32_t ramAddr, uint8_t bitNb);
	byte	clearOneBit(uint32_t ramAddr, uint8_t bitNb);
	byte	toggleBit(uint32_t ramAddr, uint8_t bitNb);
	
	//void	readArray_old (uint32_t ramAddr, uint32_t items, uint8_t data[]);
	//void	writeArray_old (uint32_t ramAddr, uint32_t items, uint8_t value[]);
	void	readArray (uint32_t ramAddr, uint32_t items, uint8_t data[]);
	void	writeArray (uint32_t ramAddr, uint32_t items, uint8_t value[]);
	
	void	readByte (uint32_t ramAddr, uint8_t *value);
	void	writeByte (uint32_t ramAddr, uint8_t value);
	void	copyByte (uint32_t origAddr, uint32_t destAddr);
	void	readWord(uint32_t ramAddr, uint16_t *value);
	void	writeWord(uint32_t ramAddr, uint16_t value);
	void	readLong(uint32_t ramAddr, uint32_t *value);
	void	writeLong(uint32_t ramAddr, uint32_t value);

	void	eraseDevice(void);
	
	void printStatusRegs();
	
	void readmem(uint32_t addr, void *data, uint32_t length);
	void writemem(uint32_t addr, const void *data, uint32_t length);
	static void flexspi_ip_command(uint32_t index, uint32_t addr);
	static void flexspi_ip_read(uint32_t index, uint32_t addr, void *data, uint32_t length);
	static void flexspi_ip_write(uint32_t index, uint32_t addr, const void *data, uint32_t length);
	
	void fs_mount();
	static s32_t fs_erase(u32_t addr, u32_t size);	
	static s32_t spiffs_write(u32_t addr, u32_t size, u8_t * src);
	static s32_t spiffs_read(u32_t addr, u32_t size, u8_t * dst);
	void eraseFlashChip();
	static bool waitFlash(uint32_t timeout = 0);
	void fs_listDir();
	
	int f_open(spiffs_file &fd, const char* fname, spiffs_flags flags);
	int f_write(spiffs_file fd, const char *dst, int szLen);
	int f_read(spiffs_file fd, const char *dst, int szLen);
	int f_writeFile(const char* fname, const char *dst, spiffs_flags flags);
	int f_readFile(const char* fname, const char *dst, int szLen, spiffs_flags);

	int f_close_write(spiffs_file fd);
	void f_close(spiffs_file fd);

	int32_t f_position(spiffs_file fd );
	int f_eof( spiffs_file fd );
	int f_seek(spiffs_file fd ,int32_t offset, int start);
	int f_rename(const char* fname_old, const char* fname_new);
	int f_remove(const char* fname);
	void f_info(const char* fname, spiffs_stat *s);


	
	// overwrite print functions:
	void printTo(spiffs_file fd);
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buffer, size_t size);
	



 private:
	

};

#endif
