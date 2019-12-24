-- Helper thread impl for the Loader class

require 'lovr.filesystem'
require "engine.thread.helper.boot"
namespace "minimal"

local Pump = require "engine.thread.helper.pump"
local action = require "engine.thread.action.loader"

Pump{
	name="loader",
	handler={
		load={2, function(self, kind, path)
			return action[kind](path)
		end}
	}
}:run()
