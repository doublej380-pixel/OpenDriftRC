#include "WebConfigurator.h"

#include <limits.h>
#include <stdlib.h>

#if defined(OPENDRIFT_INPUT_CRSF) && defined(OPENDRIFT_BOARD_AMOLED_164)
#include "AuxChannelOutputs.h"
#endif

WebConfigurator::WebConfigurator()
:
server(80)
{

}


#if defined(OPENDRIFT_BOARD_AMOLED_164)
void WebConfigurator::setBackgroundStore(Backgrounds& store)
{
    backgrounds = &store;
}


void WebConfigurator::setBlackboxArchive(BlackboxArchive& archive)
{
    blackboxArchive = &archive;
}
#endif



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
        "/live-status",
        HTTP_GET,
        [this]()
        {
            handleLiveStatus();
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

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    server.on(
        "/blackbox-saved.csv",
        HTTP_GET,
        [this]() { handleArchivedLogDownload(); }
    );

    server.on(
        "/clear-saved-log",
        HTTP_POST,
        [this]() { handleArchivedLogClear(); }
    );
    #endif

    server.on(
        "/restart",
        HTTP_POST,
        [this]()
        {
            handleRestart();
        }
    );

    server.on(
        "/factory-reset",
        HTTP_POST,
        [this]()
        {
            handleFactoryReset();
        }
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    server.on(
        "/upload-background",
        HTTP_POST,
        [this]() { handleBackgroundUpload(); },
        [this]() { handleBackgroundUploadChunk(); }
    );

    server.on(
        "/use-background",
        HTTP_POST,
        [this]() { handleBackgroundUse(); }
    );

    server.on(
        "/delete-background",
        HTTP_POST,
        [this]() { handleBackgroundDelete(); }
    );
    #endif

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

    if(
        restartPending &&
        (int32_t)(millis() - restartAtMs) >= 0
    )
    {
        ESP.restart();
    }
}



bool WebConfigurator::isRunning()
{
    return running;
}

bool WebConfigurator::isRestartPending()
{
    return restartPending;
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

    html.reserve(30000);

    html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += F("<title>OpenDrift Config</title><style>");
    html += F("body{font-family:system-ui,Arial,sans-serif;margin:0;background:#101214;color:#f5f5f5}");
    html += F("main{max-width:760px;margin:0 auto;padding:18px}");
    html += F("h1{font-size:28px;margin:8px 0 2px}h2{font-size:18px;margin:22px 0 10px}");
    html += F(".sub{color:#aeb4bb;margin-bottom:20px}.card{border:1px solid #33383f;border-radius:8px;padding:14px;margin:12px 0;background:#171a1f}");
    html += F("label{display:block;font-size:13px;color:#c8cdd2;margin:12px 0 5px}input,select{width:100%;box-sizing:border-box;background:#0b0d10;color:#fff;border:1px solid #3b4148;border-radius:6px;padding:10px;font-size:16px}");
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
    html += F("</div><div class='pill'>Gain: ");
    html += String(gainRadio->getPulseWidth());
    html += gainRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    html += F("</div><div class='pill'>Active gain: <strong id='activeGain'>");
    html += gyro != nullptr ? String(gyro->getGain(), 2) : F("--");
    html += F("</strong><br><small id='gainOverride'>Checking gain source...</small>");
    html += F("</div><div class='pill'>Throttle: ");
    html += String(throttleRadio->getPulseWidth());
    html += throttleRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    #if defined(OPENDRIFT_INPUT_CRSF)
    #if defined(OPENDRIFT_CRSF_OOPS_SWAPPED_PINS)
    html += F("</div><div class='pill'>CRSF OOPS: receiver TX to GPIO 17 / RX to GPIO 18");
    #elif defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
    html += F("</div><div class='pill'>CRSF: GPIO 1 RX / 2 TX &middot; ESC: GPIO 8");
    #elif defined(OPENDRIFT_BOARD_MATRIX)
    html += F("</div><div class='pill'>CRSF: GPIO 3 RX / 4 TX");
    #elif defined(OPENDRIFT_AMOLED_V2)
    html += F("</div><div class='pill'>CRSF: GPIO 1 RX / 2 TX");
    #else
    html += F("</div><div class='pill'>CRSF: GPIO 17 RX / 18 TX");
    #endif
    #else
    #if defined(OPENDRIFT_BOARD_MATRIX)
    html += F("</div><div class='pill'>GPIO 4: ");
    #elif defined(OPENDRIFT_AMOLED_V2)
    html += F("</div><div class='pill'>GPIO 2: ");
    #else
    html += F("</div><div class='pill'>GPIO 18: ");
    #endif
    html += settings->getThrottleOutputEnabled()
        ? F("THROTTLE OUT")
        : F("GAIN INPUT");
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
        html += String(profile->predictionStrength);
        html += F(" &middot; Hold ");
        html += String(profile->gyroHoldBoost);
        html += F(" &middot; Countersteer ");
        html += String(profile->gyroCounterSteerAssist);
        html += F(" &middot; Transition speed ");
        html += String(profile->gyroTransitionSpeed);
        html += F(" &middot; Driver priority ");
        html += String(profile->driverPriority);
        html += F(" &middot; Anti Wobble ");
        html += String(profile->gyroHuntStrength);
        html += F("</small></div>");

        html += F("<form method='post' action='/activate-profile'><input type='hidden' name='profile' value='");
        html += String(i);
        html += F("'><input type='hidden' name='name' value='");
        html += profile->name;
        html += F("'><button type='submit'>Activate</button></form>");

        html += F("<form method='post' action='/delete-profile' onsubmit=\"return confirm('Delete this profile?')\"><input type='hidden' name='profile' value='");
        html += String(i);
        html += F("'><input type='hidden' name='name' value='");
        html += profile->name;
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
    html += parameterInput(OpenDriftParameters::Id::GYRO_GAIN, "gain", String(settings->getGain(), 2), "Saved gain (fallback)");
    html += parameterInput(OpenDriftParameters::Id::DEADBAND, "deadband", String(settings->getDeadband(), 2));
    html += parameterInput(OpenDriftParameters::Id::MAX_CORRECTION, "gyroMax", String(settings->getGyroMaxCorrection()), "Max correction (% full steering span)");
    html += F("<p class='sub'>This is the gyro's maximum endpoint-to-endpoint authority. 50% can move from center to one calibrated endpoint; 100% can override one endpoint all the way to the other. Physical endpoint calibration remains the final hard limit.</p>");
    html += F("</div>");
    html += checkbox("Reverse gyro correction", "gyroReverse", settings->getGyroReverse());
    html += F("</div>");

    html += F("<div class='card'><h2>OpenDrift v1.0 Response</h2><div class='row'>");
    html += parameterInput(OpenDriftParameters::Id::SMOOTHING, "gyroSmoothing", String(settings->getGyroSmoothing(), 2));
    html += F("<label>Gyro sensor LPF</label><select name='gyroLpfMode'><option value='0'");
    if(settings->getGyroLpfMode() == 0) html += F(" selected");
    html += F(">24 Hz - original</option><option value='1'");
    if(settings->getGyroLpfMode() == 1) html += F(" selected");
    html += F(">120 Hz - low latency</option><option value='2'");
    if(settings->getGyroLpfMode() == 2) html += F(" selected");
    html += F(">Off - raw bandwidth</option></select>");
    html += parameterInput(OpenDriftParameters::Id::PREDICTION, "predictionStrength", String(settings->getPredictionStrength()));
    html += parameterInput(OpenDriftParameters::Id::ANTI_WOBBLE, "huntStrength", String(settings->getGyroHuntStrength()));
    html += parameterInput(OpenDriftParameters::Id::GYRO_HYSTERESIS, "gyroOutputHysteresis", String(settings->getGyroOutputHysteresis()));
    html += F("<label>Anti Wobble scale</label><select name='antiWobbleScale'><option value='0'");
    if(settings->getAntiWobbleScale() == 0) html += F(" selected");
    html += F(">1/10 scale</option><option value='1'");
    if(settings->getAntiWobbleScale() == 1) html += F(" selected");
    html += F(">Micro (1/24-1/28)</option></select>");
    html += F("<p class='sub'>Anti Wobble controls the depth of OpenDrift's narrow, phase-aware wheel-wobble notch. Use 1/10 for the proven 2.5-3.6 Hz steering mode, or Micro for faster 5-15 Hz steering systems. Gyro output hysteresis is an experimental diagnostic that holds only tiny gyro-correction changes; it never reduces driver steering resolution. Leave it at zero for the unchanged 1.0.9 response.</p>");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Transition &amp; Driver Priority</h2><p class='sub'>Transition Speed controls how quickly gyro correction reverses during a direction change. Driver Priority progressively reduces only fast direct gyro gain as you hold more steering, giving the driver more authority near full lock. Start at 0 and test 10-20; steady Countersteer Assist, Drift Memory, and Max Correction remain unchanged.</p><div class='row'>");
    html += parameterInput(OpenDriftParameters::Id::TRANSITION_SPEED, "transitionSpeed", String(settings->getGyroTransitionSpeed()));
    html += parameterInput(OpenDriftParameters::Id::DRIVER_PRIORITY, "driverPriority", String(settings->getDriverPriority()));
    html += F("</div></div>");

    html += F("<div class='card'><h2>Drift Assist</h2><p class='sub'>Countersteer Assist changes only the steady steering workload. Zero preserves the base v1.0 response; higher values let OpenDrift carry more of a settled drift.</p><div class='row'>");
    html += parameterInput(OpenDriftParameters::Id::COUNTERSTEER, "counterSteerAssist", String(settings->getGyroCounterSteerAssist()));
    html += parameterInput(OpenDriftParameters::Id::HOLD_ASSIST, "gyroHoldBoost", String(settings->getGyroHoldBoost()));
    html += parameterInput(OpenDriftParameters::Id::DRIFT_MEMORY, "gyroIGain", String(settings->getGyroIntegralGain(), 2));
    html += parameterInput(OpenDriftParameters::Id::MEMORY_LIMIT, "gyroILimit", String(settings->getGyroIntegralLimit()));
    html += F("</div></div>");

    html += F("<div class='card'><h2>Servo</h2>");
    html += checkbox("Reverse servo", "servoReverse", settings->getServoReverse());
    html += F("<label>Control and servo rate</label><select name='controlLoopHz'><option value='250'");
    if(settings->getControlLoopHz() == 250) html += F(" selected");
    html += F(">250 Hz - broad servo compatibility</option><option value='333'");
    if(settings->getControlLoopHz() == 333) html += F(" selected");
    html += F(">333 Hz - supported servos only</option></select><p class='sub'>250 Hz supports a broader range of digital servos. Select 333 Hz only when the servo manufacturer explicitly supports it. A restart is required after changing this setting.</p>");
    html += F("<label>Throttle output rate</label><select name='throttleOutputHz'><option value='50'");
    if(settings->getThrottleOutputHz() == 50) html += F(" selected");
    html += F(">50 Hz - broad ESC compatibility</option><option value='250'");
    if(settings->getThrottleOutputHz() == 250) html += F(" selected");
    html += F(">250 Hz - high-rate PWM</option><option value='333'");
    if(settings->getThrottleOutputHz() == 333) html += F(" selected");
    html += F(">333 Hz - supported ESCs only</option></select><p class='sub'>Higher rates reduce throttle command latency and increase effective pulse resolution. Use 250 or 333 Hz only when the ESC explicitly supports that input rate. A restart is required.</p>");
    html += F("<div class='row'>");
    html += parameterInput(OpenDriftParameters::Id::SERVO_CENTER, "servoCenter", String(settings->getServoCenter()), "Center pulse");
    html += parameterInput(OpenDriftParameters::Id::SERVO_TRAVEL, "servoTravel", String(settings->getServoTravel()), "Travel percent");
    html += parameterInput(OpenDriftParameters::Id::SERVO_QUIET, "servoQuiet", String(settings->getServoQuiet()), "Quiet band us");
    html += F("</div></div>");

    #if defined(OPENDRIFT_BOARD_MATRIX)
    html += F("<div class='card'><h2>LED Matrix Orientation</h2><label>Status rotation</label><select name='displayRotation'><option value='0'");
    if(settings->getDisplayRotation() == 0) html += F(" selected");
    html += F(">0&deg;</option><option value='1'");
    if(settings->getDisplayRotation() == 1) html += F(" selected");
    html += F(">90&deg; clockwise</option><option value='2'");
    if(settings->getDisplayRotation() == 2) html += F(" selected");
    html += F(">180&deg;</option><option value='3'");
    if(settings->getDisplayRotation() == 3) html += F(" selected");
    html += F(">90&deg; counter-clockwise (default)</option></select><p class='sub'>Rotates the complete 8x8 status display, including WiFi and blackbox indicators. Changes apply immediately.</p></div>");
    #elif defined(OPENDRIFT_BOARD_AMOLED_164)
    html += F("<div class='card'><h2>Display Orientation</h2><label>AMOLED orientation</label><select name='displayRotation'><option value='0'");
    if(settings->getDisplayRotation() == 0) html += F(" selected");
    html += F(">Normal</option><option value='2'");
    if(settings->getDisplayRotation() == 2) html += F(" selected");
    html += F(">180&deg; flipped</option></select><p class='sub'>Flips both the AMOLED image and touchscreen coordinates. Changes apply immediately.</p><div class='row'>");
    html += input("Brightness (10-100%)", "displayBrightness", String(settings->getDisplayBrightness()), "number", "10");
    html += input("Idle dim timeout (seconds, 0=off)", "displayDimTimeout", String(settings->getDisplayDimTimeout()), "number", "1");
    html += F("</div><label>Text colour</label><select name='themeText'><option value='0'");
    if(settings->getThemeText() == 0) html += F(" selected");
    html += F(">Light text (default)</option><option value='1'");
    if(settings->getThemeText() == 1) html += F(" selected");
    html += F(">Dark text, for light backgrounds</option></select>");

    html += F("<label>Accent colour</label><select name='themeAccent'>");
    for(uint8_t accent = 0; accent < Settings::THEME_ACCENT_COUNT; accent++)
    {
        html += F("<option value='");
        html += String((int)accent);
        html += F("'");
        if(settings->getThemeAccent() == accent) html += F(" selected");
        html += F(">");
        html += Settings::themeAccentName(accent);
        html += F("</option>");
    }
    html += F("</select><p class='sub'>Mixed keeps the original page colours. The other presets apply one accent throughout the AMOLED UI. Use dark text with a bright background. Translucent panels keep controls readable while preserving the artwork.</p></div>");
    #endif

    html += F("<div class='card'><h2>Physical Servo Endpoints</h2><p class='sub'>Status: <strong>");
    html += settings->isSteeringCalibrated() ? F("CALIBRATED") : F("NOT CALIBRATED");
    html += F("</strong>. These are the servo's physical PWM stops and the final hard limits for both driver and gyro movement. Position the wheels at each safe physical endpoint and capture it from the display or EdgeTX tool, or enter all three pulse values below.</p><div class='row'>");
    html += input("Max left", "steeringMin", String(settings->getSteeringMin()));
    html += input("Center", "steeringCenter", String(settings->getSteeringCenter()));
    html += input("Max right", "steeringMax", String(settings->getSteeringMax()));
    html += F("<input type='hidden' name='steeringMinWas' value='");
    html += String(settings->getSteeringMin());
    html += F("'><input type='hidden' name='steeringCenterWas' value='");
    html += String(settings->getSteeringCenter());
    html += F("'><input type='hidden' name='steeringMaxWas' value='");
    html += String(settings->getSteeringMax());
    html += F("'>");
    html += parameterInput(OpenDriftParameters::Id::STEERING_TRAVEL, "radioSteeringTravel", String(settings->getRadioSteeringTravel()), "Steering travel percent");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Gain Channel Calibration</h2><div class='row'>");
    #if defined(OPENDRIFT_INPUT_CRSF)
    #if defined(OPENDRIFT_CRSF_OOPS_SWAPPED_PINS)
    html += F("Personal swapped-pin build: CRSF channel 3 controls gyro gain. GPIO 16 drives the steering servo. GPIO 15 actively outputs neutral throttle during failsafe and passes throttle only after a valid neutral hold. Receiver TX feeds GPIO 17; receiver RX connects to GPIO 18.");
    #elif defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
    html += F("Personal V2 GPIO8 recovery build: CRSF channel 3 controls gyro gain. GPIO 15 drives the steering servo. GPIO 8 actively outputs neutral throttle during failsafe and passes throttle only after a valid neutral hold. Receiver TX feeds GPIO 1; receiver RX connects to GPIO 2. GPIO 8 is reserved and unavailable as an auxiliary output.");
    #elif defined(OPENDRIFT_BOARD_MATRIX)
    html += F("CRSF channel 3 controls gyro gain. GPIO 1 drives the steering servo. GPIO 2 actively outputs neutral throttle during failsafe and passes throttle only after a valid neutral hold. Receiver TX feeds GPIO 3; receiver RX connects to GPIO 4.");
    #elif defined(OPENDRIFT_AMOLED_V2)
    html += F("CRSF channel 3 controls gyro gain. GPIO 15 drives the steering servo. GPIO 16 actively outputs neutral throttle during failsafe and passes throttle only after a valid neutral hold. Receiver TX feeds GPIO 1; receiver RX connects to GPIO 2.");
    #else
    html += F("CRSF channel 3 controls gyro gain. GPIO 15 drives the steering servo. GPIO 16 actively outputs neutral throttle during failsafe and passes throttle only after a valid neutral hold.");
    #endif
    html += F("</div>");
    #else
    html += input("Gain low", "gainMin", String(settings->getGainMin()));
    html += input("Gain high", "gainMax", String(settings->getGainMax()));
    html += F("</div>");
    html += checkbox(
        #if defined(OPENDRIFT_BOARD_MATRIX)
        "Use GPIO 4 as throttle output instead of gyro gain input",
        #elif defined(OPENDRIFT_AMOLED_V2)
        "Use GPIO 2 as throttle output instead of gyro gain input",
        #else
        "Use GPIO 18 as throttle output instead of gyro gain input",
        #endif
        "throttleOutputEnabled",
        settings->getThrottleOutputEnabled()
    );
    #endif
    html += F("<div class='row'>");
    html += parameterInput(OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN, "channel3GainMin", String(settings->getChannel3GainMin(), 2));
    html += parameterInput(OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX, "channel3GainMax", String(settings->getChannel3GainMax(), 2));
    html += F("</div><p class='sub'>Maps the full Channel 3 control range to gyro gain. Defaults are 0.50 to 3.00; both ends support 0.00 to 6.00.</p>");
    html += F("</div>");

    #if defined(OPENDRIFT_INPUT_CRSF) && defined(OPENDRIFT_BOARD_AMOLED_164)
    html += F("<div class='card'><h2>Auxiliary Channel Outputs</h2><p class='sub'>Route any CRSF channel to a standard 50 Hz receiver-style PWM signal. Outputs return to 1500 us on signal loss. GPIO is 3.3 V signal only: power accessories externally and connect a common ground.</p><div class='row'>");

    for(uint8_t gpio = 1; gpio <= 8; gpio++)
    {
        html += F("<div><label>GPIO ");
        html += String(gpio);

        if(!AuxChannelOutputs::isPinAvailable(gpio))
        {
            html += F("</label><div class='pill'>Reserved for CRSF UART</div></div>");
            continue;
        }

        html += F("</label><select name='auxGpio");
        html += String(gpio);
        html += F("'><option value='0'");

        uint8_t selectedChannel =
            settings->getAuxChannelForGpio(gpio);

        if(selectedChannel == 0)
        {
            html += F(" selected");
        }

        html += F(">Disabled</option>");

        for(uint8_t channel = 1; channel <= 16; channel++)
        {
            html += F("<option value='");
            html += String(channel);
            html += F("'");

            if(selectedChannel == channel)
            {
                html += F(" selected");
            }

            html += F(">CRSF Channel ");
            html += String(channel);
            html += F("</option>");
        }

        html += F("</select></div>");
    }

    html += F("</div><p class='sub'>Mappings take effect immediately after Save Settings. Multiple GPIOs may mirror the same channel.</p></div>");
    #endif

    html += F("<div class='card'><h2>WiFi</h2>");
    html += checkbox("Enable WiFi on boot", "wifiEnabled", settings->getWifiEnabled());
    html += input("WiFi network name", "wifiSsid", String(settings->getWifiSsid()), "text", "1");
    html += input("Auto-off timeout ms", "wifiTimeout", String(settings->getWifiTimeout()));
    html += F("<p class='sub'>Auto-off counts only while no device is connected. A connected phone pauses the timer; a disconnect starts a fresh timeout.</p>");
    html += F("</div>");

    html += F("<div class='card'><h2>Blackbox</h2>");
    html += checkbox("Enable onboard logging", "blackboxEnabled", settings->getBlackboxEnabled());
    html += F("</div>");

    html += F("<div class='card'><h2>System</h2><p class='sub'>Restart applies settings that require a reboot. Factory reset erases the tune, profiles, endpoint calibration, GPIO mappings, and board settings.</p>");
    html += F("<button type='submit' form='restartForm'>Restart OpenDrift</button>");
    html += F("<button class='danger' type='submit' form='factoryResetForm'>Factory Reset</button></div>");

    html += F("<button type='submit'>Save Settings</button></form>");
    html += F("<form id='restartForm' method='post' action='/restart' onsubmit=\"return confirm('Restart OpenDrift now? Steering will be unavailable during boot.')\"></form>");
    html += F("<form id='factoryResetForm' method='post' action='/factory-reset' onsubmit=\"return confirm('Erase all OpenDrift settings and restart? This cannot be undone.')\"></form>");

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    html += F("<div class='card' id='backgrounds'><h2>AMOLED Backgrounds</h2>");

    if(backgrounds == nullptr || !backgrounds->isReady())
    {
        html += F("<p class='sub'>Background storage is unavailable. The built-in background remains active.</p></div>");
    }
    else
    {
        const char* activeBackground = settings->getBackgroundName();
        html += F("<p class='sub'>Active: <strong>");
        html += activeBackground[0] == 0 ? "Built-in" : activeBackground;
        html += F("</strong> &middot; ");
        html += String((int)backgrounds->getCount());
        html += F(" / 16 stored &middot; ");
        html += String((unsigned long)(backgrounds->getFreeBytes() / 1024));
        html += F(" KB free</p>");

        html += F("<div class='profile");
        if(activeBackground[0] == 0) html += F(" active");
        html += F("'><div><strong>Built-in</strong><small>Permanent OpenDrift fallback</small></div><form method='post' action='/use-background'><input type='hidden' name='name' value=''><button type='submit'>Use</button></form><div></div></div>");

        for(uint8_t i = 0; i < backgrounds->getCount(); i++)
        {
            const char* name = backgrounds->getName(i);
            html += F("<div class='profile");
            if(strcmp(name, activeBackground) == 0) html += F(" active");
            html += F("'><div><strong>");
            html += name;
            html += F("</strong><small>456 x 280 RGB565</small></div><form method='post' action='/use-background'><input type='hidden' name='name' value='");
            html += name;
            html += F("'><button type='submit'>Use</button></form><form method='post' action='/delete-background' onsubmit=\"return confirm('Delete this background?')\"><input type='hidden' name='name' value='");
            html += name;
            html += F("'><button class='danger' type='submit'>Delete</button></form></div>");
        }

        html += F("<label>Source image or converted file</label><input id='bgFile' type='file' accept='.rgb,image/png,image/jpeg,image/webp'>");
        html += F("<label>Background name</label><input id='bgName' type='text' maxlength='23' placeholder='track-night'>");
        html += F("<canvas id='bgPreview' width='456' height='280' style='display:none;width:100%;height:auto;border:1px solid #3b4148;border-radius:6px;margin-top:12px'></canvas>");
        html += F("<button type='button' onclick='uploadBackground()'>Convert and upload</button>");
        html += F("<p class='sub' id='bgStatus'>JPG, PNG, or WebP. The browser center-crops and converts locally; the original image never leaves your device. Do not drive while writing a background.</p></div>");
    }
    #endif

    html += F("<div class='card'><h2>Blackbox Log</h2>");

    if(!settings->getBlackboxEnabled())
    {
        html += F("<p class='sub'>Logging disabled. Enable onboard logging and save settings to record logs.</p>");
    }
    else if(blackbox != nullptr && blackbox->isReady())
    {
        html += F("<p class='sub'>Binary records in PSRAM: ");
        html += String(blackbox->getRecordCount());
        html += F(" &middot; used: ");
        html += String(blackbox->getSize() / 1024);
        html += F(" / ");
        html += String(blackbox->getCapacityBytes() / 1024);
        html += F(" KB &middot; duration: ");
        html += String(blackbox->getDurationMs() / 60000);
        html += F("m ");
        html += String((blackbox->getDurationMs() / 1000) % 60);
        html += F("s");

        if(blackbox->isFull())
        {
            html += F(" &middot; retaining newest records");
        }

        if(blackbox->getOverwrittenRows() > 0)
        {
            html += F(" &middot; overwritten: ");
            html += String(blackbox->getOverwrittenRows());
        }

        html += F("</p><p class='sub'>While driving, records stay entirely in volatile PSRAM. Internal flash is written only when you explicitly archive after parking. Download converts the binary records to CSV; power cycling clears only the RAM copy.</p>");
        html += F("<a href='/blackbox.csv'>Download CSV</a>");
        html += F("<form method='post' action='/clear-log'><button type='submit'>Clear RAM Log</button></form>");

    }
    else
    {
        html += F("<p class='sub'>PSRAM log buffer unavailable.</p>");
    }

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(blackboxArchive != nullptr && blackboxArchive->hasArchive())
    {
        html += F("<p class='sub'>Saved internal archive: ");
        html += String(blackboxArchive->getArchiveRecordCount());
        html += F(" records &middot; ");
        html += String(blackboxArchive->getArchiveBytes() / 1024);
        html += F(" KB. This copy survives power cycles.</p>");
        html += F("<a href='/blackbox-saved.csv'>Download saved CSV</a>");
        html += F("<form method='post' action='/clear-saved-log'><button type='submit'>Delete Saved Log</button></form>");
    }
    else
    {
        html += F("<p class='sub'>No persistent log saved. Park the car for two seconds, then press Archive Log in the EdgeTX tool.</p>");
    }
    #endif

    html += F("</div>");

    html += F("</main><script>function updateLive(){fetch('/live-status',{cache:'no-store'}).then(r=>r.json()).then(s=>{document.getElementById('activeGain').textContent=Number(s.gain).toFixed(2);document.getElementById('gainOverride').textContent=s.override?'CH3 gain override active':'Saved gain active';}).catch(()=>{});}updateLive();setInterval(updateLive,500);");

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    html += F("function uploadBackground(){const f=document.getElementById('bgFile').files[0],n=document.getElementById('bgName').value.trim(),st=document.getElementById('bgStatus'),c=document.getElementById('bgPreview');if(!f||!n){st.textContent='Choose an image and enter a name.';return;}const send=b=>{const fd=new FormData();fd.append('image',b,n+'.rgb');st.textContent='Uploading background...';fetch('/upload-background',{method:'POST',body:fd}).then(async r=>{const t=await r.text();if(r.ok)location.href='/?background='+Date.now()+'#backgrounds';else st.textContent=t;}).catch(()=>st.textContent='Upload failed. Stay connected to OpenDrift WiFi and try again.');};if(f.name.toLowerCase().endsWith('.rgb')){if(f.size!==255360){st.textContent='Converted .rgb file must be exactly 255,360 bytes.';return;}send(f);return;}const img=new Image();img.onload=()=>{URL.revokeObjectURL(img.src);const x=c.getContext('2d'),s=Math.max(456/img.width,280/img.height),w=img.width*s,h=img.height*s;x.clearRect(0,0,456,280);x.drawImage(img,(456-w)/2,(280-h)/2,w,h);c.style.display='block';const d=x.getImageData(0,0,456,280).data,out=new Uint8Array(255360);for(let i=0,j=0;i<d.length;i+=4,j+=2){const v=((d[i]&248)<<8)|((d[i+1]&252)<<3)|(d[i+2]>>3);out[j]=v&255;out[j+1]=v>>8;}send(new Blob([out],{type:'application/octet-stream'}));};img.onerror=()=>st.textContent='That image could not be decoded.';img.src=URL.createObjectURL(f);}");
    #endif

    html += F("</script></body></html>");

    server.send(
        200,
        "text/html",
        html
    );
}


void WebConfigurator::handleLiveStatus()
{
    if(
        settings == nullptr ||
        gyro == nullptr ||
        gainRadio == nullptr
    )
    {
        server.send(
            503,
            "application/json",
            "{\"error\":\"unavailable\"}"
        );

        return;
    }

    bool gainOverride = gainRadio->hasSignal();

    #if !defined(OPENDRIFT_INPUT_CRSF)
    gainOverride =
        gainOverride &&
        !settings->getThrottleOutputEnabled();
    #endif

    String json;
    json.reserve(72);
    json += F("{\"gain\":");
    json += String(gyro->getGain(), 2);
    json += F(",\"pulse\":");
    json += String(gainRadio->getPulseWidth());
    json += F(",\"override\":");
    json += gainOverride ? F("true") : F("false");
    json += F("}");

    server.sendHeader(
        "Cache-Control",
        "no-store"
    );

    server.send(
        200,
        "application/json",
        json
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

    bool gyroReversePosted = false;
    if(checkboxChanged("gyroReverse", gyroReversePosted))
    {
        settings->setGyroReverse(gyroReversePosted);
    }

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

    settings->setGyroLpfMode(
        getIntArg(
            "gyroLpfMode",
            settings->getGyroLpfMode()
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

    settings->setGyroTransitionSpeed(
        getIntArg(
            "transitionSpeed",
            settings->getGyroTransitionSpeed()
        )
    );

    settings->setDriverPriority(
        getIntArg(
            "driverPriority",
            settings->getDriverPriority()
        )
    );

    settings->setPredictionStrength(
        getIntArg(
            "predictionStrength",
            settings->getPredictionStrength()
        )
    );

    settings->setGyroHuntStrength(
        getIntArg(
            "huntStrength",
            settings->getGyroHuntStrength()
        )
    );

    settings->setGyroOutputHysteresis(
        getIntArg(
            "gyroOutputHysteresis",
            settings->getGyroOutputHysteresis()
        )
    );

    settings->setAntiWobbleScale(
        getIntArg(
            "antiWobbleScale",
            settings->getAntiWobbleScale()
        )
    );

    bool servoReversePosted = false;
    bool servoReverseChanged =
        checkboxChanged("servoReverse", servoReversePosted);

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

    settings->setControlLoopHz(
        getIntArg(
            "controlLoopHz",
            settings->getControlLoopHz()
        )
    );

    settings->setThrottleOutputHz(
        getIntArg(
            "throttleOutputHz",
            settings->getThrottleOutputHz()
        )
    );

    settings->setDisplayRotation(
        getIntArg(
            "displayRotation",
            settings->getDisplayRotation()
        )
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    settings->setDisplayBrightness(
        getIntArg(
            "displayBrightness",
            settings->getDisplayBrightness()
        )
    );

    settings->setDisplayDimTimeout(
        getIntArg(
            "displayDimTimeout",
            settings->getDisplayDimTimeout()
        )
    );

    settings->setThemeText(
        getIntArg(
            "themeText",
            settings->getThemeText()
        )
    );

    settings->setThemeAccent(
        getIntArg(
            "themeAccent",
            settings->getThemeAccent()
        )
    );
    #endif

    int requestedSteeringMin =
        getIntArg(
            "steeringMin",
            settings->getSteeringMin()
        );

    int requestedSteeringCenter =
        getIntArg(
            "steeringCenter",
            settings->getSteeringCenter()
        );

    int requestedSteeringMax =
        getIntArg(
            "steeringMax",
            settings->getSteeringMax()
        );

    bool steeringCalibrationChanged =
        endpointFieldEdited(
            "steeringMin",
            "steeringMinWas",
            requestedSteeringMin
        ) ||
        endpointFieldEdited(
            "steeringCenter",
            "steeringCenterWas",
            requestedSteeringCenter
        ) ||
        endpointFieldEdited(
            "steeringMax",
            "steeringMaxWas",
            requestedSteeringMax
        );

    if(steeringCalibrationChanged)
    {
        settings->setStoredSteeringEndpoints(
            requestedSteeringMin,
            requestedSteeringCenter,
            requestedSteeringMax
        );
    }

    if(servoReverseChanged)
    {
        settings->setServoReverse(servoReversePosted);
    }

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

    settings->setChannel3GainMin(
        getFloatArg(
            "channel3GainMin",
            settings->getChannel3GainMin()
        )
    );

    settings->setChannel3GainMax(
        getFloatArg(
            "channel3GainMax",
            settings->getChannel3GainMax()
        )
    );

    #if !defined(OPENDRIFT_INPUT_CRSF)
    bool throttleOutputPosted = false;
    if(checkboxChanged("throttleOutputEnabled", throttleOutputPosted))
    {
        settings->setThrottleOutputEnabled(throttleOutputPosted);
    }
    #endif

    #if defined(OPENDRIFT_INPUT_CRSF) && defined(OPENDRIFT_BOARD_AMOLED_164)
    for(uint8_t gpio = 1; gpio <= 8; gpio++)
    {
        if(!AuxChannelOutputs::isPinAvailable(gpio))
        {
            continue;
        }

        char argument[12];

        snprintf(
            argument,
            sizeof(argument),
            "auxGpio%u",
            gpio
        );

        settings->setAuxChannelForGpio(
            gpio,
            getIntArg(
                argument,
                settings->getAuxChannelForGpio(gpio)
            )
        );
    }
    #endif

    bool wifiEnabledPosted = false;
    if(checkboxChanged("wifiEnabled", wifiEnabledPosted))
    {
        settings->setWifiEnabled(wifiEnabledPosted);
    }

    settings->setWifiTimeout(
        getIntArg(
            "wifiTimeout",
            settings->getWifiTimeout()
        )
    );

    if(server.hasArg("wifiSsid"))
    {
        settings->setWifiSsid(server.arg("wifiSsid"));
    }

    bool blackboxPosted = false;
    if(checkboxChanged("blackboxEnabled", blackboxPosted))
    {
        settings->setBlackboxEnabled(blackboxPosted);
    }

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
            settings->getGyroMaxCorrection() * 10
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

        gyro->setTransitionSpeed(
            settings->getGyroTransitionSpeed()
        );

        gyro->setPredictionStrength(
            settings->getPredictionStrength()
        );

        gyro->setHuntStrength(
            settings->getGyroHuntStrength()
        );

        gyro->setAntiWobbleScale(
            settings->getAntiWobbleScale()
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

    if(!profileRowMatches(index))
    {
        server.send(409, "text/plain", "Profile list changed; reload and try again");
        return;
    }

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

    if(!profileRowMatches(index))
    {
        server.send(409, "text/plain", "Profile list changed; reload and try again");
        return;
    }

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


bool WebConfigurator::profileRowMatches(int index)
{
    if(
        settings == nullptr ||
        index < 0 ||
        index >= settings->getProfileCount()
    )
    {
        return false;
    }

    if(!server.hasArg("name"))
    {
        return true;
    }

    const Settings::DrivingProfile* profile =
        settings->getProfile(index);

    return
        profile != nullptr &&
        server.arg("name").equalsIgnoreCase(profile->name);
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

    server.sendHeader(
        "Content-Disposition",
        "attachment; filename=opendrift-blackbox.csv"
    );

    server.setContentLength(
        CONTENT_LENGTH_UNKNOWN
    );

    server.send(
        200,
        "text/csv",
        ""
    );

    server.sendContent(
        blackbox->getCsvHeader()
    );
    server.sendContent("\n");

    size_t recordCount =
        blackbox->getRecordCount();

    char line[704];
    String chunk;
    chunk.reserve(8192);

    for(size_t index = 0; index < recordCount; index++)
    {
        size_t length =
            blackbox->formatCsvRecord(
                index,
                line,
                sizeof(line)
            );

        if(length == 0)
        {
            continue;
        }

        if(chunk.length() + length > 8192)
        {
            server.sendContent(chunk);
            chunk = "";

            if(!server.client().connected())
            {
                return;
            }
        }

        chunk.concat(line, length);

        if((index & 0x7F) == 0)
        {
            delay(0);
        }
    }

    if(chunk.length() > 0)
    {
        server.sendContent(chunk);
    }

    server.sendContent("");
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

    blackbox->clear();

    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        303
    );
}


#if defined(OPENDRIFT_BOARD_AMOLED_164)
void WebConfigurator::handleArchivedLogDownload()
{
    if(
        blackboxArchive == nullptr ||
        blackbox == nullptr ||
        blackboxArchive->isSaving()
    )
    {
        server.send(503, "text/plain", "Saved blackbox log unavailable");
        return;
    }

    File file;
    size_t recordCount = 0;
    size_t recordSize = 0;

    if(!blackboxArchive->openArchive(file, recordCount, recordSize))
    {
        server.send(404, "text/plain", "No valid saved blackbox log");
        return;
    }

    if(recordSize > 512)
    {
        file.close();
        server.send(500, "text/plain", "Saved blackbox format is unsupported");
        return;
    }

    server.sendHeader(
        "Content-Disposition",
        "attachment; filename=opendrift-blackbox-saved.csv"
    );
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/csv", "");
    server.sendContent(blackbox->getCsvHeader());
    server.sendContent("\n");

    uint8_t record[512];
    char line[704];
    String chunk;
    chunk.reserve(8192);

    for(size_t index = 0; index < recordCount; index++)
    {
        if(file.read(record, recordSize) != recordSize) break;

        const size_t length = blackbox->formatBinaryRecord(
            record,
            recordSize,
            line,
            sizeof(line)
        );

        if(length == 0) continue;

        if(chunk.length() + length > 8192)
        {
            server.sendContent(chunk);
            chunk = "";

            if(!server.client().connected())
            {
                file.close();
                return;
            }
        }

        chunk.concat(line, length);
        if((index & 0x7F) == 0) delay(0);
    }

    file.close();
    if(chunk.length() > 0) server.sendContent(chunk);
    server.sendContent("");
}


void WebConfigurator::handleArchivedLogClear()
{
    if(blackboxArchive == nullptr || !blackboxArchive->clearArchive())
    {
        server.send(503, "text/plain", "Could not delete saved blackbox log");
        return;
    }

    server.sendHeader("Location", "/");
    server.send(303);
}
#endif


#if defined(OPENDRIFT_BOARD_AMOLED_164)
void WebConfigurator::handleBackgroundUploadChunk()
{
    if(backgrounds == nullptr) return;
    HTTPUpload& upload = server.upload();

    switch(upload.status)
    {
        case UPLOAD_FILE_START:
        {
            String name = upload.filename;
            const int dot = name.lastIndexOf('.');
            if(dot > 0) name = name.substring(0, dot);
            backgroundUploadOk = backgrounds->beginUpload(name.c_str());
            break;
        }

        case UPLOAD_FILE_WRITE:
            if(backgroundUploadOk)
            {
                backgroundUploadOk = backgrounds->writeUpload(
                    upload.buf,
                    upload.currentSize
                );
            }
            break;

        case UPLOAD_FILE_END:
            if(backgroundUploadOk)
            {
                backgroundUploadOk = backgrounds->endUpload();
            }
            else
            {
                backgrounds->abortUpload();
            }
            break;

        default:
            backgrounds->abortUpload();
            backgroundUploadOk = false;
            break;
    }
}


void WebConfigurator::handleBackgroundUpload()
{
    if(backgrounds == nullptr || !backgrounds->isReady())
    {
        server.send(503, "text/plain", "Background storage is unavailable");
        return;
    }

    server.sendHeader("Cache-Control", "no-store");

    if(backgroundUploadOk)
    {
        backgroundUploadOk = false;
        server.send(200, "text/plain", "OK");
        return;
    }

    const char* error = backgrounds->getUploadError();
    server.send(
        400,
        "text/plain",
        error[0] != 0 ? error : "No complete image was received"
    );
}


void WebConfigurator::handleBackgroundUse()
{
    String name = Backgrounds::sanitizeName(server.arg("name"));

    if(
        name.length() > 0 &&
        (backgrounds == nullptr || !backgrounds->exists(name.c_str()))
    )
    {
        name = "";
    }

    settings->setBackgroundName(name);
    server.sendHeader("Location", "/#backgrounds");
    server.send(303);
}


void WebConfigurator::handleBackgroundDelete()
{
    if(backgrounds == nullptr)
    {
        server.send(503, "text/plain", "Background storage is unavailable");
        return;
    }

    const String name = Backgrounds::sanitizeName(server.arg("name"));
    if(name.length() > 0)
    {
        backgrounds->remove(name.c_str());

        if(strcmp(settings->getBackgroundName(), name.c_str()) == 0)
        {
            settings->setBackgroundName("");
        }
    }

    server.sendHeader("Location", "/#backgrounds");
    server.send(303);
}
#endif


void WebConfigurator::handleRestart()
{
    server.send(
        200,
        "text/html",
        "<!doctype html><meta name='viewport' content='width=device-width'><title>OpenDrift</title><body style='font-family:sans-serif;background:#101318;color:#eee;padding:2rem'><h1>Restarting OpenDrift</h1><p>Reconnect to the OpenDrift WiFi network in a few seconds.</p></body>"
    );

    restartPending = true;
    restartAtMs = millis() + 750;
}


void WebConfigurator::handleFactoryReset()
{
    if(settings == nullptr)
    {
        server.send(503, "text/plain", "Settings unavailable");
        return;
    }

    settings->factoryReset();

    server.send(
        200,
        "text/html",
        "<!doctype html><meta name='viewport' content='width=device-width'><title>OpenDrift</title><body style='font-family:sans-serif;background:#101318;color:#eee;padding:2rem'><h1>Factory reset complete</h1><p>OpenDrift is restarting with default settings.</p></body>"
    );

    restartPending = true;
    restartAtMs = millis() + 750;
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


String WebConfigurator::parameterInput(
    OpenDriftParameters::Id id,
    const char* name,
    String value,
    const char* label
)
{
    const OpenDriftParameters::Definition* definition =
        OpenDriftParameters::find(id);

    if(definition == nullptr)
    {
        return input(label != nullptr ? label : name, name, value);
    }

    String html;
    html += F("<div><label>");
    html += label != nullptr ? label : definition->name;
    html += F("</label><input name='");
    html += name;
    html += F("' type='number' min='");
    html += String(definition->minimum, (unsigned int)definition->decimals);
    html += F("' max='");
    html += String(definition->maximum, (unsigned int)definition->decimals);
    html += F("' step='");
    html += String(definition->step, (unsigned int)definition->decimals);
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

    html += F("<input type='hidden' name='");
    html += name;
    html += F("Was' value='");
    html += checked ? '1' : '0';
    html += F("'><label><input name='");
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


bool WebConfigurator::checkboxChanged(
    const char* name,
    bool& posted
)
{
    posted = server.hasArg(name);
    String snapshotName = String(name) + "Was";

    if(!server.hasArg(snapshotName))
    {
        return true;
    }

    return posted != (server.arg(snapshotName) == "1");
}


bool WebConfigurator::endpointFieldEdited(
    const char* name,
    const char* snapshotName,
    int requested
)
{
    if(!server.hasArg(name) || server.arg(name).length() == 0)
    {
        return false;
    }

    if(!server.hasArg(snapshotName) || server.arg(snapshotName).length() == 0)
    {
        return true;
    }

    return requested != server.arg(snapshotName).toInt();
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

    String raw = server.arg(name);

    if(raw.length() == 0)
    {
        return fallback;
    }

    const char* text = raw.c_str();
    char* end = nullptr;
    long value = strtol(text, &end, 10);

    if(end == text)
    {
        return fallback;
    }

    return (int)constrain(value, (long)INT_MIN, (long)INT_MAX);
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

    String raw = server.arg(name);

    if(raw.length() == 0)
    {
        return fallback;
    }

    const char* text = raw.c_str();
    char* end = nullptr;
    float value = strtof(text, &end);

    return end == text ? fallback : value;
}
