-- 2D UI class that displays a waveform

namespace "standard"

local FloaterEnt = require "midi.support.ent.floater"
local AudioScope = classNamed("AudioScope", FloaterEnt)
local shaderBasic = require "shader.basic"
local flat = require "engine.flat"

function AudioScope:onLoad()
	if not self.material then
		local w = 4
		local data = lovr.data.newTextureData(w, 1, "r32f")
		data:setPixel(0,0,0)
		data:setPixel(1,0,0.75)
		data:setPixel(2,0,0.25)
		data:setPixel(3,0,1)
		local texture = lovr.graphics.newTexture(data, {mipmaps=false})
		texture:setWrap("clamp", "clamp")
		self.material = lovr.graphics.newMaterial(texture)

		local uvInset = 1/w
		self.uv = vec2(uvInset/2, 0)
		self.uvSize = vec2(1-uvInset, 1)
	end

	self.size = self.size or vec2(flat.aspect*1.5,0.5)

	FloaterEnt.onLoad(self)

	self.scopeShader = lovr.graphics.newShader(shaderBasic.defaultVertex, [[
		uniform vec4 bgColor;
		vec4 color(vec4 graphicsColor, sampler2D image, vec2 uv) {
		  float y = texture(image, vec2(uv.x, 0.0f)).r;
		  float dist = min(1.0f, abs(uv.y - y));
		  return graphicsColor * (1-dist) + bgColor * dist;
		}
	]], {} )
end

function AudioScope:willRender()
	lovr.graphics.setShader(self.scopeShader)
end

function AudioScope:didRender()
	lovr.graphics.setShader()
end

return AudioScope
