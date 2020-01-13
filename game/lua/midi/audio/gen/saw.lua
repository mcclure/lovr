-- Sawtooth generator
namespace ("sawtooth", "minimal")

local ffi = require("ffi")

local shortptr = "short*"

local Generator = classNamed("Generator")

function Generator:_init()
	self.ticks = 0
end

local USHRT_MAX = 0xffff
local SHRT_MIN = -32767
local function saw(x)
	return x % USHRT_MAX + SHRT_MIN
end

function Generator:audio(blob)
	local f = self.f -- Something kinda interesting
	if not self.f or self.ticks - self.lastf > 500000 then self.f = math.random(64,512) self.lastf = self.ticks f = self.f end

	local bytes = blob:getSize()
	local samples = bytes/2
	local _ptr = blob:getPointer()
	local ptr = ffi.cast(shortptr, _ptr)
	for i=0,(samples-1) do
		ptr[i] = saw(self.ticks + i*f)
	end
	self.ticks = self.ticks + samples*f
	return blob
end

return Generator
