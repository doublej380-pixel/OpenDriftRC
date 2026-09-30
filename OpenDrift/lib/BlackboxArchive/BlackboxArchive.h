#pragma once

#include <Arduino.h>
#include <FS.h>

#include "BlackboxLogger.h"


class BlackboxArchive
{
public:
    enum Status : uint8_t
    {
        PARK_CAR = 0,
        READY = 1,
        SAVING = 2,
        SAVED = 3,
        NO_LOG = 4,
        NO_SPACE = 5,
        FAILED = 6,
        CANCELLED = 7,
        UNAVAILABLE = 8
    };

    bool begin(
        BlackboxLogger& logger,
        fs::FS* preferredStorage = nullptr,
        const char* preferredStorageName = nullptr,
        uint64_t preferredFreeBytes = 0
    );
    void update(bool parked);
    bool requestSave(bool allowNoSignalBench = false);

    Status getStatus() const;
    uint8_t getProgress() const;
    bool isSaving() const;
    bool hasArchive() const;
    size_t getArchiveBytes() const;
    size_t getArchiveRecordCount() const;
    uint32_t getArchiveDurationMs() const;
    size_t getRamRecordCount() const;
    size_t getRamBytes() const;
    size_t getRamCapacityBytes() const;
    uint32_t getRamDurationMs() const;
    bool clearRamLog();
    const char* getStorageName() const;
    bool isCsvArchive() const;
    bool clearArchive();

    bool openArchive(
        File& file,
        size_t& recordCount,
        size_t& recordSize
    ) const;
    bool openCsvArchive(File& file) const;

private:
    struct Header
    {
        uint32_t magic;
        uint16_t version;
        uint16_t headerSize;
        uint32_t recordSize;
        uint32_t recordCount;
        uint32_t payloadBytes;
        uint32_t checksum;
        uint32_t durationMs;
    };

    static constexpr uint32_t MAGIC = 0x4F444242UL; // ODBB
    static constexpr uint16_t VERSION = 1;
    static constexpr const char* BINARY_ARCHIVE_PATH = "/blackbox.odbb";
    static constexpr const char* BINARY_TEMP_PATH = "/blackbox.tmp";
    static constexpr const char* BINARY_BACKUP_PATH = "/blackbox.bak";
    static constexpr const char* CSV_LEGACY_ARCHIVE_PATH = "/opendrift-blackbox.csv";
    static constexpr const char* CSV_TEMP_PATH = "/opendrift-blackbox.tmp";
    static constexpr size_t CSV_PATH_BYTES = 48;
    static constexpr size_t WRITE_BUFFER_BYTES = 8192;
    static constexpr size_t FREE_SPACE_MARGIN = 65536;

    BlackboxLogger* logger = nullptr;
    fs::FS* storage = nullptr;
    const char* storageLabel = "internal flash";
    bool csvOutput = false;
    bool allowUnconfirmedParkedDuringSave = false;
    uint64_t freeStorageBytes = 0;
    File output;
    Status state = UNAVAILABLE;
    bool parkedNow = false;
    bool archivePresent = false;
    size_t snapshotRecords = 0;
    size_t snapshotRecordSize = 0;
    size_t archiveIndex = 0;
    size_t archiveBytes = 0;
    size_t archivedRecords = 0;
    uint32_t archivedDurationMs = 0;
    uint32_t checksum = 2166136261UL;
    uint32_t resultShownAtMs = 0;
    uint8_t writeBuffer[WRITE_BUFFER_BYTES] = {};
    char currentCsvPath[CSV_PATH_BYTES] = {};
    char pendingCsvPath[CSV_PATH_BYTES] = {};
    uint32_t nextCsvIndex = 1;

    const char* archivePath() const;
    const char* tempPath() const;
    const char* backupPath() const;
    void findLatestCsvArchive();
    bool prepareNextCsvArchivePath();

    bool startSave();
    void writeNextChunk();
    void finishSave();
    void abortSave(Status reason);
    bool validateArchive(
        const char* path,
        Header* header = nullptr
    ) const;
    void recoverFiles();
    static uint32_t updateChecksum(
        uint32_t value,
        const uint8_t* data,
        size_t length
    );
};
