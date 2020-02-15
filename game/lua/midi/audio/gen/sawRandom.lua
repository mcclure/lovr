-- Sawtooth generator
namespace ("sawtoothRandom", "minimal")

local ffi = require("ffi")

local Generator = classNamed("Generator")

function Generator:_init()
	self.ticks = 0
end

local USHRT_MAX = 0xffff
local SHRT_MIN = -32767
local UN_SHRT_MIN = -SHRT_MIN

local function conv(x)
	return math.max(-1, math.min(1, x))*SHRT_MIN
end

local function saw(x)
	x = (x*4) % 4
	if x > 2 then x = 4 - x end
	return x - 1
end

local SawSource = classNamed("SawSource")

function SawSource:pre(follow)
	if not self.ticks then self.ticks = 0 self.changes = 0 end
	if false and follow then 
		if follow.f ~= self.lastf then
			self.incr = self.incr and (self.incr+1) or 1
			self.lastf = follow.f
		end
		self.f = follow.f + (self.incr/(44100*8))
	elseif not self.f or self.ticks - self.lastf > 1000 then
		self.f = math.random(32,512)/44100
		if self.factor then self.f = self.f*self.factor end
		self.lastf = self.ticks
		self.changes = self.changes + 1
	end
end

function SawSource:get(i)
	return saw(self.ticks + i*self.f)
end

function SawSource:post(lasti)
	self.ticks = self.ticks + lasti*self.f
end

function Generator:audio(blob)
	if not self.gens then self.gens = {SawSource(), SawSource(), SawSource(), SawSource(), SawSource()} end
	self.gens[3].factor = 1/500

	local bytes = blob:getSize()
	local samples = bytes/2
	local _ptr = blob:getPointer()
	local ptr = ffi.cast("short*", _ptr)

	for i,v in ipairs(self.gens) do v:pre(self.gens[i-1]) end

	for i=1,samples do
		local y = 0
		if self.gens[1].changes >= 1 then y = y + self.gens[2]:get(i + self.gens[4]:get(i)*100) end
		local x = self.gens[1]:get(i + self.gens[4]:get(i + y*1000)*50)
		ptr[i-1] = conv( x * (self.gens[3]:get(i)+1.5) )
	end

	for i,v in ipairs(self.gens) do v:post(samples) end

	return blob
end

return Generator
