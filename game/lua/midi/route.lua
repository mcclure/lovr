-- Input monitor

namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"
local selector = require "midi.ent.selector"

local TestMidi = classNamed("TestMidi", ui2.ScreenEnt)

function TestMidi:onLoad()
	self.pair = selector.PairEnt{}:insert(self)
end

function TestMidi:onUpdate()
	local from = self.pair:inDevice()
	local to = self.pair:outDevice()

	if from and to then
		for _,msg in ipairs(midi.recv(from)) do
			midi.send(to, msg)
		end
	end
end

return TestMidi
