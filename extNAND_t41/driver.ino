/*
 * Flash partitioning
 *
 * Partitions are required so that Badblock management (inc spare blocks), FlashFS (Blackbox Logging), Configuration and Firmware can be kept separate and tracked.
 *
 */

static void flashConfigurePartitions(void)
{

    //flashPartitionSet(FLASH_PARTITION_TYPE_BADBLOCK_MANAGEMENT,
    //        W25N01G_BB_MANAGEMENT_START_BLOCK,
    //        W25N01G_BB_MANAGEMENT_START_BLOCK + W25N01G_BB_MANAGEMENT_BLOCKS - 1);
    W28N01G_Partitions.type = FLASH_PARTITION_TYPE_BADBLOCK_MANAGEMENT;
    W28N01G_Partitions.startSector = W25N01G_BB_MANAGEMENT_START_BLOCK;
    W28N01G_Partitions.endSector = W25N01G_BB_MANAGEMENT_START_BLOCK + W25N01G_BB_MANAGEMENT_BLOCKS - 1;

    W28N01G_Partitions.type = FLASH_PARTITION_TYPE_FLASHFS;
    W28N01G_Partitions.startSector = 0;
    W28N01G_Partitions.endSector = W25N01G_BB_MANAGEMENT_START_BLOCK - 1;

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
    Serial.println(data, HEX);
    // cmd index 10 = write Status register 
    FLEXSPI2_LUT40 = LUT0(CMD_SDR, PINS1, W25N01G_WRITE_STATUS_REG) | LUT1(CMD_SDR, PINS1, reg); 
    FLEXSPI2_LUT42 = LUT0(WRITE_SDR, PINS1, 1);

    flexspi_ip_write(10, flashBaseAddr, buf, 1);
  
}

static void w25n01g_writeCommandWithPageAddress(uint8_t reg, uint8_t addr)
{
    // cmd index 12 = CommandWithPageAddress
    FLEXSPI2_LUT48 = LUT0(CMD_SDR, PINS1, reg) | LUT1(CMD_SDR, PINS1, addr);

    flexspi_ip_command(12, flashBaseAddr);
   
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

    // Protection for upper 1/32 (BP[3:0] = 0101, TB=0), WP-E on; to protect bad block replacement area
    // DON'T DO THIS. This will prevent writes through the bblut as well.
    // w25n01g_writeRegister(W25N01G_PROT_REG, W25N01G_PROT_PB0_ENABLE|W25N01G_PROT_PB2_ENABLE|W25N01G_PROT_WP_E_ENABLE);

    // No protection, WP-E off, WP-E prevents use of IO2
    w25n01g_writeStatusRegister(W25N01G_PROT_REG, W25N01G_PROT_CLEAR);
   
    // Buffered read mode (BUF = 1), ECC enabled (ECC = 1)
    w25n01g_writeStatusRegister(W25N01G_CONF_REG, W25N01G_CONFIG_ECC_ENABLE | W25N01G_CONFIG_BUFFER_READ_MODE);
    //w25n01g_writeStatusRegister(W25N01G_CONF_REG, W25N01G_CONFIG_ECC_ENABLE);
    //w25n01g_writeStatusRegister(W25N01G_CONF_REG,  W25N01G_CONFIG_BUFFER_READ_MODE);

    w25n01g_readStatusRegister(W25N01G_CONF_REG, true);
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
        if (now >= timeoutAt) {
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

bool w25n01g_detect()
{
    geometry.sectors = 1024;      // Blocks
    geometry.pagesPerSector = 64; // Pages/Blocks
    geometry.pageSize = 2048;

    geometry.sectorSize = geometry.pagesPerSector * geometry.pageSize;
    geometry.totalSize = geometry.sectorSize * geometry.sectors;

    flashConfigurePartitions();
    couldBeBusy = true; // Just for luck we'll assume the chip could be busy even though it isn't specced to be

    w25n01g_deviceReset();

    // Upper 4MB (32 blocks * 128KB/block) will be used for bad block replacement area.

    // Blocks in this area are only written through bad block LUT,
    // and factory written bad block marker in unused blocks are retained.

    // When a replacement block is required,
    // (1) "Read BB LUT" command is used to obtain the last block mapped,
    // (2) blocks after the last block is scanned for a good block,
    // (3) the first good block is used for replacement, and the BB LUT is updated.

    // There are only 20 BB LUT entries, and there are 32 replacement blocks.
    // There will be a least chance of running out of replacement blocks.
    // If it ever run out, the device becomes unusable.

    return true;
}

/**
 * Erase a sector full of bytes to all 1's at the given byte offset in the flash chip.
 */
void w25n01g_eraseSector(uint32_t address)
{

    w25n01g_waitForReady();
    w25n01g_writeEnable(true);
    
    w25n01g_writeCommandWithPageAddress(W25N01G_BLOCK_ERASE, W25N01G_LINEAR_TO_PAGE(address));

    w25n01g_setTimeout( W25N01G_TIMEOUT_BLOCK_ERASE_MS);
}

//
// W25N01G does not support full chip erase.
// Call eraseSector repeatedly.

void w25n01g_eraseCompletely()
{
    for (uint32_t block = 0; block < geometry.sectors; block++) {
        w25n01g_eraseSector(W25N01G_BLOCK_TO_LINEAR(block));
        Serial.print(".");
        if((block % 128) == 0) Serial.println();
     }
}

static void w25n01g_programDataLoad(uint16_t Address, const uint8_t *data, int length)
{

    w25n01g_waitForReady();

    FLEXSPI2_LUT52 = LUT0(CMD_SDR, PINS1, W25N01G_PROGRAM_DATA_LOAD) | LUT1(CADDR_SDR, PINS1, 16);
    FLEXSPI2_LUT53 = LUT0(WRITE_SDR, PINS1, 1);

    flexspi_ip_write(13, W25N01G_LINEAR_TO_COLUMN(Address), data, length);
    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_PROGRAM_MS);
}

static void w25n01g_randomProgramDataLoad(uint16_t Address, const uint8_t *data, int length)
{
  
  uint16_t columnAddress = W25N01G_LINEAR_TO_COLUMN(Address); 

    w25n01g_waitForReady();
    //quadSpiTransmitWithAddress1LINE(W25N01G_INSTRUCTION_RANDOM_PROGRAM_DATA_LOAD, 0, columnAddress, W28N01G_STATUS_COLUMN_ADDRESS_SIZE, data, length);

    FLEXSPI2_LUT52 = LUT0(CMD_SDR, PINS1, W25N01G_RANDOM_PROGRAM_DATA_LOAD) | LUT1(CADDR_SDR, PINS1, 16);
    FLEXSPI2_LUT53 = LUT0(WRITE_SDR, PINS1, 1);
  
  flexspi_ip_read(14, columnAddress, data, length);

  w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_PROGRAM_MS);

}

static void w25n01g_programExecute(uint32_t Address)
{
    Serial.println("w25n01g_programExecute:");
    w25n01g_waitForReady();

    FLEXSPI2_LUT60 = LUT0(CMD_SDR, PINS1, W25N01G_PROGRAM_EXECUTE) | LUT1(DUMMY_SDR, PINS1, 8);
    FLEXSPI2_LUT61 = LUT0(CMD_SDR, PINS1, W25N01G_LINEAR_TO_PAGE(Address));
    
    flexspi_ip_command(15, flashBaseAddr);
    
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

int w25n01g_readBytes(uint32_t address, uint8_t *buffer, int length)
{
    uint32_t targetPage = W25N01G_LINEAR_TO_PAGE(address);

    Serial.printf("%d, %d\n", currentPage, targetPage);

    if (currentPage != targetPage) {
        if (!w25n01g_waitForReady()) {
          Serial.println("Read Returning");
            return 0;
        }

        currentPage = UINT32_MAX;

        w25n01g_writeCommandWithPageAddress( W25N01G_PAGE_DATA_READ, targetPage);
        Serial.println("Write Page Addr Complete");

        w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_READ_MS);
        if (!w25n01g_waitForReady()) {
            return 0;
        }

        currentPage = targetPage;
        Serial.printf("%d, %d\n", currentPage, targetPage);

    }

    uint16_t column = W25N01G_LINEAR_TO_COLUMN(address);
    uint16_t transferLength;

    if (length > W25N01G_PAGE_SIZE - column) {
        transferLength = W25N01G_PAGE_SIZE - column;
    } else {
        transferLength = length;
    }
Serial.println("READING DATA START");
  FLEXSPI2_LUT56 = LUT0(CMD_SDR, PINS1, W25N01G_FAST_READ_QUAD_OUTPUT) | LUT1(CADDR_SDR, PINS1, 0x10);
  FLEXSPI2_LUT57 = LUT0(DUMMY_SDR, PINS1, 8) | LUT1(READ_SDR, PINS1, 1);

   flexspi_ip_read(14, column, buffer, length);
   Serial.println("Command 14 Complete");   
       
    // XXX Don't need this?
    w25n01g_setTimeout(W25N01G_TIMEOUT_PAGE_READ_MS);
    if (!w25n01g_waitForReady()) {
        return 0;
    }

    // Check ECC
    Serial.println("CHECKING ECC-CODE");
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
