-- Sawtooth generator
namespace ("sawtooth", "minimal")

ffi.cdef[[
typedef unsigned short* shortptr;
]]

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
	local _ptr = blob:getPointer()
	local ptr = ffi.cast(shortptr, _ptr)
	for i=0,(bytes-1) do
		ptr[i] = saw(self.ticks + i)
	end
	self.ticks = self.ticks + bytes
end

return Generator
