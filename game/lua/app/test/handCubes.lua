-- Show skeletons and finger movement
namespace "standard"

local HandCubes = classNamed("HandCubes", Ent)
local DebugCubes = require "ent.debug.debugCubes"

function HandCubes:onLoad()
	lovr.graphics.setBackgroundColor(0.8,0.8,0.8)
	self.cubes = DebugCubes{size=0.025}:insert(self)
end

function HandCubes:onUpdate()
	for i,controllerName in ipairs(lovr.headset.getHands()) do
		-- Attempt to load a model
		local points = lovr.headset.hand.getPoints(controllerName)
		for i2,point in ipairs(points) do
			local at = vec3(unpack(point[1]))
			self.cubes:add({at=at}, true)
		end
		self.lastPoints = points
	end
	self.cubes:add({at=vec3(self.cubes.time%1, 0,0)}, true)
end

return HandCubes
