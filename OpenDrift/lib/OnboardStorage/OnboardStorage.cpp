#include "OnboardStorage.h"

#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include <SD.h>
#include <vfs_api.h>
extern "C" {
#include <ff.h>
#include <diskio.h>
}

DRESULT ff_sd_read(uint8_t pdrv, uint8_t* buffer, DWORD sector, UINT count);

static constexpr int SD_CS_PIN = 38;
static constexpr int SD_MOSI_PIN = 39;
static constexpr int SD_MISO_PIN = 40;
static constexpr int SD_CLOCK_PIN = 41;
static constexpr uint32_t SD_FREQUENCY_HZ = 20000000;
static constexpr uint32_t USB_SD_READ_CHUNK_SIZE = 4096;

class OpenDriftSDFS : public fs::SDFS
{
public:
    OpenDriftSDFS() : SDFS(fs::FSImplPtr(new VFSImpl())) {}

    bool readRAW(
        uint8_t* buffer,
        uint32_t sector,
        uint32_t sectorCount
    )
    {
        return
            _pdrv != 0xFF &&
            sectorCount > 0 &&
            ff_sd_read(_pdrv, buffer, sector, sectorCount) == RES_OK;
    }
};

static OpenDriftSDFS storageSd;

#if defined(OPENDRIFT_USB_MAINTENANCE)
OnboardStorage* OnboardStorage::activeInstance = nullptr;

// TinyUSB's callback expects the active LUN count and converts it to the
// highest zero-based LUN in the USB response. OpenDrift always constructs
// FirmwareMSC as LUN 0 and the SD disk as LUN 1, so the count is two.
extern "C" uint8_t tud_msc_get_maxlun_cb(void)
{
    return 2;
}

// UsbMaintenance is constructed first, making the firmware updater LUN 0 and
// the SD card LUN 1. TinyUSB uses this callback for the
// SCSI write-protect bit reported to the host.
extern "C" bool tud_msc_is_writable_cb(uint8_t lun)
{
    return lun == 0;
}

// The ESP32-S3 uses a full-speed PHY. Advertising USB 1.1 avoids Windows 11
// dual-LUN enumeration failures seen with the framework's USB 2.0 descriptor.
// bcdDevice is advanced so Windows refreshes the corrected device layout.
static constexpr uint8_t OPENDRIFT_USB_DEVICE_DESCRIPTOR[] = {
    18, 0x01,
    0x10, 0x01,
    0xEF, 0x02, 0x01,
    64,
    0x3A, 0x30,
    0x02, 0x00,
    0x01, 0x01,
    0x01, 0x02, 0x03,
    0x01
};

extern "C" const uint8_t* tud_descriptor_device_cb(void)
{
    return OPENDRIFT_USB_DEVICE_DESCRIPTOR;
}
#endif
#endif


bool OnboardStorage::begin()
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(sdAvailable)
    {
        return true;
    }

    sdSpi.begin(
        SD_CLOCK_PIN,
        SD_MISO_PIN,
        SD_MOSI_PIN,
        SD_CS_PIN
    );

    sdAvailable = storageSd.begin(
        SD_CS_PIN,
        sdSpi,
        SD_FREQUENCY_HZ,
        "/sd",
        5,
        false
    );

    if(!sdAvailable || storageSd.cardType() == CARD_NONE)
    {
        storageSd.end();
        sdAvailable = false;
    }

    return sdAvailable;
    #else
    return false;
    #endif
}


bool OnboardStorage::isSdAvailable() const
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    return sdAvailable;
    #else
    return false;
    #endif
}


fs::FS* OnboardStorage::fileSystem()
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    return sdAvailable ? static_cast<fs::FS*>(&storageSd) : nullptr;
    #else
    return nullptr;
    #endif
}


const char* OnboardStorage::storageName() const
{
    return isSdAvailable() ? "SD card" : "internal flash";
}


uint64_t OnboardStorage::cardSizeBytes() const
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    return sdAvailable ? storageSd.cardSize() : 0;
    #else
    return 0;
    #endif
}


uint64_t OnboardStorage::freeBytes() const
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(!sdAvailable) return 0;

    const uint64_t total = storageSd.totalBytes();
    const uint64_t used = storageSd.usedBytes();
    return total > used ? total - used : 0;
    #else
    return 0;
    #endif
}


#if defined(OPENDRIFT_USB_MAINTENANCE) && defined(OPENDRIFT_BOARD_AMOLED_164)
bool OnboardStorage::beginUsbMassStorage()
{
    if(
        !sdAvailable ||
        storageSd.numSectors() == 0 ||
        storageSd.sectorSize() == 0
    )
    {
        return false;
    }

    if(usbReadTaskHandle == nullptr)
    {
        if(usbReadComplete == nullptr)
        {
            usbReadComplete = xSemaphoreCreateBinary();
            if(usbReadComplete == nullptr) return false;
        }
        const BaseType_t taskCreated = xTaskCreatePinnedToCore(
            readWorkerTask,
            "usb-sd-read",
            4096,
            this,
            1,
            &usbReadTaskHandle,
            ARDUINO_RUNNING_CORE
        );
        if(taskCreated != pdPASS)
        {
            usbReadTaskHandle = nullptr;
            return false;
        }
    }

    activeInstance = this;
    ejected = false;

    usbDisk.vendorID("OpenDrft");
    usbDisk.productID("Blackbox SD");
    usbDisk.productRevision("1.0");
    usbDisk.onRead(readBlocks);
    usbDisk.onWrite(rejectWrites);
    usbDisk.onStartStop(handleStartStop);

    const bool started = usbDisk.begin(
        storageSd.numSectors(),
        storageSd.sectorSize()
    );

    usbDisk.mediaPresent(started);
    return started;
}


bool OnboardStorage::wasEjected() const
{
    return ejected;
}


OnboardStorage::UsbReadDiagnostics
OnboardStorage::getUsbReadDiagnostics() const
{
    UsbReadDiagnostics diagnostics;
    diagnostics.reads = usbReadCount;
    diagnostics.failures = usbReadFailures;
    diagnostics.bytes = usbReadBytes;
    diagnostics.lastDurationUs = usbReadLastDurationUs;
    diagnostics.maxDurationUs = usbReadMaxDurationUs;
    diagnostics.lastLba = usbReadLastLba;
    diagnostics.lastOffset = usbReadLastOffset;
    diagnostics.lastSize = usbReadLastSize;
    diagnostics.lastReadAtMs = usbReadLastAtMs;
    diagnostics.lastError = usbReadLastError;
    diagnostics.callbacks = usbReadCallbackCount;
    diagnostics.busyReturns = usbReadBusyReturns;
    diagnostics.requestMismatches = usbReadRequestMismatches;
    diagnostics.invalidRequests = usbReadInvalidRequests;
    diagnostics.lastCallbackWaitUs = usbReadLastCallbackWaitUs;
    diagnostics.maxCallbackWaitUs = usbReadMaxCallbackWaitUs;
    diagnostics.lastCallbackAtMs = usbReadLastCallbackAtMs;
    diagnostics.workerState = static_cast<uint8_t>(usbReadState);
    return diagnostics;
}


void OnboardStorage::endUsbMassStorage()
{
    usbDisk.mediaPresent(false);
    delay(100);
    usbDisk.end();
    ejected = true;
}


int32_t OnboardStorage::readBlocks(
    uint32_t lba,
    uint32_t offset,
    void* buffer,
    uint32_t bufferSize
)
{
    OnboardStorage* instance = activeInstance;
    if(instance != nullptr)
    {
        instance->usbReadCallbackCount++;
        instance->usbReadLastCallbackAtMs = millis();
    }

    if(
        instance == nullptr ||
        buffer == nullptr ||
        bufferSize == 0 ||
        bufferSize > sizeof(instance->usbReadWorkerBuffer)
    )
    {
        if(instance != nullptr) instance->usbReadInvalidRequests++;
        return -1;
    }

    // Fill the existing TinyUSB-sized buffer in one worker request rather
    // than forcing eight 512-byte callbacks for each 4 KB host read. The
    // worker still uses CMD17 per sector; batching does not enable CMD18.
    const uint32_t readSize = min(bufferSize, USB_SD_READ_CHUNK_SIZE);

    bool startRead = false;
    bool requestMismatch = false;

    portENTER_CRITICAL(&instance->usbReadMux);
    const bool sameRequest =
        instance->usbReadRequestLba == lba &&
        instance->usbReadRequestOffset == offset &&
        instance->usbReadRequestSize == readSize;

    if(instance->usbReadState == USB_READ_IDLE)
    {
        instance->usbReadRequestLba = lba;
        instance->usbReadRequestOffset = offset;
        instance->usbReadRequestSize = readSize;
        instance->usbReadState = USB_READ_REQUESTED;
        startRead = true;
    }
    else if(!sameRequest)
    {
        requestMismatch = true;
    }
    portEXIT_CRITICAL(&instance->usbReadMux);

    if(startRead)
    {
        // A completion token is only a wakeup hint: the protected state above
        // remains authoritative, including after a slow-read/busy retry.
        xSemaphoreTake(instance->usbReadComplete, 0);
        xTaskNotifyGive(instance->usbReadTaskHandle);
    }

    if(requestMismatch)
    {
        instance->usbReadRequestMismatches++;
        return -1;
    }

    const uint32_t waitStartedUs = micros();
    for(;;)
    {
        bool deliverRead = false;
        int32_t result = 0;

        portENTER_CRITICAL(&instance->usbReadMux);
        if(instance->usbReadState == USB_READ_READY)
        {
            instance->usbReadState = USB_READ_DELIVERING;
            result = instance->usbReadWorkerResult;
            deliverRead = true;
        }
        portEXIT_CRITICAL(&instance->usbReadMux);

        if(deliverRead)
        {
            const uint32_t waitUs = micros() - waitStartedUs;
            instance->usbReadLastCallbackWaitUs = waitUs;
            if(waitUs > instance->usbReadMaxCallbackWaitUs)
            {
                instance->usbReadMaxCallbackWaitUs = waitUs;
            }

            if(result > 0)
            {
                memcpy(buffer, instance->usbReadWorkerBuffer, result);
            }

            portENTER_CRITICAL(&instance->usbReadMux);
            instance->usbReadState = USB_READ_IDLE;
            portEXIT_CRITICAL(&instance->usbReadMux);
            return result;
        }

        // Wait cooperatively so a normal SD transaction completes as one USB
        // callback. If a card operation is unusually slow, return busy after
        // a bounded interval and let TinyUSB retry the same request later.
        const uint32_t waitUs = micros() - waitStartedUs;
        if(waitUs >= 250000UL)
        {
            instance->usbReadBusyReturns++;
            instance->usbReadLastCallbackWaitUs = waitUs;
            if(waitUs > instance->usbReadMaxCallbackWaitUs)
            {
                instance->usbReadMaxCallbackWaitUs = waitUs;
            }
            return 0;
        }
        const uint32_t remainingUs = 250000UL - waitUs;
        const TickType_t waitTicks = pdMS_TO_TICKS(
            (remainingUs + 999UL) / 1000UL
        );
        // Block until the worker signals completion, not until the next
        // millisecond polling tick. Never hold the state mutex while waiting.
        xSemaphoreTake(instance->usbReadComplete, waitTicks);
    }
}


void OnboardStorage::readWorkerTask(void* context)
{
    OnboardStorage* instance = static_cast<OnboardStorage*>(context);
    for(;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint32_t lba = 0;
        uint32_t offset = 0;
        uint32_t size = 0;
        bool run = false;

        portENTER_CRITICAL(&instance->usbReadMux);
        if(instance->usbReadState == USB_READ_REQUESTED)
        {
            instance->usbReadState = USB_READ_RUNNING;
            lba = instance->usbReadRequestLba;
            offset = instance->usbReadRequestOffset;
            size = instance->usbReadRequestSize;
            run = true;
        }
        portEXIT_CRITICAL(&instance->usbReadMux);

        if(!run) continue;

        const int32_t result = instance->performRead(
            lba,
            offset,
            instance->usbReadWorkerBuffer,
            size
        );

        portENTER_CRITICAL(&instance->usbReadMux);
        instance->usbReadWorkerResult = result;
        instance->usbReadState = USB_READ_READY;
        portEXIT_CRITICAL(&instance->usbReadMux);
        xSemaphoreGive(instance->usbReadComplete);
    }
}


int32_t OnboardStorage::performRead(
    uint32_t lba,
    uint32_t offset,
    void* buffer,
    uint32_t bufferSize
)
{
    OnboardStorage* instance = this;

    const uint32_t startedUs = micros();
    instance->usbReadCount++;
    instance->usbReadLastLba = lba;
    instance->usbReadLastOffset = offset;
    instance->usbReadLastSize = bufferSize;

    const auto finish = [instance, startedUs](
        int32_t result,
        uint8_t error
    ) -> int32_t
    {
        const uint32_t duration = micros() - startedUs;
        instance->usbReadLastDurationUs = duration;
        instance->usbReadLastAtMs = millis();
        if(duration > instance->usbReadMaxDurationUs)
        {
            instance->usbReadMaxDurationUs = duration;
        }
        instance->usbReadLastError = error;
        if(result < 0)
        {
            instance->usbReadFailures++;
        }
        else
        {
            instance->usbReadBytes += static_cast<uint32_t>(result);
        }
        return result;
    };

    if(!instance->sdAvailable || buffer == nullptr)
    {
        return finish(-1, 1);
    }

    const uint32_t sectorSize = storageSd.sectorSize();
    static uint8_t partialSector[512];

    if(sectorSize != sizeof(partialSector))
    {
        return finish(-1, 2);
    }

    // Normalize the callback's block/byte address. TinyUSB revisions may
    // advance lba or offset between chunks; both forms map to the same bytes.
    const uint64_t firstByte =
        static_cast<uint64_t>(lba) * sectorSize + offset;
    const uint64_t mediaBytes =
        static_cast<uint64_t>(storageSd.numSectors()) * sectorSize;

    if(firstByte >= mediaBytes || bufferSize > mediaBytes - firstByte)
    {
        return finish(-1, 3);
    }

    uint8_t* destination = static_cast<uint8_t*>(buffer);
    uint32_t remaining = bufferSize;
    uint32_t currentLba = static_cast<uint32_t>(firstByte / sectorSize);
    uint32_t currentOffset = static_cast<uint32_t>(firstByte % sectorSize);
    const uint32_t sectorCount = storageSd.numSectors();

    while(remaining > 0)
    {
        if(currentLba >= sectorCount)
        {
            return finish(-1, 4);
        }

        // Keep the known-working single-block SD path even when the USB
        // request spans multiple sectors. CMD18 previously caused stalls.
        if(currentOffset == 0 && remaining >= sectorSize)
        {
            const uint32_t sectorsToRead = 1;
            if(!storageSd.readRAW(destination, currentLba, sectorsToRead))
            {
                return finish(-1, 5);
            }

            const uint32_t bytesRead = sectorsToRead * sectorSize;
            destination += bytesRead;
            remaining -= bytesRead;
            currentLba += sectorsToRead;
            continue;
        }

        if(!storageSd.readRAW(partialSector, currentLba, 1))
        {
            return finish(-1, 6);
        }

        const uint32_t amount = min(
            remaining,
            sectorSize - currentOffset
        );

        memcpy(destination, partialSector + currentOffset, amount);
        destination += amount;
        remaining -= amount;
        currentLba++;
        currentOffset = 0;
    }

    // Sustained host scans can otherwise keep this worker continuously ready.
    // Yield periodically so the same-core loop and idle tasks remain healthy.
    if((instance->usbReadCount & 0x1FU) == 0)
    {
        delay(1);
    }

    return finish(static_cast<int32_t>(bufferSize), 0);
}


int32_t OnboardStorage::rejectWrites(
    uint32_t,
    uint32_t,
    uint8_t*,
    uint32_t bufferSize
)
{
    (void)bufferSize;
    return -1;
}


bool OnboardStorage::handleStartStop(
    uint8_t,
    bool start,
    bool loadEject
)
{
    if(activeInstance != nullptr && loadEject && !start)
    {
        activeInstance->ejected = true;
        activeInstance->usbDisk.mediaPresent(false);
    }

    return true;
}
#endif
