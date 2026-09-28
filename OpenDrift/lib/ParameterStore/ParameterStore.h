#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "ParameterCatalog.h"


class ParameterStore
{
public:

    static constexpr size_t VALUE_COUNT =
        OpenDriftParameters::MAX_PUBLISHED_ID + 1;

    ParameterStore();

    void resetDefaults();

    float get(OpenDriftParameters::Id id) const;
    int getInt(OpenDriftParameters::Id id) const;
    bool getBool(OpenDriftParameters::Id id) const;

    bool set(OpenDriftParameters::Id id, float value);
    void setInitial(OpenDriftParameters::Id id, float value);

    uint32_t generation() const;
    bool snapshot(
        uint32_t& lastGeneration,
        float* destination,
        size_t count
    ) const;
    bool hasDirtyValues() const;
    void clearDirty();

    // New firmware stores the catalog-backed values as one versioned blob.
    // Existing per-setting Preferences keys are imported by Settings once.
    bool load(Preferences& preferences);
    bool save(Preferences& preferences);

    void captureProfile(float* destination, size_t count) const;
    bool applyProfile(const float* source, size_t count);
    void clampProfile(float* values, size_t count) const;

private:

    static constexpr uint32_t PERSIST_MAGIC = 0x4F445053; // "ODPS"
    static constexpr uint16_t PERSIST_VERSION = 1;
    static constexpr const char* PERSIST_KEY = "paramStore";

    struct PersistedValues
    {
        uint32_t magic;
        uint16_t version;
        uint16_t count;
        float values[VALUE_COUNT];
    };

    mutable portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    float values[VALUE_COUNT] = {0.0f};
    bool dirtyValues = false;
    uint32_t valueGeneration = 1;
};
