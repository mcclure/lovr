-- 2D UI class

namespace "standard"

local FloaterEnt = classNamed("FloaterEnt", Ent)

function FloaterEnt:onLoad()
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
	self.material = self.material or 'fill'
end

function FloaterEnt:willRender()
	lovr.graphics.setColor(0.5,0.5,0.5)
end

function FloaterEnt:didRender()
end

function FloaterEnt:render()
	if self.transform then self.transform:push() end

	local x,y,z = self.at:unpack()
	local w,h = self.size:unpack()
	local u,v,uW,vH
	if self.uv then u,v = self.uv:unpack() end
	if self.uvSize then uW,vH = self.uvSize:unpack() end
	self:willRender()
	lovr.graphics.plane(self.material, x,y,z,w,h, 0,0,1,0, u,v,uW,vH)
	self:didRender()

	if self.transform then lovr.graphics.pop() end
end

return FloaterEnt
