-- Sawtooth generator
namespace ("sawtooth", "minimal")

local ffi = require("ffi")

local shortptr = "unsigned short*"

local Generator = classNamed("Generator")

function Generator:_init()
	self.ticks = 0
end

local USHRT_MAX = 0xffff
local function saw(x)
	return x % USHRT_MAX
end

function Generator:audio(blob)
	local bytes = blob:getSize()
	local samples = bytes/2
	local _ptr = blob:getPointer()
	local ptr = ffi.cast(shortptr, _ptr)
	for i=0,(samples-1) do
		ptr[i] = saw(self.ticks + i)
	end
	self.ticks = self.ticks + bytes
	return blob
end

return Generator
