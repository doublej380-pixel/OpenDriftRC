#include "ParameterStore.h"

#include <math.h>
#include <string.h>


ParameterStore::ParameterStore()
{
    resetDefaults();
}


void ParameterStore::resetDefaults()
{
    portENTER_CRITICAL(&mux);
    memset(values, 0, sizeof(values));

    const OpenDriftParameters::Definition* definitions =
        OpenDriftParameters::definitions();

    for(size_t index = 0; index < OpenDriftParameters::definitionCount(); index++)
    {
        const OpenDriftParameters::Definition& definition = definitions[index];
        values[(uint8_t)definition.id] = definition.defaultValue;
    }

    dirtyValues = false;
    valueGeneration++;
    portEXIT_CRITICAL(&mux);
}


float ParameterStore::get(OpenDriftParameters::Id id) const
{
    uint8_t index = (uint8_t)id;
    if(index >= VALUE_COUNT) return 0.0f;

    portENTER_CRITICAL(&mux);
    float value = values[index];
    portEXIT_CRITICAL(&mux);
    return value;
}


int ParameterStore::getInt(OpenDriftParameters::Id id) const
{
    return lroundf(get(id));
}


bool ParameterStore::getBool(OpenDriftParameters::Id id) const
{
    return get(id) >= 0.5f;
}


bool ParameterStore::set(OpenDriftParameters::Id id, float value)
{
    const OpenDriftParameters::Definition* definition =
        OpenDriftParameters::find(id);

    uint8_t index = (uint8_t)id;
    if(definition == nullptr || index >= VALUE_COUNT) return false;

    value = OpenDriftParameters::clamp(id, value);

    portENTER_CRITICAL(&mux);
    bool changed = fabsf(values[index] - value) > 0.00001f;
    if(changed)
    {
        values[index] = value;
        dirtyValues = true;
        valueGeneration++;
    }
    portEXIT_CRITICAL(&mux);
    return changed;
}


void ParameterStore::setInitial(OpenDriftParameters::Id id, float value)
{
    const OpenDriftParameters::Definition* definition =
        OpenDriftParameters::find(id);

    uint8_t index = (uint8_t)id;
    if(definition == nullptr || index >= VALUE_COUNT) return;

    value = OpenDriftParameters::clamp(id, value);

    portENTER_CRITICAL(&mux);
    values[index] = value;
    portEXIT_CRITICAL(&mux);
}


uint32_t ParameterStore::generation() const
{
    portENTER_CRITICAL(&mux);
    uint32_t current = valueGeneration;
    portEXIT_CRITICAL(&mux);
    return current;
}


bool ParameterStore::snapshot(
    uint32_t& lastGeneration,
    float* destination,
    size_t count
) const
{
    if(destination == nullptr) return false;
    if(count > VALUE_COUNT) count = VALUE_COUNT;

    portENTER_CRITICAL(&mux);
    if(lastGeneration == valueGeneration)
    {
        portEXIT_CRITICAL(&mux);
        return false;
    }

    memcpy(destination, values, count * sizeof(float));
    lastGeneration = valueGeneration;
    portEXIT_CRITICAL(&mux);
    return true;
}


bool ParameterStore::hasDirtyValues() const
{
    portENTER_CRITICAL(&mux);
    bool current = dirtyValues;
    portEXIT_CRITICAL(&mux);
    return current;
}


void ParameterStore::clearDirty()
{
    portENTER_CRITICAL(&mux);
    dirtyValues = false;
    portEXIT_CRITICAL(&mux);
}


bool ParameterStore::load(Preferences& preferences)
{
    const size_t storedSize = preferences.getBytesLength(PERSIST_KEY);
    if(storedSize < offsetof(PersistedValues, values)) return false;

    PersistedValues stored = {};
    size_t readSize = storedSize < sizeof(stored) ? storedSize : sizeof(stored);

    if(preferences.getBytes(PERSIST_KEY, &stored, readSize) != readSize)
    {
        return false;
    }

    if(stored.magic != PERSIST_MAGIC || stored.version != PERSIST_VERSION)
    {
        return false;
    }

    resetDefaults();

    size_t availableValues =
        readSize > offsetof(PersistedValues, values)
        ? (readSize - offsetof(PersistedValues, values)) / sizeof(float)
        : 0;

    size_t copyCount = stored.count;
    if(copyCount > availableValues) copyCount = availableValues;
    if(copyCount > VALUE_COUNT) copyCount = VALUE_COUNT;

    const OpenDriftParameters::Definition* definitions =
        OpenDriftParameters::definitions();

    for(size_t index = 0; index < OpenDriftParameters::definitionCount(); index++)
    {
        const OpenDriftParameters::Definition& definition = definitions[index];
        uint8_t id = (uint8_t)definition.id;

        if(id < copyCount && (definition.flags & OpenDriftParameters::PERSISTENT) != 0)
        {
            setInitial(definition.id, stored.values[id]);
        }
    }

    clearDirty();
    return true;
}


bool ParameterStore::save(Preferences& preferences)
{
    PersistedValues stored = {};
    stored.magic = PERSIST_MAGIC;
    stored.version = PERSIST_VERSION;
    stored.count = VALUE_COUNT;

    portENTER_CRITICAL(&mux);
    memcpy(stored.values, values, sizeof(values));
    portEXIT_CRITICAL(&mux);

    bool saved =
        preferences.putBytes(PERSIST_KEY, &stored, sizeof(stored)) == sizeof(stored);

    if(saved) clearDirty();
    return saved;
}


void ParameterStore::captureProfile(float* destination, size_t count) const
{
    if(destination == nullptr) return;

    for(size_t index = 0; index < count; index++) destination[index] = 0.0f;

    const OpenDriftParameters::Definition* definitions =
        OpenDriftParameters::definitions();

    portENTER_CRITICAL(&mux);
    for(size_t index = 0; index < OpenDriftParameters::definitionCount(); index++)
    {
        const OpenDriftParameters::Definition& definition = definitions[index];
        uint8_t id = (uint8_t)definition.id;
        if(id < count && (definition.flags & OpenDriftParameters::PROFILE) != 0)
        {
            destination[id] = values[id];
        }
    }
    portEXIT_CRITICAL(&mux);
}


bool ParameterStore::applyProfile(const float* source, size_t count)
{
    if(source == nullptr) return false;
    if(count > VALUE_COUNT) count = VALUE_COUNT;
    bool changed = false;

    const OpenDriftParameters::Definition* definitions =
        OpenDriftParameters::definitions();

    portENTER_CRITICAL(&mux);
    for(size_t index = 0; index < OpenDriftParameters::definitionCount(); index++)
    {
        const OpenDriftParameters::Definition& definition = definitions[index];
        uint8_t id = (uint8_t)definition.id;
        if(id < count && (definition.flags & OpenDriftParameters::PROFILE) != 0)
        {
            float value = source[id];
            if(value < definition.minimum) value = definition.minimum;
            if(value > definition.maximum) value = definition.maximum;

            if(fabsf(values[id] - value) > 0.00001f)
            {
                values[id] = value;
                changed = true;
            }
        }
    }

    if(changed)
    {
        dirtyValues = true;
        valueGeneration++;
    }
    portEXIT_CRITICAL(&mux);

    return changed;
}


void ParameterStore::clampProfile(float* profileValues, size_t count) const
{
    if(profileValues == nullptr) return;

    const OpenDriftParameters::Definition* definitions =
        OpenDriftParameters::definitions();

    for(size_t index = 0; index < OpenDriftParameters::definitionCount(); index++)
    {
        const OpenDriftParameters::Definition& definition = definitions[index];
        uint8_t id = (uint8_t)definition.id;
        if(id < count && (definition.flags & OpenDriftParameters::PROFILE) != 0)
        {
            profileValues[id] = OpenDriftParameters::clamp(
                definition.id,
                profileValues[id]
            );
        }
    }
}
