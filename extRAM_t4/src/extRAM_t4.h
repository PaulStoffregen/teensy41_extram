/**************************************************************************/
/*! 
    @file     extRAM_t4.h
	
    @section  HISTORY

    v1.0 - First release

    Driver for the T4.1 external RAM
	
	This library is based on the Arduino library for I2C FRAM - Fujitsu MB85RC & Cypress FM24, CY15B by sosandroid, https://github.com/sosandroid/FRAM_MB85RC_I2C
	
	Function names are consistent with the specified library but the code
	to make them work with the T4.1 external ram has been changed.
	

*/
/**************************************************************************/
#ifndef _EXTRAM_T4_H_
#define _EXTRAM_T4_H_

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#if __has_include(<spiffs_t4.h>)
	#define spiffs 1
#else
	#define spiffs 0
#endif
	
// Enabling debug I2C - comment to disable / normal operations
#ifndef SERIAL_DEBUG
//  #define SERIAL_DEBUG 1
#endif

#define ERROR_9 9 // Bit position out of range

class extRAM_t4
{
 public:
	extRAM_t4();
	int8_t	begin();
	
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
	
	uint32_t eramBaseAddr = 0x70400000;
	uint16_t bytesAvailableMB = 4;
	uint32_t bytesAvailable;

 private:


};

#endif
