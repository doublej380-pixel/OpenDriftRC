-- OpenDrift CRSF tuning tool for EdgeTX monochrome radios (including MT12).

local DEVICE = 0xC8
local RADIO = 0xEA

local fields = {
  { 1, "Active Gain",     50,  500,   5, 2 },
  { 2, "Deadband",         0,  200,   1, 1 },
  { 3, "Max Correction",   0, 1000,  10, 0 },
  { 4, "Smoothing",        1,  100,   1, 2 },
  { 5, "Drift Memory",     0, 2000,   1, 2 },
  { 6, "Memory Limit",     0,  500,   5, 0 },
  { 7, "Hold Assist",      0,  100,   1, 0 },
  { 8, "Countersteer",     0,  100,   1, 0 },
  { 9, "Tail Slide Speed", 0,  100,   1, 0 },
  {10, "Prediction",       0,  100,   1, 0 },
  {11, "Servo Quiet",      0,   50,   1, 0 },
  {12, "Steering Travel",  0,  100,   1, 0 },
  {13, "Servo Travel",    10,  150,   1, 0 },
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
  {24, "GPIO 8 Output",    0,   16,   1, 0, true, true}
}

local selected = 1
local scroll = 1
local editing = false
local connected = false
local lastRx = 0
local nextRequest = 0
local requestIndex = 2
local nextGainRequest = 0

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
  if crossfireTelemetryPush then
    crossfireTelemetryPush(0x2C, {DEVICE, RADIO, field[1], 0})
  end
end

local function writeField(field)
  if not crossfireTelemetryPush or field.value == nil then return end
  if field[7] then
    crossfireTelemetryPush(0x2D, {DEVICE, RADIO, field[1], field.value})
  else
    local b1, b2, b3, b4 = int32Bytes(field.value)
    crossfireTelemetryPush(0x2D, {DEVICE, RADIO, field[1], b1, b2, b3, b4})
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
        local dataType = data[6]
        local index = 7
        while index <= #data and data[index] ~= 0 do index = index + 1 end
        index = index + 1

        if dataType == 0x08 and index + 3 <= #data then
          field.value = readInt32(data, index)
        elseif dataType == 0x09 then
          while index <= #data and data[index] ~= 0 do index = index + 1 end
          index = index + 1
          if index <= #data then field.value = data[index] end
          if index + 2 <= #data then
            field[3] = data[index + 1]
            field[4] = data[index + 2]
          end
        end
        connected = true
        lastRx = getTime()
      end
    elseif command == 0x2D and #data >= 4 and data[1] == RADIO and data[2] == DEVICE then
      local field = findField(data[3])
      if field then
        if field[7] then
          field.value = data[4]
        elseif #data >= 7 then
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
  if field[8] then
    if field[4] == 0 then return "RES" end
    return field.value == 0 and "OFF" or "CH" .. tostring(field.value)
  end
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
  if field.value == nil then return end
  field.value = math.max(field[3], math.min(field[4], field.value + step * field[5]))
  writeField(field)
end

local function init()
  for i = 1, #fields do fields[i].value = nil end
  requestIndex = 2
  nextRequest = 0
  nextGainRequest = 0
end

local function run(event)
  consumeTelemetry()

  local now = getTime()
  if now - lastRx > 200 then connected = false end

  if now >= nextRequest then
    requestField(fields[requestIndex])
    requestIndex = requestIndex + 1
    if requestIndex > #fields then requestIndex = 2 end
    nextRequest = now + 15
  end

  -- Keep the displayed gain following channel 3 instead of waiting for a
  -- complete parameter-list polling cycle.
  if now >= nextGainRequest then
    requestField(fields[1])
    nextGainRequest = now + 25
  end

  local right = event == EVT_ROT_RIGHT or event == EVT_VIRTUAL_NEXT
  local left = event == EVT_ROT_LEFT or event == EVT_VIRTUAL_PREV
  local enter = event == EVT_ENTER_BREAK or event == EVT_VIRTUAL_ENTER

  if enter then
    editing = not editing
    if not editing then requestField(fields[selected]) end
  elseif right then
    if editing then adjust(1) else moveSelection(1) end
  elseif left then
    if editing then adjust(-1) else moveSelection(-1) end
  end

  lcd.clear()
  lcd.drawText(1, 0, "OpenDrift CRSF", INVERS)
  lcd.drawText(127, 0, connected and "LINK" or "WAIT", RIGHT + INVERS)
  lcd.drawText(1, 10, "CH3 OVERRIDES GAIN", 0)

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
