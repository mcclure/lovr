-- Sawtooth generator
namespace ("sawtooth", "minimal")

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
	return (x*2) % 2 - 1
end

local SawSource = classNamed("SawSource")

function SawSource:pre(follow)
	if not self.ticks then self.ticks = 0 end
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
	end
end

function SawSource:get(i)
	return saw(self.ticks + i*self.f)
end

function SawSource:post(lasti)
	self.ticks = self.ticks + lasti*self.f
end

function Generator:audio(blob)
	if not self.gens then self.gens = {SawSource(), SawSource(), SawSource()} end
	self.gens[3].factor = 1/100

	local bytes = blob:getSize()
	local samples = bytes/2
	local _ptr = blob:getPointer()
	local ptr = ffi.cast("short*", _ptr)

	for i,v in ipairs(self.gens) do v:pre(self.gens[i-1]) end

	for i=1,samples do
		ptr[i-1] = conv( (self.gens[1]:get(i) + self.gens[2]:get(i)) * (self.gens[3]:get(i)+1) )
	end

	for i,v in ipairs(self.gens) do v:post(samples) end

	return blob
end

return Generator
