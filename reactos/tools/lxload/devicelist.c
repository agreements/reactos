#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "luserhw.h"

struct dev_t error_device = {
    "vaccum",
    0,
    -1,
    NULL,
    NULL,

    readPortError,
    readPortError,
    readPortError,

    writePortError,
    writePortError,
    writePortError,
};

struct dev_t pit_8254 = {
    "PIT-8254",
    0x40,
    4,
    &pit_data,
    &error_device,
    
    pitReadByte,
    defaultReadWord,
    defaultReadLong,

    pitWriteByte,
    defaultWriteWord,
    defaultWriteLong
};

struct dev_t pic_8259[2] = {
    { "PIC-8259-Primary",
      0x20,
      2,
      &pic_data1,
      &pit_8254,
      
      picReadByte,
      defaultReadWord,
      defaultReadLong,
      
      picWriteByte,
      defaultWriteWord,
      defaultWriteLong
    },

    { "PIC-8259-Secondary",
      0xa0,
      2,
      &pic_data2,
      &pic_8259[0],
      
      picReadByte,
      defaultReadWord,
      defaultReadLong,
      
      picWriteByte,
      defaultWriteWord,
      defaultWriteLong
    }
};

struct dev_t cmos_dev = {
    "CMOS-NVRAM",
    0x70,
    2,
    &cmos,
    &pic_8259[1],

    cmosReadByte,
    defaultReadWord,
    defaultReadLong,

    cmosWriteByte,
    defaultWriteWord,
    defaultWriteLong
};

struct dev_t com1_dev = {
    "UART-8250-COM1",
    0x3f8,
    8,
    &com_data1,
    &cmos_dev,
    serialReadByte,
    defaultReadWord,
    defaultReadLong,
    
    serialWriteByte,
    defaultWriteWord,
    defaultWriteLong
};

struct dev_t ide1_dev = {
    "IDE1-DISK",
    0x1f0,
    8,
    &ide_data1,
    &com1_dev,

    ideReadByte,
    ideReadWord,
    defaultReadLong,

    ideWriteByte,
    ideWriteWord,
    defaultWriteLong
};

struct dev_t ide1_altdev = {
    "IDE1-ALT-PORT",
    0x3f6,
    2,
    &ide_data1,
    &ide1_dev,

    ideAltReadByte,
    defaultReadWord,
    defaultReadLong,

    ideAltWriteByte,
    defaultWriteWord,
    defaultWriteLong
};

struct dev_t pci_dev = {
    "PCI Bus",
    0xcf8,
    8,
    &pci_data,
    &ide1_altdev,

    pciReadByte,
    pciReadWord,
    pciReadDword,

    pciWriteByte,
    pciWriteWord,
    pciWriteDword
};

struct dev_t *head = &pci_dev;
