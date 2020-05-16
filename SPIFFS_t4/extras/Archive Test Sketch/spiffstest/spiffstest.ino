#include <SPI.h>
#include <SerialFlash.h>
#include <spiffs.h>

//#define TEENSY3_WITH_AUDIOSHIELD //Uncomment for T3.x

#ifdef TEENSY3_WITH_AUDIOSHIELD
#define SERIALFLASH_CS 6
#else
#define SERIALFLASH_CS 9
#endif

static SerialFlashChip SPIFlash;
static spiffs fs;

#define LOG_PAGE_SIZE       256

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * 4];
static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];

static s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
  SPIFlash.read(addr, dst, size);
  return SPIFFS_OK;
}

static s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
  SPIFlash.write(addr, (const void *)src, size);
  return SPIFFS_OK;
}

static s32_t my_spiffs_erase(u32_t addr, u32_t size) {
  uint32_t blockSize = SPIFlash.blockSize();
  int s = size;
  while (s > 0) {
    SPIFlash.eraseBlock(addr);
    addr += blockSize;
    s -= blockSize;
  }
  return SPIFFS_OK;
}

void my_spiffs_mount() {
  spiffs_config cfg;
  uint8_t buf[8];
  
  SPIFlash.begin(SERIALFLASH_CS);
  SPIFlash.readID(buf);
  cfg.phys_size = SPIFlash.capacity(buf); // use all spi flash
  cfg.phys_addr = 0; // start spiffs at start of spi flash
  cfg.phys_erase_block = SPIFlash.blockSize(); // according to datasheet
  cfg.log_block_size = SPIFlash.blockSize(); // let us not complicate things
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

static void test_spiffs() {
  char buf[512] = "Hello World! What a wonderful World :)";
  char buf2[512] = "Hello World! What a wonderful World :)";
  Serial.printf("--> Test Start ...\n");

  // Surely, I've mounted spiffs before entering here
  int szLen = strlen( buf );
  spiffs_file fd = SPIFFS_open(&fs, "my_file", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  if (SPIFFS_write(&fs, fd, (u8_t *)buf, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
  strcpy( buf, "xxxxxx");
  fd = SPIFFS_open(&fs, "my_file", SPIFFS_RDWR, 0);
  if (SPIFFS_read(&fs, fd, (u8_t *)buf, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);

  Serial.printf("--> %s <--\n", buf);
  Serial.printf("--> ... Test Middle ... \n--> ");
  for ( int ii=0; 0!=buf[ii]; ii++ ) {
    if ( buf[ii] != buf2[ii] ) Serial.printf( "[Bad #%d==%c|%X]", ii, buf[ii], buf[ii] );
    else Serial.printf( "%c", buf[ii] );
  }
  Serial.printf(" <--\n--> ... Test End.\n");
}

void setup() {
  #ifdef TEENSY3_WITH_AUDIOSHIELD
  SPI.setSCK(14);  // T3 Audio shield has SCK on pin 14
  SPI.setMOSI(7);  // T3 Audio shield has MOSI on pin 7
  #endif
  while (!Serial);
  my_spiffs_mount();
  for (int i = 0; i<100; i++) {
  test_spiffs();
  }
}

void loop() {
}
