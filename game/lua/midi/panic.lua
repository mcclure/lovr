-- Send "MIDI panic" (all notes off) to every device, then quit. Known buggy.

namespace "midi"

local PanicMidi = classNamed("PanicMidi", Ent)

function PanicMidi:panic()
	local devices = midi.devices()
	for i,d in ipairs(devices) do
		if d.out then
			for note=0,127 do
				midi.note(i, false, note)
			end
		end
	end
end

function PanicMidi:onLoad()
	self:panic()
	if self.noquit then self:die()
	else lovr.event.quit() end
end

return PanicMidi
