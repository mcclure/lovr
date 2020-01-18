-- Reusable ent that draws a hand pair as wireframe skeletons
namespace "standard"

-- spec:
--     updateColors() -- Called once per update at start, return color, linecolor
--     handColors(handI, hand, points, handAt) -- Like updateColors, called once per hand
--     pointColors(handI, hand, points, pointI, point, handAt) -- Like updateColors, called once per point
--     onHand(handI, hand, points, handAt) -- Called once per update (pseudoevent)
local HandCubes = classNamed("HandCubes", Ent)
local DebugCubes = require "ent.debug.debugCubes"

HandCubes.skeletonSkeleton = {
  [1]=2, -- Forearm
  [3]=1, [4]=3, [5]=4, [6]=5, [20]=6, -- Thumb
  [7]=1, [8]=7, [9]=8, [21]=9,        -- Index
  [10]=1, [11]=10, [12]=11, [22]=12,  -- Middle
  [13]=1, [14]=13, [15]=14, [23]=15,  -- Ring
  [16]=1, [17]=16, [18]=17, [19]=18, [24]=19 -- Pinky
}
HandCubes.skeletonTipStart = 20
HandCubes.expectedLength = 20
HandCubes.skeletonFingers = {
	{4, 5, 6, 20}, -- Thumb
	{7, 8, 9, 21},    -- Index
	{10, 11, 12, 22}, -- Middle
	{13, 14, 15, 23}, -- Ring
	{17, 18, 19, 24}, -- Pinky
}

function HandCubes:onLoad()
	self.cubes = DebugCubes{size=0.01, color=self.color}:insert(self)
end

function HandCubes:onUpdate()
	local tickCubeColor, tickLineColor
	if self.updateColors then tickCubeColor, tickLineColor = self:updateColors() end

	for i,controllerName in ipairs(lovr.headset.getHands()) do
		local handAt = Loc(unpackPose(controllerName))

		-- Attempt to load a model
		local points = tablex.map(
			function(point)
				return Loc( vec3(unpack(point[1])), quat(unpack(point[2])) ):compose(handAt)
			end,
			lovr.headset.hand.getPoints(controllerName)
		)

		if tableTrue(points) then -- FIXME: Should compare #points against expectedLength?
			local handCubeColor, handLineColor
			if self.handColors then handCubeColor, handLineColor = self:handColors(i, controllerName, points, handAt) end
			handCubeColor = handCubeColor or tickCubeColor
			handLineColor = handLineColor or tickLineColor
			for i2,at in ipairs(points) do
				local pointCubeColor, pointLineColor
				if self.pointColors then pointCubeColor, pointLineColor = self:pointColors(i, controllerName, points, i2, at) end
				pointCubeColor = pointCubeColor or handCubeColor
				pointLineColor = pointLineColor or handLineColor

				local lineToPt = self.skeletonSkeleton[i2]
				self.cubes:add(
					{at=at.at, rotate=at.rotate, lineTo=lineToPt and points[lineToPt].at, color=pointCubeColor, lineColor=pointLineColor},
					true
				)
			end
			if self.onHand then self:onHand(i, controllerName, points) end
		end
	end
end

return HandCubes
