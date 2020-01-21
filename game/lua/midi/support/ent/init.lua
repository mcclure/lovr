namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"
local midiEnt = {}

local lineHeight = flat.font:getHeight()*flat.fontscale*2
local maxLines = math.floor((flat.height - ui2.screenmargin*2)/lineHeight)

-- Text scroll
-- members:
--     lines: text label

midiEnt.ConsoleEnt = classNamed("ConsoleEnt", ui2.UiBaseEnt)

midiEnt.ConsoleEnt.maxLines = maxLines

function midiEnt.ConsoleEnt:onMirror()
	lovr.graphics.setFont(flat.font)
	local at = vec2(-flat.xspan + ui2.screenmargin, flat.yspan - ui2.screenmargin)
	if self.lines then for i,v in ipairs(self.lines) do
		local s
		local color
		if type(v) == "string" then
			s = v

		else
			s = v[1]
			color = v.color
		end
		if color then
			lovr.graphics.setColor(unpack(color))
		else
			lovr.graphics.setColor(1,1,1)
		end
		lovr.graphics.print(s, at.x, at.y, 0, flat.fontscale*2, nil, nil, "left", "top")
		at.y = at.y - lineHeight
	end end
end

function midiEnt.ConsoleEnt:clear()
	self.lines = nil
end

function midiEnt.ConsoleEnt:push(v)
	if not self.lines then self.lines = {} end
	tableScroll(self.lines, maxLines - 1)
	table.insert(self.lines, v)
end

return midiEnt