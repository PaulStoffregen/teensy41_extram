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


// The density codes gives the memory's adressing scheme


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

	static const uint32_t flashBaseAddr = 0x01000000u;
	static const uint32_t eramBaseAddr = 0x07000000u;
	static char flashID[8];
	


class extRAM_t4 {
 public:
	extRAM_t4();
	int8_t  begin(uint8_t config);
	byte	readBit(uint32_t ramAddr, uint8_t bitNb, byte *bit);
	byte	setOneBit(uint32_t ramAddr, uint8_t bitNb);
	byte	clearOneBit(uint32_t ramAddr, uint8_t bitNb);
	byte	toggleBit(uint32_t ramAddr, uint8_t bitNb);
	
	void	readArray (uint32_t ramAddr, uint32_t items, uint8_t data[]);
	void	writeArray (uint32_t ramAddr, uint32_t items, uint8_t value[]);
	void	readArrayDMA (uint32_t ramAddr, uint32_t items, uint8_t data[]);
	void	writeArrayDMA (uint32_t ramAddr, uint32_t items, uint8_t value[]);
	
	void	readByte (uint32_t ramAddr, uint8_t *value);
	void	writeByte (uint32_t ramAddr, uint8_t value);
	void	copyByte (uint32_t origAddr, uint32_t destAddr);
	void	readWord(uint32_t ramAddr, uint16_t *value);
	void	writeWord(uint32_t ramAddr, uint16_t value);
	void	readLong(uint32_t ramAddr, uint32_t *value);
	void	writeLong(uint32_t ramAddr, uint32_t value);

	void	eraseDevice(void);
	
	void setupFLASH1();
	void setupFlash2();
	void printStatusRegs();
	
	void readmem(uint32_t addr, void *data, uint32_t length);
	void writemem(uint32_t addr, const void *data, uint32_t length);
	void flexspi_ip_command(uint32_t index, uint32_t addr);
	void flexspi_ip_read(uint32_t index, uint32_t addr, void *data, uint32_t length);
	static void flexspi_ip_write(uint32_t index, uint32_t addr, const void *data, uint32_t length);
	

 private:


};

#endif
