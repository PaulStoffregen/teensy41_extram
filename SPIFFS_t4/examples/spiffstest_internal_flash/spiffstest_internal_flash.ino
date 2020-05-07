/*
   This test uses the internal flash
*/

#include <spiffs.h>

#define LOG_PAGE_SIZE       256

static spiffs fs;
static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * 4];
static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];


#define LUT0(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)))
#define LUT1(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define CMD_SDR         FLEXSPI_LUT_OPCODE_CMD_SDR
#define ADDR_SDR        FLEXSPI_LUT_OPCODE_RADDR_SDR
#define READ_SDR        FLEXSPI_LUT_OPCODE_READ_SDR
#define WRITE_SDR       FLEXSPI_LUT_OPCODE_WRITE_SDR
#define PINS1           FLEXSPI_LUT_NUM_PADS_1
#define PINS4           FLEXSPI_LUT_NUM_PADS_4

static void flash_wait()
{
  FLEXSPI_LUT60 = LUT0(CMD_SDR, PINS1, 0x05) | LUT1(READ_SDR, PINS1, 1); // 05 = read status
  FLEXSPI_LUT61 = 0;
  uint8_t status;
  do {
    FLEXSPI_IPRXFCR = FLEXSPI_IPRXFCR_CLRIPRXF; // clear rx fifo
    FLEXSPI_IPCR0 = 0;
    FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15) | FLEXSPI_IPCR1_IDATSZ(1);
    FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
    while (!(FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE)) {
      asm("nop");
    }
    FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
    status = *(uint8_t *)&FLEXSPI_RFDR0;
  } while (status & 1);
  FLEXSPI_MCR0 |= FLEXSPI_MCR0_SWRESET; // purge stale data from FlexSPI's AHB FIFO
  while (FLEXSPI_MCR0 & FLEXSPI_MCR0_SWRESET) ; // wait
  __enable_irq();
}

// write bytes into flash memory (which is already erased to 0xFF)
static void flash_write(void *addr, const void *data, uint32_t len)
{
  __disable_irq();
  FLEXSPI_LUTKEY = FLEXSPI_LUTKEY_VALUE;
  FLEXSPI_LUTCR = FLEXSPI_LUTCR_UNLOCK;
  FLEXSPI_IPCR0 = 0;
  FLEXSPI_LUT60 = LUT0(CMD_SDR, PINS1, 0x06); // 06 = write enable
  FLEXSPI_LUT61 = 0;
  FLEXSPI_LUT62 = 0;
  FLEXSPI_LUT63 = 0;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  arm_dcache_delete(addr, len); // purge old data from ARM's cache
  while (!(FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE)) ; // wait
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
  FLEXSPI_LUT60 = LUT0(CMD_SDR, PINS1, 0x32) | LUT1(ADDR_SDR, PINS1, 24); // 32 = quad write
  FLEXSPI_LUT61 = LUT0(WRITE_SDR, PINS4, 1);
  FLEXSPI_IPTXFCR = FLEXSPI_IPTXFCR_CLRIPTXF; // clear tx fifo
  FLEXSPI_IPCR0 = (uint32_t)addr & 0x007FFFFF;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15) | FLEXSPI_IPCR1_IDATSZ(len);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  const uint8_t *src = (const uint8_t *)data;
  uint32_t n;
  while (!((n = FLEXSPI_INTR) & FLEXSPI_INTR_IPCMDDONE)) {
    if (n & FLEXSPI_INTR_IPTXWE) {
      uint32_t wrlen = len;
      if (wrlen > 8) wrlen = 8;
      if (wrlen > 0) {
        memcpy((void *)&FLEXSPI_TFDR0, src, wrlen);
        src += wrlen;
        len -= wrlen;
      }
      FLEXSPI_INTR = FLEXSPI_INTR_IPTXWE;
    }
  }
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE | FLEXSPI_INTR_IPTXWE;
  flash_wait();
}

// erase a 4K sector
static void flash_erase_sector(void *addr)
{
  __disable_irq();
  FLEXSPI_LUTKEY = FLEXSPI_LUTKEY_VALUE;
  FLEXSPI_LUTCR = FLEXSPI_LUTCR_UNLOCK;
  FLEXSPI_LUT60 = LUT0(CMD_SDR, PINS1, 0x06); // 06 = write enable
  FLEXSPI_LUT61 = 0;
  FLEXSPI_LUT62 = 0;
  FLEXSPI_LUT63 = 0;
  FLEXSPI_IPCR0 = 0;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  arm_dcache_delete((void *)((uint32_t)addr & 0xFFFFF000), 4096); // purge data from cache
  while (!(FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE)) ; // wait
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
  FLEXSPI_LUT60 = LUT0(CMD_SDR, PINS1, 0x20) | LUT1(ADDR_SDR, PINS1, 24); // 20 = sector erase
  FLEXSPI_IPCR0 = (uint32_t)addr & 0x007FF000;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  while (!(FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE)) ; // wait
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
  flash_wait();
}


static s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
  memcpy((void *)dst, (void *)addr, size);
  return SPIFFS_OK;
}

static s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
  flash_write((void *)addr, (const void *)src, size);

#if 0
  Serial.printf("write %0X len %d : ", addr, size);
  for (unsigned ii = 0; ii < size; ii++) Serial.printf("%0x ",src[ii]);
  Serial.println();
#endif

  return SPIFFS_OK;
}

static s32_t my_spiffs_erase(u32_t addr, u32_t size) {
  int s = size;
  while (s > 0) {
    flash_erase_sector((void *)addr);
    addr += 4096;
    s -= 4096;
  }
  Serial.printf("erase %0X len %d\n", addr, size);
  return SPIFFS_OK;
}

extern unsigned long _flashimagelen;
const unsigned long _startFS = 0x60000000UL + ((unsigned)&_flashimagelen & ~0x3fff) + 0x4000;

void my_spiffs_mount() {

  spiffs_config cfg;

  cfg.phys_size = 1024 * 1024 * 1; // use 1 MB flash
  cfg.phys_addr = _startFS; // start spiffs here
  cfg.phys_erase_block = 4096;
  cfg.log_block_size = cfg.phys_erase_block; // let us not complicate things
  cfg.log_page_size = LOG_PAGE_SIZE; // as we said

  cfg.hal_read_f = my_spiffs_read;
  cfg.hal_write_f = my_spiffs_write;
  cfg.hal_erase_f = my_spiffs_erase;

  int res = SPIFFS_mount(&fs,
                         &cfg,
                         spiffs_work_buf,
                         spiffs_fds,
                         sizeof(spiffs_fds),
                         spiffs_cache_buf,
                         sizeof(spiffs_cache_buf),
                         0);
  Serial.printf("mount res: %i\n", res);
}


char buf[512] = "Hello World! What a wonderful World :)";
char buf2[512] = "Hello World! What a wonderful World :)";
int szLen = strlen( buf );

void test_spiffs_write() {

  // Surely, I've mounted spiffs before entering here

  spiffs_file fd = SPIFFS_open(&fs, "my_file", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  if (SPIFFS_write(&fs, fd, (u8_t *)buf, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
  SPIFFS_fflush(&fs, fd);
}

static void test_spiffs_read() {

  // Surely, I've mounted spiffs before entering here

  spiffs_file  fd = SPIFFS_open(&fs, "my_file", SPIFFS_RDWR, 0);
  if (SPIFFS_read(&fs, fd, (u8_t *)buf, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
}

void setup() {
  while (!Serial);
  Serial.println();
  Serial.printf("SPIFFS filesystem starts at: 0x%0X\n\n", _startFS);
  Serial.flush();
  delay(10);
  
  my_spiffs_mount();

  Serial.printf("--> Test Start ...\n");

  test_spiffs_write();

  Serial.printf("--> %s <--\n", buf);
  Serial.printf("--> ... Test Middle ... \n--> ");
  
  strcpy( buf, "xxxxxx");
  
  Serial.printf("--> %s <--\n", buf);
  Serial.printf("--> ... Test Middle ... \n--> ");
  
  test_spiffs_read();
  
  Serial.printf("--> %s <--\n", buf);
  Serial.printf("--> ... Test Middle ... \n--> ");
  
  for ( int ii = 0; 0 != buf[ii]; ii++ ) {
    if ( buf[ii] != buf2[ii] ) Serial.printf( "[Bad #%d==%c|%X]", ii, buf[ii], buf[ii] );
    else Serial.printf( "%c", buf[ii] );
  }
  
  Serial.printf(" <--\n--> ... Test End.\n");
}

void loop() {
}
