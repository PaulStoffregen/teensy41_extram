/**************************************************************************/
/*!
    @file     FRAM_MB85RC_I2C.cpp
    @author   SOSAndroid (E. Ha.)
    @license  BSD (see license.txt)

    Driver for the T4.1 external RAM
	
	This library is based on the Arduino library for I2C FRAM - Fujitsu MB85RC & Cypress FM24, CY15B by sosandroid, https://github.com/sosandroid/FRAM_MB85RC_I2C
	
	Function names are consistent with the specified library but the code
	to make them work with the T4.1 external ram has been changed.

    @section  HISTORY

    v1.0 - First release

	
*/
/**************************************************************************/
#include <stdlib.h>
#include "extRAM_t4.h"

extern "C" uint8_t external_psram_size;
/*========================================================================*/
/*                            CONSTRUCTORS                                */
/*========================================================================*/

/**************************************************************************/
/*!
    Constructor
*/
/**************************************************************************/
extRAM_t4::extRAM_t4() 
{
}

int8_t extRAM_t4::begin() 
{
	if(external_psram_size == 16){
		if(spiffs == 1){
			Serial.println("2 PSRAM Chips Installed !!!");
			Serial.println("SPIFFS requested on 2nd PSRAM chip,");
			Serial.println("4Mb in upper half of 1st PSRAM chip available");
			Serial.println("for direct writes !!");
			bytesAvailable = bytesAvailableMB * 1024 * 1024;
		} else if(spiffs == 0){
			Serial.println("2 PSRAM Chips Installed !!!");
			Serial.println("12Mb available for direct writes !!");
			bytesAvailable = (external_psram_size-bytesAvailableMB) * 1024 * 1024;
		}
	} else if(external_psram_size == 8){
		Serial.println("1 PSRAM Chip Installed !!!");
		Serial.println("4Mb in upper half of 1st PSRAM chip available");
		Serial.println("for direct writes !!");
		bytesAvailable = bytesAvailableMB * 1024 * 1024;
	} else {
		Serial.println("NO PSRAM Chips Installed !!!");
		exit(1);
	}
	Serial.println();
}

/*========================================================================*/
/*                           PUBLIC FUNCTIONS                             */
/*========================================================================*/


void extRAM_t4::writeArray (uint32_t ramAddr, uint32_t items, uint8_t values[])
{ 
  uint32_t ii;
	uint8_t *ptrERAM = (uint8_t *)(eramBaseAddr + ramAddr);
  
  //Serial.printf("write @%06X:", ramAddr);
  //for (uint32_t i=0; i < items; i++) Serial.printf(" %02X", *(((uint8_t *)values) + i));
  //Serial.printf("\n");
  //uint32_t timenow = micros();
  	for ( ii = 0; ii < items; ii++ ) {
		ptrERAM[ii] = values[ii];
	}
  //Serial.printf("Write (us): %d\n", micros()-timenow);

}

/**************************************************************************/
/*!
    @brief  Writes a single byte to a specific address
    

*/
/**************************************************************************/

void extRAM_t4::writeByte (uint32_t ramAddr, uint8_t value)
{
	uint8_t buffer[] = {value}; 
	extRAM_t4::writeArray(ramAddr, 1, buffer);
}


void extRAM_t4::readArray (uint32_t ramAddr, uint32_t length, uint8_t *data)
{
  uint32_t ii;
  uint8_t *ptrERAM = (uint8_t *)(eramBaseAddr + ramAddr);

  //uint32_t timenow = micros();
  for ( ii = 0; ii < length; ii++ ) {
	  data[ii] = ptrERAM[ii];
  }
  //Serial.printf("Read (us): %d\n", micros()-timenow);

  //Serial.printf("read @%06X: ", ramAddr);
  //for (uint32_t i=0; i < length; i++) Serial.printf(" %02X", *(((uint8_t *)data) + i));
  //Serial.printf("\n");
  //Serial.printf("rd @%08X: ", (uint32_t)ptrERAM);
  //for (uint32_t i=0; i < length; i++) Serial.printf(" %02X", ptrERAM[i]);
  //Serial.printf("\n");
}

/**************************************************************************/
/*!
    @brief  Reads one byte from the specified FRAM address


*/
/**************************************************************************/
void extRAM_t4::readByte (uint32_t ramAddr, uint8_t *value) 
{
	uint8_t buffer[1];
	extRAM_t4::readArray(ramAddr, 1, buffer);
	*value = buffer[0];
}

/**************************************************************************/
/*!
    @brief  Copy a byte from one address to another in the memory scope


*/
/**************************************************************************/
void extRAM_t4::copyByte (uint32_t origAddr, uint32_t destAddr) 
{
	uint8_t buffer[1];
	extRAM_t4::readByte(origAddr, buffer);
	extRAM_t4::writeByte(destAddr, buffer[0]);

}


/**************************************************************************/
/*!
    @brief  Reads a 16bits value from the specified FRAM address


*/
/**************************************************************************/
void extRAM_t4::readWord(uint32_t ramAddr, uint16_t *value)
{
	uint8_t buffer[2];
	extRAM_t4::readArray(ramAddr, 2, buffer);
	*value = *reinterpret_cast<uint16_t *>(buffer);
	
}

/**************************************************************************/
/*!
    @brief  Write a 16bits value from the specified FRAM address

 
*/
/**************************************************************************/
void extRAM_t4::writeWord(uint32_t ramAddr, uint16_t value)
{
	uint8_t *buffer = reinterpret_cast<uint8_t *>(&value);
	extRAM_t4::writeArray(ramAddr, 2, buffer);
}
/**************************************************************************/
/*!
    @brief  Read a 32bits value from the specified FRAM address

 
*/
/**************************************************************************/
void extRAM_t4::readLong(uint32_t ramAddr, uint32_t *value)
{
	uint8_t buffer[4];
	extRAM_t4::readArray(ramAddr, 4, buffer);
	*value = *reinterpret_cast<uint32_t *>(buffer);

}
/**************************************************************************/
/*!
    @brief  Write a 32bits value to the specified FRAM address


*/
/**************************************************************************/
void extRAM_t4::writeLong(uint32_t ramAddr, uint32_t value)
{
	uint8_t *buffer = reinterpret_cast<uint8_t *>(&value);
	writeArray(ramAddr, 4, buffer);
	
}

/**************************************************************************/
/*!
    @brief  Erase device by overwriting it to 0x00

*/
/**************************************************************************/
void extRAM_t4::eraseDevice(void) {
		uint32_t i=0;
		uint32_t jj = 0;

		Serial.println("Start erasing device");
		Serial.flush();
		
		while(i < bytesAvailable){
		  extRAM_t4::writeByte(i, 0xFF);
		  if(i % 100000 == 0){
			  Serial.print(".");
			  jj++;
			  if(jj % 50 == 0) Serial.println();
		  }
		  i++;
		}
		Serial.println();
		/*
		#if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
			if (Serial){
				if (result !=0) {
						Serial.print("ERROR: device erasing stopped at position ");
						Serial.println(i, DEC);
						Serial.println("...... ...... ......");
				}
				else {
						Serial.println("device erased");
						Serial.println("...... ...... ......");
				}
			}
		#endif
		*/
		Serial.println("device erased");
		Serial.println("...... ...... ......");
}
