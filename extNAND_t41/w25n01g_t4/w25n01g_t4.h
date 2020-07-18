/**************************************************************************/
#ifndef _W25N01G_T4_H_
#define _W25N01G_T4_H_

#if !defined(ARDUINO_TEENSY41)
#error "Sorry, extFlashSpiffs_t4 only works only with Teensy 4.1"
#endif

#include <algorithm>
#include <inttypes.h>
#include <Arduino.h>

#define LUT0(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)))
#define LUT1(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define CMD_SDR         FLEXSPI_LUT_OPCODE_CMD_SDR
#define CADDR_SDR       FLEXSPI_LUT_OPCODE_CADDR_SDR
#define ADDR_SDR        FLEXSPI_LUT_OPCODE_RADDR_SDR
#define READ_SDR        FLEXSPI_LUT_OPCODE_READ_SDR
#define WRITE_SDR       FLEXSPI_LUT_OPCODE_WRITE_SDR
#define DUMMY_SDR       FLEXSPI_LUT_OPCODE_DUMMY_SDR
#define PINS1           FLEXSPI_LUT_NUM_PADS_1
#define PINS4           FLEXSPI_LUT_NUM_PADS_4


// Device size parameters
#define PAGE_SIZE         2048
#define PAGE_ECCSIZE	  2112
#define PAGES_PER_BLOCK   64
#define BLOCKS_PER_DIE    1024

// BB replacement area
#define BB_MARKER_BLOCKS           1
#define BB_REPLACEMENT_BLOCKS      20
#define BB_MANAGEMENT_BLOCKS       (BB_REPLACEMENT_BLOCKS + BB_MARKER_BLOCKS)
// blocks are zero-based index
#define BB_REPLACEMENT_START_BLOCK (BLOCKS_PER_DIE - BB_REPLACEMENT_BLOCKS)
#define BB_MANAGEMENT_START_BLOCK  (BLOCKS_PER_DIE - BB_MANAGEMENT_BLOCKS)
#define BB_MARKER_BLOCK            (BB_REPLACEMENT_START_BLOCK - BB_MARKER_BLOCKS)

// Instructions

#define RDID                            0x9F
#define DEVICE_RESET                    0xFF
#define READ_STATUS_REG                 0x05
#define READ_STATUS_ALTERNATE_REG       0x0F
#define WRITE_STATUS_REG                0x01
#define WRITE_STATUS_ALTERNATE_REG      0x1F
#define WRITE_ENABLE                    0x06
#define WRITE_DISABLE                   0x04
#define DIE_SELECT                      0xC2
#define BLOCK_ERASE                     0xD8
#define READ_BBM_LUT                    0xA5
#define BB_MANAGEMENT                   0xA1
#define PROGRAM_DATA_LOAD               0x02
#define RANDOM_PROGRAM_DATA_LOAD        0x84
#define QUAD_PROGRAM_DATA_DATA_LOAD     0x32
#define QUAD_RANDOM_LOAD_PROGRAM_DATA   0x34
#define PROGRAM_EXECUTE                 0x10
#define PAGE_DATA_READ                  0x13
#define READ_DATA                       0x03
#define FAST_READ                       0x0B
#define FAST_READ_QUAD_OUTPUT           0x6B
#define FAST_READ_QUAD_IO               0xEB

// Config/status register addresses
#define PROT_REG 0xA0
#define CONF_REG 0xB0
#define STAT_REG 0xC0

// Bits in config/status register 1 (PROT_REG)
#define PROT_CLEAR                (0)
#define PROT_SRP1_ENABLE          (1 << 0)
#define PROT_WP_E_ENABLE          (1 << 1)
#define PROT_TB_ENABLE            (1 << 2)
#define PROT_PB0_ENABLE           (1 << 3)
#define PROT_PB1_ENABLE           (1 << 4)
#define PROT_PB2_ENABLE           (1 << 5)
#define PROT_PB3_ENABLE           (1 << 6)
#define PROT_SRP2_ENABLE          (1 << 7)

// Bits in config/status register 2 (CONF_REG)
#define CONFIG_ECC_ENABLE         (1 << 4)
#define CONFIG_BUFFER_READ_MODE   (1 << 3)

// Bits in config/status register 3 (STATREG)
#define STATUS_BBM_LUT_FULL       (1 << 6)
#define STATUS_FLAG_ECC_POS       4
#define STATUS_FLAG_ECC_MASK      ((1 << 5)|(1 << 4))
#define STATUS_FLAG_ECC(status)   (((status) & STATUS_FLAG_ECC_MASK) >> 4)
#define STATUS_PROGRAM_FAIL       (1 << 3)
#define STATUS_ERASE_FAIL         (1 << 2)
#define STATUS_FLAG_WRITE_ENABLED (1 << 1)
#define STATUS_FLAG_BUSY          (1 << 0)

#define BBLUT_TABLE_ENTRY_COUNT     20
#define BBLUT_TABLE_ENTRY_SIZE      4  // in bytes

// Bits in LBA for BB LUT
#define BBLUT_STATUS_ENABLED (1 << 15)
#define BBLUT_STATUS_INVALID (1 << 14)
#define BBLUT_STATUS_MASK    (BBLUT_STATUS_ENABLED | BBLUT_STATUS_INVALID)

// Some useful defs and macros
#define LINEAR_TO_COLUMNECC(laddr) ((laddr) % PAGE_ECCSIZE)
#define LINEAR_TO_COLUMN(laddr) ((laddr) % PAGE_SIZE)
#define LINEAR_TO_PAGE(laddr) ((laddr) / PAGE_SIZE)
#define LINEAR_TO_PAGEECC(laddr) ((laddr) / PAGE_ECCSIZE)
#define LINEAR_TO_BLOCK(laddr) (LINEAR_TO_PAGE(laddr) / PAGES_PER_BLOCK)
#define BLOCK_TO_PAGE(block) ((block) * PAGES_PER_BLOCK)
#define BLOCK_TO_LINEAR(block) (BLOCK_TO_PAGE(block) * PAGE_SIZE)

// IMPORTANT: Timeout values are currently required to be set to the highest value required by any of the supported flash chips by this driver

// The timeout values (2ms minimum to avoid 1 tick advance in consecutive calls to millis).
#define TIMEOUT_PAGE_READ_MS        2   // tREmax = 60us (ECC enabled)
#define TIMEOUT_PAGE_PROGRAM_MS     2   // tPPmax = 700us
#define TIMEOUT_BLOCK_ERASE_MS      15  // tBEmax = 10ms
#define TIMEOUT_RESET_MS            500 // tRSTmax = 500ms

// Sizes (in bits)
#define W28N01G_STATUS_REGISTER_SIZE        8
#define W28N01G_STATUS_PAGE_ADDRESS_SIZE    16
#define W28N01G_STATUS_COLUMN_ADDRESS_SIZE  16

//Geometry
#define sectors              1024
#define pagesPerSector         64
#define pageSize              2048
#define sectorSize          pagesPerSector * pageSize
#define totalSize           sectorSize * geometry.sectors
#define eccSize                 64


//
static const uint32_t flashBaseAddr = 0x01000000u;
static const uint32_t psramAddr =     0x070000000;
static char flashID[3];

static uint32_t currentPage = UINT32_MAX;
static uint32_t currentPageRead = UINT32_MAX;
static uint32_t timeoutAt;

class w25n01g_t4 
{

public:
	w25n01g_t4();
	void  begin();
	void configure_flash();
	void init();

	FLASHMEM static uint32_t flexspi2_flash_id(uint32_t addr);
	static uint8_t readStatusRegister(uint8_t reg, bool dump);
	static void writeStatusRegister(uint8_t reg, uint8_t data);
	static void setTimeout(uint32_t timeoutMillis);
	static void deviceReset();
	static void writeEnable(bool wpE);
	static void eraseSector(uint32_t address);
	void deviceErase();
	static void programDataLoad(uint32_t Address, const uint8_t *data, int length);
	static void randomProgramDataLoad(uint32_t Address, const uint8_t *data, int length);
	static void programExecute(uint32_t Address);
	void writeBytes(uint32_t Address, const uint8_t *data, int length );
	int readSector(uint32_t address, uint8_t *data, int length);
	void readBytes(uint32_t Address, uint8_t *data, int length);
	void read(uint32_t address, uint8_t *data, int length);
	void readECC(uint32_t address, uint8_t *data, int length);
	void readBBLUT();

	static bool isReady();
	static bool waitForReady();

private:
	static void flexspi_ip_read(uint32_t index, uint32_t addr, void *data, uint32_t length);
	static void flexspi_ip_write(uint32_t index, uint32_t addr, const void *data, uint32_t length);
	static void flexspi_ip_command(uint32_t index, uint32_t addr);

};

#endif

