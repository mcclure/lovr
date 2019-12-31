-- Hand gesture demos
namespace "standard"

local HandCubes = require "ent.debug.handCubes"
local Floor = require "ent.debug.floor"
local GestureTest = classNamed("GestureTest", HandCubes)

local lightgray = {0.85,0.85,0.85}
local cubeTickRate=0.075
local fingerHighlight = true

function GestureTest:onLoad()
	lovr.graphics.setBackgroundColor(0.95,0.975,1)
	self.color = lightgray
	HandCubes.onLoad(self)
	Floor():insert(self)

	self.handData = {{curl={}},{curl={}}}

	-- Positioning for infoboxes
	local baseTransform = Loc(vec3(0,0,2)):precompose(Loc(nil, quat.from_angle_axis(math.pi, 0,1,0)))
	self.screenTransforms = {
		baseTransform:compose(Loc(nil, quat.from_angle_axis(math.pi + math.pi/4, 0,1,0))),
		baseTransform:compose(Loc(nil, quat.from_angle_axis(math.pi + -math.pi/4, 0,1,0))),
	}
end

function GestureTest:handColors(handI, handName, points)
	-- Calculate gesture proxies. Do this here in case we want to establish a color based on gesture
	local hand = self.handData[handI]

	if not hand.name then hand.name = handName end

	-- Curl
	for i,joints in ipairs(HandCubes.skeletonFingers) do
		local curl = 0
		local jointsN = #joints
		for i2=3,jointsN do
			local at_0 = points[joints[i2]].at
			local at_1 = points[joints[i2-1]].at
			local at_2 = points[joints[i2-2]].at
			curl = curl + (at_0-at_1):dot(at_1-at_2)
		end
		hand.curl[i] = curl
	end

	-- Thumb/index alignment
	local thumb = HandCubes.skeletonFingers[1]
	local index = HandCubes.skeletonFingers[2]
	local indexN = #index
	local gunPoint = points[thumb[#thumb]].at-points[thumb[1]].at
	local gunClick = 0
	for i=2,indexN do
		local at_0 = points[index[i]].at
		local at_1 = points[index[i-1]].at
		gunClick = gunClick + (at_0-at_1):dot(gunPoint)
	end
	hand.gunClick = gunClick
end

function GestureTest:onDraw()
	for i,hand in ipairs(self.handData) do
		self.screenTransforms[i]:push()
		lovr.graphics.translate(0,1.5,0)
		lovr.graphics.scale(0.25)

		-- Name
		local str = (hand.name or "(nil)") .. "\n"
		for i2,curl in ipairs(hand.curl) do
			str = string.format("%s\n%d: %.07f", str, i2, curl)
		end
		str = string.format("%s\n\nFingergun: %.07f", str, hand.gunClick)
		lovr.graphics.print(str)
		
		lovr.graphics.pop()
	end
end

-- Only relevant when fingerHighlight on
function GestureTest:pointColors(handI, hand, points, pointI, point)
	if fingerHighlight then -- Mark joints used for gesture calc
		if not self.whichFingerColor then
			self.whichFingerColor = {{1,0,1}, {0,1,1}, {0.5,0,1}, {0,1,0.5}, {1,0.3647,0.7176}} --Thumb=magenta, Index=cyan, Middle=purple, Ring=lime, Pinky=pink
			self.whichFinger = {}
			for i,joints in ipairs(HandCubes.skeletonFingers) do
				for i2, joint in ipairs(joints) do
					self.whichFinger[joint] = i
				end
			end
		end
		local which = self.whichFinger[pointI]
		local color = which and self.whichFingerColor[which]
		if color then
			local from = self.whichFinger[self.skeletonSkeleton[pointI]]
			return color, from and color or lightgray
		end
	end
end

return GestureTest
