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

#include "extRAM_t4.h"
#include "spiffs.h"

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
	static const uint32_t flashBaseAddr[3] = { 0x01000000u, 0x00000000u, 0x00000000u};
	static const uint32_t eramBaseAddr = 0x07000000u;
	static char flashID[8];
	static const void* extBase = (void*)0x70000000u;
	//4meg = 4,194,304‬bytes
	static uint32_t flashCapacity[3] = {16u * 1024u * 1024u, 8u * 1024u * 1024u, 4194305u};
	
extRAM_t4::extRAM_t4()
{

}

/*========================================================================*/
/*                           PUBLIC FUNCTIONS                             */
/*========================================================================*/

//int8_t extRAM_t4::begin(uint8_t config, uint8_t spiffs_region) {
int8_t extRAM_t4::begin(uint8_t _config) {
	uint8_t config;
	  memset(flashID, 0, sizeof(flashID));
	  uint8_t result = 0;
	
	if(_config == 0){
		config = 0;
		_spiffs_region = 1;
		result = 1;
	} else if (_config == 1) {
		config = 1;
		_spiffs_region = 0;
		result = 0;
	} else if (_config == 0) {
		config = 1;
		_spiffs_region = 2;
		result = 0;		
	} else {
		config = 2;
		_spiffs_region = 0;
		result = 0;
	}

	 // _spiffs_region = spiffs_region;


	  // initialize pins
	  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_22 = 0xB0E1; // 100K pullup, medium drive, max speed
	  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_23 = 0x10E1; // keeper, medium drive, max speed
	  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_24 = 0xB0E1; // 100K pullup, medium drive, max speed
	  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_25 = 0x00E1; // medium drive, max speed
	  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_26 = 0x70E1; // 47K pullup, medium drive, max speed
	  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_27 = 0x70E1; // 47K pullup, medium drive, max speed
	  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_28 = 0x70E1; // 47K pullup, medium drive, max speed
	  IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_29 = 0x70E1; // 47K pullup, medium drive, max speed

	  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_22 = 8 | 0x10; // ALT1 = FLEXSPI2_A_SS1_B (Flash)
	  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_23 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DQS
	  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_24 = 8 | 0x10; // ALT1 = FLEXSPI2_A_SS0_B (RAM)
	  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_25 = 8 | 0x10; // ALT1 = FLEXSPI2_A_SCLK
	  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_26 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DATA0
	  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_27 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DATA1
	  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_28 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DATA2
	  IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_29 = 8 | 0x10; // ALT1 = FLEXSPI2_A_DATA3

	  IOMUXC_FLEXSPI2_IPP_IND_DQS_FA_SELECT_INPUT = 1; // GPIO_EMC_23 for Mode: ALT8, pg 986
	  IOMUXC_FLEXSPI2_IPP_IND_IO_FA_BIT0_SELECT_INPUT = 1; // GPIO_EMC_26 for Mode: ALT8
	  IOMUXC_FLEXSPI2_IPP_IND_IO_FA_BIT1_SELECT_INPUT = 1; // GPIO_EMC_27 for Mode: ALT8
	  IOMUXC_FLEXSPI2_IPP_IND_IO_FA_BIT2_SELECT_INPUT = 1; // GPIO_EMC_28 for Mode: ALT8
	  IOMUXC_FLEXSPI2_IPP_IND_IO_FA_BIT3_SELECT_INPUT = 1; // GPIO_EMC_29 for Mode: ALT8
	  IOMUXC_FLEXSPI2_IPP_IND_SCK_FA_SELECT_INPUT = 1; // GPIO_EMC_25 for Mode: ALT8

	  // turn on clock
	  CCM_CBCMR = (CCM_CBCMR & ~(CCM_CBCMR_FLEXSPI2_PODF_MASK | CCM_CBCMR_FLEXSPI2_CLK_SEL_MASK))
			| CCM_CBCMR_FLEXSPI2_PODF(4) | CCM_CBCMR_FLEXSPI2_CLK_SEL(2); // 528/5 = 132 MHz

	  CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_ON);

	  FLEXSPI2_MCR0 |= FLEXSPI_MCR0_MDIS;
	  FLEXSPI2_MCR0 = (FLEXSPI2_MCR0 & ~(FLEXSPI_MCR0_AHBGRANTWAIT_MASK
		 | FLEXSPI_MCR0_IPGRANTWAIT_MASK | FLEXSPI_MCR0_SCKFREERUNEN
		 | FLEXSPI_MCR0_COMBINATIONEN | FLEXSPI_MCR0_DOZEEN
		 | FLEXSPI_MCR0_HSEN | FLEXSPI_MCR0_ATDFEN | FLEXSPI_MCR0_ARDFEN
		 | FLEXSPI_MCR0_RXCLKSRC_MASK | FLEXSPI_MCR0_SWRESET))
		| FLEXSPI_MCR0_AHBGRANTWAIT(0xFF) | FLEXSPI_MCR0_IPGRANTWAIT(0xFF)
		| FLEXSPI_MCR0_RXCLKSRC(1) | FLEXSPI_MCR0_MDIS;
	  FLEXSPI2_MCR1 = FLEXSPI_MCR1_SEQWAIT(0xFFFF) | FLEXSPI_MCR1_AHBBUSWAIT(0xFFFF);
	  FLEXSPI2_MCR2 = (FLEXSPI_MCR2 & ~(FLEXSPI_MCR2_RESUMEWAIT_MASK
		 | FLEXSPI_MCR2_SCKBDIFFOPT | FLEXSPI_MCR2_SAMEDEVICEEN
		 | FLEXSPI_MCR2_CLRLEARNPHASE | FLEXSPI_MCR2_CLRAHBBUFOPT))
		| FLEXSPI_MCR2_RESUMEWAIT(0x20) /*| FLEXSPI_MCR2_SAMEDEVICEEN*/;

	  FLEXSPI2_AHBCR = FLEXSPI2_AHBCR & ~(FLEXSPI_AHBCR_READADDROPT | FLEXSPI_AHBCR_PREFETCHEN
		| FLEXSPI_AHBCR_BUFFERABLEEN | FLEXSPI_AHBCR_CACHABLEEN);
	  uint32_t mask = (FLEXSPI_AHBRXBUFCR0_PREFETCHEN | FLEXSPI_AHBRXBUFCR0_PRIORITY_MASK
		| FLEXSPI_AHBRXBUFCR0_MSTRID_MASK | FLEXSPI_AHBRXBUFCR0_BUFSZ_MASK);
	  FLEXSPI2_AHBRXBUF0CR0 = (FLEXSPI2_AHBRXBUF0CR0 & ~mask)
		| FLEXSPI_AHBRXBUFCR0_PREFETCHEN | FLEXSPI_AHBRXBUFCR0_BUFSZ(64);
	  FLEXSPI2_AHBRXBUF1CR0 = (FLEXSPI2_AHBRXBUF0CR0 & ~mask)
		| FLEXSPI_AHBRXBUFCR0_PREFETCHEN | FLEXSPI_AHBRXBUFCR0_BUFSZ(64);
	  FLEXSPI2_AHBRXBUF2CR0 = mask;
	  FLEXSPI2_AHBRXBUF3CR0 = mask;

	  // RX watermark = one 64 bit line
	  FLEXSPI2_IPRXFCR = (FLEXSPI_IPRXFCR & 0xFFFFFFC0) | FLEXSPI_IPRXFCR_CLRIPRXF;
	  // TX watermark = one 64 bit line
	  FLEXSPI2_IPTXFCR = (FLEXSPI_IPTXFCR & 0xFFFFFFC0) | FLEXSPI_IPTXFCR_CLRIPTXF;

	  FLEXSPI2_INTEN = 0;
	  FLEXSPI2_FLSHA1CR0 = 0x4000;
	  FLEXSPI2_FLSHA1CR1 = FLEXSPI_FLSHCR1_CSINTERVAL(2)
		| FLEXSPI_FLSHCR1_TCSH(3) | FLEXSPI_FLSHCR1_TCSS(3);
	  FLEXSPI2_FLSHA1CR2 = FLEXSPI_FLSHCR2_AWRSEQID(6) | FLEXSPI_FLSHCR2_AWRSEQNUM(0)
		| FLEXSPI_FLSHCR2_ARDSEQID(5) | FLEXSPI_FLSHCR2_ARDSEQNUM(0);

	  FLEXSPI2_FLSHA2CR0 = 0x40000;
	  FLEXSPI2_FLSHA2CR1 = FLEXSPI_FLSHCR1_CSINTERVAL(2)
		| FLEXSPI_FLSHCR1_TCSH(3) | FLEXSPI_FLSHCR1_TCSS(3);
	  FLEXSPI2_FLSHA2CR2 = FLEXSPI_FLSHCR2_AWRSEQID(6) | FLEXSPI_FLSHCR2_AWRSEQNUM(0)
		| FLEXSPI_FLSHCR2_ARDSEQID(5) | FLEXSPI_FLSHCR2_ARDSEQNUM(0);

	  FLEXSPI2_MCR0 &= ~FLEXSPI_MCR0_MDIS;

	  FLEXSPI2_LUTKEY = FLEXSPI_LUTKEY_VALUE;
	  FLEXSPI2_LUTCR = FLEXSPI_LUTCR_UNLOCK;
	  volatile uint32_t *luttable = &FLEXSPI2_LUT0;
	  for (int i=0; i < 64; i++) luttable[i] = 0;
	  FLEXSPI2_MCR0 |= FLEXSPI_MCR0_SWRESET;
	  while (FLEXSPI2_MCR0 & FLEXSPI_MCR0_SWRESET) ; // wait
	  Serial.println("hardware initialized");


	  // CBCMR[FLEXSPI2_SEL]
	  // CBCMR[FLEXSPI2_PODF]

	  FLEXSPI2_LUTKEY = FLEXSPI_LUTKEY_VALUE;
	  FLEXSPI2_LUTCR = FLEXSPI_LUTCR_UNLOCK;

	  // cmd index 0 = exit QPI mode
	  FLEXSPI2_LUT0 = LUT0(CMD_SDR, PINS4, 0xF5);
	  // cmd index 1 = reset enable
	  FLEXSPI2_LUT4 = LUT0(CMD_SDR, PINS1, 0x66);
	  // cmd index 2 = reset
	  FLEXSPI2_LUT8 = LUT0(CMD_SDR, PINS1, 0x99);
	  // cmd index 3 = read ID bytes
	  FLEXSPI2_LUT12 = LUT0(CMD_SDR, PINS1, 0x9F) | LUT1(DUMMY_SDR, PINS1, 24);
	  FLEXSPI2_LUT13 = LUT0(READ_SDR, PINS1, 1);
	  // cmd index 4 = enter QPI mode
	  FLEXSPI2_LUT16 = LUT0(CMD_SDR, PINS1, 0x35);
	  // cmd index 5 = read QPI
	  FLEXSPI2_LUT20 = LUT0(CMD_SDR, PINS4, 0xEB) | LUT1(ADDR_SDR, PINS4, 24);
	  FLEXSPI2_LUT21 = LUT0(DUMMY_SDR, PINS4, 6) | LUT1(READ_SDR, PINS4, 1);
	  // cmd index 6 = write QPI
	  FLEXSPI2_LUT24 = LUT0(CMD_SDR, PINS4, 0x38) | LUT1(ADDR_SDR, PINS4, 24);
	  FLEXSPI2_LUT25 = LUT0(WRITE_SDR, PINS4, 1);

	  // cmd index 7 = read ID bytes
	  FLEXSPI2_LUT28 = LUT0(CMD_SDR, PINS1, 0x9F) | LUT1(READ_SDR, PINS1, 1);


	  if(config == 0 || config == 2){
		  // reset the chip
		  flexspi_ip_command(0, 0);
		  flexspi_ip_command(1, 0);
		  flexspi_ip_command(2, 0);
		  delayMicroseconds(100);
		  
		  // read chip ID
		  uint8_t id[8];
		  flexspi_ip_read(3, 0, id, 8);
		  Serial.print("ERAM ID:");
		  for (int i=0; i < 8; i++) {
			Serial.printf(" %02X", id[i]);
		  }
		  Serial.printf("\n");
		  // ID: 0D 5D 50 B1 BF EE C2 49
		  Serial.printf("at 0x %x\n",  eramBaseAddr); 

		  if (id[0] != 0x0D || id[1] != 0x5D) {
			Serial.println("Wrong ID  :-(");
			result = -1;
		  } else {
			Serial.println("Device found!");
			//result = 0;
		  }
		  // configure for memory mapping
		  flexspi_ip_command(4, 0);
	  }
	  
	  if(config == 1 || config == 2){
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
		  flexspi_ip_command(10, flashBaseAddr[_spiffs_region]); //exit QPI
		  flexspi_ip_command(1, flashBaseAddr[_spiffs_region]); //reset enable
		  flexspi_ip_command(2, flashBaseAddr[_spiffs_region]); //reset
		  delayMicroseconds(50);

		  flexspi_ip_read(7, flashBaseAddr[_spiffs_region], flashID, sizeof(flashID) ); // flash begins at offset 0x01000000

		#if 1
		  Serial.print("FLASH ID:");
		  for (unsigned i = 0; i < sizeof(flashID); i++) Serial.printf(" %02X", flashID[i]);
		  Serial.printf("\n");
		  Serial.printf("at 0x %x\n", flashBaseAddr[_spiffs_region]);
		#endif

		  printStatusRegs();
		  //TODO!!!!! set QPI enable bit in status reg #2 if not factory set!!!!!

		  //  Serial.println("ENTER QPI MODE");
		  flexspi_ip_command(15, flashBaseAddr[_spiffs_region]);

		  //patch LUT for QPI:
		  // cmd index 8 = read Status register #1
		  FLEXSPI2_LUT32 = LUT0(CMD_SDR, PINS4, 0x05) | LUT1(READ_SDR, PINS4, 1);
		  // cmd index 9 = read Status register #2
		  FLEXSPI2_LUT36 = LUT0(CMD_SDR, PINS4, 0x35) | LUT1(READ_SDR, PINS4, 1);

		  flexspi_ip_command(14, flashBaseAddr[_spiffs_region]);

		  printStatusRegs();
		  
	  }
	  
	  
	return result;

}


void extRAM_t4::printStatusRegs() {
#if 0
  uint8_t val;

  flexspi_ip_read(8, flashBaseAddr[_spiffs_region], &val, 1 );
  Serial.print("Status 1:");
  Serial.printf(" %02X", val);
  Serial.printf("\n");

  // cmd index 9 = read Status register #2 SPI
  flexspi_ip_read(9, flashBaseAddr[_spiffs_region], &val, 1 );
  Serial.print("Status 2:");
  Serial.printf(" %02X", val);
  Serial.printf("\n");
#endif
}
/**************************************************************************/
/*!
    @brief  Writes an array of bytes from a specific address
    

*/
/**************************************************************************/
//void extRAM_t4::writeArray_old (uint32_t ramAddr, uint32_t items, uint8_t values[])
//{ 
  //Serial.printf("write @%06X:", ramAddr);
  //for (uint32_t i=0; i < items; i++) Serial.printf(" %02X", *(((uint8_t *)values) + i));
  //Serial.printf("\n");
  //uint32_t timenow = micros();
//  flexspi_ip_write(6, ramAddr, values, items);
  //Serial.printf("Write (us): %d\n", micros()-timenow);

//}

void extRAM_t4::writeArray (uint32_t ramAddr, uint32_t items, uint8_t values[])
{ 
  uint32_t ii;
	uint8_t *ptrERAM = (uint8_t *)(0x70000000 + ramAddr);
  
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



/**************************************************************************/
/*!
    @brief  Reads an array of bytes from the specified FRAM address


*/
/**************************************************************************/
//void extRAM_t4::readArray_old (uint32_t ramAddr, uint32_t length, uint8_t data[])
//{
	//if ((ramAddr >= maxaddress) || ((ramAddr + (uint16_t) items - 1) >= maxaddress)) rvoid readArray (uint16_t ramAddr, uint32_t length, uint8_t *data)

//  uint8_t *p = (uint8_t *)(0x70000000 + ramAddr);
//  arm_dcache_flush(p, sizeof(data));
//  memset(data, 0xFF, sizeof(data));

  //uint32_t timenow = micros();
//  flexspi_ip_read(5, ramAddr, data, length);
  //Serial.printf("Read (us): %d\n", micros()-timenow);

  //Serial.printf("read @%06X: ", ramAddr);
  //for (uint32_t i=0; i < length; i++) Serial.printf(" %02X", *(((uint8_t *)data) + i));
  //Serial.printf("\n");
//  arm_dcache_delete(p, length);
  //Serial.printf("rd @%08X: ", (uint32_t)p);
  //for (uint32_t i=0; i < length; i++) Serial.printf(" %02X", p[i]);
  //Serial.printf("\n");

//}

void extRAM_t4::readArray (uint32_t ramAddr, uint32_t length, uint8_t *data)
{
  uint32_t ii;
  uint8_t *ptrERAM = (uint8_t *)(0x70000000 + ramAddr);

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
    @brief  Reads one bit from the specified FRAM address


*/
/**************************************************************************/
byte extRAM_t4::readBit(uint32_t ramAddr, uint8_t bitNb, byte *bit)
{
	byte result;
	if (bitNb > 7) {
		result = ERROR_9;
	}
	else {
		uint8_t buffer[1];
		extRAM_t4::readArray(ramAddr, 1, buffer);
		*bit = bitRead(buffer[0], bitNb);
		result =  0;
	}
	return result;
}

/**************************************************************************/
/*!
    @brief  Set one bit to the specified FRAM address


*/
/**************************************************************************/
byte extRAM_t4::setOneBit(uint32_t ramAddr, uint8_t bitNb)
{
	byte result;
	if (bitNb > 7)  {
		result = ERROR_9;
	}
	else {
		uint8_t buffer[1];
		extRAM_t4::readArray(ramAddr, 1, buffer);
		bitSet(buffer[0], bitNb);
		extRAM_t4::writeArray(ramAddr, 1, buffer);
		result = 0;
	}
	return result;
}
/**************************************************************************/
/*!
    @brief  Clear one bit to the specified FRAM address


*/
/**************************************************************************/
byte extRAM_t4::clearOneBit(uint32_t ramAddr, uint8_t bitNb)
{
	byte result;
	if (bitNb > 7) {
		result = ERROR_9;
	}
	else {
		uint8_t buffer[1];
		extRAM_t4::readArray(ramAddr, 1, buffer);
		bitClear(buffer[0], bitNb);
		extRAM_t4::writeArray(ramAddr, 1, buffer);
		result = 0;
	}
	return result;
}
/**************************************************************************/
/*!
    @brief  Toggle one bit to the specified FRAM address


*/
/**************************************************************************/
byte extRAM_t4::toggleBit(uint32_t ramAddr, uint8_t bitNb)
{
	byte result;
	if (bitNb > 7) {
		result = ERROR_9;
	}
	else {
		uint8_t buffer[1];
		extRAM_t4::readArray(ramAddr, 1, buffer);
		
		if ( (buffer[0] & (1 << bitNb)) == (1 << bitNb) )
		{
			bitClear(buffer[0], bitNb);
		}
		else {
			bitSet(buffer[0], bitNb);
		}
		extRAM_t4::writeArray(ramAddr, 1, buffer);
		result = 0;
	}
	return result;
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
		
		while(i < 8388608){
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


void extRAM_t4::readmem(uint32_t addr, void *data, uint32_t length)
{
  uint8_t *p = (uint8_t *)(0x70000000 + addr);
  arm_dcache_flush(p, length);
  memset(data, 0xFF, length);
  flexspi_ip_read(5, addr, data, length);
  Serial.printf("read @%06X: ", addr);
  for (uint32_t i=0; i < length; i++) Serial.printf(" %02X", *(((uint8_t *)data) + i));
  Serial.printf("\n");
  arm_dcache_delete(p, length);
  Serial.printf("rd @%08X: ", (uint32_t)p);
  for (uint32_t i=0; i < length; i++) Serial.printf(" %02X", p[i]);
  Serial.printf("\n");
}

void extRAM_t4::writemem(uint32_t addr, const void *data, uint32_t length)
{
  Serial.printf("write @%06X:", addr);
  for (uint32_t i=0; i < length; i++) Serial.printf(" %02X", *(((uint8_t *)data) + i));
  Serial.printf("\n");
  flexspi_ip_write(6, addr, data, length);
}

static void extRAM_t4::flexspi_ip_command(uint32_t index, uint32_t addr)
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

static void extRAM_t4::flexspi_ip_read(uint32_t index, uint32_t addr, void *data, uint32_t length)
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

static void extRAM_t4::flexspi_ip_write(uint32_t index, uint32_t addr, const void *data, uint32_t length)
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

void extRAM_t4::fs_listDir() {
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
int extRAM_t4::f_writeFile(const char* fname, const char *dst, spiffs_flags flags) {
	int szLen = strlen( dst );

  // Surely, I've mounted spiffs before entering here
  spiffs_file fd = SPIFFS_open(&fs, fname, flags, 0);
  if (SPIFFS_write(&fs, fd, (u8_t *)dst, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
  SPIFFS_fflush(&fs, fd);
  return SPIFFS_errno(&fs);
}

int extRAM_t4::f_readFile(const char* fname, const char *dst, int szLen, spiffs_flags flags) {
  // Surely, I've mounted spiffs before entering here
  spiffs_file  fd = SPIFFS_open(&fs, fname, flags, 0);
  if (SPIFFS_read(&fs, fd, (u8_t *)dst, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
  return SPIFFS_errno(&fs);
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
int extRAM_t4::f_open(spiffs_file &fd, const char* fname, spiffs_flags flags){
	fd = SPIFFS_open(&fs, fname, flags, 0);
	SPIFFS_errno(&fs);
	//fd1 = fd;
	return SPIFFS_errno(&fs);
}

int extRAM_t4::f_write(spiffs_file fd, const char *dst, int szLen) {
	int res;
	//if (SPIFFS_write(&fs, fd1, (u8_t *)dst, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
	res = SPIFFS_write(&fs, fd, (u8_t *)dst, szLen);
	return res;
	
}

int extRAM_t4::f_close_write(spiffs_file fd){
	int res;
	res = SPIFFS_close(&fs, fd);
	res = SPIFFS_fflush(&fs, fd);
	return res;
}

int extRAM_t4::f_read(spiffs_file fd, const char *dst, int szLen) {
	int res;
	res = SPIFFS_read(&fs, fd, (u8_t *)dst, szLen);
	//if (SPIFFS_read(&fs, fd1, (u8_t *)dst, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
	return res;
}

void extRAM_t4::f_close(spiffs_file fd ){
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

int extRAM_t4::f_seek(spiffs_file fd, int32_t offset, int start){
	int res;
	res = SPIFFS_lseek(&fs, fd, offset, start);
	return res;
}

int extRAM_t4::f_rename(const char* fname_old, const char* fname_new) {
	int res;
	res = SPIFFS_rename(&fs, fname_old, fname_new);
	return res;
}

int extRAM_t4::f_remove(const char* fname) {
	int res;
	res = SPIFFS_remove(&fs, fname);
	return res;
}

int32_t extRAM_t4::f_position(spiffs_file fd ) { 
	int res;
	res = SPIFFS_tell(&fs, fd);
	return res;
}

int extRAM_t4::f_eof(spiffs_file fd ) {
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
void extRAM_t4::f_info(const char* fname, spiffs_stat *s){
	SPIFFS_stat(&fs, fname, s);
}

/*
   Waits for busy bit = 0 (statusregister #1 )
   Timeout is optional
*/
static bool extRAM_t4::waitFlash(uint32_t timeout) {
  uint8_t val;
  uint32_t t = millis();
  FLEXSPI_IPRXFCR = FLEXSPI_IPRXFCR_CLRIPRXF; // clear rx fifo
  do {
	if(_spiffs_region == 0) {
		flexspi_ip_read(8, flashBaseAddr[_spiffs_region], &val, 1 );
	} else {
		flexspi_ip_read(5, flashBaseAddr[_spiffs_region], &val, 1 );
	}
    if (timeout && (millis() - t > timeout)) return 1;
  } while  ((val & 0x01) == 1);
  return 0;
}

void extRAM_t4::eraseFlashChip() {
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

static s32_t extRAM_t4::spiffs_read(u32_t addr, u32_t size, u8_t * dst) {
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

static s32_t extRAM_t4::spiffs_write(u32_t addr, u32_t size, u8_t * src) {
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

static s32_t extRAM_t4::fs_erase(u32_t addr, u32_t size) {
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
void extRAM_t4::printTo(spiffs_file fd){
	fd1 = fd;
}

size_t extRAM_t4::write(uint8_t c) {
	return write(&c, 1);
}

size_t extRAM_t4::write(const uint8_t *buffer, size_t size)
{
	f_write(fd1, buffer, size);
	return 1;
}


//********************************************************************************************************

void extRAM_t4::fs_mount() {

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
