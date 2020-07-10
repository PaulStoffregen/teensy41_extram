
FLASHMEM void configure_flash()
{
    memset(flashID, 0, sizeof(flashID));

    delay(500);
    
    FLEXSPI2_FLSHA2CR0 = 134218;          //Flash Size in KByte, 1F400
    FLEXSPI2_FLSHA2CR1 = FLEXSPI_FLSHCR1_CSINTERVAL(2)  //minimum interval between flash device Chip selection deassertion and flash device Chip selection assertion.
    | FLEXSPI_FLSHCR1_CAS(12)
    | FLEXSPI_FLSHCR1_TCSH(3)                           //Serial Flash CS Hold time.
    | FLEXSPI_FLSHCR1_TCSS(3);                          //Serial Flash CS setup time
    
    FLEXSPI2_FLSHA2CR2 = FLEXSPI_FLSHCR2_AWRSEQID(6)    //Sequence Index for AHB Write triggered Command
    | FLEXSPI_FLSHCR2_AWRSEQNUM(0)                      //Sequence Number for AHB Read triggered Command in LUT.
    | FLEXSPI_FLSHCR2_ARDSEQID(5)                       //Sequence Index for AHB Read triggered Command in LUT
    | FLEXSPI_FLSHCR2_ARDSEQNUM(0);                     //Sequence Number for AHB Read triggered Command in LUT.
 
    // cmd index 7 = read ID bytes, 4*index number
    FLEXSPI2_LUT28 = LUT0(CMD_SDR, PINS1, W25N01G_RDID) | LUT1(DUMMY_SDR, PINS1, 8);
    FLEXSPI2_LUT29 = LUT0(READ_SDR, PINS1, 1);     //9fh Read JDEC

    // cmd index 8 = read Status register
    // set in function w25n01g_readStatusRegister(uint8_t reg, bool dump)
    
    //cmd index 9 - WG reset, see function w25n01g_deviceReset()
    FLEXSPI2_LUT36 = LUT0(CMD_SDR, PINS1, W25N01G_DEVICE_RESET); 

    // cmd index 10 = write Status register
    // see function w25n01g_writeStatusRegister(uint8_t reg, uint8_t data)

    // cmd 11 index write enable cmd
    // see function w25n01g_writeEnable(bool wpE)

    //cmd 12 Command based on PageAddress
    // see functions:
    // w25n01g_eraseSector(uint32_t addr) and
    // w25n01g_readBytes(uint32_t address, uint8_t *data, int length)
    
    //cmd 13 program load Data
    // see functions:
    // w25n01g_programDataLoad(uint16_t columnAddress, const uint8_t *data, int length)
    // and
    // w25n01g_randomProgramDataLoad(uint16_t columnAddress, const uint8_t *data, int length)

    //cmd 14 program read Data -- reserved
    //FLEXSPI2_LUT56 = LUT0(CMD_SDR, PINS1, W25N01G_FAST_READ_QUAD_IO) | LUT1(CADDR_SDR, PINS4, 0x10);
    FLEXSPI2_LUT56 = LUT0(CMD_SDR, PINS1, W25N01G_FAST_READ_QUAD_OUTPUT) | LUT1(CADDR_SDR, PINS1, 0x10);
    FLEXSPI2_LUT57 = LUT0(DUMMY_SDR, PINS4, 8) | LUT1(READ_SDR, PINS4, 1);
    
    //cmd 15 - program execute
    FLEXSPI2_LUT60 = LUT0(CMD_SDR, PINS1, W25N01G_PROGRAM_EXECUTE) | LUT1(DUMMY_SDR, PINS1, 8);
    FLEXSPI2_LUT61 = LUT0(ADDR_SDR, PINS1, 0x20);

}

void w25n01g_init() {
    if (flexspi2_flash_id(flashBaseAddr) == 0x21AA) {
      Serial.println("Found W25N01G Flash Chip");
    } else {
      Serial.println("no chip found!");
      exit(1);
    }

    // reset the chip
    w25n01g_deviceReset();
}





void flexspi_ip_read(uint32_t index, uint32_t addr, void *data, uint32_t length)
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
    Serial.printf("Error (READ): FLEXSPI2_IPRXFSTS=%08lX\r\n", FLEXSPI2_IPRXFSTS);
    //Serial.print("ERROR: FLEXSPI2_INTR = "), Serial.println(FLEXSPI2_INTR, BIN);
  }
  
  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
  //Serial.printf(" FLEXSPI2_RFDR0=%08lX\r\n", FLEXSPI2_RFDR0);
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


static void flexspi_ip_write(uint32_t index, uint32_t addr, const void *data, uint32_t length)
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


void flexspi_ip_command(uint32_t index, uint32_t addr)
{
  uint32_t n;
  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(index);
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
  while (!((n = FLEXSPI2_INTR) & FLEXSPI_INTR_IPCMDDONE)); // wait
  if (n & FLEXSPI_INTR_IPCMDERR) {
    Serial.print("ERROR: FLEXSPI2_INTR = "), Serial.println(FLEXSPI2_INTR, BIN);
    Serial.printf("Error: FLEXSPI2_IPRXFSTS=%08lX\n", FLEXSPI2_IPRXFSTS);
  }
  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
}

FLASHMEM static uint32_t flexspi2_flash_id(uint32_t addr)
{
  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(7) | FLEXSPI_IPCR1_IDATSZ(5);
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
  while (!(FLEXSPI2_INTR & FLEXSPI_INTR_IPCMDDONE)); // wait
  uint32_t id = FLEXSPI2_RFDR0;
  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE | FLEXSPI_INTR_IPRXWA;
  //Serial.println(id >> 8 , HEX);
  return id >> 8;
}

static uint8_t w25n01g_readStatusRegister(uint8_t reg, bool dump)
{
    uint8_t val;
    
    // cmd index 8 = read Status register #1 SPI
    FLEXSPI2_LUT32 = LUT0(CMD_SDR, PINS1, W25N01G_READ_STATUS_REG) | LUT1(CMD_SDR, PINS1, reg); 
    FLEXSPI2_LUT33 = LUT0(READ_SDR, PINS1, 1);

    flexspi_ip_read(8, flashBaseAddr, &val, 1 );

    if(dump) {
      Serial.printf("Status of reg 0x%x: \n", reg);
      Serial.printf("(HEX: ) 0x%02X, (Binary: )", val);
      Serial.println(val, BIN);
      Serial.println();
    }

    return val;
    
}

static void w25n01g_writeStatusRegister(uint8_t reg, uint8_t data)
{
    uint8_t buf[1];
    buf[0] = data;
    // cmd index 10 = write Status register 
    FLEXSPI2_LUT40 = LUT0(CMD_SDR, PINS1, W25N01G_WRITE_STATUS_REG) | LUT1(CMD_SDR, PINS1, reg); 
    FLEXSPI2_LUT41 = LUT0(WRITE_SDR, PINS1, 1);

    flexspi_ip_write(10, flashBaseAddr, buf, 1);
  
}


static void w25n01g_setTimeout(uint32_t timeoutMillis)
{
    uint32_t now = millis();
    timeoutAt = now + timeoutMillis;
}

static void w25n01g_deviceReset()
{
    flexspi_ip_command(9, flashBaseAddr); //reset

    w25n01g_setTimeout(W25N01G_TIMEOUT_RESET_MS);
    w25n01g_waitForReady();

    // No protection, WP-E off, WP-E prevents use of IO2
    w25n01g_writeStatusRegister(W25N01G_PROT_REG, W25N01G_PROT_CLEAR);
    w25n01g_readStatusRegister(W25N01G_PROT_REG, false);
    
    // Buffered read mode (BUF = 1), ECC enabled (ECC = 1)
    w25n01g_writeStatusRegister(W25N01G_CONF_REG, W25N01G_CONFIG_ECC_ENABLE | W25N01G_CONFIG_BUFFER_READ_MODE);
    //w25n01g_writeStatusRegister(W25N01G_CONF_REG, W25N01G_CONFIG_ECC_ENABLE);
    //w25n01g_writeStatusRegister(W25N01G_CONF_REG,  W25N01G_CONFIG_BUFFER_READ_MODE);
    w25n01g_readStatusRegister(W25N01G_CONF_REG, false);

}

bool w25n01g_isReady()
{
    uint8_t status = w25n01g_readStatusRegister(W25N01G_STAT_REG, false);
    return ((status & W25N01G_STATUS_FLAG_BUSY) == 0);
}

static bool w25n01g_waitForReady()
{
    while (!w25n01g_isReady()) {
        uint32_t now = millis();
        //if (now >= timeoutAt) {
        if (millis() - now >= timeoutAt) {
            return false;
        }
    }
    timeoutAt = 0;

    return true;
}

/**
 * The flash requires this write enable command to be sent before commands that would cause
 * a write like program and erase.
 */
static void w25n01g_writeEnable(bool wpE)
{
    if(wpE == true){
          FLEXSPI2_LUT44 = LUT0(CMD_SDR, PINS1, W25N01G_WRITE_ENABLE); 
    } else {
          FLEXSPI2_LUT44 = LUT0(CMD_SDR, PINS1, W25N01G_WRITE_DISABLE); 
    }
    flexspi_ip_command(11, flashBaseAddr); //Write Enable
    // Assume that we're about to do some writing, so the device is just about to become busy
    couldBeBusy = true;
}


/**
 * Erase a sector full of bytes to all 1's at the given byte offset in the flash chip.
 */
void w25n01g_eraseSector(uint32_t address)
{
    w25n01g_writeEnable(true);
    w25n01g_waitForReady();
   
    // cmd index 12
    FLEXSPI2_LUT48 = LUT0(CMD_SDR, PINS1, W25N01G_BLOCK_ERASE) | LUT1(DUMMY_SDR, PINS1, 8);
    FLEXSPI2_LUT49 = LUT1(ADDR_SDR, PINS1, 0x20);
    flexspi_ip_command(12, flashBaseAddr + W25N01G_LINEAR_TO_PAGE(address));

    uint8_t status = w25n01g_readStatusRegister(W25N01G_STAT_REG, false );
    if((status &  W25N01G_STATUS_ERASE_FAIL) == 1)
        DTprint( erase Status: FAILED ); 

    w25n01g_setTimeout( W25N01G_TIMEOUT_BLOCK_ERASE_MS);
}

//
// W25N01G does not support full chip erase.
// Call eraseSector repeatedly.
void w25n01g_deviceErase()
{
    for (uint32_t block = 0; block < sectors; block++) {
        w25n01g_eraseSector(W25N01G_BLOCK_TO_LINEAR(block));
        Serial.print(".");
        if((block % 128) == 0) Serial.println();
     }
}

static void w25n01g_programDataLoad(uint16_t Address, const uint8_t *data, int length)
{
  //DTprint(w25n01g_programDataLoad);
    w25n01g_writeEnable(true);   //sets the WEL in Status Reg to 1 (bit 2)
    uint16_t columnAddress = W25N01G_LINEAR_TO_COLUMN(Address);
  
    w25n01g_waitForReady();


    FLEXSPI2_LUT52 = LUT0(CMD_SDR, PINS1, W25N01G_QUAD_PROGRAM_DATA_DATA_LOAD) | LUT1(CADDR_SDR, PINS1, 0x10);
    FLEXSPI2_LUT53 = LUT0(WRITE_SDR, PINS4, 1);
    flexspi_ip_write(13, flashBaseAddr + columnAddress, data, length);
    
    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_PROGRAM_MS);
    w25n01g_waitForReady();
    uint8_t status = w25n01g_readStatusRegister(W25N01G_STAT_REG, false );
    if((status &  W25N01G_STATUS_PROGRAM_FAIL) == 1)
        DTprint( Programed Status: FAILED );

}

static void w25n01g_randomProgramDataLoad(uint16_t Address, const uint8_t *data, int length)
{
    w25n01g_writeEnable(true);   //sets the WEL in Status Reg to 1 (bit 2)
    uint16_t columnAddress = W25N01G_LINEAR_TO_COLUMN(Address);

    w25n01g_waitForReady();

    FLEXSPI2_LUT52 = LUT0(CMD_SDR, PINS1, W25N01G_RANDOM_PROGRAM_DATA_LOAD) | LUT1(CADDR_SDR, PINS1, 0x10);
    FLEXSPI2_LUT53 = LUT0(WRITE_SDR, PINS1, 1);
    flexspi_ip_write(13, flashBaseAddr + columnAddress, data, length);
  
    w25n01g_waitForReady();
    uint8_t status = w25n01g_readStatusRegister(W25N01G_STAT_REG, false );
    if((status &  W25N01G_STATUS_PROGRAM_FAIL) == 1)
            DTprint( Programed Status: FAILED );
    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_PROGRAM_MS);

}

static void w25n01g_programExecute(uint32_t Address)
{
    //DTprint(w25n01g_programExecute);
    w25n01g_writeEnable(true);   //sets the WEL in Status Reg to 1 (bit 2)

    uint32_t pageAddress = W25N01G_LINEAR_TO_PAGE(Address);
    w25n01g_waitForReady();
    
    flexspi_ip_command(15, flashBaseAddr + pageAddress);
    
    w25n01g_waitForReady();
    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_PROGRAM_MS);
}

/**
 * Read `length` bytes into the provided `buffer` from the flash starting from the given `address` (which need not lie
 * on a page boundary).
 *
 * Waits up to W25N01G_TIMEOUT_PAGE_READ_MS milliseconds for the flash to become ready before reading.
 *
 * The number of bytes actually read is returned, which can be zero if an error or timeout occurred.
 */

// Continuous read mode (BUF = 0):
// (1) "Page Data Read" command is executed for the page pointed by address
// (2) "Read Data" command is executed for bytes not requested and data are discarded
// (3) "Read Data" command is executed and data are stored directly into caller's buffer
//
// Buffered read mode (BUF = 1), non-read ahead
// (1) If currentBufferPage != requested page, then issue PAGE_DATA_READ on requested page.
// (2) Compute transferLength as smaller of remaining length and requested length.
// (3) Issue READ_DATA on column address.
// (4) Return transferLength.

int w25n01g_readBytes(uint32_t address, uint8_t *data, int length)
{
    w25n01g_writeEnable(false);
    uint32_t targetPage = W25N01G_LINEAR_TO_PAGE(address);

    if (currentPage != targetPage) {
        if (!w25n01g_waitForReady()) {
          Serial.println("Read Returning");
            return 0;
        }

        currentPage = UINT32_MAX;

        FLEXSPI2_LUT48 = LUT0(CMD_SDR, PINS1, W25N01G_PAGE_DATA_READ) | LUT1(DUMMY_SDR, PINS1, 8);
        FLEXSPI2_LUT49 = LUT0(ADDR_SDR, PINS1, 0x20);
        flexspi_ip_command(12, flashBaseAddr + targetPage);
        //Serial.println("Write Page Addr Complete");

        w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_READ_MS);
        if (!w25n01g_waitForReady()) {
            return 0;
        }

        currentPage = targetPage;
    }

    uint16_t column = W25N01G_LINEAR_TO_COLUMN(address);
    uint16_t transferLength;

    if (length > W25N01G_PAGE_SIZE - column) {
        transferLength = W25N01G_PAGE_SIZE - column;
    } else {
        transferLength = length;
    }

   flexspi_ip_read(14, flashBaseAddr + column, data, transferLength);
       
    // XXX Don't need this?
    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_READ_MS);
    if (!w25n01g_waitForReady()) {
        return 0;
    }

    // Check ECC
    uint8_t statReg = w25n01g_readStatusRegister(W25N01G_STAT_REG, false);
    uint8_t eccCode = W25N01G_STATUS_FLAG_ECC(statReg);

    switch (eccCode) {
        case 0: // Successful read, no ECC correction
            break;
        case 1: // Successful read with ECC correction
        case 2: // Uncorrectable ECC in a single page
        case 3: // Uncorrectable ECC in multiple pages
            //w25n01g_addError(address, eccCode);
            Serial.printf("ECC Error (addr, code): %x, %x\n", address, eccCode);
            w25n01g_deviceReset();
            break;
    }

    return transferLength;
}

/* Not used yet */
void w25n01g_setBufMode(uint8_t bufMode) {
    /* bufMode = 1:
      * The Buffer Read Mode (BUF=1) requires a Column Address to start outputting the 
      * existing data inside the Data Buffer, and once it reaches the end of the data 
      * buffer (Byte 2,111)
      * 
     * bufMode = 0:
      * The Continuous Read Mode (BUF=0) doesn’t require the starting Column Address. 
      * The device will always start output the data from the first column (Byte 0) of the 
      * Data buffer, and once the end of the data buffer (Byte 2,048) is reached, the data 
      * output will continue through the next memory page. 
      * 
      * With Continuous Read Mode, it is possible to read out the entire memory array using 
      * a single read command. 
      */

    // Set mode after device reset.

    if(bufMode == 1) {
        w25n01g_writeStatusRegister(W25N01G_CONF_REG, W25N01G_CONFIG_ECC_ENABLE | W25N01G_CONFIG_BUFFER_READ_MODE);
    } else if(bufMode == 0) {
        w25n01g_writeStatusRegister(W25N01G_CONF_REG, W25N01G_CONFIG_ECC_ENABLE);
    } else {
        Serial.println("Mode not supported !!!!");
    }

}

