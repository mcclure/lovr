-- Input monitor-- prints "current state" of one device (ie which notes are down) to screen

namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"
local midiEnt = require "midi.support.ent"
local selector = require "midi.support.ent.selector"

local midiOn = 0x90 -- 144
local midiOff = 0x80 -- 128

local TestMidi = classNamed("TestMidi", ui2.ScreenEnt)

function TestMidi:onLoad()
	self.down = {}
	self.strings = {}
	self.deadStrings = {}
	self.pair = selector.PairEnt{}:insert(self)
	self.console = midiEnt.ConsoleEnt{}:insert(self)
end

local gray = {0.5, 0.5, 0.5, 0.5}
local DELETEME = 0
function TestMidi:onUpdate()
	local from = self.pair:inDevice()
	local to = self.pair:outDevice()

	if from then
		self.console:clear()
		for _,msg in ipairs(midi.recv(from)) do
			local midiNote = msg[1] == midiOn or msg[1] == midiOff
			if midiNote then
				if msg[1] == midiOn and msg[3] > 0 then
					self.down[msg[2]] = msg[3]
				else
					local n = msg[2]
					if self.strings[n] then
						table.insert(self.deadStrings, self.strings[n])
						self.strings[n] = nil
					end
					self.down[n] = nil
				end
			end

			if to then midi.send(to, msg) end
		end

		local count = 0
		for note,velocity in tablex.sort(self.down) do
			local s = string.format("%d: %d .. %d", note, velocity, DELETEME)
			self.strings[note] = s
			self.console:push(s)
			count = count + 1
			DELETEME = DELETEME + 1
		end

		tableScroll(self.deadStrings, midiEnt.ConsoleEnt.maxLines - count)

		for _,v in ipairsReverse(self.deadStrings) do
			self.console:push{v, color=gray}
		end
	end
end

return TestMidi
