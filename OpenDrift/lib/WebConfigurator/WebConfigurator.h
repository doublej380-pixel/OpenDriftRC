#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "Settings.h"
#include "GyroController.h"
#include "RadioInput.h"
#include "BlackboxLogger.h"
#include "ControlDiagnostics.h"

#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include "Backgrounds.h"
#include "BlackboxArchive.h"
#endif


class WebConfigurator
{
public:

    WebConfigurator();

    void begin(
        Settings& settings,
        GyroController& gyro,
        RadioInput& steeringRadio,
        RadioInput& gainRadio,
        RadioInput& throttleRadio,
        BlackboxLogger& blackbox
    );

    void update();
    void setControlDiagnostics(ControlDiagnostics& capture) { diagnostics = &capture; }

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    void setBackgroundStore(
        Backgrounds& store
    );

    void setBlackboxArchive(
        BlackboxArchive& archive
    );
    #endif

    bool isRunning();

    bool isRestartPending();


private:

    WebServer server;

    Settings* settings = nullptr;

    GyroController* gyro = nullptr;

    RadioInput* steeringRadio = nullptr;

    RadioInput* gainRadio = nullptr;

    RadioInput* throttleRadio = nullptr;

    BlackboxLogger* blackbox = nullptr;
    ControlDiagnostics* diagnostics = nullptr;
    void handleDiagnosticsDownload();

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    Backgrounds* backgrounds = nullptr;
    BlackboxArchive* blackboxArchive = nullptr;
    bool backgroundUploadOk = false;
    #endif

    bool running = false;

    bool restartPending = false;
    uint32_t restartAtMs = 0;

    void handleRoot();

    void handleLiveStatus();

    void handleSave();

    void handleProfileCreate();

    void handleProfileActivate();

    void handleProfileDelete();

    bool profileRowMatches(int index);

    void handleLogDownload();

    void handleLogClear();

    void handleRestart();
    void handleFactoryReset();

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    void handleBackgroundUpload();
    void handleBackgroundUploadChunk();
    void handleBackgroundUse();
    void handleBackgroundDelete();
    void handleArchivedLogDownload();
    void handleArchivedLogClear();
    void handleArchivedLogSave();
    #endif

    void handleNotFound();

    String input(
        const char* label,
        const char* name,
        String value,
        const char* type = "number",
        const char* step = "1"
    );

    String parameterInput(
        OpenDriftParameters::Id id,
        const char* name,
        String value,
        const char* label = nullptr
    );

    String checkbox(
        const char* label,
        const char* name,
        bool checked
    );

    bool checkboxChanged(
        const char* name,
        bool& posted
    );

    bool endpointFieldEdited(
        const char* name,
        const char* snapshotName,
        int requested
    );

    int getIntArg(
        const char* name,
        int fallback
    );

    float getFloatArg(
        const char* name,
        float fallback
    );
};
