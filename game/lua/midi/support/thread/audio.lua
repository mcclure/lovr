-- Class that can fetch a resource with the loading happening off-thread

namespace "standard"

local PumpEnt = require "engine.thread.connection.pump"
local Audio = classNamed("Audio", PumpEnt)

local singleThread = ent and ent.singleThread

function Audio:_init(spec)
	if singleThread then error("Can't create an Audio in single threaded mode") end

	self:super(spec)

	self.name = self.name or "audio"
end

function Audio:args()
	return self.generator
end

function Audio:insert(parent)
	PumpEnt.insert(self, parent)

	local audio = require "ext.audio"
	local audioName = stringTag(self.name, self.tag) .. "-callback"
	audio.start(audioName)
end

return Audio