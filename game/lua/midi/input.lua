-- Input monitor-- prints all device input to stdout

namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"

local TestMidi = classNamed("TestMidi", ui2.ScreenEnt)

function TestMidi:onUpdate()
	for i,d in ipairs(midi.devices()) do
		if d.hasIn then
			for _,msg in ipairs(midi.recv(i)) do
				print(i, pretty.write(msg))
			end
		end
	end
end

return TestMidi
