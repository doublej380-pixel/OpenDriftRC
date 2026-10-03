-- Run from the firmware directory: lua test/lua/menu_groups.lua
-- Match the radio sandbox: desktop Lua normally provides this library,
-- masking compatibility errors in menu discovery.
table = nil
INVERS, RIGHT, CENTER, BLINK = 1, 2, 4, 8
EVT_ROT_RIGHT, EVT_ROT_LEFT, EVT_ENTER_BREAK, EVT_EXIT_BREAK = 101, 102, 103, 104
local now, incoming, drawn, sent = 0, nil, {}, {}
function getTime() return now end
function crossfireTelemetryPush(command, payload)
  sent[#sent + 1] = {command, payload}
  return true
end
function crossfireTelemetryPop()
  local data = incoming
  incoming = nil
  if data then return 0x2B, data end
end
lcd = {
  clear = function() drawn = {} end,
  drawText = function(x, y, text, flags)
    drawn[#drawn + 1] = {x=x, y=y, text=text, flags=flags}
  end
}
local tool = dofile("radio/edgetx/SCRIPTS/TOOLS/OpenDrift.lua")
local function tick(event)
  now = now + 20
  tool.run(event or 0)
end
local function selectedId()
  for _, item in ipairs(drawn) do
    if item.x == 1 and item.y >= 21 and item.flags == INVERS then
      return tonumber(item.text:match("Parameter (%d+)"))
    end
  end
end
local function discover(ids)
  incoming = {0xEA, 0xC8, 0, 0, 0, 0, 82, 79, 79, 84, 0}
  for _, id in ipairs(ids) do incoming[#incoming + 1] = id end
  incoming[#incoming + 1] = 0xFF
  tick()
end
tool.init()
local ids = {}
for id = 1, 43 do if id ~= 41 then ids[#ids + 1] = id end end
ids[#ids + 1] = 44 -- Future fields must remain reachable.
discover(ids)
local expected = {
  1,37,2,3,16, 4,32,10,26,36, 9,38, 8,7,5,6,
  15,25,39,14,13,11,12, 35, 27,28,29,30,31,
  33,34, 17,18,19,20,21,22,23,24, 40,42,43,44
}
for index, id in ipairs(expected) do
  assert(selectedId() == id, "Forward selection " .. index)
  -- Live gain/calibration stay visible regardless of the selected group.
  if id >= 28 and id <= 31 then
    assert(drawn[3].text == "HOLD POSITION + ENTER")
  else
    assert(drawn[3].text == "END: ---")
    assert(drawn[4].text == "GAIN ---")
  end
  tick(EVT_ROT_RIGHT)
end
assert(drawn[1].text == "Other Settings")
for index = #expected, 1, -1 do
  assert(selectedId() == expected[index], "Backward selection " .. index)
  tick(EVT_ROT_LEFT)
end
-- Board-filtered root lists are supported; unavailable groups add no blanks.
discover({43, 32, 1, 26})
assert(selectedId() == 1 and drawn[1].text == "Drive & Limits")
tick(EVT_ROT_RIGHT)
assert(selectedId() == 32 and drawn[1].text == "Response")
tick(EVT_ROT_RIGHT)
assert(selectedId() == 26)
tick(EVT_ROT_RIGHT)
assert(selectedId() == 43 and drawn[1].text == "System / USB")
print("Lua menu grouping tests passed")
