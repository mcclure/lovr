-- Base class for helper-thread main loop sending back and forth messages

namespace "minimal"

lovrRequire("thread")
lovrRequire("data")

local AudioPump = classNamed("AudioPump")
local extaudio = require "ext.audio" -- Only used for copy blob

-- Note: Will malfunction if "nil" is sent
-- spec:
--     name: channel name
--     handler: map of message name -> {num args, function}
function AudioPump:_init(spec)
	pull(self, spec)
	local name = stringTag(self.name, self.tag)
	local scopeName = name .. "-scope"

	self.scopeSend = self.channelSend or lovr.thread.getChannel(scopeName.."-dn")
	self.scopeRecv = self.channelRecv or lovr.thread.getChannel(scopeName.."-up")
	self.channelSend = self.channelSend or lovr.thread.getChannel(name.."-dn")
	self.channelRecv = self.channelRecv or lovr.thread.getChannel(name.."-up")
end

-- TODO: Some code duplication with Pump. Can they be unified?
function AudioPump:audio(blob)
	local kind, any

	while true do -- TODO: Break and do some audio after say 10 msgs handled? Or a certain amount of time?
		-- Handle control messages
		kind, any = self.channelRecv:pop(false) -- TODO: Like with Pump: id first, to support cancel?
		if not any then break end
		if kind == "die" then return end

		local handler = self.handler[kind]
		if not handler then error(string.format("Don't understand message %s", kind or "[nil]")) end
		local argc, fn = unpack(handler)

		local result
		if argc==0 then
			result = {fn(self)}
		else
			local arg = {}
			for i=1,argc do table.insert(arg, self.channelRecv:pop(true)) end
			result = {fn(self, unpack(arg))}
		end

		for _,v in ipairs(result) do
			self.channelSend:push(v, false)
		end
	end

	-- Create audio
	if blob then
		blob = self.generator:audio(blob)
		if blob == nil then error("Generator returned nil") end
		if blob == false then return false end

		-- Service audio scope
		local scopeBlob
		if self.inited then
			scopeBlob = self.scopeRecv:pop(false)
			if scopeBlob then extaudio.blobCopy(scopeBlob, blob) end
		else
			scopeBlob = lovr.data.newBlob(blob)
			self.inited = true
		end
		if scopeBlob then self.scopeSend:push(scopeBlob) end

		return blob
	end
end

return AudioPump
