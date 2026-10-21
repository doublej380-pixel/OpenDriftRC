// Compile both with and without OPENDRIFT_IMU_MPU6050.
#include "ParameterCatalog.h"
#include "ParameterStore.h"
#include <cassert>
#include <cstdio>

int main()
{
    using namespace OpenDriftParameters;
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    assert(!Defaults::WIFI_ON_BOOT);
    #else
    assert(Defaults::WIFI_ON_BOOT);
    #endif
    ParameterStore store;
    #if defined(OPENDRIFT_IMU_MPU6050)
    assert(store.get(Id::SMOOTHING) == 0.10f);
    assert(store.getInt(Id::GYRO_LPF) == 0);
    assert(store.getInt(Id::MEMORY_LIMIT) == 120);
    assert(isAvailable(*find(Id::SERVO_RATE)));
    assert(store.getInt(Id::SERVO_RATE) == 0);
    assert(isAvailable(*find(Id::SERVO_QUIET)));
    store.set(Id::SERVO_QUIET, 5);
    assert(store.getInt(Id::SERVO_QUIET) == 5);
    assert(isAvailable(*find(Id::DRIFT_MEMORY)));
    assert(isAvailable(*find(Id::HOLD_ASSIST)));
    assert(isAvailable(*find(Id::GYRO_LPF)));
    store.setInitial(Id::DRIFT_MEMORY, 3.0f);
    store.setInitial(Id::HOLD_ASSIST, 75);
    store.setInitial(Id::GYRO_LPF, 1);
    assert(store.get(Id::DRIFT_MEMORY) == 3.0f);
    assert(store.getInt(Id::HOLD_ASSIST) == 75);
    assert(store.getInt(Id::GYRO_LPF) == 1);
    #else
    assert(store.get(Id::SMOOTHING) == 0.0f);
    assert(!isAvailable(*find(Id::SERVO_RATE)));
    store.setInitial(Id::SERVO_RATE, 0);
    assert(store.getInt(Id::SERVO_RATE) == 1);
    store.set(Id::SERVO_RATE, 0);
    assert(store.getInt(Id::SERVO_RATE) == 1);
    // Throttle is independently selectable, not forced to the steering rate.
    store.set(Id::THROTTLE_RATE, 1);
    assert(store.getInt(Id::THROTTLE_RATE) == 1);
    for(Id id : {Id::DRIFT_MEMORY, Id::MEMORY_LIMIT, Id::HOLD_ASSIST, Id::GYRO_LPF, Id::SERVO_QUIET})
    {
        assert(!isAvailable(*find(id)));
        // Simulate old persisted values/profile imports and stale client writes.
        const float expected = id == Id::GYRO_LPF ? 2.0f : 0.0f;
        store.setInitial(id, 75);
        assert(store.get(id) == expected);
        store.set(id, -1);
        assert(store.get(id) == expected);
        store.set(id, 1);
        assert(store.get(id) == expected);
    }
    #endif
    assert(isAvailable(*find(Id::SMOOTHING)));
    store.set(Id::SMOOTHING, 0.25f);
    assert(store.get(Id::SMOOTHING) == 0.25f);
    float profile[ParameterStore::VALUE_COUNT] = {};
    store.captureProfile(profile, ParameterStore::VALUE_COUNT);
    profile[(uint8_t)Id::DRIFT_MEMORY] = 9;
    profile[(uint8_t)Id::HOLD_ASSIST] = 90;
    store.applyProfile(profile, ParameterStore::VALUE_COUNT);
    #if defined(OPENDRIFT_IMU_MPU6050)
    assert(store.get(Id::DRIFT_MEMORY) == 9);
    assert(store.getInt(Id::HOLD_ASSIST) == 90);
    #else
    assert(store.get(Id::DRIFT_MEMORY) == 0);
    assert(store.getInt(Id::HOLD_ASSIST) == 0);
    #endif
    Preferences preferences;
    assert(store.save(preferences));
    ParameterStore restored;
    assert(restored.load(preferences));
    assert(restored.get(Id::DRIFT_MEMORY) == store.get(Id::DRIFT_MEMORY));
    assert(restored.get(Id::SMOOTHING) == 0.25f);
    #if !defined(OPENDRIFT_IMU_MPU6050)
    // A version-1 blob produced by older firmware can still contain active
    // values. Loading it must sanitize these, without resetting the whole tune.
    struct OldBlob {
        uint32_t magic;
        uint16_t version, count;
        float values[ParameterStore::VALUE_COUNT];
    } old = {};
    assert(preferences.getBytes("paramStore", &old, sizeof(old)) == sizeof(old));
    old.values[(uint8_t)Id::DRIFT_MEMORY] = 8;
    old.values[(uint8_t)Id::MEMORY_LIMIT] = 120;
    old.values[(uint8_t)Id::HOLD_ASSIST] = 75;
    old.values[(uint8_t)Id::GYRO_LPF] = 0;
    old.values[(uint8_t)Id::SERVO_RATE] = 0;
    old.values[(uint8_t)Id::SERVO_QUIET] = 12;
    preferences.putBytes("paramStore", &old, sizeof(old));
    assert(restored.load(preferences));
    assert(restored.get(Id::DRIFT_MEMORY) == 0);
    assert(restored.getInt(Id::MEMORY_LIMIT) == 0);
    assert(restored.getInt(Id::HOLD_ASSIST) == 0);
    assert(restored.getInt(Id::GYRO_LPF) == 2);
    assert(restored.getInt(Id::SERVO_RATE) == 1);
    assert(restored.getInt(Id::SERVO_QUIET) == 0);
    assert(restored.get(Id::SMOOTHING) == 0.25f);
    #endif
    std::puts("Parameter retirement tests passed");
}
