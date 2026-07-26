#include "WebConfigurator.h"

#include <FFat.h>


WebConfigurator::WebConfigurator()
:
server(80)
{

}



void WebConfigurator::begin(
    Settings& settingsRef,
    GyroController& gyroRef,
    RadioInput& steeringRadioRef,
    RadioInput& gainRadioRef,
    RadioInput& throttleRadioRef,
    BlackboxLogger& blackboxRef
)
{
    settings =
        &settingsRef;

    gyro =
        &gyroRef;

    steeringRadio =
        &steeringRadioRef;

    gainRadio =
        &gainRadioRef;

    throttleRadio =
        &throttleRadioRef;

    blackbox =
        &blackboxRef;

    server.on(
        "/",
        HTTP_GET,
        [this]()
        {
            handleRoot();
        }
    );

    server.on(
        "/save",
        HTTP_POST,
        [this]()
        {
            handleSave();
        }
    );

    server.on(
        "/toggle-terrain",
        HTTP_POST,
        [this]()
        {
            handleTerrainToggle();
        }
    );

    server.on(
        "/create-profile",
        HTTP_POST,
        [this]()
        {
            handleProfileCreate();
        }
    );

    server.on(
        "/activate-profile",
        HTTP_POST,
        [this]()
        {
            handleProfileActivate();
        }
    );

    server.on(
        "/delete-profile",
        HTTP_POST,
        [this]()
        {
            handleProfileDelete();
        }
    );

    server.on(
        "/blackbox.csv",
        HTTP_GET,
        [this]()
        {
            handleLogDownload();
        }
    );

    server.on(
        "/clear-log",
        HTTP_POST,
        [this]()
        {
            handleLogClear();
        }
    );

    server.on(
        "/flush-log",
        HTTP_POST,
        [this]()
        {
            handleLogFlush();
        }
    );

    server.onNotFound(
        [this]()
        {
            handleNotFound();
        }
    );

    server.begin();

    running = true;

    Serial.println(
        "Web configurator started"
    );
}



void WebConfigurator::update()
{
    if(!running)
    {
        return;
    }

    server.handleClient();
}



bool WebConfigurator::isRunning()
{
    return running;
}



void WebConfigurator::handleRoot()
{
    if(settings == nullptr)
    {
        server.send(
            503,
            "text/plain",
            "Settings unavailable"
        );

        return;
    }

    String html;

    html.reserve(14000);

    html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += F("<title>OpenDrift Config</title><style>");
    html += F("body{font-family:system-ui,Arial,sans-serif;margin:0;background:#101214;color:#f5f5f5}");
    html += F("main{max-width:760px;margin:0 auto;padding:18px}");
    html += F("h1{font-size:28px;margin:8px 0 2px}h2{font-size:18px;margin:22px 0 10px}");
    html += F(".sub{color:#aeb4bb;margin-bottom:20px}.card{border:1px solid #33383f;border-radius:8px;padding:14px;margin:12px 0;background:#171a1f}");
    html += F("label{display:block;font-size:13px;color:#c8cdd2;margin:12px 0 5px}input{width:100%;box-sizing:border-box;background:#0b0d10;color:#fff;border:1px solid #3b4148;border-radius:6px;padding:10px;font-size:16px}");
    html += F("input[type=checkbox]{width:auto;transform:scale(1.3);margin-right:8px}.row{display:grid;grid-template-columns:1fr 1fr;gap:10px}");
    html += F(".status{display:grid;grid-template-columns:1fr 1fr;gap:8px}.pill{background:#0b0d10;border:1px solid #33383f;border-radius:6px;padding:10px}");
    html += F("button{width:100%;padding:13px 16px;border:0;border-radius:6px;background:#24a36b;color:#fff;font-size:17px;font-weight:700;margin-top:16px}");
    html += F(".profile{display:grid;grid-template-columns:1fr 96px 82px;gap:8px;align-items:center;background:#0b0d10;border:1px solid #33383f;border-radius:6px;padding:9px;margin:8px 0}.profile.active{border-color:#24a36b}.profile strong{display:block}.profile small{color:#aeb4bb}.profile form{margin:0}.profile button{margin:0;padding:9px 6px;font-size:13px}.profile .danger{background:#973b45}.create-profile{display:grid;grid-template-columns:1fr 150px;gap:10px;align-items:end}.create-profile button{margin:0;height:43px}");
    html += F("a{color:#65b7ff}@media(max-width:560px){.row,.status,.create-profile{grid-template-columns:1fr}.profile{grid-template-columns:1fr 1fr}.profile>div{grid-column:1/-1}}");
    html += F("</style></head><body><main>");
    html += F("<h1>OpenDrift</h1><div class='sub'>Web configurator</div>");

    html += F("<div class='card'><h2>Live Radio</h2><div class='status'>");
    html += F("<div class='pill'>Steering: ");
    html += String(steeringRadio->getPulseWidth());
    html += steeringRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    html += F("</div><div class='pill'>Gain: ");
    html += String(gainRadio->getPulseWidth());
    html += gainRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    #endif
    html += F("</div><div class='pill'>Throttle: ");
    html += String(throttleRadio->getPulseWidth());
    html += throttleRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    html += F("</div><div class='pill'>GPIO 18: ");
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    html += settings->getThrottleOutputEnabled()
        ? F("THROTTLE OUT")
        : F("GAIN INPUT");
    #else
    html += F("THROTTLE INPUT");
    #endif
    html += F("</div></div></div>");

    html += F("<div class='card'><h2>Driving Profiles</h2><p class='sub'>Active: <strong>");
    html += settings->getActiveProfileName();
    html += F("</strong>. Active profiles automatically keep trackside tune changes.</p>");

    for(uint8_t i = 0; i < settings->getProfileCount(); i++)
    {
        const Settings::DrivingProfile* profile =
            settings->getProfile(i);

        if(profile == nullptr)
        {
            continue;
        }

        html += F("<div class='profile");

        if(settings->getActiveProfileIndex() == i)
        {
            html += F(" active");
        }

        html += F("'><div><strong>");
        html += profile->name;
        html += F("</strong><small>Gain ");
        html += String(profile->gain, 2);
        html += F(" &middot; Prediction ");
        html += String(profile->gyroHuntDamping);
        html += F(" &middot; Hold ");
        html += String(profile->gyroHoldBoost);
        html += F(" &middot; Countersteer ");
        html += String(profile->gyroCounterSteerAssist);
        html += F("</small></div>");

        html += F("<form method='post' action='/activate-profile'><input type='hidden' name='profile' value='");
        html += String(i);
        html += F("'><button type='submit'>Activate</button></form>");

        html += F("<form method='post' action='/delete-profile' onsubmit=\"return confirm('Delete this profile?')\"><input type='hidden' name='profile' value='");
        html += String(i);
        html += F("'><button class='danger' type='submit'>Delete</button></form></div>");
    }

    if(settings->getProfileCount() < Settings::MAX_PROFILES)
    {
        html += F("<form class='create-profile' method='post' action='/create-profile'><div><label>New profile name</label><input name='name' type='text' maxlength='23' required placeholder='Example: P-tile'></div><button type='submit'>Create from current tune</button></form>");
    }
    else
    {
        html += F("<p class='sub'>Profile limit reached. Delete one to create another.</p>");
    }

    html += F("</div>");

    html += F("<form method='post' action='/save'>");

    html += F("<div class='card'><h2>Drive &amp; Limits</h2><div class='row'>");
    html += input("Gain", "gain", String(settings->getGain(), 2), "number", "0.01");
    html += input("Deadband", "deadband", String(settings->getDeadband(), 2), "number", "1");
    html += input("Max correction (us)", "gyroMax", String(settings->getGyroMaxCorrection()), "number", "1");
    html += F("</div>");
    html += checkbox("Reverse gyro correction", "gyroReverse", settings->getGyroReverse());
    html += F("</div>");

    html += F("<div class='card'><h2>OpenDrift v1.0 Response</h2><div class='row'>");
    html += input("Smoothing", "gyroSmoothing", String(settings->getGyroSmoothing(), 2), "number", "0.01");
    html += input("Prediction strength (0-100)", "gyroHuntDamping", String(settings->getGyroHuntDamping()), "number", "1");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Drift Assist</h2><p class='sub'>Countersteer Assist changes only the steady steering workload. Zero preserves the base v1.0 response; higher values let OpenDrift carry more of a settled drift.</p><div class='row'>");
    html += input("Countersteer assist (0-100)", "counterSteerAssist", String(settings->getGyroCounterSteerAssist()), "number", "1");
    html += input("Hold assist (0-100)", "gyroHoldBoost", String(settings->getGyroHoldBoost()), "number", "1");
    html += input("Drift memory", "gyroIGain", String(settings->getGyroIntegralGain(), 2), "number", "0.01");
    html += input("Memory limit (us)", "gyroILimit", String(settings->getGyroIntegralLimit()), "number", "1");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Servo</h2>");
    html += checkbox("Reverse servo", "servoReverse", settings->getServoReverse());
    html += F("<div class='row'>");
    html += input("Center pulse", "servoCenter", String(settings->getServoCenter()));
    html += input("Travel percent", "servoTravel", String(settings->getServoTravel()));
    html += input("Quiet band us", "servoQuiet", String(settings->getServoQuiet()), "number", "1");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Steering Calibration</h2><div class='row'>");
    html += input("Max left", "steeringMin", String(settings->getSteeringMin()));
    html += input("Center", "steeringCenter", String(settings->getSteeringCenter()));
    html += input("Max right", "steeringMax", String(settings->getSteeringMax()));
    html += input("Steering travel percent", "radioSteeringTravel", String(settings->getRadioSteeringTravel()), "number", "1");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Gain Channel Calibration</h2><div class='row'>");
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    html += input("Gain low", "gainMin", String(settings->getGainMin()));
    html += input("Gain high", "gainMax", String(settings->getGainMax()));
    html += F("</div>");
    html += checkbox(
        "Use GPIO 18 as throttle output instead of gyro gain input",
        "throttleOutputEnabled",
        settings->getThrottleOutputEnabled()
    );
    #else
    html += F("GPIO 18 is dedicated to throttle input on the round board. Gyro gain uses the saved Gain setting.");
    html += F("</div>");
    #endif
    html += F("</div>");

    html += F("<div class='card'><h2>WiFi</h2>");
    html += checkbox("Enable WiFi on boot", "wifiEnabled", settings->getWifiEnabled());
    html += input("Auto-off timeout ms", "wifiTimeout", String(settings->getWifiTimeout()));
    html += F("</div>");

    html += F("<div class='card'><h2>Blackbox</h2>");
    html += checkbox("Enable onboard logging", "blackboxEnabled", settings->getBlackboxEnabled());
    html += F("</div>");

    html += F("<button type='submit'>Save Settings</button></form>");

    html += F("<div class='card'><h2>Blackbox Log</h2>");

    if(!settings->getBlackboxEnabled())
    {
        html += F("<p class='sub'>Logging disabled. Enable onboard logging and save settings to record logs.</p>");
    }
    else if(blackbox != nullptr && blackbox->isReady())
    {
        html += F("<p class='sub'>Size: ");
        html += String(blackbox->getSize() / 1024);
        html += F(" KB");

        if(blackbox->isFull())
        {
            html += F(" - full");
        }

        html += F("</p><a href='/blackbox.csv'>Download CSV</a>");
        html += F("<form method='post' action='/flush-log'><button type='submit'>Flush Log</button></form>");
        html += F("<form method='post' action='/clear-log'><button type='submit'>Clear Log</button></form>");
    }
    else
    {
        html += F("<p class='sub'>Log storage unavailable.</p>");
    }

    html += F("</div>");

    html += F("</main></body></html>");

    server.send(
        200,
        "text/html",
        html
    );
}


void WebConfigurator::handleTerrainToggle()
{
    if(settings == nullptr)
    {
        server.send(
            503,
            "text/plain",
            "Settings unavailable"
        );

        return;
    }

    settings->setTerrainAssistEnabled(
        !settings->getTerrainAssistEnabled()
    );

    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        303
    );
}



void WebConfigurator::handleSave()
{
    if(settings == nullptr)
    {
        server.send(
            503,
            "text/plain",
            "Settings unavailable"
        );

        return;
    }

    settings->setGain(
        getFloatArg(
            "gain",
            settings->getGain()
        )
    );

    settings->setDeadband(
        getFloatArg(
            "deadband",
            settings->getDeadband()
        )
    );

    settings->setGyroReverse(
        server.hasArg("gyroReverse")
    );

    settings->setGyroMaxCorrection(
        getIntArg(
            "gyroMax",
            settings->getGyroMaxCorrection()
        )
    );

    settings->setGyroSmoothing(
        getFloatArg(
            "gyroSmoothing",
            settings->getGyroSmoothing()
        )
    );

    settings->setGyroAttackSpeed(
        getIntArg(
            "gyroAttack",
            settings->getGyroAttackSpeed()
        )
    );

    settings->setGyroReturnSpeed(
        getIntArg(
            "gyroReturn",
            settings->getGyroReturnSpeed()
        )
    );

    settings->setGyroIntegralGain(
        getFloatArg(
            "gyroIGain",
            settings->getGyroIntegralGain()
        )
    );

    settings->setGyroIntegralLimit(
        getIntArg(
            "gyroILimit",
            settings->getGyroIntegralLimit()
        )
    );

    settings->setGyroHoldBoost(
        getIntArg(
            "gyroHoldBoost",
            settings->getGyroHoldBoost()
        )
    );

    settings->setGyroCounterSteerAssist(
        getIntArg(
            "counterSteerAssist",
            settings->getGyroCounterSteerAssist()
        )
    );

    settings->setGyroAntiWobble(
        getIntArg(
            "gyroAntiWobble",
            settings->getGyroAntiWobble()
        )
    );

    settings->setGyroHuntDamping(
        getIntArg(
            "gyroHuntDamping",
            settings->getGyroHuntDamping()
        )
    );

    settings->setSteeringDamper(
        getIntArg(
            "steeringDamper",
            settings->getSteeringDamper()
        )
    );

    settings->setServoReverse(
        server.hasArg("servoReverse")
    );

    settings->setServoCenter(
        getIntArg(
            "servoCenter",
            settings->getServoCenter()
        )
    );

    settings->setServoTravel(
        getIntArg(
            "servoTravel",
            settings->getServoTravel()
        )
    );

    settings->setServoQuiet(
        getIntArg(
            "servoQuiet",
            settings->getServoQuiet()
        )
    );

    settings->setSteeringMin(
        getIntArg(
            "steeringMin",
            settings->getSteeringMin()
        )
    );

    settings->setSteeringCenter(
        getIntArg(
            "steeringCenter",
            settings->getSteeringCenter()
        )
    );

    settings->setSteeringMax(
        getIntArg(
            "steeringMax",
            settings->getSteeringMax()
        )
    );

    settings->setRadioSteeringTravel(
        getIntArg(
            "radioSteeringTravel",
            settings->getRadioSteeringTravel()
        )
    );

    settings->setGainMin(
        getIntArg(
            "gainMin",
            settings->getGainMin()
        )
    );

    settings->setGainMax(
        getIntArg(
            "gainMax",
            settings->getGainMax()
        )
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    settings->setThrottleOutputEnabled(
        server.hasArg("throttleOutputEnabled")
    );
    #endif

    settings->setWifiEnabled(
        server.hasArg("wifiEnabled")
    );

    settings->setWifiTimeout(
        getIntArg(
            "wifiTimeout",
            settings->getWifiTimeout()
        )
    );

    settings->setBlackboxEnabled(
        server.hasArg("blackboxEnabled")
    );

    if(gyro != nullptr)
    {
        gyro->setGain(
            settings->getGain()
        );

        gyro->setDeadband(
            settings->getDeadband()
        );

        gyro->setSmoothing(
            settings->getGyroSmoothing()
        );

        gyro->setMaxCorrection(
            settings->getGyroMaxCorrection()
        );

        gyro->setIntegralGain(
            settings->getGyroIntegralGain()
        );

        gyro->setIntegralLimit(
            settings->getGyroIntegralLimit()
        );

        gyro->setHoldBoost(
            settings->getGyroHoldBoost()
        );

        gyro->setCounterSteerAssist(
            settings->getGyroCounterSteerAssist()
        );

        gyro->setAntiWobble(
            settings->getGyroAntiWobble()
        );

        gyro->setHuntDamping(
            settings->getGyroHuntDamping()
        );
    }

    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        303
    );
}



void WebConfigurator::handleProfileCreate()
{
    if(
        settings == nullptr ||
        !server.hasArg("name")
    )
    {
        server.send(
            400,
            "text/plain",
            "Profile name required"
        );

        return;
    }

    if(settings->createProfile(server.arg("name")) < 0)
    {
        server.send(
            400,
            "text/plain",
            "Could not create profile. Use a unique name and check the profile limit."
        );

        return;
    }

    server.sendHeader("Location", "/");
    server.send(303);
}


void WebConfigurator::handleProfileActivate()
{
    if(
        settings == nullptr ||
        !server.hasArg("profile")
    )
    {
        server.send(400, "text/plain", "Profile required");
        return;
    }

    int index = server.arg("profile").toInt();

    if(
        index < 0 ||
        index >= settings->getProfileCount() ||
        !settings->activateProfile(index)
    )
    {
        server.send(404, "text/plain", "Profile not found");
        return;
    }

    server.sendHeader("Location", "/");
    server.send(303);
}


void WebConfigurator::handleProfileDelete()
{
    if(
        settings == nullptr ||
        !server.hasArg("profile")
    )
    {
        server.send(400, "text/plain", "Profile required");
        return;
    }

    int index = server.arg("profile").toInt();

    if(
        index < 0 ||
        index >= settings->getProfileCount() ||
        !settings->deleteProfile(index)
    )
    {
        server.send(404, "text/plain", "Profile not found");
        return;
    }

    server.sendHeader("Location", "/");
    server.send(303);
}



void WebConfigurator::handleLogDownload()
{
    if(
        settings != nullptr &&
        !settings->getBlackboxEnabled()
    )
    {
        server.send(
            503,
            "text/plain",
            "Blackbox logging disabled"
        );

        return;
    }

    if(
        blackbox == nullptr ||
        !blackbox->isReady()
    )
    {
        server.send(
            503,
            "text/plain",
            "Blackbox log unavailable"
        );

        return;
    }

    blackbox->flush();

    File file =
        FFat.open(
            blackbox->getPath(),
            FILE_READ
        );

    if(!file)
    {
        server.send(
            404,
            "text/plain",
            "Log file not found"
        );

        return;
    }

    server.sendHeader(
        "Content-Disposition",
        "attachment; filename=opendrift-blackbox.csv"
    );

    server.streamFile(
        file,
        "text/csv"
    );

    file.close();
}



void WebConfigurator::handleLogFlush()
{
    if(
        settings != nullptr &&
        !settings->getBlackboxEnabled()
    )
    {
        server.send(
            503,
            "text/plain",
            "Blackbox logging disabled"
        );

        return;
    }

    if(
        blackbox == nullptr ||
        !blackbox->flush()
    )
    {
        server.send(
            503,
            "text/plain",
            "Could not flush blackbox log"
        );

        return;
    }

    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        303
    );
}



void WebConfigurator::handleLogClear()
{
    if(
        settings != nullptr &&
        !settings->getBlackboxEnabled()
    )
    {
        server.send(
            503,
            "text/plain",
            "Blackbox logging disabled"
        );

        return;
    }

    if(
        blackbox == nullptr ||
        !blackbox->clear()
    )
    {
        server.send(
            503,
            "text/plain",
            "Could not clear blackbox log"
        );

        return;
    }

    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        303
    );
}



void WebConfigurator::handleNotFound()
{
    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        302
    );
}



String WebConfigurator::input(
    const char* label,
    const char* name,
    String value,
    const char* type,
    const char* step
)
{
    String html;

    html += F("<div><label>");
    html += label;
    html += F("</label><input name='");
    html += name;
    html += F("' type='");
    html += type;
    html += F("' step='");
    html += step;
    html += F("' value='");
    html += value;
    html += F("'></div>");

    return html;
}



String WebConfigurator::checkbox(
    const char* label,
    const char* name,
    bool checked
)
{
    String html;

    html += F("<label><input name='");
    html += name;
    html += F("' type='checkbox'");

    if(checked)
    {
        html += F(" checked");
    }

    html += F(">");
    html += label;
    html += F("</label>");

    return html;
}



int WebConfigurator::getIntArg(
    const char* name,
    int fallback
)
{
    if(!server.hasArg(name))
    {
        return fallback;
    }

    return server.arg(name).toInt();
}



float WebConfigurator::getFloatArg(
    const char* name,
    float fallback
)
{
    if(!server.hasArg(name))
    {
        return fallback;
    }

    return server.arg(name).toFloat();
}
