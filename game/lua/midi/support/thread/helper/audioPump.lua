-- Base class for helper-thread main loop sending back and forth messages

namespace "minimal"

lovrRequire("thread")
lovrRequire("data")

local AudioPump = classNamed("AudioPump")

-- Note: Will malfunction if "nil" is sent
-- spec:
--     name: channel name
--     handler: map of message name -> {num args, function}
function AudioPump:_init(spec)
	pull(self, spec)
	local name = stringTag(self.name, self.tag)
	local audioName = name .. "-callback"

	self.audioSend = self.audioSend or lovr.thread.getChannel(audioName.."-dn")
	self.audioRecv = self.audioRecv or lovr.thread.getChannel(audioName.."-up")
	self.channelSend = self.channelSend or lovr.thread.getChannel(name.."-dn")
	self.channelRecv = self.channelRecv or lovr.thread.getChannel(name.."-up")
end

-- TODO: Some code duplication with Pump. Can they be unified?
function AudioPump:run()
	while true do
		local kind, any

		while true do -- TODO: Break after say 10 msgs? Or a certain amount of time?
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
		blob = self.audioRecv:pop(0.1)
		if blob then
			if type(blob) == "string" then error(blob) end -- Halt
			blob = self.generator:audio(blob)
			if blob == nil then error("Generator returned nil") end
			if blob == false then return end
			self.audioSend:push(blob)
		end
	end
end

return AudioPump
