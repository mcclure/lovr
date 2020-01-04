-- Hand gesture demos
namespace "standard"

local HandCubes = require "ent.debug.handCubes"
local Floor = require "ent.debug.floor"
local GestureTest = classNamed("GestureTest", HandCubes)

local cubeTickRate=0.075
local fingerHighlight = false

local gunCurledUnder = .001
local gunThumbCockUnder = .004
local gunThumbFireOver = .005

local crayon = { magenta = {1,0,1}, pink = {1,0.3647,0.7176},
	cyan = {0,1,1}, purple = {0.5,0,1}, lime = {0,1,0.5}, lavender= {0.5, 0.5, 1}, yellow = {1, 1, 0.5},
	lightcyan = {0.9,1,1}, red={1,0,0}, mustard={0.5, 0.5, 0}
}

local deselected = fingerHighlight and {0.85,0.85,0.85} or crayon.lightcyan

function GestureTest:onLoad()
	lovr.graphics.setBackgroundColor(0.95,0.975,1)
	self.color = deselected
	HandCubes.onLoad(self)
	local floor = Floor():insert(self)

	self.handData = {{curl={}, gunGesture={}},{curl={}, gunGesture={}}}

	-- Can
	self.shader = lovr.graphics.newShader('standard', { flags = {
		emissive = true, tonemap = true
	} })
	self.shader:send("lovrLightDirection", {-1,-1,-1})
	self.shader:send("lovrLightColor", {1,1,1,2})
	self.canBaseAt = vec3(-1,1,-2)
	self.postRad = 0.2
	self.canScale = 1/1000
	self.canOrient = {math.pi/2, 1,0,0}
	self.canModel = lovr.graphics.newModel("gestureDemo/can_soup/scene.gltf")
	self.cowbellSound = lovr.audio.newSource("gestureDemo/cowbell.ogg", "static")
	self.bangSound = lovr.audio.newSource("gestureDemo/whitenoise.ogg", "static")
	self.world = lovr.physics.newWorld()
	self.world:setLinearDamping(.01)
	self.world:setAngularDamping(.005)
	self.world:newBoxCollider(0, -0.5, 0, 50, 1, 50):setKinematic(true) -- Ground??
	local post = self.world:newBoxCollider(self.canBaseAt.x, self.canBaseAt.y/2, self.canBaseAt.z, self.postRad, self.canBaseAt.y, self.postRad)
	post:setKinematic(true)
	self.debugPost = post
	-- The can's collider needs to be based on the metrics of the model, but the model is actually very messy
	local canBox = {}
	canBox.minx, canBox.maxx, canBox.miny, canBox.maxy, canBox.minz, canBox.maxz = self.canModel:getAABB()
	for k,v in pairs(canBox) do canBox[k] = v * self.canScale end
	local canHeight, canRadius = canBox.maxy-canBox.miny, (canBox.maxx-canBox.minx)/2
	self.canCenter = -(vec3(canBox.maxx, canBox.maxy, canBox.maxz) + vec3(canBox.minx, canBox.miny, canBox.minz))/2 / self.canScale
	self.canDebug = {canHeight, canRadius}
	self.can = self.world:newCylinderCollider(self.canBaseAt.x, self.canBaseAt.y + canHeight, self.canBaseAt.z, canRadius, canHeight) -- Can
	self.can:setOrientation(unpack(self.canOrient))

	-- Positioning for infoboxes
	local baseTransform = Loc(vec3(0,0,2)):precompose(Loc(nil, quat.from_angle_axis(math.pi, 0,1,0)))
	self.screenTransforms = {
		baseTransform:compose(Loc(nil, quat.from_angle_axis(math.pi + math.pi/2, 0,1,0))),
		baseTransform:compose(Loc(nil, quat.from_angle_axis(math.pi + -math.pi/4, 0,1,0))),
	}
end

--local tt = 0
function GestureTest:onUpdate(dt)
	--if tt then tt = tt + dt if tt > 3 then self.cowbellSound:play() self.can:applyForce(0, 0, 300, 0,0,0) tt = nil end end
	HandCubes.onUpdate(self, dt)
	if dt > 0.1 then dt = 0.1 end
	for i=1,3 do
		self.world:update(dt/3)
	end
end

function GestureTest:handColors(handI, handName, points)
	-- Calculate gesture proxies. Do this here in case we want to establish a color based on gesture
	local hand = self.handData[handI]
	local gunGesture = true

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
		if i>1 then
			local fingerCurled = curl < gunCurledUnder
			local fingerGunGesture = (i==2 and not fingerCurled) or (i>2 and fingerCurled)
			if not fingerGunGesture then gunGesture = false end
			hand.gunGesture[i] = fingerGunGesture
		end
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

	local canFire = gunClick > gunThumbFireOver
	local canCock = gunClick < gunThumbCockUnder

	local wasCocked = hand.cocked
	local wasFire = hand.fire
	hand.fire = (wasCocked or wasFire) and canFire
	hand.trigger = wasCocked and not wasFire and hand.fire
	hand.cocked = not hand.trigger and ( (gunGesture and wasCocked) or canCock )
	hand.cockState = canFire and 3 or (canCock and 2 or 1)

	if hand.trigger then -- KLUDGE: for now ignore left hand
		local indexRoot = points[index[1]].at
		local indexTip = points[index[#index]].at
		local indexTo = indexTip + (indexTip-indexRoot)*30
		self.bangSound:play()
		self.world:raycast(indexRoot.x, indexRoot.y, indexRoot.z, indexTo.x, indexTo.y, indexTo.z, function(shape, x, y, z, nx, ny, nz)
			local collider = shape:getCollider()
			if collider == self.can then
				local zap = vec3(nx, ny, nz):normalize()
				zap.y = 1
				zap = zap:normalize() * 2000
				local push = vec3.zero--vec3(x,y,z)
				print("input", nx,ny,nz)
				print("ZAP", zap.x, zap.y, zap.z, push:unpack())
				collider:setAwake(true)
				collider:applyForce(zap.x, zap.y, zap.z, push:unpack())
				self.cowbellSound:play()
			end
		end)

		self.cubes:add{at=indexTip, lineTo=indexTo, lineColor=crayon.red, noCube=true}
	end
	if hand.fire then
		return crayon.red
	elseif hand.cocked then
		return crayon.pink
	end
end

local font
function printColors(spec, baseColor)
	font = font or lovr.graphics.getFont()
	local top = #spec * font:getLineHeight() / 2
	for i,v in ipairs(spec) do
		local text, color
		if type(v) == "table" then text,color = unpack(v) else text = v end
		if text then
			color = color or baseColor
			lovr.graphics.setColor(unpack(color))
			lovr.graphics.print(text, 0, top - i + 1, 0, nil,nil,nil,nil,nil,nil, 'center', 'bottom')
		end
	end
end

local graytext = {0.5, 0.5, 0.5}
local cockColors = {false, crayon.pink, crayon.red}
function GestureTest:onDraw()
	lovr.graphics.setShader()
	for i,hand in ipairs(self.handData) do
		self.screenTransforms[i]:push()
		lovr.graphics.translate(0,1.5,0)
		lovr.graphics.scale(0.25)

		-- Name
		local t = {}
		table.insert(t, hand.name or "(nil)")
		table.insert(t, false)
		for i2,curl in ipairs(hand.curl) do
			table.insert(t, {string.format("%d: %.07f", i2, curl), hand.gunGesture[i2] and (i2==2 and crayon.red or crayon.pink)})
		end
		if hand.gunClick then
			table.insert(t, false)
			table.insert(t, {string.format("Fingergun: %.07f", hand.gunClick), cockColors[hand.cockState]})
		end
		printColors(t, graytext)
		
		lovr.graphics.pop()
	end

	lovr.graphics.setShader(self.shader)
	lovr.graphics.setColor(unpack(crayon.mustard))

	do
		local x,y,z = self.debugPost:getPosition()
		local a,ax,ay,az = self.debugPost:getOrientation()
		lovr.graphics.box('fill', x, y, z, self.postRad, self.canBaseAt.y, self.postRad, a,ax,ay,az)
	end

	lovr.graphics.setColor(1,1,1)
	do
		Loc(vec3(self.can:getPosition()), quat.from_angle_axis(self.can:getOrientation())):push()
		lovr.graphics.scale(self.canScale)
		lovr.graphics.rotate(-math.pi/2,1,0,0)
		lovr.graphics.translate(self.canCenter:unpack())
		self.canModel:draw()
		--local a,ax,ay,az = self.can:getOrientation()
		--lovr.graphics.cylinder(x,y,z, self.canDebug[1], a,ax,ay,az, self.canDebug[2],self.canDebug[2])
		lovr.graphics.pop()
	end
end

-- Only relevant when fingerHighlight on
function GestureTest:pointColors(handI, hand, points, pointI, point)
	if fingerHighlight then -- Mark joints used for gesture calc
		if not self.whichFingerColor then
			self.whichFingerColor = {crayon.cyan, crayon.purple, crayon.lime, crayon.lavender, crayon.yellow}
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
			return color, from and color or deselected
		end
	end
end

return GestureTest
