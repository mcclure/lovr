-- Helper thread impl for the Loader class

local tag, generatorPath = ...

require 'lovr.filesystem'
require "engine.thread.helper.boot"
namespace "minimal"

local AudioPump = require "midi.support.thread.helper.audioPump"
local Generator = require(generatorPath)
local generator = Generator{}

AudioPump{
	name="audio", tag=tag,
	generator=generator,
	handler=generator.getHandlers and generator.getHandlers() or {}
}:run()
