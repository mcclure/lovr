-- Show skeletons and finger movement
namespace "standard"

local HandCubes = classNamed("HandCubes", Ent)
local DebugCubes = require "ent.debug.debugCubes"

local useColor = true

local skeletonSkeleton = {
  [1]=2, -- Forearm
  [3]=1, [4]=3, [5]=4, [6]=5, [20]=6, -- Thumb
  [7]=1, [8]=7, [9]=8, [21]=9, -- Index
  [10]=1, [11]=10, [12]=11, [22]=12, -- Middle
  [13]=1, [14]=13, [15]=14, [23]=15, -- Ring
  [16]=1, [17]=16, [18]=17, [19]=18, [24]=19 -- Pinky
}
local skeletonTipStart=20
local gray = {0.5,0.5,0.5}
local floorSize = 20
local cubeTickRate=0.075

function HandCubes:onLoad()
	lovr.graphics.setBackgroundColor(0.8,0.8,0.8)
	self.cubes = DebugCubes{size=0.01}:insert(self)
	self.lastPoints = {}

	-- Create a nice floor
	local floorPixels = 24
	local data = lovr.data.newTextureData(floorPixels, floorPixels, "rgba")
	for x=0,(floorPixels-1) do
		for y=0,(floorPixels-1) do
			local bright = (x+y)%2 == 0 and 0.5 or 0.75
			local function sq(x) return x*x end
			local alpha = math.min(1, (1-math.max(0, math.min(1, 2*math.sqrt(sq(0.5-x/(floorPixels-1))+sq(0.5-y/(floorPixels-1))))))*1.2)
			print(x,y,bright,alpha)
			data:setPixel(x,y,bright,bright,bright,alpha)
		end
	end
	local texture = lovr.graphics.newTexture(data, {mipmaps=false})
	texture:setFilter("nearest")
	self.floorMaterial = lovr.graphics.newMaterial(texture)
end

function HandCubes:onUpdate()
	local time = lovr.timer.getTime()
	local cubeColor, lineColor, cubeTick
	if useColor then -- In color mode draw trails that update every so often
		cubeColor = color.from_hsv((time * 100)%360, 1, 1)
		lineColor = gray
		if not self.lastCubeTick or time-self.lastCubeTick>=cubeTickRate then
			self.lastCubeTick = time
			cubeTick = true
		end
	end
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
			if cubeTick and self.lastPoints[i] then
				self.cubes:add({at=at.at, lineTo=self.lastPoints[i][i2].at, noCube=true, lineColor=cubeColor}, 1)
			end
			local lineToPt = skeletonSkeleton[i2]
			self.cubes:add(
				{at=at.at, rotate=at.rotate, lineTo=lineToPt and points[lineToPt].at, color=cubeColor, lineColor=lineColor},
				true
			)
		end
		if cubeTick then
			self.lastPoints[i] = points
		end
	end
end

function HandCubes:onDraw()
	-- Draw floor
	lovr.graphics.setBlendMode("alpha", "alphamultiply")
	lovr.graphics.setColor(1,1,1,1)
	lovr.graphics.plane(self.floorMaterial, 0,0,0, floorSize, floorSize, math.pi/2,1,0,0)

	-- Turn off blend mode so DebugCubes can batch more efficiently
	lovr.graphics.setBlendMode(nil)
end

return HandCubes
