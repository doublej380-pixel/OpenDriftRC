#pragma once

#include <Arduino.h>
#include <FS.h>

#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include <SPI.h>

#if defined(OPENDRIFT_USB_MAINTENANCE)
#include <USBMSC.h>
#endif
#endif


class OnboardStorage
{
public:
    bool begin();
    bool isSdAvailable() const;
    fs::FS* fileSystem();
    const char* storageName() const;
    uint64_t cardSizeBytes() const;
    uint64_t freeBytes() const;

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    bool beginUsbMassStorage();
    void endUsbMassStorage();
    bool wasEjected() const;
    #endif

private:
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    SPIClass sdSpi = SPIClass(FSPI);
    bool sdAvailable = false;

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    USBMSC usbDisk;
    volatile bool ejected = false;

    static OnboardStorage* activeInstance;
    static int32_t readBlocks(
        uint32_t lba,
        uint32_t offset,
        void* buffer,
        uint32_t bufferSize
    );
    static int32_t rejectWrites(
        uint32_t lba,
        uint32_t offset,
        uint8_t* buffer,
        uint32_t bufferSize
    );
    static bool handleStartStop(
        uint8_t powerCondition,
        bool start,
        bool loadEject
    );
    #endif
    #endif
};
