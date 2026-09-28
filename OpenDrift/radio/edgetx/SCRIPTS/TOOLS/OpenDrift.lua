-- OpenDrift CRSF tuning tool for EdgeTX monochrome radios (including MT12).

local DEVICE = 0xC8
local RADIO = 0xEA

local fields = {
  { 1, "Saved Gain",       0,  600,   5, 2 },
  {37, "Live Gain",        0,  600,   5, 2, false, false, false, false, false, false, false, false, true},
  {33, "CH3 Gain Min",     0,  600,   5, 2 },
  {34, "CH3 Gain Max",     0,  600,   5, 2 },
  { 2, "Deadband",         0, 1000,   1, 1 },
  { 3, "Max Corr %",       0,  100,   1, 0 },
  { 4, "Smoothing",        0,  100,   1, 2 },
  {32, "Gyro LPF",         0,    2,   1, 0, true, false, false, false, false, true},
  { 5, "Drift Memory",     0, 2000,   1, 2 },
  { 6, "Memory Limit",     0,  500,   5, 0 },
  { 7, "Hold Assist",      0,  100,   1, 0 },
  { 8, "Countersteer",     0,  100,   1, 0 },
  { 9, "Transition Speed", 0,  100,   1, 0 },
  {38, "Driver Priority",  0,   50,   1, 0 },
  {10, "Prediction",       0,  100,   1, 0 },
  {26, "Anti Wobble",      0,  100,   1, 0 },
  {41, "Gyro Hyst.",       0,    4,   1, 0 },
  {36, "Wobble Scale",     0,    1,   1, 0, true, false, false, false, false, false, false, true},
  {11, "Servo Quiet",      0,   50,   1, 0 },
  {12, "Steering Travel",  0,  100,   1, 0 },
  {27, "Endpoints",        0,    2,   1, 0, true, false, false, true, false},
  {28, "Capture Left",     0,    1,   1, 0, true, false, false, false, true},
  {29, "Capture Center",   0,    1,   1, 0, true, false, false, false, true},
  {30, "Capture Right",    0,    1,   1, 0, true, false, false, false, true},
  {31, "Reset Cal",        0,    1,   1, 0, true, false, false, false, true},
  {13, "Servo Travel",     1,  100,   1, 0 },
  {14, "Servo Center",  1000, 2000,   1, 0 },
  {15, "Servo Reverse",    0,    1,   1, 0, true},
  {16, "Gyro Reverse",     0,    1,   1, 0, true},
  {17, "GPIO 1 Output",    0,   16,   1, 0, true, true},
  {18, "GPIO 2 Output",    0,   16,   1, 0, true, true},
  {19, "GPIO 3 Output",    0,   16,   1, 0, true, true},
  {20, "GPIO 4 Output",    0,   16,   1, 0, true, true},
  {21, "GPIO 5 Output",    0,   16,   1, 0, true, true},
  {22, "GPIO 6 Output",    0,   16,   1, 0, true, true},
  {23, "GPIO 7 Output",    0,   16,   1, 0, true, true},
  {24, "GPIO 8 Output",    0,   16,   1, 0, true, true},
  {25, "Servo Rate*",      0,    1,   1, 0, true, false, true},
  {39, "Throttle Rate*",   0,    2,   1, 0, true, false, false, false, false, false, false, false, false, true},
  {35, "Display Rotate",   0,    3,   1, 0, true, false, false, false, false, false, true},
  {40, "Archive Log",      0,    8,   1, 0, true, false, false, false, false, false, false, false, false, false, true}
}

local selected = 1
local scroll = 1
local editing = false
local connected = false
local lastRx = 0
local nextRequest = 0
local requestIndex = 1
local nextGainRequest = 0
local nextCalibrationRequest = 0
local nextArchiveRequest = 0
local pushFailed = 0
local pushedThisFrame = false

local function push(command, payload)
  if not crossfireTelemetryPush or pushedThisFrame then return false end
  pushedThisFrame = true
  if crossfireTelemetryPush(command, payload) then return true end
  pushFailed = getTime()
  return false
end

local function readInt32(data, index)
  local value = data[index] * 16777216
              + data[index + 1] * 65536
              + data[index + 2] * 256
              + data[index + 3]
  if value >= 2147483648 then value = value - 4294967296 end
  return value
end

local function int32Bytes(value)
  if value < 0 then value = value + 4294967296 end
  local b1 = math.floor(value / 16777216) % 256
  local b2 = math.floor(value / 65536) % 256
  local b3 = math.floor(value / 256) % 256
  local b4 = value % 256
  return b1, b2, b3, b4
end

local function requestField(field)
  if field == nil then return false end
  return push(0x2C, {DEVICE, RADIO, field[1], 0})
end

local function writeField(field)
  if field.value == nil then return end
  if field[7] then
    push(0x2D, {DEVICE, RADIO, field[1], field.value})
  else
    local b1, b2, b3, b4 = int32Bytes(field.value)
    push(0x2D, {DEVICE, RADIO, field[1], b1, b2, b3, b4})
  end
end

local function findField(id)
  for i = 1, #fields do
    if fields[i][1] == id then return fields[i] end
  end
  return nil
end

local function consumeTelemetry()
  if not crossfireTelemetryPop then return end
  while true do
    local command, data = crossfireTelemetryPop()
    if command == nil then break end

    if command == 0x2B and #data >= 7 and data[1] == RADIO and data[2] == DEVICE then
      local field = findField(data[3])
      if field then
        local keepValue = editing and field == fields[selected]
        local dataType = data[6]
        local index = 7
        local name = ""
        while index <= #data and data[index] ~= 0 do
          name = name .. string.char(data[index])
          index = index + 1
        end
        if #name > 0 then field[2] = name end
        index = index + 1

        if dataType == 0x08 and index + 3 <= #data then
          if not keepValue then field.value = readInt32(data, index) end
          if index + 20 <= #data then
            field[3] = readInt32(data, index + 4)
            field[4] = readInt32(data, index + 8)
            field[6] = data[index + 16]
            field[5] = readInt32(data, index + 17)
            field[7] = false
          end
        elseif dataType == 0x09 then
          while index <= #data and data[index] ~= 0 do index = index + 1 end
          index = index + 1
          if index <= #data and not keepValue then field.value = data[index] end
          if index + 2 <= #data then
            field[3] = data[index + 1]
            field[4] = data[index + 2]
            field[5] = 1
            field[6] = 0
            field[7] = true
          end
        end
        connected = true
        lastRx = getTime()
      end
    elseif command == 0x2D and #data >= 4 and data[1] == RADIO and data[2] == DEVICE then
      local field = findField(data[3])
      if field then
        local keepValue = editing and field == fields[selected]
        if field[7] then
          if not keepValue then field.value = data[4] end
        elseif #data >= 7 and not keepValue then
          field.value = readInt32(data, 4)
        end
        connected = true
        lastRx = getTime()
      end
    end
  end
end

local function valueText(field)
  if field.value == nil then return "---" end
  if field[17] then
    local states = {"PARK CAR", "PRESS", "SAVING", "SAVED", "NO LOG", "NO SPACE", "FAILED", "CANCELLED", "UNAVAILABLE"}
    return states[field.value + 1] or "---"
  end
  if field[10] then
    if field.value == 2 then return "YES" end
    if field.value == 1 then return "PARTIAL" end
    return "NO"
  end
  if field[11] then return "PRESS" end
  if field[12] then
    if field.value == 2 then return "OFF" end
    if field.value == 1 then return "120 Hz" end
    return "24 Hz"
  end
  if field[13] then
    if field[4] == 1 then return field.value == 0 and "NORMAL" or "180 DEG" end
    if field.value == 1 then return "90 CW" end
    if field.value == 2 then return "180 DEG" end
    if field.value == 3 then return "90 CCW" end
    return "0 DEG"
  end
  if field[14] then return field.value == 0 and "1/10" or "MICRO" end
  if field[16] then
    if field.value == 2 then return "333 Hz" end
    if field.value == 1 then return "250 Hz" end
    return "50 Hz"
  end
  if field[8] then
    if field[4] == 0 then return "RES" end
    return field.value == 0 and "OFF" or "CH" .. tostring(field.value)
  end
  if field[9] then return field.value == 0 and "250 Hz" or "333 Hz" end
  if field[7] then return field.value == 0 and "OFF" or "ON" end
  local decimals = field[6]
  if decimals == 0 then return tostring(field.value) end
  local scale = 10 ^ decimals
  return string.format("%." .. decimals .. "f", field.value / scale)
end

local function moveSelection(step)
  selected = math.max(1, math.min(#fields, selected + step))
  if selected < scroll then scroll = selected end
  if selected > scroll + 3 then scroll = selected - 3 end
  requestField(fields[selected])
end

local function adjust(step)
  local field = fields[selected]
  if field[10] or field[11] or field[15] or field[17] then return end
  if field.value == nil then return end
  field.value = math.max(field[3], math.min(field[4], field.value + step * field[5]))
  writeField(field)
end

local function init()
  for i = 1, #fields do fields[i].value = nil end
  requestIndex = 1
  nextRequest = 0
  nextGainRequest = 0
  nextCalibrationRequest = 0
  nextArchiveRequest = 0
  pushFailed = 0
end

local function run(event)
  consumeTelemetry()
  pushedThisFrame = false

  local now = getTime()
  if now - lastRx > 200 then connected = false end

  local right = event == EVT_ROT_RIGHT or event == EVT_VIRTUAL_NEXT
  local left = event == EVT_ROT_LEFT or event == EVT_VIRTUAL_PREV
  local enter = event == EVT_ENTER_BREAK or event == EVT_VIRTUAL_ENTER
  local back = (EVT_EXIT_BREAK ~= nil and event == EVT_EXIT_BREAK)
            or (EVT_VIRTUAL_EXIT ~= nil and event == EVT_VIRTUAL_EXIT)

  if back then
    if editing then
      editing = false
      requestField(fields[selected])
    else
      return 2
    end
  elseif enter then
    local field = fields[selected]
    if field[17] then
      -- READY starts the first archive. SAVED permits replacing a log that
      -- survived a reboot; the firmware still refuses unless the car is parked.
      if field.value == 1 or field.value == 3 then
        local status = field.value
        field.value = 1
        writeField(field)
        field.value = status
        nextArchiveRequest = 0
      end
    elseif field[11] then
      field.value = 1
      writeField(field)
      field.value = 0
      nextCalibrationRequest = 0
    elseif not field[10] and not field[15] then
      editing = not editing
      if not editing then requestField(field) end
    end
  elseif right then
    if editing then adjust(1) else moveSelection(1) end
  elseif left then
    if editing then adjust(-1) else moveSelection(-1) end
  end

  if now >= nextCalibrationRequest and requestField(findField(27)) then
    nextCalibrationRequest = now + 25
  end

  if now >= nextGainRequest and requestField(findField(37)) then
    nextGainRequest = now + 25
  end

  if now >= nextArchiveRequest and requestField(findField(40)) then
    nextArchiveRequest = now + 25
  end

  if now >= nextRequest and requestField(fields[requestIndex]) then
    requestIndex = requestIndex + 1
    if requestIndex > #fields then requestIndex = 1 end
    nextRequest = now + 15
  end

  lcd.clear()
  lcd.drawText(1, 0, "OpenDrift CRSF", INVERS)
  local linkText = connected and "LINK" or "WAIT"
  if pushFailed ~= 0 and now - pushFailed < 50 then linkText = "BUSY" end
  lcd.drawText(127, 0, linkText, RIGHT + INVERS)
  if fields[selected][11] then
    lcd.drawText(1, 10, "HOLD POSITION + ENTER", 0)
  else
    local calibration = findField(27)
    local calibrationText = "END: ---"
    if calibration and calibration.value == 2 then calibrationText = "END: YES"
    elseif calibration and calibration.value == 1 then calibrationText = "END: PART"
    elseif calibration and calibration.value == 0 then calibrationText = "END: NO" end
    lcd.drawText(1, 10, calibrationText, 0)
    lcd.drawText(127, 10, "CH3 GAIN", RIGHT)
  end

  for row = 0, 3 do
    local index = scroll + row
    if index <= #fields then
      local field = fields[index]
      local flags = index == selected and INVERS or 0
      if editing and index == selected then flags = flags + BLINK end
      lcd.drawText(1, 21 + row * 10, field[2], flags)
      lcd.drawText(127, 21 + row * 10, valueText(field), RIGHT + flags)
    end
  end

  return 0
end

return {init=init, run=run}
