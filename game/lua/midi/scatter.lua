-- Play random notes


namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"
local selector = require "midi.support.ent.selector"

local ScatterMidi = classNamed("ScatterMidi", ui2.ScreenEnt)

function ScatterMidi:findDevice()
	self.device = nil
	for i,v in ipairs(self.devices) do
		--if not stringx.startswith(v.name, "IAC Driver Bus") then
			self.device = i
		--end
	end
end

function ScatterMidi:allOff()
	for _, v in ipairs(self.notes) do
		midi.note(self.device, false, v)
	end
	self.notes = {}
end

function ScatterMidi:onLoad()
	self.devices = midi.devices()
	self:findDevice()
	self.notes = {}
	self.next = 0
	self.t = 0

	-- UI
	ui2.routeMouse()
	local off = ui2.ButtonEnt{label="Off", onButton = function() self:allOff() end}
	self.freeze = ui2.ToggleEnt{label="Freeze"}

	local layout = ui2.PileLayout{managed={off, self.freeze}, parent=self}
	layout:layout()
end

function ScatterMidi:onUpdate(dt)
	if not self.freeze.down then
		self.t = self.t + dt
	end
	if midi.updateDevices() then self.devices = midi.devices(true) self:findDevice() end
	if self.device and self.t > self.next then
		self.next = self.next + lovr.math.random()
		if self.next < self.t then self.next = self.t end

		local noten = #self.notes
		if noten < 1 or lovr.math.random() > 0.5 then
			local note = lovr.math.random(0, 127)
			local collide = false
			for i,v in ipairs(self.notes) do if v == note then collide=true break end end
			if collide then midi.note(self.device, false, note)
			else table.insert(self.notes, note)
			end
			midi.note(self.device, true, note, lovr.math.random(0x40, 0x7F))
		else
			local drop = lovr.math.random(1, noten)
			local dropped = self.notes[drop]
			for i=drop,noten do self.notes[i] = self.notes[i+1] end
			midi.note(self.device, false, dropped)
		end
	end
end

function ScatterMidi:onMirror()
	ui2.ScreenEnt.onMirror(self)
	lovr.graphics.setFont(flat.font)

	local str
	if not self.device then
		str = "No usable devices"
	else
		str = self.devices[self.device].name .. "\n\n"
		for _,v in ipairs(self.notes) do
			str = str .. v .. " "
		end
	end
	lovr.graphics.print(str, -0.8, 0.8, 0, flat.fontscale * 2, 0,0,0,0,0, 'left', 'top')
end

return ScatterMidi
