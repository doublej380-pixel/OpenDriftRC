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
    struct UsbReadDiagnostics
    {
        uint32_t reads = 0;
        uint32_t failures = 0;
        uint32_t bytes = 0;
        uint32_t lastDurationUs = 0;
        uint32_t maxDurationUs = 0;
        uint32_t lastLba = 0;
        uint32_t lastOffset = 0;
        uint32_t lastSize = 0;
        uint32_t lastReadAtMs = 0;
        uint8_t lastError = 0;
        uint32_t callbacks = 0;
        uint32_t busyReturns = 0;
        uint32_t requestMismatches = 0;
        uint32_t invalidRequests = 0;
        uint32_t lastCallbackWaitUs = 0;
        uint32_t maxCallbackWaitUs = 0;
        uint32_t lastCallbackAtMs = 0;
        uint8_t workerState = 0;
    };

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
    UsbReadDiagnostics getUsbReadDiagnostics() const;
    #endif

private:
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    SPIClass sdSpi = SPIClass(FSPI);
    bool sdAvailable = false;

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    enum UsbReadState : uint8_t
    {
        USB_READ_IDLE,
        USB_READ_REQUESTED,
        USB_READ_RUNNING,
        USB_READ_READY,
        USB_READ_DELIVERING
    };

    USBMSC usbDisk;
    volatile bool ejected = false;
    TaskHandle_t usbReadTaskHandle = nullptr;
    portMUX_TYPE usbReadMux = portMUX_INITIALIZER_UNLOCKED;
    volatile UsbReadState usbReadState = USB_READ_IDLE;
    uint32_t usbReadRequestLba = 0;
    uint32_t usbReadRequestOffset = 0;
    uint32_t usbReadRequestSize = 0;
    int32_t usbReadWorkerResult = -1;
    uint8_t usbReadWorkerBuffer[4096];
    volatile uint32_t usbReadCount = 0;
    volatile uint32_t usbReadFailures = 0;
    volatile uint32_t usbReadBytes = 0;
    volatile uint32_t usbReadLastDurationUs = 0;
    volatile uint32_t usbReadMaxDurationUs = 0;
    volatile uint32_t usbReadLastLba = 0;
    volatile uint32_t usbReadLastOffset = 0;
    volatile uint32_t usbReadLastSize = 0;
    volatile uint32_t usbReadLastAtMs = 0;
    volatile uint8_t usbReadLastError = 0;
    volatile uint32_t usbReadCallbackCount = 0;
    volatile uint32_t usbReadBusyReturns = 0;
    volatile uint32_t usbReadRequestMismatches = 0;
    volatile uint32_t usbReadInvalidRequests = 0;
    volatile uint32_t usbReadLastCallbackWaitUs = 0;
    volatile uint32_t usbReadMaxCallbackWaitUs = 0;
    volatile uint32_t usbReadLastCallbackAtMs = 0;

    static OnboardStorage* activeInstance;
    static void readWorkerTask(void* context);
    static int32_t readBlocks(
        uint32_t lba,
        uint32_t offset,
        void* buffer,
        uint32_t bufferSize
    );
    int32_t performRead(
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
