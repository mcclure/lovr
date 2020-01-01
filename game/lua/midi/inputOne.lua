-- Input monitor-- prints one device input to on-screen console

namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"
local midiEnt = require "midi.support.ent"
local selector = require "midi.support.ent.selector"

local TestMidi = classNamed("TestMidi", ui2.ScreenEnt)

function TestMidi:onLoad()
	self.pair = selector.PairEnt{}:insert(self)
	self.console = midiEnt.ConsoleEnt{}:insert(self)
end

function TestMidi:onUpdate()
	local d = self.pair:inDevice()

	if d then
		for _,msg in ipairs(midi.recv(d)) do
			print(pretty.write(msg))
			self.console:push(pretty.write(msg, ""))
		end
	end
end

return TestMidi
