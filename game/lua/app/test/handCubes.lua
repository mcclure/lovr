-- Show skeletons and finger movement
namespace "standard"

local HandCubes = classNamed("HandCubes", Ent)
local DebugCubes = require "ent.debug.debugCubes"

function HandCubes:onLoad()
	lovr.graphics.setBackgroundColor(0.8,0.8,0.8)
	self.cubes = DebugCubes{size=0.01}:insert(self)
end

function HandCubes:onUpdate()
	for i,controllerName in ipairs(lovr.headset.getHands()) do
		local handAt = Loc(unpackPose(controllerName))

		-- Attempt to load a model
		local points = lovr.headset.hand.getPoints(controllerName)
		for i2,point in ipairs(points) do
			local at = Loc( vec3(unpack(point[1])), quat(unpack(point[2])) ):compose(handAt)
			self.cubes:add({at=at.at, rotate=at.rotate}, true)
		end
		self.lastPoints = points
	end
end

return HandCubes
