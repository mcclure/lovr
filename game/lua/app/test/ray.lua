-- Test screenToWorldRay when clicking in mirror

namespace "skategirl"

local ClickRay = classNamed("ClickRay", Ent)
local ui2 = require "ent.ui2"
local skategirl = require "ext.skategirl"
local desktop = lovr.headset.getDriver() == "desktop"

function ClickRay:onLoad()
	ui2.routeMouse()
	self.lines = {}
end

function ClickRay:onPress(at)
	self.click = self.click or {}
	table.insert(self.click, at)
end

function ClickRay:onDraw()
	if self.click then
		for i,at in ipairs(self.click) do
			local x1, y1, z1, x2, y2, z2 = skategirl.screenToWorldRay(at.x, at.y)
			local a, b = vec3(x1, y1, z1), vec3(x2, y2, z2)
			local rb = a + (b - a):normalize()
			print(a, b, rb)
			table.insert(self.lines, {a.x, a.y, a.z, b.x, b.y, b.z})
		end
		self.click = nil
	end

	for x=-5,5 do
		for y=-5,5 do
			local color = (x+y)%2
			local red = (x+5)/10
			local grn = (y+5)/10
			if color == 0 then
				lovr.graphics.setColor(red, grn, 0.15, 1)
			else
				lovr.graphics.setColor(red, grn, 0.85, 1)
			end
			lovr.graphics.plane('fill', x/2, y/2, 0, 0.5, 0.5)
		end
	end
	lovr.graphics.setColor(1, 1, 1, 1)
	for i,v in ipairs(self.lines) do
		lovr.graphics.line(unpack(v))
	end
end

return ClickRay
