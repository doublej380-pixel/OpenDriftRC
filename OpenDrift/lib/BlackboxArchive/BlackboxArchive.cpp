#include "BlackboxArchive.h"

#include <FFat.h>


bool BlackboxArchive::begin(BlackboxLogger& activeLogger)
{
    logger = &activeLogger;

    if(FFat.totalBytes() == 0)
    {
        state = UNAVAILABLE;
        return false;
    }

    recoverFiles();
    Header header = {};
    archivePresent = validateArchive(ARCHIVE_PATH, &header);

    if(archivePresent)
    {
        archiveBytes = sizeof(Header) + header.payloadBytes;
        archivedRecords = header.recordCount;
    }

    state = archivePresent ? SAVED : NO_LOG;
    return true;
}


void BlackboxArchive::update(bool parked)
{
    parkedNow = parked;

    if(state == SAVING)
    {
        if(!parkedNow)
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


bool BlackboxArchive::requestSave()
{
    if(state == SAVING || logger == nullptr || !logger->isReady())
    {
        return false;
    }

    if(!parkedNow)
    {
        state = PARK_CAR;
        return false;
    }

    if(logger->getRecordCount() == 0)
    {
        state = NO_LOG;
        return false;
    }

    return startSave();
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


bool BlackboxArchive::clearArchive()
{
    if(state == SAVING) return false;

    bool ok = true;
    if(FFat.exists(ARCHIVE_PATH)) ok = FFat.remove(ARCHIVE_PATH);
    if(FFat.exists(BACKUP_PATH)) FFat.remove(BACKUP_PATH);
    if(FFat.exists(TEMP_PATH)) FFat.remove(TEMP_PATH);

    archivePresent = false;
    archiveBytes = 0;
    archivedRecords = 0;
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
    if(!validateArchive(ARCHIVE_PATH, &header)) return false;

    file = FFat.open(ARCHIVE_PATH, FILE_READ);
    if(!file || !file.seek(sizeof(Header)))
    {
        if(file) file.close();
        return false;
    }

    recordCount = header.recordCount;
    recordSize = header.recordSize;
    return true;
}


bool BlackboxArchive::startSave()
{
    snapshotRecords = logger->getRecordCount();
    snapshotRecordSize = logger->getBinaryRecordSize();

    const size_t required =
        sizeof(Header) +
        snapshotRecords * snapshotRecordSize +
        FREE_SPACE_MARGIN;

    if(FFat.freeBytes() < required)
    {
        state = NO_SPACE;
        resultShownAtMs = millis();
        return false;
    }

    logger->setPaused(true);

    if(FFat.exists(TEMP_PATH)) FFat.remove(TEMP_PATH);
    output = FFat.open(TEMP_PATH, FILE_WRITE);

    if(!output)
    {
        logger->setPaused(false);
        state = FAILED;
        resultShownAtMs = millis();
        return false;
    }

    Header placeholder = {};
    if(output.write(
        reinterpret_cast<const uint8_t*>(&placeholder),
        sizeof(placeholder)
    ) != sizeof(placeholder))
    {
        abortSave(FAILED);
        return false;
    }

    archiveIndex = 0;
    checksum = 2166136261UL;
    state = SAVING;
    return true;
}


void BlackboxArchive::writeNextChunk()
{
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

    bool ok =
        output.seek(0) &&
        output.write(
            reinterpret_cast<const uint8_t*>(&header),
            sizeof(header)
        ) == sizeof(header);

    output.flush();
    output.close();

    if(ok && validateArchive(TEMP_PATH))
    {
        if(FFat.exists(BACKUP_PATH)) FFat.remove(BACKUP_PATH);

        if(FFat.exists(ARCHIVE_PATH))
        {
            ok = FFat.rename(ARCHIVE_PATH, BACKUP_PATH);
        }

        if(ok)
        {
            ok = FFat.rename(TEMP_PATH, ARCHIVE_PATH);
        }

        if(ok)
        {
            if(FFat.exists(BACKUP_PATH)) FFat.remove(BACKUP_PATH);
        }
        else if(FFat.exists(BACKUP_PATH) && !FFat.exists(ARCHIVE_PATH))
        {
            FFat.rename(BACKUP_PATH, ARCHIVE_PATH);
        }
    }

    logger->setPaused(false);

    if(!ok)
    {
        if(FFat.exists(TEMP_PATH)) FFat.remove(TEMP_PATH);
        state = FAILED;
        resultShownAtMs = millis();
        return;
    }

    archivePresent = true;
    archiveBytes = sizeof(Header) + header.payloadBytes;
    archivedRecords = header.recordCount;
    state = SAVED;
    resultShownAtMs = millis();
}


void BlackboxArchive::abortSave(Status reason)
{
    if(output) output.close();
    if(FFat.exists(TEMP_PATH)) FFat.remove(TEMP_PATH);
    if(logger != nullptr) logger->setPaused(false);
    state = reason;
    resultShownAtMs = millis();
}


bool BlackboxArchive::validateArchive(
    const char* path,
    Header* returnedHeader
) const
{
    if(logger == nullptr || !FFat.exists(path)) return false;

    File file = FFat.open(path, FILE_READ);
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
    if(FFat.exists(TEMP_PATH)) FFat.remove(TEMP_PATH);

    if(FFat.exists(BACKUP_PATH))
    {
        if(validateArchive(ARCHIVE_PATH)) FFat.remove(BACKUP_PATH);
        else
        {
            if(FFat.exists(ARCHIVE_PATH)) FFat.remove(ARCHIVE_PATH);
            FFat.rename(BACKUP_PATH, ARCHIVE_PATH);
        }
    }
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
