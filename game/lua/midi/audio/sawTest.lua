-- Establish that all the basic features of ui2 work

namespace "standard"

local ui2 = require "ent.ui2"
local Audio = require "midi.support.thread.audio"

local TestUi = classNamed("TestUi", ui2.ScreenEnt)

function TestUi:onLoad()
	ui2.routeMouse()
	-- Create some buttons that do different things
	local ents = {
		ui2.ButtonEnt{label="IDK", onButton = function(self) -- Test die
			print "IDK"
		end}
	}

	-- Lay all the buttons out
	local layout = ui2.PileLayout{managed=ents, parent=self}
	layout:layout()

	self.audio = Audio{generator="midi.audio.gen.saw"}:insert(self)
end

return TestUi
