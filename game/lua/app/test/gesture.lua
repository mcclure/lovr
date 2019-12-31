-- Hand gesture demos
namespace "standard"

local HandCubes = require "ent.debug.handCubes"
local Floor = require "ent.debug.floor"
local GestureTest = classNamed("GestureTest", HandCubes)

local gray = {0.5,0.5,0.5}
local cubeTickRate=0.075

function GestureTest:onLoad()
	lovr.graphics.setBackgroundColor(0.95,0.975,1)
	HandCubes.onLoad(self)
	Floor():insert(self)
	self.handData = {{curl={}},{curl={}}}
	local baseTransform = Loc(vec3(0,0,2)):precompose(Loc(nil, quat.from_angle_axis(math.pi, 0,1,0)))
	self.screenTransforms = {
		baseTransform:compose(Loc(nil, quat.from_angle_axis(math.pi + math.pi/4, 0,1,0))),
		baseTransform:compose(Loc(nil, quat.from_angle_axis(math.pi + -math.pi/4, 0,1,0))),
	}
end

function GestureTest:handColors(handI, hand, points)
	local hand = self.handData[handI]
	if not hand.name then hand.name = hand end
	for i,joints in ipairs(HandCubes.skeletonFingers) do
		local curl = 0
		local jointsN = #joints
		for i2=3,jointsN do
			local at_0 = points[joints[i2]].at
			local at_1 = points[joints[i2-1]].at
			local at_2 = points[joints[i2-2]].at
			curl = curl + (at_1-at_0):dot(at_2-at_1)
		end
		hand.curl[i] = curl
	end
end

function GestureTest:onDraw()
	for i,hand in ipairs(self.handData) do
		self.screenTransforms[i]:push()
		lovr.graphics.translate(0,1,0)
		lovr.graphics.scale(0.25)

		local str = (self.handData.name or "(nil)") .. "\n"
		for i2,curl in ipairs(hand.curl) do
			str = string.format("%s\n%d: %.06f", str, i2, curl)
		end
		lovr.graphics.print(str)
		
		lovr.graphics.pop()
	end
end

return GestureTest
