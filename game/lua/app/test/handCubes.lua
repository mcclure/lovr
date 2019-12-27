-- Show skeletons and finger movement
namespace "standard"

local HandCubes = classNamed("HandCubes", Ent)
local DebugCubes = require "ent.debug.debugCubes"

local skeletonSkeleton = {
  [1]=2, -- Forearm
  [3]=1, [4]=3, [5]=4, [6]=5, [20]=6, -- Thumb
  [7]=1, [8]=7, [9]=8, [21]=9, -- Index
  [10]=1, [11]=10, [12]=11, [22]=12, -- Middle
  [13]=1, [14]=13, [15]=14, [23]=15, -- Ring
  [16]=1, [17]=16, [18]=17, [19]=18, [24]=19 -- Pinky
}
local skeletonTipStart=20

function HandCubes:onLoad()
	lovr.graphics.setBackgroundColor(0.8,0.8,0.8)
	self.cubes = DebugCubes{size=0.01}:insert(self)
end

function HandCubes:onUpdate()
	for i,controllerName in ipairs(lovr.headset.getHands()) do
		local handAt = Loc(unpackPose(controllerName))

		-- Attempt to load a model
		local points = tablex.map(
			function(point)
				return Loc( vec3(unpack(point[1])), quat(unpack(point[2])) ):compose(handAt)
			end,
			lovr.headset.hand.getPoints(controllerName)
		)
		for i2,at in ipairs(points) do
			local lineToPt = skeletonSkeleton[i2]
			self.cubes:add({at=at.at, rotate=at.rotate, lineTo=lineToPt and points[lineToPt].at}, true)
		end
		self.lastPoints = points
	end
end

return HandCubes
