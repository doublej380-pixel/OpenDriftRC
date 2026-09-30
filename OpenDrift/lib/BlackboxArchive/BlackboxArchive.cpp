#include "BlackboxArchive.h"

#include <FFat.h>


bool BlackboxArchive::begin(
    BlackboxLogger& activeLogger,
    fs::FS* preferredStorage,
    const char* preferredStorageName,
    uint64_t preferredFreeBytes
)
{
    logger = &activeLogger;

    storage = preferredStorage;
    csvOutput = storage != nullptr;
    freeStorageBytes = preferredFreeBytes;
    storageLabel = preferredStorageName != nullptr
        ? preferredStorageName
        : "internal flash";

    if(storage == nullptr)
    {
        storage = &FFat;
        csvOutput = false;
        storageLabel = "internal flash";
        freeStorageBytes = FFat.freeBytes();
    }

    if(freeStorageBytes == 0)
    {
        state = UNAVAILABLE;
        return false;
    }

    if(csvOutput) findLatestCsvArchive();
    recoverFiles();
    Header header = {};
    archivePresent = csvOutput
        ? storage->exists(archivePath())
        : validateArchive(archivePath(), &header);

    if(archivePresent)
    {
        if(csvOutput)
        {
            File file = storage->open(archivePath(), FILE_READ);
            archiveBytes = file ? file.size() : 0;
            archivedRecords = 0;
            archivedDurationMs = 0;

            if(file)
            {
                bool headerRow = true;
                bool atLineStart = false;
                bool timestampValid = false;
                uint32_t timestamp = 0;
                uint32_t firstTimestamp = 0;
                uint32_t lastTimestamp = 0;

                while(file.available())
                {
                    const char value = (char)file.read();

                    if(headerRow)
                    {
                        if(value == '\n')
                        {
                            headerRow = false;
                            atLineStart = true;
                        }
                        continue;
                    }

                    if(atLineStart)
                    {
                        if(value >= '0' && value <= '9')
                        {
                            timestamp = timestamp * 10U + (uint32_t)(value - '0');
                            timestampValid = true;
                            continue;
                        }

                        if(value == ',' && timestampValid)
                        {
                            if(archivedRecords == 0) firstTimestamp = timestamp;
                            lastTimestamp = timestamp;
                        }

                        atLineStart = false;
                    }

                    if(value == '\n')
                    {
                        archivedRecords++;
                        atLineStart = true;
                        timestampValid = false;
                        timestamp = 0;
                    }
                }
                file.close();

                if(archivedRecords > 1)
                {
                    archivedDurationMs = lastTimestamp - firstTimestamp;
                }
            }
        }
        else
        {
            archiveBytes = sizeof(Header) + header.payloadBytes;
            archivedRecords = header.recordCount;
            archivedDurationMs = header.durationMs;
        }
    }

    state = archivePresent ? SAVED : NO_LOG;
    return true;
}


void BlackboxArchive::update(bool parked)
{
    parkedNow = parked;

    if(state == SAVING)
    {
        if(!parkedNow && !allowUnconfirmedParkedDuringSave)
        {
            abortSave(CANCELLED);
            return;
        }

        writeNextChunk();
        return;
    }

    if(
        (state == SAVED || state == FAILED || state == NO_SPACE || state == CANCELLED) &&
        resultShownAtMs != 0 &&
        millis() - resultShownAtMs > 4000
    )
    {
        resultShownAtMs = 0;
        state = logger != nullptr && logger->getRecordCount() > 0
            ? (parkedNow ? READY : PARK_CAR)
            : NO_LOG;
    }
}


bool BlackboxArchive::requestSave(bool allowNoSignalBench)
{
    if(state == SAVING || logger == nullptr || !logger->isReady())
    {
        return false;
    }

    if(!parkedNow && !allowNoSignalBench)
    {
        state = PARK_CAR;
        return false;
    }

    if(logger->getRecordCount() == 0)
    {
        state = NO_LOG;
        return false;
    }

    allowUnconfirmedParkedDuringSave = allowNoSignalBench;

    if(!startSave())
    {
        allowUnconfirmedParkedDuringSave = false;
        return false;
    }

    return true;
}


BlackboxArchive::Status BlackboxArchive::getStatus() const
{
    if(state == SAVING || state == SAVED || state == FAILED ||
       state == NO_SPACE || state == CANCELLED || state == UNAVAILABLE)
    {
        return state;
    }

    if(logger == nullptr || !logger->isReady() || logger->getRecordCount() == 0)
    {
        return NO_LOG;
    }

    return parkedNow ? READY : PARK_CAR;
}


uint8_t BlackboxArchive::getProgress() const
{
    if(state == SAVED) return 100;
    if(state != SAVING || snapshotRecords == 0) return 0;

    return (uint8_t)min(
        (size_t)99,
        (archiveIndex * 100U) / snapshotRecords
    );
}


bool BlackboxArchive::isSaving() const { return state == SAVING; }
bool BlackboxArchive::hasArchive() const { return archivePresent; }
size_t BlackboxArchive::getArchiveBytes() const { return archiveBytes; }
size_t BlackboxArchive::getArchiveRecordCount() const { return archivedRecords; }
uint32_t BlackboxArchive::getArchiveDurationMs() const { return archivedDurationMs; }
size_t BlackboxArchive::getRamRecordCount() const
{
    return logger != nullptr ? logger->getRecordCount() : 0;
}
size_t BlackboxArchive::getRamBytes() const
{
    return logger != nullptr ? logger->getSize() : 0;
}
size_t BlackboxArchive::getRamCapacityBytes() const
{
    return logger != nullptr ? logger->getCapacityBytes() : 0;
}
uint32_t BlackboxArchive::getRamDurationMs() const
{
    return logger != nullptr ? logger->getDurationMs() : 0;
}
bool BlackboxArchive::clearRamLog()
{
    if(logger == nullptr || isSaving()) return false;
    logger->clear();
    state = NO_LOG;
    return true;
}
const char* BlackboxArchive::getStorageName() const { return storageLabel; }
bool BlackboxArchive::isCsvArchive() const { return csvOutput; }


bool BlackboxArchive::clearArchive()
{
    if(state == SAVING) return false;

    bool ok = true;
    if(storage == nullptr) return false;
    if(storage->exists(archivePath())) ok = storage->remove(archivePath());
    if(storage->exists(backupPath())) storage->remove(backupPath());
    if(storage->exists(tempPath())) storage->remove(tempPath());

    if(csvOutput)
    {
        BlackboxLogger* activeLogger = logger;
        fs::FS* activeStorage = storage;
        const char* activeStorageLabel = storageLabel;
        const uint64_t activeFreeBytes = freeStorageBytes;
        return ok && activeLogger != nullptr && begin(
            *activeLogger,
            activeStorage,
            activeStorageLabel,
            activeFreeBytes
        );
    }

    archivePresent = false;
    archiveBytes = 0;
    archivedRecords = 0;
    archivedDurationMs = 0;
    state = logger != nullptr && logger->getRecordCount() > 0
        ? (parkedNow ? READY : PARK_CAR)
        : NO_LOG;
    return ok;
}


bool BlackboxArchive::openArchive(
    File& file,
    size_t& recordCount,
    size_t& recordSize
) const
{
    Header header = {};
    if(csvOutput || !validateArchive(archivePath(), &header)) return false;

    file = storage->open(archivePath(), FILE_READ);
    if(!file || !file.seek(sizeof(Header)))
    {
        if(file) file.close();
        return false;
    }

    recordCount = header.recordCount;
    recordSize = header.recordSize;
    return true;
}


bool BlackboxArchive::openCsvArchive(File& file) const
{
    if(!csvOutput || storage == nullptr || !storage->exists(archivePath()))
    {
        return false;
    }

    file = storage->open(archivePath(), FILE_READ);
    return file && !file.isDirectory();
}


bool BlackboxArchive::startSave()
{
    snapshotRecords = logger->getRecordCount();
    snapshotRecordSize = logger->getBinaryRecordSize();

    const size_t required = csvOutput
        ? snapshotRecords * 704UL + FREE_SPACE_MARGIN
        : sizeof(Header) + snapshotRecords * snapshotRecordSize + FREE_SPACE_MARGIN;

    if(storage == nullptr || freeStorageBytes < required)
    {
        state = NO_SPACE;
        resultShownAtMs = millis();
        return false;
    }

    logger->setPaused(true);

    if(csvOutput && !prepareNextCsvArchivePath())
    {
        logger->setPaused(false);
        state = FAILED;
        resultShownAtMs = millis();
        return false;
    }

    if(storage->exists(tempPath())) storage->remove(tempPath());
    output = storage->open(tempPath(), FILE_WRITE);

    if(!output)
    {
        logger->setPaused(false);
        state = FAILED;
        resultShownAtMs = millis();
        return false;
    }

    if(csvOutput)
    {
        const char* header = logger->getCsvHeader();
        if(
            output.print(header) != strlen(header) ||
            output.write((uint8_t)'\n') != 1
        )
        {
            abortSave(FAILED);
            return false;
        }
    }
    else
    {
        Header placeholder = {};
        if(output.write(
            reinterpret_cast<const uint8_t*>(&placeholder),
            sizeof(placeholder)
        ) != sizeof(placeholder))
        {
            abortSave(FAILED);
            return false;
        }
    }

    archiveIndex = 0;
    checksum = 2166136261UL;
    state = SAVING;
    return true;
}


void BlackboxArchive::writeNextChunk()
{
    if(csvOutput)
    {
        char line[704];
        const size_t remaining = snapshotRecords - archiveIndex;
        const size_t count = min((size_t)12, remaining);

        for(size_t index = 0; index < count; index++)
        {
            const size_t length = logger->formatCsvRecord(
                archiveIndex + index,
                line,
                sizeof(line)
            );

            if(length == 0 || output.write(
                reinterpret_cast<const uint8_t*>(line),
                length
            ) != length)
            {
                abortSave(FAILED);
                return;
            }
        }

        archiveIndex += count;
        if(archiveIndex >= snapshotRecords) finishSave();
        return;
    }

    const size_t recordsPerChunk =
        max((size_t)1, WRITE_BUFFER_BYTES / snapshotRecordSize);
    const size_t remaining = snapshotRecords - archiveIndex;
    const size_t count = min(recordsPerChunk, remaining);
    size_t bytes = 0;

    for(size_t index = 0; index < count; index++)
    {
        if(!logger->copyBinaryRecord(
            archiveIndex + index,
            writeBuffer + bytes,
            WRITE_BUFFER_BYTES - bytes
        ))
        {
            abortSave(FAILED);
            return;
        }

        bytes += snapshotRecordSize;
    }

    if(output.write(writeBuffer, bytes) != bytes)
    {
        abortSave(FAILED);
        return;
    }

    checksum = updateChecksum(checksum, writeBuffer, bytes);
    archiveIndex += count;

    if(archiveIndex >= snapshotRecords)
    {
        finishSave();
    }
}


void BlackboxArchive::finishSave()
{
    Header header = {
        MAGIC,
        VERSION,
        (uint16_t)sizeof(Header),
        (uint32_t)snapshotRecordSize,
        (uint32_t)snapshotRecords,
        (uint32_t)(snapshotRecords * snapshotRecordSize),
        checksum,
        (uint32_t)logger->getDurationMs()
    };

    bool ok = csvOutput || (
        output.seek(0) &&
        output.write(
            reinterpret_cast<const uint8_t*>(&header),
            sizeof(header)
        ) == sizeof(header)
    );

    output.flush();
    output.close();

    if(ok && csvOutput)
    {
        ok = pendingCsvPath[0] != '\0' &&
            !storage->exists(pendingCsvPath) &&
            storage->rename(tempPath(), pendingCsvPath);

        if(ok)
        {
            snprintf(
                currentCsvPath,
                sizeof(currentCsvPath),
                "%s",
                pendingCsvPath
            );
        }

        pendingCsvPath[0] = '\0';
    }
    else if(ok && validateArchive(tempPath()))
    {
        if(storage->exists(backupPath())) storage->remove(backupPath());

        if(storage->exists(archivePath()))
        {
            ok = storage->rename(archivePath(), backupPath());
        }

        if(ok)
        {
            ok = storage->rename(tempPath(), archivePath());
        }

        if(ok)
        {
            if(storage->exists(backupPath())) storage->remove(backupPath());
        }
        else if(storage->exists(backupPath()) && !storage->exists(archivePath()))
        {
            storage->rename(backupPath(), archivePath());
        }
    }

    logger->setPaused(false);

    if(!ok)
    {
        if(storage->exists(tempPath())) storage->remove(tempPath());
        pendingCsvPath[0] = '\0';
        allowUnconfirmedParkedDuringSave = false;
        state = FAILED;
        resultShownAtMs = millis();
        return;
    }

    archivePresent = true;
    if(csvOutput)
    {
        File completed = storage->open(archivePath(), FILE_READ);
        archiveBytes = completed ? completed.size() : 0;
        if(completed) completed.close();
    }
    else
    {
        archiveBytes = sizeof(Header) + header.payloadBytes;
    }
    archivedRecords = header.recordCount;
    archivedDurationMs = header.durationMs;
    allowUnconfirmedParkedDuringSave = false;
    state = SAVED;
    resultShownAtMs = millis();
}


void BlackboxArchive::abortSave(Status reason)
{
    if(output) output.close();
    if(storage != nullptr && storage->exists(tempPath())) storage->remove(tempPath());
    pendingCsvPath[0] = '\0';
    if(logger != nullptr) logger->setPaused(false);
    allowUnconfirmedParkedDuringSave = false;
    state = reason;
    resultShownAtMs = millis();
}


bool BlackboxArchive::validateArchive(
    const char* path,
    Header* returnedHeader
) const
{
    if(logger == nullptr || storage == nullptr || !storage->exists(path)) return false;

    File file = storage->open(path, FILE_READ);
    if(!file || file.isDirectory() || file.size() < sizeof(Header))
    {
        if(file) file.close();
        return false;
    }

    Header header = {};
    const bool readOk = file.read(
        reinterpret_cast<uint8_t*>(&header),
        sizeof(header)
    ) == sizeof(header);

    const uint64_t expectedPayloadBytes =
        (uint64_t)header.recordCount * header.recordSize;
    const uint64_t expectedFileBytes =
        (uint64_t)sizeof(Header) + expectedPayloadBytes;

    const bool valid =
        readOk &&
        header.magic == MAGIC &&
        header.version == VERSION &&
        header.headerSize == sizeof(Header) &&
        header.recordSize == logger->getBinaryRecordSize() &&
        expectedPayloadBytes <= UINT32_MAX &&
        header.payloadBytes == expectedPayloadBytes &&
        file.size() == expectedFileBytes;

    bool checksumValid = valid;

    if(valid)
    {
        uint8_t buffer[1024];
        size_t remaining = header.payloadBytes;
        uint32_t calculated = 2166136261UL;

        while(remaining > 0)
        {
            const size_t requested = min(remaining, sizeof(buffer));
            const size_t amount = file.read(buffer, requested);
            if(amount != requested)
            {
                checksumValid = false;
                break;
            }

            calculated = updateChecksum(calculated, buffer, amount);
            remaining -= amount;
        }

        checksumValid = checksumValid && calculated == header.checksum;
    }

    file.close();
    if(checksumValid && returnedHeader != nullptr) *returnedHeader = header;
    return checksumValid;
}


void BlackboxArchive::recoverFiles()
{
    if(storage == nullptr) return;
    if(storage->exists(tempPath())) storage->remove(tempPath());

    if(csvOutput) return;

    if(storage->exists(backupPath()))
    {
        const bool currentValid = csvOutput
            ? storage->exists(archivePath())
            : validateArchive(archivePath());

        if(currentValid) storage->remove(backupPath());
        else
        {
            if(storage->exists(archivePath())) storage->remove(archivePath());
            storage->rename(backupPath(), archivePath());
        }
    }
}


const char* BlackboxArchive::archivePath() const
{
    if(!csvOutput) return BINARY_ARCHIVE_PATH;
    return currentCsvPath[0] != '\0'
        ? currentCsvPath
        : CSV_LEGACY_ARCHIVE_PATH;
}


const char* BlackboxArchive::tempPath() const
{
    return csvOutput ? CSV_TEMP_PATH : BINARY_TEMP_PATH;
}


const char* BlackboxArchive::backupPath() const
{
    return BINARY_BACKUP_PATH;
}


void BlackboxArchive::findLatestCsvArchive()
{
    currentCsvPath[0] = '\0';
    pendingCsvPath[0] = '\0';
    nextCsvIndex = 1;

    if(storage == nullptr) return;

    uint32_t highestIndex = 0;
    File root = storage->open("/");

    if(root && root.isDirectory())
    {
        File file = root.openNextFile();
        while(file)
        {
            const char* fullName = file.name();
            const char* name = strrchr(fullName, '/');
            name = name != nullptr ? name + 1 : fullName;

            static constexpr const char* PREFIX = "opendrift-blackbox-";
            static constexpr const char* SUFFIX = ".csv";
            const size_t prefixLength = strlen(PREFIX);
            const size_t nameLength = strlen(name);
            const size_t suffixLength = strlen(SUFFIX);

            if(
                !file.isDirectory() &&
                nameLength > prefixLength + suffixLength &&
                strncmp(name, PREFIX, prefixLength) == 0 &&
                strcmp(name + nameLength - suffixLength, SUFFIX) == 0
            )
            {
                bool digitsOnly = true;
                uint32_t index = 0;
                for(
                    size_t position = prefixLength;
                    position < nameLength - suffixLength;
                    position++
                )
                {
                    if(name[position] < '0' || name[position] > '9')
                    {
                        digitsOnly = false;
                        break;
                    }
                    index = index * 10U + (uint32_t)(name[position] - '0');
                }

                if(digitsOnly && index > highestIndex)
                {
                    highestIndex = index;
                    snprintf(
                        currentCsvPath,
                        sizeof(currentCsvPath),
                        "/%s",
                        name
                    );
                }
            }

            file.close();
            file = root.openNextFile();
        }
        root.close();
    }

    if(highestIndex == 0 && storage->exists(CSV_LEGACY_ARCHIVE_PATH))
    {
        snprintf(
            currentCsvPath,
            sizeof(currentCsvPath),
            "%s",
            CSV_LEGACY_ARCHIVE_PATH
        );
    }

    nextCsvIndex = highestIndex + 1U;
}


bool BlackboxArchive::prepareNextCsvArchivePath()
{
    if(storage == nullptr) return false;

    for(uint32_t attempts = 0; attempts < 10000U; attempts++)
    {
        snprintf(
            pendingCsvPath,
            sizeof(pendingCsvPath),
            "/opendrift-blackbox-%04lu.csv",
            (unsigned long)nextCsvIndex++
        );

        if(!storage->exists(pendingCsvPath)) return true;
    }

    pendingCsvPath[0] = '\0';
    return false;
}


uint32_t BlackboxArchive::updateChecksum(
    uint32_t value,
    const uint8_t* data,
    size_t length
)
{
    for(size_t index = 0; index < length; index++)
    {
        value ^= data[index];
        value *= 16777619UL;
    }

    return value;
}
