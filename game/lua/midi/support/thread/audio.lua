-- Class that can fetch a resource with the loading happening off-thread

namespace "standard"

local PumpEnt = require "engine.thread.connection.pump"
local Audio = classNamed("Audio", PumpEnt)

local singleThread = ent and ent.singleThread

function Audio:_init(spec)
	if singleThread then error("Can't create an Audio in single threaded mode") end

	self:super(spec)

	self.name = self.name or "audio"
	self.boot = self.boot or "midi/support/thread/helper/audio.lua"
end

function Audio:args()
	return self.generator
end

function Audio:insert(parent)
	self.thread = lovr.thread.newThread(self.boot)
	local audio = require "ext.audio"
	local audioName = stringTag(self.name, self.tag) .. "-callback"
	audio.start(self.thread, self.tag, self:args())

	PumpEnt.insert(self, parent)
end

return Audio
