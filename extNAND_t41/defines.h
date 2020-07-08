#define LUT0(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)))
#define LUT1(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define CMD_SDR         FLEXSPI_LUT_OPCODE_CMD_SDR
#define CADDR_SDR       FLEXSPI_LUT_OPCODE_CADDR_SDR
#define ADDR_SDR        FLEXSPI_LUT_OPCODE_RADDR_SDR
#define READ_SDR        FLEXSPI_LUT_OPCODE_READ_SDR
#define WRITE_SDR       FLEXSPI_LUT_OPCODE_WRITE_SDR
#define DUMMY_SDR       FLEXSPI_LUT_OPCODE_DUMMY_SDR
#define STOP            FLEXSPI_LUT_OPCODE_STOP
#define PINS1           FLEXSPI_LUT_NUM_PADS_1
#define PINS4           FLEXSPI_LUT_NUM_PADS_4


// Device size parameters
#define W25N01G_PAGE_SIZE         2048
#define W25N01G_PAGES_PER_BLOCK   64
#define W25N01G_BLOCKS_PER_DIE    1024

// BB replacement area
#define W25N01G_BB_MARKER_BLOCKS           1
#define W25N01G_BB_REPLACEMENT_BLOCKS      20
#define W25N01G_BB_MANAGEMENT_BLOCKS       (W25N01G_BB_REPLACEMENT_BLOCKS + W25N01G_BB_MARKER_BLOCKS)
// blocks are zero-based index
#define W25N01G_BB_REPLACEMENT_START_BLOCK (W25N01G_BLOCKS_PER_DIE - W25N01G_BB_REPLACEMENT_BLOCKS)
#define W25N01G_BB_MANAGEMENT_START_BLOCK  (W25N01G_BLOCKS_PER_DIE - W25N01G_BB_MANAGEMENT_BLOCKS)
#define W25N01G_BB_MARKER_BLOCK            (W25N01G_BB_REPLACEMENT_START_BLOCK - W25N01G_BB_MARKER_BLOCKS)

// Instructions

#define W25N01G_RDID                            0x9F
#define W25N01G_DEVICE_RESET                    0xFF
#define W25N01G_READ_STATUS_REG                 0x05
#define W25N01G_READ_STATUS_ALTERNATE_REG       0x0F
#define W25N01G_WRITE_STATUS_REG                0x01
#define W25N01G_WRITE_STATUS_ALTERNATE_REG      0x1F
#define W25N01G_WRITE_ENABLE                    0x06
#define W25N01G_WRITE_DISABLE                   0x04
#define W25N01G_DIE_SELECT                      0xC2
#define W25N01G_BLOCK_ERASE                     0xD8
#define W25N01G_READ_BBM_LUT                    0xA5
#define W25N01G_BB_MANAGEMENT                   0xA1
#define W25N01G_PROGRAM_DATA_LOAD               0x02
#define W25N01G_RANDOM_PROGRAM_DATA_LOAD        0x84
#define W25N01G_QUAD_PROGRAM_DATA_DATA_LOAD     0x32
#define W25N01G_QUAD_RANDOM_LOAD_PROGRAM_DATA   0x34
#define W25N01G_PROGRAM_EXECUTE                 0x10
#define W25N01G_PAGE_DATA_READ                  0x13
#define W25N01G_READ_DATA                       0x03
#define W25N01G_FAST_READ                       0x1B
#define W25N01G_FAST_READ_QUAD_OUTPUT           0x6B
#define W25N01G_FAST_READ_QUAD_IO               0xEB

// Config/status register addresses
#define W25N01G_PROT_REG 0xA0
#define W25N01G_CONF_REG 0xB0
#define W25N01G_STAT_REG 0xC0

// Bits in config/status register 1 (W25N01G_PROT_REG)
#define W25N01G_PROT_CLEAR                (0)
#define W25N01G_PROT_SRP1_ENABLE          (1 << 0)
#define W25N01G_PROT_WP_E_ENABLE          (1 << 1)
#define W25N01G_PROT_TB_ENABLE            (1 << 2)
#define W25N01G_PROT_PB0_ENABLE           (1 << 3)
#define W25N01G_PROT_PB1_ENABLE           (1 << 4)
#define W25N01G_PROT_PB2_ENABLE           (1 << 5)
#define W25N01G_PROT_PB3_ENABLE           (1 << 6)
#define W25N01G_PROT_SRP2_ENABLE          (1 << 7)

// Bits in config/status register 2 (W25N01G_CONF_REG)
#define W25N01G_CONFIG_ECC_ENABLE         (1 << 4)
#define W25N01G_CONFIG_BUFFER_READ_MODE   (1 << 3)

// Bits in config/status register 3 (W25N01G_STATREG)
#define W25N01G_STATUS_BBM_LUT_FULL       (1 << 6)
#define W25N01G_STATUS_FLAG_ECC_POS       4
#define W25N01G_STATUS_FLAG_ECC_MASK      ((1 << 5)|(1 << 4))
#define W25N01G_STATUS_FLAG_ECC(status)   (((status) & W25N01G_STATUS_FLAG_ECC_MASK) >> 4)
#define W25N01G_STATUS_PROGRAM_FAIL       (1 << 3)
#define W25N01G_STATUS_ERASE_FAIL         (1 << 2)
#define W25N01G_STATUS_FLAG_WRITE_ENABLED (1 << 1)
#define W25N01G_STATUS_FLAG_BUSY          (1 << 0)

#define W25N01G_BBLUT_TABLE_ENTRY_COUNT     20
#define W25N01G_BBLUT_TABLE_ENTRY_SIZE      4  // in bytes

// Bits in LBA for BB LUT
#define W25N01G_BBLUT_STATUS_ENABLED (1 << 15)
#define W25N01G_BBLUT_STATUS_INVALID (1 << 14)
#define W25N01G_BBLUT_STATUS_MASK    (W25N01G_BBLUT_STATUS_ENABLED | W25N01G_BBLUT_STATUS_INVALID)

// Some useful defs and macros
#define W25N01G_LINEAR_TO_COLUMN(laddr) ((laddr) % W25N01G_PAGE_SIZE)
#define W25N01G_LINEAR_TO_PAGE(laddr) ((laddr) / W25N01G_PAGE_SIZE)
#define W25N01G_LINEAR_TO_BLOCK(laddr) (W25N01G_LINEAR_TO_PAGE(laddr) / W25N01G_PAGES_PER_BLOCK)
#define W25N01G_BLOCK_TO_PAGE(block) ((block) * W25N01G_PAGES_PER_BLOCK)
#define W25N01G_BLOCK_TO_LINEAR(block) (W25N01G_BLOCK_TO_PAGE(block) * W25N01G_PAGE_SIZE)

// IMPORTANT: Timeout values are currently required to be set to the highest value required by any of the supported flash chips by this driver

// The timeout values (2ms minimum to avoid 1 tick advance in consecutive calls to millis).
#define W25N01G_TIMEOUT_PAGE_READ_MS        2   // tREmax = 60us (ECC enabled)
#define W25N01G_TIMEOUT_PAGE_PROGRAM_MS     2   // tPPmax = 700us
#define W25N01G_TIMEOUT_BLOCK_ERASE_MS      15  // tBEmax = 10ms
#define W25N01G_TIMEOUT_RESET_MS            500 // tRSTmax = 500ms

// Sizes (in bits)
#define W28N01G_STATUS_REGISTER_SIZE        8
#define W28N01G_STATUS_PAGE_ADDRESS_SIZE    16
#define W28N01G_STATUS_COLUMN_ADDRESS_SIZE  16

//
typedef uint16_t flashSector_t;

typedef struct flashGeometry_s {
    flashSector_t sectors; // Count of the number of erasable blocks on the device
    uint16_t pageSize; // In bytes
    uint32_t sectorSize; // This is just pagesPerSector * pageSize
    uint32_t totalSize;  // This is just sectorSize * sectors
    uint16_t pagesPerSector;
} flashGeometry_t;

flashGeometry_t geometry;

//
// flash partitioning api
//

typedef struct flashPartition_s {
    uint8_t type;
    flashSector_t startSector;
    flashSector_t endSector;
} flashPartition_t;

flashPartition_t W28N01G_Partitions;

// + 1 for inclusive, start and end sector can be the same sector.
#define FLASH_PARTITION_SECTOR_COUNT(partition) (partition->endSector + 1 - partition->startSector)

#define FLASH_PARTITION_TYPE_BADBLOCK_MANAGEMENT 1
#define FLASH_PARTITION_TYPE_FLASHFS 0

// 
static const uint32_t flashBaseAddr = 0x01000000u;
static const uint32_t psramAddr =     0x070000000;
static char flashID[3];

//
uint32_t timeoutAt;
bool couldBeBusy;

static uint32_t currentPage = UINT32_MAX;
