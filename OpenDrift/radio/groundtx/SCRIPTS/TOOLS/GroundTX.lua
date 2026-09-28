-- GroundTX surface-radio UI prototype for EdgeTX monochrome radios.
-- This demo intentionally does not modify the active model.

local SCREEN_DASHBOARD = 1
local SCREEN_MENU = 2
local SCREEN_MIXES = 3
local SCREEN_4WS = 4
local SCREEN_PREVIEW = 5
local SCREEN_SOON = 6
local SCREEN_DRIVE = 7
local SCREEN_MODELS = 8
local SCREEN_RADIO = 9
local SCREEN_ABOUT = 10

local VERSION = "0.2 demo"

local screen = SCREEN_DASHBOARD
local selected = 1
local scroll = 1
local editing = false
local editOriginal = nil
local notice = nil
local noticeUntil = 0
local modelName = "TRAIL TRUCK"
local detailTitle = "Coming soon"
local detailLines = {}
local detailReturn = SCREEN_MENU

local config = {
  mode = 5,
  frontChannel = 1,
  rearChannel = 3,
  rearRate = 80,
  rearReverse = 1,
  control = 1
}

local modes = {"Front only", "Opposite", "Crab", "Rear only", "Selectable"}
local controls = {"SA", "SB", "SC", "SD", "TR1", "TR2"}

local drive = {
  steeringRate = 100,
  steeringExpo = 20,
  steeringTrim = 0,
  throttleRate = 100,
  brakeStrength = 70,
  abs = 0
}

local radio = {
  sound = 1,
  vibration = 1,
  backlight = 70
}

local mainMenu = {
  {label="Drive setup", target=SCREEN_DRIVE},
  {label="Mixes", target=SCREEN_MIXES},
  {label="Models", target=SCREEN_MODELS},
  {label="Radio", target=SCREEN_RADIO},
  {label="About / feedback", target=SCREEN_ABOUT}
}

local mixMenu = {
  {label="Crawler 4WS", target=SCREEN_4WS},
  {label="Dual ESC / Dig", target=SCREEN_SOON,
    lines={"Front + rear motors", "Dig and burn modes", "Adjustable balance"}},
  {label="Tank steering", target=SCREEN_SOON,
    lines={"Throttle + steering", "Dual ESC outputs", "Pivot-turn option"}},
  {label="Lights", target=SCREEN_SOON,
    lines={"Head and brake lights", "Turn signals", "Switch or automatic"}},
  {label="Winch", target=SCREEN_SOON,
    lines={"In / stop / out", "Momentary control", "Neutral failsafe"}}
}

local driveFields = {
  {label="Steering rate", table=drive, key="steeringRate", min=10, max=100, step=5, suffix="%"},
  {label="Steering expo", table=drive, key="steeringExpo", min=-100, max=100, step=5, suffix="%"},
  {label="Steering trim", table=drive, key="steeringTrim", min=-100, max=100, step=1},
  {label="Throttle rate", table=drive, key="throttleRate", min=10, max=100, step=5, suffix="%"},
  {label="Brake strength", table=drive, key="brakeStrength", min=0, max=100, step=5, suffix="%"},
  {label="ABS braking", table=drive, key="abs", min=0, max=1, values={"Off", "On"}},
  {label="Steering endpoints", action="endpoints"},
  {label="Throttle endpoints", action="endpoints"}
}

local modelMenu = {
  {label="TRAIL TRUCK", value="TRAIL TRUCK"},
  {label="SHORT COURSE", value="SHORT COURSE"},
  {label="CRAWLER", value="CRAWLER"},
  {label="BOAT", value="BOAT"},
  {label="TANK", value="TANK"},
  {label="+ New model", action="new"}
}

local radioFields = {
  {label="Sound", table=radio, key="sound", min=0, max=1, values={"Off", "On"}},
  {label="Vibration", table=radio, key="vibration", min=0, max=1, values={"Off", "On"}},
  {label="Backlight", table=radio, key="backlight", min=10, max=100, step=10, suffix="%"},
  {label="Controls calibration", action="calibration"},
  {label="RF and receiver", action="rf"},
  {label="Advanced EdgeTX", action="advanced"}
}

local fields4ws = {
  {label="Mode", key="mode", min=1, max=#modes, values=modes},
  {label="Front output", key="frontChannel", min=1, max=16, prefix="CH"},
  {label="Rear output", key="rearChannel", min=1, max=16, prefix="CH"},
  {label="Rear amount", key="rearRate", min=0, max=100, step=5, suffix="%"},
  {label="Rear reverse", key="rearReverse", min=0, max=1, values={"No", "Yes"}},
  {label="Mode control", key="control", min=1, max=#controls, values=controls},
  {label="Preview wheels", action="preview"},
  {label="Use this setup", action="apply"}
}

local function isRight(event)
  return event == EVT_ROT_RIGHT or event == EVT_VIRTUAL_NEXT
end

local function isLeft(event)
  return event == EVT_ROT_LEFT or event == EVT_VIRTUAL_PREV
end

local function isEnter(event)
  return event == EVT_ENTER_BREAK or event == EVT_VIRTUAL_ENTER
end

local function isExit(event)
  return event == EVT_EXIT_BREAK or event == EVT_VIRTUAL_EXIT
end

local function showNotice(message)
  notice = message
  noticeUntil = getTime() + 180
end

local function goTo(nextScreen)
  screen = nextScreen
  selected = 1
  scroll = 1
  editing = false
  editOriginal = nil
end

local function listMove(count, direction)
  selected = math.max(1, math.min(count, selected + direction))
  if selected < scroll then scroll = selected end
  if selected > scroll + 3 then scroll = selected - 3 end
end

local function drawHeader(title)
  lcd.drawFilledRectangle(0, 0, LCD_W, 9, FORCE)
  lcd.drawText(1, 0, title, INVERS)
  lcd.drawText(LCD_W - 1, 0, "DEMO", RIGHT + INVERS)
end

local function drawFooter(text)
  lcd.drawText(1, 57, text, SMLSIZE)
end

local function drawList(items)
  for row = 0, 3 do
    local index = scroll + row
    if index <= #items then
      local flags = index == selected and INVERS or 0
      lcd.drawText(3, 13 + row * 10, items[index].label, flags)
      if items[index].value == modelName then
        lcd.drawText(LCD_W - 2, 13 + row * 10, "ACTIVE", RIGHT + flags)
      else
        lcd.drawText(LCD_W - 2, 13 + row * 10, ">", RIGHT + flags)
      end
    end
  end
end

local function fieldText(field)
  if field.action then return ">" end
  local values = field.table or config
  local value = values[field.key]
  if field.values then return field.values[value] end
  return (field.prefix or "") .. tostring(value) .. (field.suffix or "")
end

local function drawDashboard()
  drawHeader("GroundTX")
  lcd.drawText(2, 12, modelName, MIDSIZE)
  lcd.drawText(2, 29, "Steer", 0)
  lcd.drawText(46, 29, "100%", 0)
  lcd.drawText(78, 29, "4WS", 0)
  lcd.drawText(LCD_W - 2, 29, modes[config.mode], RIGHT)
  lcd.drawText(2, 41, "Radio", 0)
  lcd.drawText(46, 41, "8.1V", 0)
  lcd.drawText(78, 41, "Link", 0)
  lcd.drawText(LCD_W - 2, 41, "---", RIGHT)
  drawFooter("ENTER Menu")
end

local function drawFields(title, fields)
  drawHeader(title)
  for row = 0, 3 do
    local index = scroll + row
    if index <= #fields then
      local field = fields[index]
      local flags = index == selected and INVERS or 0
      if editing and index == selected then flags = flags + BLINK end
      lcd.drawText(2, 13 + row * 10, field.label, flags)
      lcd.drawText(LCD_W - 2, 13 + row * 10, fieldText(field), RIGHT + flags)
    end
  end
  drawFooter(editing and "TURN Change  ENTER Done" or "ENTER Select  RTN Back")
end

local function draw4ws()
  drawHeader("Crawler 4WS")
  for row = 0, 3 do
    local index = scroll + row
    if index <= #fields4ws then
      local field = fields4ws[index]
      local flags = index == selected and INVERS or 0
      if editing and index == selected then flags = flags + BLINK end
      lcd.drawText(2, 13 + row * 10, field.label, flags)
      lcd.drawText(LCD_W - 2, 13 + row * 10, fieldText(field), RIGHT + flags)
    end
  end
  drawFooter(editing and "TURN Change  ENTER Done" or "ENTER Select  RTN Back")
end

local function wheelLine(x, y, slant)
  if slant < 0 then
    lcd.drawLine(x + 3, y, x, y + 8, SOLID, FORCE)
  elseif slant > 0 then
    lcd.drawLine(x, y, x + 3, y + 8, SOLID, FORCE)
  else
    lcd.drawLine(x + 1, y, x + 1, y + 8, SOLID, FORCE)
  end
end

local function previewSlants(mode)
  if mode == 1 then return -1, -1, 0, 0 end
  if mode == 2 then return -1, -1, 1, 1 end
  if mode == 3 then return -1, -1, -1, -1 end
  if mode == 4 then return 0, 0, -1, -1 end
  return -1, -1, 1, 1
end

local function drawPreview()
  drawHeader("4WS Preview")
  local fl, fr, rl, rr = previewSlants(config.mode)
  lcd.drawRectangle(42, 15, 44, 34, FORCE)
  wheelLine(34, 17, fl)
  wheelLine(91, 17, fr)
  wheelLine(34, 38, rl)
  wheelLine(91, 38, rr)
  lcd.drawText(64, 24, modes[config.mode], CENTERED)
  drawFooter("TURN Mode  RTN Back")
end

local function drawSoon()
  drawHeader(detailTitle)
  for i = 1, math.min(3, #detailLines) do
    lcd.drawText(64, 12 + (i - 1) * 12, detailLines[i], CENTERED)
  end
  lcd.drawText(64, 48, "CONCEPT PREVIEW", CENTERED + SMLSIZE)
  drawFooter("RTN Back")
end

local function drawAbout()
  drawHeader("About GroundTX")
  lcd.drawText(2, 13, "Version", 0)
  lcd.drawText(LCD_W - 2, 13, VERSION, RIGHT)
  lcd.drawText(2, 25, "Surface-first EdgeTX", 0)
  lcd.drawText(2, 37, "Share it. Test it.", 0)
  lcd.drawText(2, 47, "Tell us what is hard.", SMLSIZE)
  drawFooter("RTN Back")
end

local function drawNotice()
  if notice and getTime() < noticeUntil then
    lcd.drawFilledRectangle(6, 19, LCD_W - 12, 25, ERASE)
    lcd.drawRectangle(6, 19, LCD_W - 12, 25, FORCE)
    lcd.drawText(64, 23, notice, CENTERED + INVERS)
  else
    notice = nil
  end
end

local function openDetail(item)
  detailTitle = item.label
  detailLines = item.lines or {"This workflow is", "planned for a", "future prototype."}
  detailReturn = screen
  goTo(SCREEN_SOON)
end

local function handleList(event, items, parent)
  if isRight(event) then
    listMove(#items, 1)
  elseif isLeft(event) then
    listMove(#items, -1)
  elseif isEnter(event) then
    local item = items[selected]
    if item.target == SCREEN_SOON then openDetail(item) else goTo(item.target) end
  elseif isExit(event) then
    goTo(parent)
  end
end


local function handleFields(event, fields, parent)
  local field = fields[selected]
  local values = field.table or config

  if editing then
    if isRight(event) or isLeft(event) then
      local direction = isRight(event) and 1 or -1
      values[field.key] = math.max(field.min,
        math.min(field.max, values[field.key] + direction * (field.step or 1)))
    elseif isEnter(event) then
      editing = false
      editOriginal = nil
    elseif isExit(event) then
      values[field.key] = editOriginal
      editing = false
      editOriginal = nil
    end
    return
  end

  if isRight(event) then
    listMove(#fields, 1)
  elseif isLeft(event) then
    listMove(#fields, -1)
  elseif isEnter(event) then
    if field.action then
      openDetail({label=field.label, lines={"Guided setup with", "safe test mode", "will live here."}})
    else
      editing = true
      editOriginal = values[field.key]
    end
  elseif isExit(event) then
    goTo(parent)
  end
end

local function handleModels(event)
  if isRight(event) then
    listMove(#modelMenu, 1)
  elseif isLeft(event) then
    listMove(#modelMenu, -1)
  elseif isEnter(event) then
    local item = modelMenu[selected]
    if item.action then
      openDetail({label="New model", lines={"Car / crawler / boat", "guided templates", "planned next."}})
    else
      modelName = item.value
      showNotice("DEMO MODEL SELECTED")
    end
  elseif isExit(event) then
    goTo(SCREEN_MENU)
  end
end

local function handle4ws(event)
  local field = fields4ws[selected]

  if editing then
    if isRight(event) or isLeft(event) then
      local direction = isRight(event) and 1 or -1
      local step = field.step or 1
      config[field.key] = math.max(field.min,
        math.min(field.max, config[field.key] + direction * step))
    elseif isEnter(event) then
      editing = false
      editOriginal = nil
    elseif isExit(event) then
      config[field.key] = editOriginal
      editing = false
      editOriginal = nil
    end
    return
  end

  if isRight(event) then
    listMove(#fields4ws, 1)
  elseif isLeft(event) then
    listMove(#fields4ws, -1)
  elseif isEnter(event) then
    if field.action == "preview" then
      goTo(SCREEN_PREVIEW)
    elseif field.action == "apply" then
      showNotice("DEMO - NOT SAVED")
    else
      editing = true
      editOriginal = config[field.key]
    end
  elseif isExit(event) then
    goTo(SCREEN_MIXES)
  end
end

local function run(event)
  if screen == SCREEN_DASHBOARD then
    if isEnter(event) then goTo(SCREEN_MENU) end
    if isExit(event) then return 1 end
  elseif screen == SCREEN_MENU then
    handleList(event, mainMenu, SCREEN_DASHBOARD)
  elseif screen == SCREEN_MIXES then
    handleList(event, mixMenu, SCREEN_MENU)
  elseif screen == SCREEN_4WS then
    handle4ws(event)
  elseif screen == SCREEN_DRIVE then
    handleFields(event, driveFields, SCREEN_MENU)
  elseif screen == SCREEN_MODELS then
    handleModels(event)
  elseif screen == SCREEN_RADIO then
    handleFields(event, radioFields, SCREEN_MENU)
  elseif screen == SCREEN_ABOUT then
    if isExit(event) or isEnter(event) then goTo(SCREEN_MENU) end
  elseif screen == SCREEN_PREVIEW then
    if isRight(event) then
      config.mode = config.mode == #modes and 1 or config.mode + 1
    elseif isLeft(event) then
      config.mode = config.mode == 1 and #modes or config.mode - 1
    elseif isExit(event) or isEnter(event) then
      goTo(SCREEN_4WS)
    end
  elseif screen == SCREEN_SOON then
    if isExit(event) or isEnter(event) then goTo(detailReturn) end
  end

  lcd.clear()
  if screen == SCREEN_DASHBOARD then
    drawDashboard()
  elseif screen == SCREEN_MENU then
    drawHeader("Main menu")
    drawList(mainMenu)
    drawFooter("ENTER Open  RTN Home")
  elseif screen == SCREEN_MIXES then
    drawHeader("Mix presets")
    drawList(mixMenu)
    drawFooter("ENTER Open  RTN Back")
  elseif screen == SCREEN_4WS then
    draw4ws()
  elseif screen == SCREEN_DRIVE then
    drawFields("Drive setup", driveFields)
  elseif screen == SCREEN_MODELS then
    drawHeader("Models")
    drawList(modelMenu)
    drawFooter("ENTER Demo select  RTN Back")
  elseif screen == SCREEN_RADIO then
    drawFields("Radio", radioFields)
  elseif screen == SCREEN_ABOUT then
    drawAbout()
  elseif screen == SCREEN_PREVIEW then
    drawPreview()
  else
    drawSoon()
  end
  drawNotice()
  return 0
end

local function init()
  goTo(SCREEN_DASHBOARD)
  notice = nil
end

return {init=init, run=run}
