-- Input monitor

namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"
local selector = require "midi.ent.selector"
local midiSeq = require "ext.midi.seq"

local TestSeq = classNamed("TestSeq", ui2.ScreenEnt)

local song = [[
(M 0 4 7)
(N 0 3 7)
(O 12 16 19)
(R M M M M)
(S N N N N)
(T M M M O)
R S R S T S R S
]]

function TestSeq:onLoad()
	ui2.routeMouse()

	self.outSelect = selector.SelectorEnt{filter="out", labelPrefix="Midi Out: "}

	self.song = song

	self.goButton = ui2.ButtonEnt{label="Go", onButton=function(e)
		if not self.seq then
			self.seq = midiSeq.new(self.song, self.outSelect.selected)
		end
		if not self.seqPlay then
			midiSeq.play(self.seq)
			self.seqPlay = true
			e.label = "Stop"
		else
			midiSeq.stop(self.seq)
			self.seqPlay = nil
			e.label = "Play"
		end
	end}

	local ents = {
		self.outSelect,
		self.goButton
	}

	local layout
	layout = ui2.PileLayout{managed=ents, parent=self, pass={relayout=function()
		for _,v in ipairs(ents) do if not v.label then return end end
		layout:layout(true)
	end}}
	layout:prelayout()
end

function TestSeq:onUpdate()
	if self.seq then
		--print(midiSeq.at(self.seq))
	end
end

return TestSeq
