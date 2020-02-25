#define LUT0(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)))
#define LUT1(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define CMD_SDR         FLEXSPI_LUT_OPCODE_CMD_SDR
#define ADDR_SDR        FLEXSPI_LUT_OPCODE_RADDR_SDR
#define READ_SDR        FLEXSPI_LUT_OPCODE_READ_SDR
#define WRITE_SDR       FLEXSPI_LUT_OPCODE_WRITE_SDR
#define DUMMY_SDR       FLEXSPI_LUT_OPCODE_DUMMY_SDR
#define PINS1           FLEXSPI_LUT_NUM_PADS_1
#define PINS4           FLEXSPI_LUT_NUM_PADS_4

static const uint32_t flashBaseAddr = 0x01000000u;
static char flashID[8];

void setupFlexSPI2() {
  memset(flashID, 0, sizeof(flashID));
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
              | CCM_CBCMR_FLEXSPI2_PODF(7) | CCM_CBCMR_FLEXSPI2_CLK_SEL(0); // 49.5 MHz
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
  for (int i = 0; i < 64; i++) luttable[i] = 0;
  FLEXSPI2_MCR0 |= FLEXSPI_MCR0_SWRESET;
  while (FLEXSPI2_MCR0 & FLEXSPI_MCR0_SWRESET) ; // wait

  // CBCMR[FLEXSPI2_SEL]
  // CBCMR[FLEXSPI2_PODF]

  FLEXSPI2_LUTKEY = FLEXSPI_LUTKEY_VALUE;
  FLEXSPI2_LUTCR = FLEXSPI_LUTCR_UNLOCK;

  // cmd index 0 = exit QPI mode
  // FLEXSPI2_LUT0 = LUT0(CMD_SDR, PINS4, 0xF5); //RAM
  FLEXSPI2_LUT0 = LUT0(CMD_SDR, PINS4, 0xFF); //FLASH

  // cmd index 1 = reset enable
  FLEXSPI2_LUT4 = LUT0(CMD_SDR, PINS1, 0x66);

  // cmd index 2 = reset
  FLEXSPI2_LUT8 = LUT0(CMD_SDR, PINS1, 0x99);

  // cmd index 3 = read ID bytes
  FLEXSPI2_LUT12 = LUT0(CMD_SDR, PINS1, 0x9F) | LUT1(DUMMY_SDR, PINS1, 24);
  FLEXSPI2_LUT13 = LUT0(READ_SDR, PINS1, 1);

  // cmd index 4 = enter QPI mode
  //FLEXSPI2_LUT16 = LUT0(CMD_SDR, PINS1, 0x35); //RAM
  FLEXSPI2_LUT16 = LUT0(CMD_SDR, PINS1, 0x38); //Flash

  // cmd index 5 = read QPI
  FLEXSPI2_LUT20 = LUT0(CMD_SDR, PINS4, 0xEB) | LUT1(ADDR_SDR, PINS4, 24);
  FLEXSPI2_LUT21 = LUT0(DUMMY_SDR, PINS4, 6) | LUT1(READ_SDR, PINS4, 1);
  ///Flash:  TODO needed to set read parameters!!

  /* TODO Check this
    // cmd index 6 = write QPI
    FLEXSPI2_LUT24 = LUT0(CMD_SDR, PINS4, 0x35) | LUT1(ADDR_SDR, PINS4, 24);
    FLEXSPI2_LUT25 = LUT0(WRITE_SDR, PINS4, 1);
  */
  // cmd index 7 = read ID bytes SPI
  FLEXSPI2_LUT28 = LUT0(CMD_SDR, PINS1, 0x9F) | LUT1(READ_SDR, PINS1, 1);



  // cmd index 8 = read Status register #1 SPI
  FLEXSPI2_LUT32 = LUT0(CMD_SDR, PINS1, 0x05) | LUT1(READ_SDR, PINS1, 1);
  // cmd index 9 = read Status register #2 SPI
  FLEXSPI2_LUT36 = LUT0(CMD_SDR, PINS1, 0x35) | LUT1(READ_SDR, PINS1, 1);
  // cmd index 10 = read Status register #3 SPI
  FLEXSPI2_LUT40 = LUT0(CMD_SDR, PINS1, 0x15) | LUT1(READ_SDR, PINS1, 1);

  //cmd index 11 = write enable QPI
  FLEXSPI2_LUT44 = LUT0(CMD_SDR, PINS4, 0x06);
  //cmd index 12 = sector erase
  FLEXSPI2_LUT48 = LUT0(CMD_SDR, PINS4, 0x20) | LUT1(ADDR_SDR, PINS4, 24);
  FLEXSPI2_LUT49 = LUT0(WRITE_SDR, PINS4, 1);
  //cmd index 13 = page program
  FLEXSPI2_LUT52 = LUT0(CMD_SDR, PINS4, 0x02) | LUT1(ADDR_SDR, PINS4, 24);
  FLEXSPI2_LUT53 = LUT0(WRITE_SDR, PINS4, 1);

}

void printStatusRegs() {
  uint8_t val;

  flexspi_ip_read(8, flashBaseAddr, &val, 1 );
  Serial.print("Status 1:");
  Serial.printf(" %02X", val);
  Serial.printf("\n");

  flexspi_ip_read(9, flashBaseAddr, &val, 1 );
  Serial.print("Status 2:");
  Serial.printf(" %02X", val);
  Serial.printf("\n");

  flexspi_ip_read(10, flashBaseAddr, &val, 1 );
  Serial.print("Status 3:");
  Serial.printf(" %02X", val);
  Serial.printf("\n");
}

void setupFlexSPI2Flash() {
  // reset the chip
  flexspi_ip_command(0, flashBaseAddr);
  flexspi_ip_command(1, flashBaseAddr);
  flexspi_ip_command(2, flashBaseAddr);

  delayMicroseconds(50);
  // flexspi_ip_command(4, flashBaseAddr);
  flexspi_ip_read(7, flashBaseAddr, flashID, sizeof(flashID) ); // flash begins at offset 0x01000000
#if 1
  Serial.print("ID:");
  for (unsigned i = 0; i < sizeof(flashID); i++) Serial.printf(" %02X", flashID[i]);
  Serial.printf("\n");
#endif

  printStatusRegs();

  //TODO!!!!! set QPI enable bit in status reg #2 if not factory set!!!!!

  Serial.println("ENTER QPI MODE");
  flexspi_ip_command(4, flashBaseAddr);
  Serial.println("WRITE ENABLE");
  flexspi_ip_command(11, flashBaseAddr);  //write enable

  //patch LUT for QPI:
  // cmd index 8 = read Status register #1
  FLEXSPI2_LUT32 = LUT0(CMD_SDR, PINS4, 0x05) | LUT1(READ_SDR, PINS4, 1);
  // cmd index 9 = read Status register #2
  FLEXSPI2_LUT36 = LUT0(CMD_SDR, PINS4, 0x35) | LUT1(READ_SDR, PINS4, 1);
  // cmd index 10 = read Status register #3
  FLEXSPI2_LUT40 = LUT0(CMD_SDR, PINS4, 0x15) | LUT1(READ_SDR, PINS4, 1);

  printStatusRegs();

  uint8_t dr[128];
  /*does not work*/
#if 0
  Serial.println("Erase Sector 0");
  flexspi_ip_command(12, flashBaseAddr); //does not work?

  while (1) { //Wait for busy-bit clear
    uint8_t val;
    flexspi_ip_read(8, flashBaseAddr, &val, 1 );
    if ((val & 0x01) == 0) break;
  }

  memset(dr, 0, sizeof(dr));
  flexspi_ip_read(5, flashBaseAddr, dr, 128);
  for (int i = 0; i < 128; i++) Serial.print(dr[i], HEX);
  Serial.println();

#endif


  Serial.println("Program Page 0");
  uint8_t data[128];
  for (int i = 0; i < 128; i++) data[i] = i;
  flexspi_ip_write(13, flashBaseAddr, data, 128);

  while (1) { //Wait for busy-bit clear
    uint8_t val;
    flexspi_ip_read(8, flashBaseAddr, &val, 1 );
    if ((val & 0x01) == 0) break;
  }

  Serial.println("Ready");


  //uint8_t dr[128];
  memset(dr, 0, sizeof(dr));
  flexspi_ip_read(5, flashBaseAddr, dr, 128);
  for (int i = 0; i < 128; i++) Serial.print(dr[i], HEX);
  Serial.println();
}

void setup() {
  setupFlexSPI2();
  while (!Serial);
  setupFlexSPI2Flash();

  //Todo ..................
  //erase sector (4k) (+check busy flag)
  //testwrite page (cmd 02) (+check busy flag)
  //read page
  //read page by memory access

}

void loop() {


}

void readmem(uint32_t addr, void *data, uint32_t length)
{
  uint8_t *p = (uint8_t *)(0x70000000 + addr);
  arm_dcache_flush(p, length);
  memset(data, 0xFF, length);
  flexspi_ip_read(5, addr, data, length);
  Serial.printf("read @%06X: ", addr);
  for (uint32_t i = 0; i < length; i++) Serial.printf(" %02X", *(((uint8_t *)data) + i));
  Serial.printf("\n");
  arm_dcache_delete(p, length);
  Serial.printf("rd @%08X: ", (uint32_t)p);
  for (uint32_t i = 0; i < length; i++) Serial.printf(" %02X", p[i]);
  Serial.printf("\n");
}

void writemem(uint32_t addr, const void *data, uint32_t length)
{
  Serial.printf("write @%06X:", addr);
  for (uint32_t i = 0; i < length; i++) Serial.printf(" %02X", *(((uint8_t *)data) + i));
  Serial.printf("\n");
  flexspi_ip_write(6, addr, data, length);
}

void flexspi_ip_command(uint32_t index, uint32_t addr)
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
        *(uint32_t *)(p + 0) = FLEXSPI2_RFDR0;
        *(uint32_t *)(p + 4) = FLEXSPI2_RFDR1;
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
        //Serial.print("%");
        memcpy((void *)&FLEXSPI2_TFDR0, src, wrlen);
        src += wrlen;
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
