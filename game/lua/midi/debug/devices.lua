-- DEBUG: Dump MIDI devices to STDOUT at startup, then continuously print internal state for RtMidi and ext.midi to screen

namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"

local TestMidi = classNamed("TestMidi", ui2.ScreenEnt)

function TestMidi:onLoad()
	pretty.dump(midi.devices())
end

function TestMidi:onMirror()
	ui2.ScreenEnt.onMirror(self)
	lovr.graphics.setFont(flat.font)

	local str = midi.debugPoke() .. "-----\n" .. midi.debugPoke(2)
	lovr.graphics.print(str, -0.8, 0.8, 0, flat.fontscale * 2, 0,0,0,0,0, 'left', 'top')
end

return TestMidi
