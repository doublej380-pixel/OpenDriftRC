#include "OnboardStorage.h"

#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include <SD.h>

static constexpr int SD_CS_PIN = 38;
static constexpr int SD_MOSI_PIN = 39;
static constexpr int SD_MISO_PIN = 40;
static constexpr int SD_CLOCK_PIN = 41;
static constexpr uint32_t SD_FREQUENCY_HZ = 10000000;

#if defined(OPENDRIFT_USB_MAINTENANCE)
OnboardStorage* OnboardStorage::activeInstance = nullptr;

// OnboardStorage is constructed before FirmwareMSC, making the SD card LUN 0
// and the writable firmware updater LUN 1. TinyUSB uses this callback for the
// SCSI write-protect bit reported to the host.
extern "C" bool tud_msc_is_writable_cb(uint8_t lun)
{
    return lun != 0;
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

    sdAvailable = SD.begin(
        SD_CS_PIN,
        sdSpi,
        SD_FREQUENCY_HZ,
        "/sd",
        5,
        false
    );

    if(!sdAvailable || SD.cardType() == CARD_NONE)
    {
        SD.end();
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
    return sdAvailable ? static_cast<fs::FS*>(&SD) : nullptr;
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
    return sdAvailable ? SD.cardSize() : 0;
    #else
    return 0;
    #endif
}


uint64_t OnboardStorage::freeBytes() const
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(!sdAvailable) return 0;

    const uint64_t total = SD.totalBytes();
    const uint64_t used = SD.usedBytes();
    return total > used ? total - used : 0;
    #else
    return 0;
    #endif
}


#if defined(OPENDRIFT_USB_MAINTENANCE) && defined(OPENDRIFT_BOARD_AMOLED_164)
bool OnboardStorage::beginUsbMassStorage()
{
    if(!sdAvailable || SD.numSectors() == 0 || SD.sectorSize() == 0)
    {
        return false;
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
        SD.numSectors(),
        SD.sectorSize()
    );

    usbDisk.mediaPresent(started);
    return started;
}


bool OnboardStorage::wasEjected() const
{
    return ejected;
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
    if(
        activeInstance == nullptr ||
        !activeInstance->sdAvailable ||
        buffer == nullptr
    )
    {
        return -1;
    }

    const uint32_t sectorSize = SD.sectorSize();
    static uint8_t partialSector[512];

    if(sectorSize != sizeof(partialSector) || offset >= sectorSize)
    {
        return -1;
    }

    uint8_t* destination = static_cast<uint8_t*>(buffer);
    uint32_t remaining = bufferSize;
    uint32_t currentLba = lba;
    uint32_t currentOffset = offset;
    const uint32_t sectorCount = SD.numSectors();

    while(remaining > 0)
    {
        if(currentLba >= sectorCount)
        {
            return -1;
        }

        // TinyUSB normally requests complete aligned sectors. Read those
        // directly into its transfer buffer to keep the USB task stack small.
        if(currentOffset == 0 && remaining >= sectorSize)
        {
            if(!SD.readRAW(destination, currentLba))
            {
                return -1;
            }

            destination += sectorSize;
            remaining -= sectorSize;
            currentLba++;
            continue;
        }

        if(!SD.readRAW(partialSector, currentLba))
        {
            return -1;
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

    return bufferSize;
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
