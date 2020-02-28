/*
   This test uses the optional quad spi flash on Teensy 4.1

   ATTENTION: Flash needs to be empty before first use of SPIFFS


   Frank B, 2020
*/

#include <extRAM_t4.h>
#include <spiffs.h>

extRAM_t4 flashRAM;

static spiffs SPIFFS; //filesystem

char buf[512] = "Hello World! What a wonderful World :)";
int szLen = strlen( buf );

void test_spiffs_write() {
  // Surely, I've mounted spiffs before entering here
  spiffs_file fd = SPIFFS_open(&SPIFFS, "my_file", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  if (SPIFFS_write(&SPIFFS, fd, (u8_t *)buf, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&SPIFFS));
  SPIFFS_close(&SPIFFS, fd);
  SPIFFS_fflush(&SPIFFS, fd);
}

static void test_spiffs_read() {
  // Surely, I've mounted spiffs before entering here
  spiffs_file  fd = SPIFFS_open(&SPIFFS, "my_file", SPIFFS_RDWR, 0);
  if (SPIFFS_read(&SPIFFS, fd, (u8_t *)buf, szLen) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&SPIFFS));
  SPIFFS_close(&SPIFFS, fd);
}


void setup() {
  while (!Serial);

#if 1
  flashRAM.flashBegin();
  eraseFlashChip();
#endif

  Serial.println("Mount SPIFFS:");
  flashRAM.flashBegin();
  my_spiffs_mount();

#if 1
  Serial.println("Write file:");
  Serial.println(buf);
  test_spiffs_write();
#endif

  memset(buf, 0, sizeof(buf)); //emtpy buffer

  Serial.println("Read file:");
  test_spiffs_read();
  Serial.println(buf);

String str = "";
  spiffs_DIR d;
  struct spiffs_dirent e;
  struct spiffs_dirent *pe = &e;

  SPIFFS_opendir(&SPIFFS, "/", &d);
  Serial.println(); Serial.println("Directory list");
  while ((pe = SPIFFS_readdir(&d, pe))) {
    Serial.printf("%s [%04x] size:%i\n", pe->name, pe->obj_id, pe->size);
  }
  SPIFFS_closedir(&d);

  Serial.println(); Serial.println("Space total/available");
  u32_t total, used;
  SPIFFS_info(&SPIFFS, &total, &used);
  Serial.printf("Total: %d / Used: %d\n", total, used);
    

}

void loop() {
}

//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************

void waitFlash(boolean visual = false) {
  uint8_t val;
  FLEXSPI_IPRXFCR = FLEXSPI_IPRXFCR_CLRIPRXF; // clear rx fifo
  do { //Wait for busy-bit clear
    flashRAM.flexspi_ip_read(8, flashBaseAddr, &val, 1 );
    if (visual) {
      Serial.print("."); delay(500);
    }
  } while  ((val & 0x01) == 1);
}


void eraseFlashChip() {
  flashRAM.flexspi_ip_command(11, flashBaseAddr);
  delay(10);

  Serial.println("Erasing.... (may take some time)");
  uint32_t t = millis();
  FLEXSPI2_LUT60 = LUT0(CMD_SDR, PINS4, 0x60); //Chip erase
  flashRAM.flexspi_ip_command(15, flashBaseAddr);
  waitFlash(true);
  asm("":::"memory");
  t = millis() - t;
  Serial.printf("Chip erased in %d seconds.\n", t / 1000);
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

static s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
  flashRAM.flexspi_ip_read(5, addr, dst, size);
  return SPIFFS_OK;
}

static s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
  flashRAM.flexspi_ip_command(11, flashBaseAddr);  //write enable
  flashRAM.flexspi_ip_write(13, addr, src, size);
  waitFlash(); //TODO: Can we wait at the beginning instead?
  return SPIFFS_OK;
}

static s32_t my_spiffs_erase(u32_t addr, u32_t size) {
  flashRAM.flexspi_ip_command(11, flashBaseAddr);  //write enable
  int s = size;
  while (s > 0) { //TODO: Is this loop needed, or is size max 4096?
    flashRAM.flexspi_ip_command(12, addr);
    addr += blocksize;
    s -= blocksize;
    waitFlash(); //TODO: Can we wait at the beginning intead?
  }
  return SPIFFS_OK;
}

//********************************************************************************************************

void my_spiffs_mount() {

  spiffs_config cfg;

  cfg.phys_size = 1024 * 1024 * 16; // use 16 MB flash TODO use ID to get capacity
  cfg.phys_addr = /* 0x70000000 + */flashBaseAddr; // start spiffs here (physical adress)
  cfg.phys_erase_block = blocksize; //4K sectors
  cfg.log_block_size = cfg.phys_erase_block; // let us not complicate things
  cfg.log_page_size = LOG_PAGE_SIZE; // as we said

  cfg.hal_read_f = my_spiffs_read;
  cfg.hal_write_f = my_spiffs_write;
  cfg.hal_erase_f = my_spiffs_erase;

  int res = SPIFFS_mount(&SPIFFS,
                         &cfg,
                         spiffs_work_buf,
                         spiffs_fds,
                         sizeof(spiffs_fds),
                         spiffs_cache_buf,
                         sizeof(spiffs_cache_buf),
                         0);
  Serial.printf("mount res: %i\n", res);
}
