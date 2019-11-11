--
namespace "midi"

local ui2 = require "ent.ui2"

local selector = {}

-- Takes "filter" = "in" or "out"
selector.SelectorEnt = classNamed("SelectorEnt", ui2.ButtonEnt) -- ButtonEnt but stick down instead of hold

function selector.SelectorEnt:next()
	local devices = midi.devices()
	local devicesn = #devices
	local last
	local selected
	local label
	if not self.selected or self.selected >= devicesn then
		last = 1
	else
		last = self.selected + 1
	end

	for i=1,devicesn do
		local check = (last + i - 2) % devicesn + 1 -- Ugly clock arithmetic
		if not self.filter
			or (self.filter == "in"  and devices[check].hasIn)
			or (self.filter == "out" and devices[check].hasOut) then
			selected = check
			break
		end
	end
	if not selected then
		label = "(No selection)"
	else
		label = devices[selected].name
	end
	if self.labelPrefix then label = self.labelPrefix .. label end

	self.selected = selected
	self.label = label

	if self.relayout then self:relayout() end
end

selector.SelectorEnt.onLoad = selector.SelectorEnt.next

selector.SelectorEnt.onButton = selector.SelectorEnt.next

selector.PairEnt = classNamed("PairEnt", ui2.ScreenEnt)

function selector.PairEnt:inDevice()
	return self.inSelect.selected
end

function selector.PairEnt:outDevice()
	return self.outSelect.selected
end

function selector.PairEnt:onLoad()
	ui2.routeMouse()
	self.inSelect = selector.SelectorEnt{filter="in", labelPrefix="Midi In: "}
	self.outSelect = selector.SelectorEnt{filter="out", labelPrefix="Midi Out: "}
	local ents = {
		self.inSelect,
		self.outSelect,
	}

	local layout
	layout = ui2.PileLayout{managed=ents, parent=self, mutable=true}
	layout:prelayout()
end

return selector
