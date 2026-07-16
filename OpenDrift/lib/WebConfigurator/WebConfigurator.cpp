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

    html.reserve(9000);

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
    html += F("a{color:#65b7ff}@media(max-width:560px){.row,.status{grid-template-columns:1fr}}");
    html += F("</style></head><body><main>");
    html += F("<h1>OpenDrift</h1><div class='sub'>Web configurator</div>");

    html += F("<div class='card'><h2>Live Radio</h2><div class='status'>");
    html += F("<div class='pill'>Steering: ");
    html += String(steeringRadio->getPulseWidth());
    html += steeringRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    html += F("</div><div class='pill'>Gain: ");
    html += String(gainRadio->getPulseWidth());
    html += gainRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    html += F("</div><div class='pill'>Throttle: ");
    html += String(throttleRadio->getPulseWidth());
    html += throttleRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    html += F("</div><div class='pill'>GPIO 18: ");
    html += settings->getThrottleOutputEnabled()
        ? F("THROTTLE OUT")
        : F("GAIN INPUT");
    html += F("</div></div></div>");

    html += F("<form method='post' action='/save'>");

    html += F("<div class='card'><h2>Gyro</h2><div class='row'>");
    html += input("Gain", "gain", String(settings->getGain(), 2), "number", "0.01");
    html += input("Deadband", "deadband", String(settings->getDeadband(), 2), "number", "1");
    html += input("Max correction us", "gyroMax", String(settings->getGyroMaxCorrection()), "number", "1");
    html += input("Smoothing", "gyroSmoothing", String(settings->getGyroSmoothing(), 2), "number", "0.01");
    html += input("I gain", "gyroIGain", String(settings->getGyroIntegralGain(), 2), "number", "0.01");
    html += input("I limit us", "gyroILimit", String(settings->getGyroIntegralLimit()), "number", "1");
    html += input("Hold boost percent", "gyroHoldBoost", String(settings->getGyroHoldBoost()), "number", "1");
    html += input("Attack speed", "gyroAttack", String(settings->getGyroAttackSpeed()), "number", "1");
    html += input("Return speed", "gyroReturn", String(settings->getGyroReturnSpeed()), "number", "1");
    html += input("Anti-wobble", "gyroAntiWobble", String(settings->getGyroAntiWobble()), "number", "1");
    html += input("Steer damper ms", "steeringDamper", String(settings->getSteeringDamper()), "number", "1");
    html += F("</div>");
    html += checkbox("Reverse gyro correction", "gyroReverse", settings->getGyroReverse());
    html += F("</div>");

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
    html += input("Gain low", "gainMin", String(settings->getGainMin()));
    html += input("Gain high", "gainMax", String(settings->getGainMax()));
    html += F("</div>");
    html += checkbox(
        "Use GPIO 18 as throttle output instead of gyro gain input",
        "throttleOutputEnabled",
        settings->getThrottleOutputEnabled()
    );
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

    settings->setGyroAntiWobble(
        getIntArg(
            "gyroAntiWobble",
            settings->getGyroAntiWobble()
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

    settings->setThrottleOutputEnabled(
        server.hasArg("throttleOutputEnabled")
    );

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

        gyro->setAntiWobble(
            settings->getGyroAntiWobble()
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
