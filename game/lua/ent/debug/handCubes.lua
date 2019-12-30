-- Reusable ent that draws a hand pair as wireframe skeletons
namespace "standard"

-- spec:
--     updateColors() -- Called once per update at start, return color, linecolor
--     pointColors(handI, hand, points, pointI, point) -- Like updateColors, Called once per point
--     onHand(handI, hand, points) -- Called once per update (pseudoevent)
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
local floorSize = 20

function HandCubes:onLoad()
	self.cubes = DebugCubes{size=0.01}:insert(self)
end

function HandCubes:onUpdate()
	local tickCubeColor, tickLineColor = self.updateColors and self:updateColors()
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
			local cubeColor, lineColor = self.pointColors and self:pointColors(i, controllerName, points, i2, at)
			cubeColor = cubeColor or tickCubeColor
			lineColor = lineColor or tickLineColor or tickCubeColor

			local lineToPt = skeletonSkeleton[i2]
			self.cubes:add(
				{at=at.at, rotate=at.rotate, lineTo=lineToPt and points[lineToPt].at, color=cubeColor, lineColor=lineColor},
				true
			)
		end
		if self.onHand then self:onHand(i, controllerName, points) end
	end
end

return HandCubes
