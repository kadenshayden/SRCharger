/* Define memory regions */
MEMORY {
    FLASH_BANK_1   (rx) : ORIGIN = 0x08000000, LENGTH = 128K
    SRAM_1        (rwx) : ORIGIN = 0x30000000, LENGTH = 16K
    SRAM_2        (rwx) : ORIGIN = 0x30004000, LENGTH = 16K
    SRAM_4        (rwx) : ORIGIN = 0x38000000, LENGTH = 16K
    RAM_DTCM       (rw) : ORIGIN = 0x20000000, LENGTH = 128K
    RAM_ITCM      (rwx) : ORIGIN = 0x00000000, LENGTH =
        DEFINED(STM32H7_TCM_AXI_SHARED_00) ? 64K :
        DEFINED(STM32H7_TCM_AXI_SHARED_01) ? 128K :
        DEFINED(STM32H7_TCM_AXI_SHARED_10) ? 192K :
        DEFINED(STM32H7_TCM_AXI_SHARED_11) ? 256K :
        64K
    SRAM_AXI_1    (rwx) : ORIGIN = 0x24000000, LENGTH =
        DEFINED(STM32H7_TCM_AXI_SHARED_00) ? 320K :
        DEFINED(STM32H7_TCM_AXI_SHARED_01) ? 256K :
        DEFINED(STM32H7_TCM_AXI_SHARED_10) ? 192K :
        DEFINED(STM32H7_TCM_AXI_SHARED_11) ? 128K :
        128K
    SRAM_BKP       (rw) : ORIGIN = 0x38800000, LENGTH = 4K
}

/* Include common linker script */
INCLUDE common.ld
