-- OpenDrift CRSF tuning tool for EdgeTX monochrome radios (including MT12).

local DEVICE = 0xC8
local RADIO = 0xEA

-- Fields are discovered from the firmware's CRSF parameter catalog. Only
-- action/read-only presentation remains ID-specific; normal parameters need
-- no Lua edit when their catalog definition changes.
local fields = {}

-- Presentation only: permanent IDs stay unchanged, and all values/limits are
-- still discovered from firmware. Match the web configurator's section order.
-- Unrecognized future parameters remain accessible in Other Settings.
local groups = {
  {"Drive & Limits", {1, 37, 2, 3, 16}},
  {"Response", {4, 32, 10, 26, 36}},
  {"Transition/PCA", {9, 38}},
  {"Drift Assist", {8, 7, 5, 6}},
  {"Servo", {15, 25, 39, 14, 13, 11, 12}},
  {"Display", {35}},
  {"Endpoints", {27, 28, 29, 30, 31}},
  {"CH3 Gain Range", {33, 34}},
  {"Aux Outputs", {17, 18, 19, 20, 21, 22, 23, 24}},
  {"Blackbox", {40}},
  {"Diagnostics", {42}},
  {"System / USB", {43}},
  {"Other Settings", {}}
}

local function presentation(id)
  for groupIndex, group in ipairs(groups) do
    for order, parameterId in ipairs(group[2]) do
      if parameterId == id then return groupIndex, order end
    end
  end
  return #groups, id
end

local function sortFields()
  -- EdgeTX's restricted Lua runtime may omit the table library. Discovery
  -- lists are small; insertion sort needs only ordinary array operations.
  for index = 2, #fields do
    local field = fields[index]
    local group, order = presentation(field[1])
    local previous = index - 1
    while previous >= 1 do
      local previousGroup, previousOrder = presentation(fields[previous][1])
      if previousGroup < group or
          (previousGroup == group and previousOrder <= order) then break end
      fields[previous + 1] = fields[previous]
      previous = previous - 1
    end
    fields[previous + 1] = field
  end
end

local function newField(id)
  local field = {id, "Parameter " .. tostring(id), 0, 0, 1, 0, false}
  if id == 27 then field[10] = true end                 -- derived status
  if id >= 28 and id <= 31 then field[11] = true end   -- calibration action
  if id == 37 then field[15] = true end                -- derived live gain
  if id == 40 then field[17] = true end                -- archive action/status
  if id == 42 then field[18] = true end                -- diagnostics action/status
  if id == 43 then field[19] = true end                -- USB maintenance reboot
  return field
end

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
local nextDiagnosticsRequest = 0
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
      local parameterId = data[3]
      if parameterId == 0 then
        local index = 7
        while index <= #data and data[index] ~= 0 do index = index + 1 end
        index = index + 1
        fields = {}
        while index <= #data and data[index] ~= 0xFF do
          fields[#fields + 1] = newField(data[index])
          index = index + 1
        end
        sortFields()
        selected = 1
        scroll = 1
        requestIndex = 1
        connected = true
        lastRx = getTime()
      else
        local field = findField(parameterId)
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
          local choices = ""
          while index <= #data and data[index] ~= 0 do
            choices = choices .. string.char(data[index])
            index = index + 1
          end
          field.choices = {}
          for choice in string.gmatch(choices .. ";", "([^;]*);") do
            field.choices[#field.choices + 1] = choice
          end
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
  if field.choices then
    return field.choices[field.value + 1] or "---"
  end
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
  local group = presentation(fields[selected][1])
  local first = selected
  while first > 1 and presentation(fields[first - 1][1]) == group do
    first = first - 1
  end
  if scroll < first or presentation(fields[scroll][1]) ~= group then
    scroll = first
  end
  if selected < scroll then scroll = selected end
  if selected > scroll + 3 then scroll = selected - 3 end
  requestField(fields[selected])
end

local function adjust(step)
  local field = fields[selected]
  if field[10] or field[11] or field[15] or field[17] or field[18] or field[19] then return end
  if field.value == nil then return end
  field.value = math.max(field[3], math.min(field[4], field.value + step * field[5]))
  writeField(field)
end

local function init()
  fields = {}
  selected = 1
  scroll = 1
  requestIndex = 1
  nextRequest = 0
  nextGainRequest = 0
  nextCalibrationRequest = 0
  nextArchiveRequest = 0
  nextDiagnosticsRequest = 0
  pushFailed = 0
  requestField({0})
end

local function run(event)
  consumeTelemetry()
  pushedThisFrame = false

  local now = getTime()
  if now - lastRx > 200 then connected = false end

  if #fields == 0 then
    if now >= nextRequest and requestField({0}) then nextRequest = now + 50 end
    lcd.clear()
    lcd.drawText(1, 0, "OpenDrift CRSF", INVERS)
    lcd.drawText(64, 25, "DISCOVERING...", CENTER)
    return 0
  end

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
    elseif field[18] then
      -- READY or CAPTURED starts a fresh timing capture. Starting again
      -- intentionally replaces the previous RAM-only diagnostic capture.
      if field.value == 0 or field.value == 3 then
        local status = field.value
        field.value = 1
        writeField(field)
        field.value = status
        nextDiagnosticsRequest = 0
      end
    elseif field[19] then
      -- Firmware centers and detaches both outputs before rebooting into the
      -- isolated USB maintenance environment.
      field.value = 1
      writeField(field)
      field.value = 0
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

  if now >= nextDiagnosticsRequest and requestField(findField(42)) then
    nextDiagnosticsRequest = now + 25
  end

  if now >= nextRequest and requestField(fields[requestIndex]) then
    requestIndex = requestIndex + 1
    if requestIndex > #fields then requestIndex = 1 end
    nextRequest = now + 15
  end

  lcd.clear()
  local selectedGroup = presentation(fields[selected][1])
  lcd.drawText(1, 0, groups[selectedGroup][1], INVERS)
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
    local liveGain = findField(37)
    local liveGainText = liveGain and valueText(liveGain) or "---"
    lcd.drawText(127, 10, "GAIN " .. liveGainText, RIGHT)
  end

  for row = 0, 3 do
    local index = scroll + row
    if index <= #fields and presentation(fields[index][1]) == selectedGroup then
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
