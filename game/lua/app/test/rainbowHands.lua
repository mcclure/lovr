-- Show skeletons and finger movement
namespace "standard"

local HandCubes = require "ent.debug.handCubes"
local Floor = require "ent.debug.floor"
local RainbowCubes = classNamed("RainbowCubes", HandCubes)

local gray = {0.5,0.5,0.5}
local cubeTickRate=0.075

function RainbowCubes:onLoad()
	lovr.graphics.setBackgroundColor(0.8,0.8,0.8)
	self.lastPoints = {}
	HandCubes.onLoad(self)
	Floor():insert(self)
end

function RainbowCubes:updateColors()
	local time = lovr.timer.getTime()
	self.cubeColor = color.from_hsv((time * 100)%360, 1, 1)
	local lineColor = gray
	if not self.lastCubeTick or time-self.lastCubeTick>=cubeTickRate then
		self.lastCubeTick = time
		self.cubeTick = true
	else
		self.cubeTick = false
	end
	return self.cubeColor, lineColor
end

function RainbowCubes:pointColors(i, _, _2, i2, at)
	if self.cubeTick and self.lastPoints[i] then
		self.cubes:add({at=at.at, lineTo=self.lastPoints[i][i2].at, noCube=true, lineColor=self.cubeColor}, 1.5)
	end
end

function RainbowCubes:onHand(i, _, points)
	if self.cubeTick then
		self.lastPoints[i] = points
	end
end

return RainbowCubes
