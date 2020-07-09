#include "defines.h"
#define mode 0
#define DHprint( a ) { Serial.print( #a); Serial.print(": ");Serial.println ( (uint32_t)a,HEX ); }
#define DTprint( a ) { Serial.println( #a); }
#define Dprint(a) Serial.print( a );

char buf[512] = "Hello World! What a wonderful World :)";
uint8_t temp[512];
uint8_t xData[2111];

void setup(){
    Serial.begin(115200);
    delay(1000);
    Serial.println("Begin Init");

    configure_flash();
    w25n01g_init();

  
    //Lets try an erase
    //w25n01g_deviceErase();  //Erase chip


    uint8_t buffer[2048];
    memset(buffer, 0xFF, 2048);
    for(uint16_t i = 0; i < 2048; i++) buffer[i] = i;

    //Serial.println("Loading data");
    w25n01g_writeEnable(true);   //sets the WEL in Status Reg to 1 (bit 2)
    //w25n01g_programDataLoad(W25N01G_LINEAR_TO_COLUMN(0), buffer, 16);
    w25n01g_randomProgramDataLoad(W25N01G_LINEAR_TO_COLUMN(0), buffer, 16);
    w25n01g_programExecute(W25N01G_LINEAR_TO_PAGE(0));
    //w25n01g_pageProgram(3900, buffer, 32);
    
    //Serial.println("Reading Data");
    memset(buffer, 0, 2048);
    w25n01g_writeEnable(false);
    w25n01g_readBytes(W25N01G_LINEAR_TO_COLUMN(0), buffer, 16);

    for(uint16_t i = 0; i < 32; i++) {
      Serial.printf("0x%02x, ",buffer[i]);
    } Serial.println();


const uint8_t beefy[] = "DEADBEEFdeadbeef\n";
   //Serial.println("Loading data");
Dprint( (char *)beefy )
    memset(buffer, 0, 2048);
    for(uint8_t j = 0; j < 20; j++) buffer[j] = beefy[j];

    w25n01g_writeEnable(true);   //sets the WEL in Status Reg to 1 (bit 2)
    //w25n01g_programDataLoad(W25N01G_LINEAR_TO_COLUMN(4000), buffer, 20);
    w25n01g_randomProgramDataLoad(W25N01G_LINEAR_TO_COLUMN(4000), buffer, 20);
    w25n01g_programExecute(W25N01G_LINEAR_TO_PAGE(4000));
    //w25n01g_pageProgram(4000, buffer, 20);

    
    //Serial.println("Reading Data");
    memset(buffer, 0, 2048);
    w25n01g_writeEnable(false);
    w25n01g_readBytes(W25N01G_LINEAR_TO_COLUMN(4000), buffer, 20);

    for(uint16_t i = 0; i < 32; i++) {
      Serial.printf("0x%02x[%c], ",buffer[i], buffer[i]);
    } Serial.println();  

}

void loop(){}

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
    FLEXSPI2_LUT56 = LUT0(CMD_SDR, PINS1, W25N01G_FAST_READ_QUAD_IO) | LUT1(CADDR_SDR, PINS4, 0x10);
    FLEXSPI2_LUT57 = LUT0(DUMMY_SDR, PINS4, 4) | LUT1(READ_SDR, PINS4, 1);
    
    //cmd 15 - program execute
    FLEXSPI2_LUT60 = LUT0(CMD_SDR, PINS1, W25N01G_PROGRAM_EXECUTE) | LUT1(DUMMY_SDR, PINS1, 8);
    FLEXSPI2_LUT61 = LUT0(ADDR_SDR, PINS1, 0x20);


    //cmd index 10 = exit QPI mode
    //FLEXSPI2_LUT40 = LUT0(CMD_SDR, PINS4, 0xFF);




}

void w25n01g_init() {
    if (flexspi2_flash_id(flashBaseAddr) == 0x21AA) {
      Serial.println("Found W25N01G Flash Chip");
    } else {
      Serial.println("no chip found!");
      exit(1);
    }

    // reset the chip
    w25n01g_startup();
}

void printStatusRegs() {
#if 1
  uint8_t val;

  flexspi_ip_read(8, flashBaseAddr, &val, 1 );
  Serial.print("Status 1:");
  Serial.printf(" %02X", val);
  Serial.printf("\n");
  Serial.print("Binary: "); Serial.println(val, BIN);
  Serial.println();

  // cmd index 9 = read Status register #2 SPI
  //flexspi_ip_read(9, flashBaseAddr[_spiffs_region], &val, 1 );
  //Serial.print("Status 2:");
  //Serial.printf(" %02X", val);
  //Serial.printf("\n");
#endif
}
