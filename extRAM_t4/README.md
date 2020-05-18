# teensy41 PSRAM Library

**WARNING: The first 4MBs of the PSRAM is reserved (for now) for use using EXTMEM or for testing of extmem_malloc.  If you want to use all 8MB of the 1st PSRAM chip change 	```uint32_t eramBaseAddr = 0x70400000;
uint16_t bytesAvailableMB = 4;**
```
to
```
uint32_t eramBaseAddr = 0x70000000;
uint16_t bytesAvailableMB = 8;
```

Instructions TBD.

