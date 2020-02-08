-- 2D UI class that displays a waveform

namespace "standard"

local FloaterEnt = require "midi.support.ent.floater"
local AudioScope = classNamed("AudioScope", FloaterEnt)
local shaderBasic = require "shader.basic"
local flat = require "engine.flat"
local ffi = require("ffi")

function AudioScope:onLoad()
	self.size = self.size or vec2(flat.aspect*1.5,0.5)

	FloaterEnt.onLoad(self)

	self.scopeShader = lovr.graphics.newShader(shaderBasic.defaultVertex, [[
		uniform vec4 bgColor;
		vec4 color(vec4 graphicsColor, sampler2D image, vec2 uv) {
		  float y = texture(image, vec2(uv.x, 0.0f)).r;
		  float dist = smoothstep( 0.0, 0.1, abs(uv.y - y));
		  return graphicsColor * (1-dist) + bgColor * dist;
		}
	]], {} )
	self.scopeShader:send("bgColor", {0.2, 0.2, 0.2});

	self.name = self.name or "audio"
	local name = stringTag(self.name, self.tag)
	local scopeName = name .. "-scope"

	self.scopeSend = self.channelSend or lovr.thread.getChannel(scopeName.."-up")
	self.scopeRecv = self.channelRecv or lovr.thread.getChannel(scopeName.."-dn")
end

function AudioScope:willRender()
	local scopeBlob = self.scopeRecv:pop(0.1)
	if scopeBlob then
		local samples = scopeBlob:getSize()/2
		if not self.textureData then
			self.textureData = lovr.data.newTextureData(samples, 1, "r32f")

			local uvInset = 1/samples
			self.uv = vec2(uvInset/2, 0)
			self.uvSize = vec2(1-uvInset, 1)
		end
		local _ptr = scopeBlob:getPointer()
		local ptr = ffi.cast("short*", _ptr)
		for i=0,(samples-1) do
			self.textureData:setPixel(i, 0, ptr[i]/65535 + 0.5)
		end
		if not self.texture then
			self.texture = lovr.graphics.newTexture(self.textureData, {mipmaps=false})
			self.texture:setWrap("clamp", "clamp")
			self.material = lovr.graphics.newMaterial(self.texture)
		else
			self.texture:replacePixels(self.textureData)
		end
		self.scopeSend:push(scopeBlob)
	end
	lovr.graphics.setShader(self.scopeShader)
	lovr.graphics.setColor(1,1,0)
end

function AudioScope:didRender()
	lovr.graphics.setShader()
end

return AudioScope
