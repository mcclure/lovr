-- 2D UI class that displays a waveform

namespace "standard"

local AudioScope = classNamed("AudioScope", Ent)

function AudioScope:onLoad()
	self.at = self.at or vec3(0,0,0)
	self.size = self.size or vec2(0.2,0.1)
	if self.target then
		local fn = self.render
		if self.target == "onMirror" then
			local f2 = fn
			fn = function(...) uiMode() f2(...) end
		end
		self[self.target] = fn
	end
end

function AudioScope:render()
	if self.transform then self.transform:push() end

	lovr.graphics.setColor(0.5,0.5,0.5)
	local x,y,z = self.at:unpack()
	local w,h = self.size:unpack()
	lovr.graphics.plane('fill', x,y,z,w,h)

	if self.transform then lovr.graphics.pop() end
end

return AudioScope
