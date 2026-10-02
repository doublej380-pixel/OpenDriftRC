#include "UsbMaintenance.h"
#include "BootRecovery.h"

#if defined(OPENDRIFT_USB_MAINTENANCE)
#include <esp_attr.h>
#include <esp_ota_ops.h>
#include <Preferences.h>
#include <tusb.h>

static constexpr uint32_t MAINTENANCE_BOOT_MAGIC = 0x4F445553UL;
RTC_NOINIT_ATTR static uint32_t maintenanceBootMagic;
RTC_NOINIT_ATTR static BootRecovery bootRecovery;

static volatile UsbMaintenance::UpdateState firmwareUpdateState =
    UsbMaintenance::UPDATE_IDLE;
static volatile size_t firmwareUpdateBytes = 0;
static volatile uint32_t firmwareUpdateRevision = 0;

static constexpr const char* UPDATE_NAMESPACE = "od-usb";
static constexpr const char* UPDATE_PENDING_KEY = "pending";
static constexpr const char* UPDATE_SOURCE_KEY = "source";


static void setUpdateMarker()
{
    const esp_partition_t* running = esp_ota_get_running_partition();
    if(running == nullptr) return;

    Preferences preferences;
    if(!preferences.begin(UPDATE_NAMESPACE, false)) return;
    preferences.putUInt(UPDATE_SOURCE_KEY, running->address);
    preferences.putBool(UPDATE_PENDING_KEY, true);
    preferences.end();
}


static void clearUpdateMarker()
{
    Preferences preferences;
    if(!preferences.begin(UPDATE_NAMESPACE, false)) return;
    preferences.remove(UPDATE_SOURCE_KEY);
    preferences.remove(UPDATE_PENDING_KEY);
    preferences.end();
}


static void handleFirmwareEvent(
    void*,
    esp_event_base_t,
    int32_t eventId,
    void* eventData
)
{
    const auto* data = static_cast<arduino_firmware_msc_event_data_t*>(
        eventData
    );

    switch(eventId)
    {
        case ARDUINO_FIRMWARE_MSC_START_EVENT:
            firmwareUpdateBytes = 0;
            firmwareUpdateState = UsbMaintenance::UPDATE_WRITING;
            break;

        case ARDUINO_FIRMWARE_MSC_WRITE_EVENT:
            if(data != nullptr)
            {
                const size_t writtenThrough =
                    data->write.offset + data->write.size;
                if(writtenThrough > firmwareUpdateBytes)
                {
                    firmwareUpdateBytes = writtenThrough;
                }
            }
            firmwareUpdateState = UsbMaintenance::UPDATE_WRITING;
            break;

        case ARDUINO_FIRMWARE_MSC_END_EVENT:
            if(data != nullptr) firmwareUpdateBytes = data->end.size;
            firmwareUpdateState = UsbMaintenance::UPDATE_COMPLETE;
            break;

        case ARDUINO_FIRMWARE_MSC_ERROR_EVENT:
            if(data != nullptr) firmwareUpdateBytes = data->error.size;
            firmwareUpdateState = UsbMaintenance::UPDATE_ERROR;
            break;

        default:
            return;
    }

    firmwareUpdateRevision++;
}
#endif


void UsbMaintenance::armNextBoot()
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    maintenanceBootMagic = MAINTENANCE_BOOT_MAGIC;
    bootRecovery.healthy(); // Intentional mode changes are not failed boots.
    #endif
}

bool UsbMaintenance::recordBootAttempt(bool coldBoot)
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    return bootRecovery.begin(coldBoot);
    #else
    return false;
    #endif
}

void UsbMaintenance::markBootHealthy()
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    bootRecovery.healthy();
    #endif
}


bool UsbMaintenance::consumeBootRequest()
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    const bool requested = maintenanceBootMagic == MAINTENANCE_BOOT_MAGIC;
    maintenanceBootMagic = 0;
    return requested;
    #else
    return false;
    #endif
}


bool UsbMaintenance::consumeCompletedUpdate()
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    Preferences preferences;
    if(!preferences.begin(UPDATE_NAMESPACE, false)) return false;

    const bool pending = preferences.getBool(UPDATE_PENDING_KEY, false);
    const uint32_t sourceAddress = preferences.getUInt(
        UPDATE_SOURCE_KEY,
        0
    );
    const esp_partition_t* running = esp_ota_get_running_partition();
    const bool completed =
        pending &&
        sourceAddress != 0 &&
        running != nullptr &&
        running->address != sourceAddress;

    preferences.remove(UPDATE_SOURCE_KEY);
    preferences.remove(UPDATE_PENDING_KEY);
    preferences.end();
    return completed;
    #else
    return false;
    #endif
}


bool UsbMaintenance::begin(OnboardStorage& storage)
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    firmwareUpdateState = UPDATE_IDLE;
    firmwareUpdateBytes = 0;
    firmwareUpdateRevision++;
    setUpdateMarker();
    firmwareDisk.onEvent(handleFirmwareEvent);
    firmwareReady = firmwareDisk.begin();
    sdReady = storage.isSdAvailable() && storage.beginUsbMassStorage();

    // Native USB is already enumerated before Arduino setup() runs. At that
    // point both MSC LUNs report no media, so desktop hosts back off to a slow
    // polling interval. Reconnect after the drives are ready so the host sees
    // their final state during enumeration instead of roughly 30 seconds later.
    tud_disconnect();
    delay(150);
    tud_connect();

    return firmwareReady || sdReady;
    #else
    (void)storage;
    return false;
    #endif
}


bool UsbMaintenance::firmwareDriveReady() const
{
    return firmwareReady;
}


void UsbMaintenance::prepareForRestart(OnboardStorage& storage)
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    if(firmwareUpdateState != UPDATE_COMPLETE) clearUpdateMarker();

    if(sdReady)
    {
        storage.endUsbMassStorage();
        sdReady = false;
    }

    if(firmwareReady)
    {
        firmwareDisk.end();
        firmwareReady = false;
    }

    // A USB peripheral cannot issue the host's unmount command. Both LUNs are
    // read-only/closed first, then a clean logical disconnect tells the host
    // that the media is gone before the ESP restarts.
    delay(150);
    tud_disconnect();
    delay(250);
    #else
    (void)storage;
    #endif
}


bool UsbMaintenance::sdDriveReady() const
{
    return sdReady;
}


UsbMaintenance::UpdateState UsbMaintenance::getUpdateState() const
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    return firmwareUpdateState;
    #else
    return UPDATE_IDLE;
    #endif
}


size_t UsbMaintenance::getUpdateBytes() const
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    return firmwareUpdateBytes;
    #else
    return 0;
    #endif
}


uint32_t UsbMaintenance::getUpdateRevision() const
{
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    return firmwareUpdateRevision;
    #else
    return 0;
    #endif
}
