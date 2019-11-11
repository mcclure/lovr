-- Piano interface. Click to play a note

namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"
local selector = require "midi.support.ent.selector"

local PianoHud = classNamed("PianoHud", ui2.ScreenEnt)

function PianoHud:device()
	return self.outSelect.selected
end

local ul = vec2(-flat.xspan, flat.yspan)
local root = vec2(0.25, -0.25)
local keyheight = 1
local keywidth = 1/4
local blackmargin = 1/3
local blackheight = keyheight-blackmargin
local blackwidth= keywidth * (13.7/23.5)
local keyspan = vec2(keywidth, -keyheight)
local blackspan = vec2(blackwidth, -blackheight)
local scale = {2,2,1,2,2,2,1}
local linerad = 8/flat.pixheight

local function corner(b, x)
	x = x%4
	if x == 0 then
		return ui2.bl(b)
	elseif x == 1 then
		return ui2.tl(b)
	elseif x == 2 then
		return ui2.tr(b)
	elseif x == 3 then
		return ui2.br(b)
	end
end

local function ipairsReverseIter(t, i)
	i = i - 1
	if i > 0 then
		return i, t[i]
	end
end

local function ipairsReverse(t)
	return ipairsReverseIter, t, #t+1
end

local function addLines(t)
	local b = t.bound
	local lines = {}
	local lineCalls = {}
	do
		local x,y = b:center():unpack()
		local w,h = b:size():unpack()
		t.call = {x,y,0,w,h}
	end
	t.lines = lines
	t.lineCalls = lineCalls
	for i=1,4 do
		local f,t = corner(b,i), corner(b, i+1)
		local bl = bound2.at(f, t):outset(vec2(linerad, linerad))
		table.insert(lines, bl)
		local x,y = bl:center():unpack()
		local w,h = bl:size():unpack()
		table.insert(lineCalls, {x,y,0,w,h})
	end
end

function PianoHud:onLoad()
	self.outSelect = selector.SelectorEnt{filter="out", labelPrefix="Midi Out: "}

	local constructAt = ul + root
	self.keys = {}
	local semitone = 0
	for i,v in ipairs(scale) do
		table.insert(self.keys, {note=semitone, bound=bound2.at(constructAt, constructAt + keyspan)})
		semitone = semitone + 1
		if v == 2 then
			local blackat = constructAt + vec2(keywidth-blackwidth/2, 0)
			table.insert(self.keys, {note=semitone, bound=bound2.at(blackat, blackat + blackspan), black=true})
			semitone = semitone + 1
		end
		constructAt = constructAt + vec2(keywidth, 0)
	end
	for _,t in ipairs(self.keys) do
		addLines(t)
	end
	table.sort(self.keys, function(a,b) return b.black and not a.black end)

	-- UI
	local ents = {self.outSelect}
	ui2.routeMouse()
	local layout = ui2.PileLayout{managed=ents, parent=self, mutable=true}
	layout:layout()
end

function PianoHud:onUpdate(dt)
	
end

function PianoHud:onMirror()
	ui2.ScreenEnt.onMirror(self)
	
	for _,v in ipairs(self.keys) do
		if v.down then
			lovr.graphics.setColor(0.5,0.5,0.5)
		elseif v.black then
			lovr.graphics.setColor(0,0,0)	
		else
			lovr.graphics.setColor(0.1,0.1,0.1)
		end
		lovr.graphics.plane('fill', unpack(v.call))
		lovr.graphics.setColor(1,1,1)
		for _2,l in ipairs(v.lineCalls) do
			lovr.graphics.plane('fill', unpack(l))
		end
	end
end

function PianoHud:toMidiNote(semitone)
	return 60+semitone
end

function PianoHud:onPress(at)
	for i,v in ipairsReverse(self.keys) do
		if v.bound:contains(at) then
			v.down = true
			self.keyClicked = i
			midi.note(self:device(), true, self:toMidiNote(v.note))
			break
		end
	end
end

function PianoHud:onRelease()
	if self.keyClicked then
		local v = self.keys[self.keyClicked]
		midi.note(self:device(), false, self:toMidiNote(v.note))
		v.down = false
		self.keyClicked = nil
	end
end

return PianoHud
