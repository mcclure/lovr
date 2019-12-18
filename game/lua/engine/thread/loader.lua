-- Class that can fetch a resource with the loading happening off-thread

namespace "standard"

local Loader = classNamed("Loader")

local loaderConnection
local loaderConnectionUp
local loaderAction
if ent and ent.singleThread then
	loaderAction = require "engine.thread.action.loader"
else
	local PumpEnt = require "engine.thread.connection.pump"
	loaderConnection = PumpEnt{
		loaders=Queue(),
		boot="engine/thread/helper/loader.lua",
		name="loader",
		handler={
			load={1, function (self, value)
				local loader = self.loaders:pop()
				if loader.filter then
					value = loader.filter(value)
					loader.filter = nil
				end
				loader.content = value
			end}
		},
		load=function(self, loader, args)
			self.loaders:push(loader)
			self:send("load", unpack(args))
		end
	}
end
local globalConnection

function Loader:_init(kind, path, filter)
	local connect = self:connect()
	if connect then
		self.filter = filter
		loaderConnection:load(self, {kind, path})
	else
		self.content = filter(loaderAction[kind](path))
	end
end

function Loader:get()
	-- Assume self.content is always set by this point if threading is off
	while not self.content do
		loaderConnection:drain(true)
	end

	return self.content
end

function Loader.connect()
	if loaderConnection and not loaderConnectionUp then
		loaderConnection:insert(ent.root) -- Bad, want before not after
		loaderConnectionUp = true
	end
	return loaderConnection
end

Loader.dataToModel = lovr.graphics.newModel
Loader.dataToTexture = lovr.graphics.newTexture
function Loader.dataToMaterial(data) return lovr.graphics.newMaterial( lovr.graphics.newTexture(data) ) end

return Loader