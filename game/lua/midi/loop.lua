-- "Looper"-- records notes and plays them back in a loop. Needs work.

namespace "midi"

local flat = require "engine.flat"
local ui2 = require "ent.ui2"
local selector = require "midi.support.ent.selector"

local TestMidi = classNamed("TestMidi", ui2.ScreenEnt)

local function listInsert(atNode, dataNode)
	dataNode.next = atNode.next
	atNode.next = dataNode
	return dataNode
end

local function timePast(at, target, loop)
	local halfloop = loop/2
	local diff = (at - target) % loop
	if diff < -halfloop then diff = diff + loop end
	return diff < 0
end

function TestMidi:onLoad()
	local msgHead = {}
	pull(self, {
		pair = selector.PairEnt{}:insert(self),
		timeClock = 0,
		timeLoop = 5,
		msgHead = msgHead, -- Points to the head of the list
		msgAt = msgHead, -- Points to the node *before* the current node (so insertion possible)
	})
end

function TestMidi:onUpdate(dt)
	local from = self.pair:inDevice()
	local to = self.pair:outDevice()
	local at = lovr.timer.getTime()

	self.timeClock = self.timeClock + dt

	print("\nROOT")
	local HEAD = self.msgHead
	while HEAD do
		print(HEAD == self.msgAt and ">" or " ", HEAD, HEAD.at and string.format("%.2f", HEAD.at%self.timeLoop) or "")
		HEAD = HEAD.next
	end

	if from and to then
		local msgNext = self.msgAt.next
		if msgNext then
			while msgNext.play < at do
				midi.send(to, msgNext.msg)
				msgNext.play = at+self.timeLoop
				self.msgAt = msgNext.next and msgNext or self.msgHead
			end
		end
		for _,msg in ipairs(midi.recv(from)) do
			midi.send(to, msg)
			self.msgAt = listInsert(self.msgAt, {at=at, play=at+self.timeLoop, msg=msg})
			if not self.msgAt.next then
				self.msgAt = self.msgHead
			end
		end
	end
end

return TestMidi
