-- 2D UI class that displays a waveform

namespace "standard"

local FloaterEnt = require "midi.support.ent.floater"
local AudioScope = classNamed("AudioScope", FloaterEnt)
local shaderBasic = require "shader.basic"

function AudioScope:onLoad()
	FloaterEnt.onLoad(self)

	self.scopeShader = lovr.graphics.newShader(basic.defaultVertex, [[
		uniform sampler2D scopeImage;
		uniform vec4 light, dark;
		vec4 color(vec4 graphicsColor, sampler2D image, vec2 uv) {
		  vec3 direction = texturePosition[lovrViewID];
		  float theta = acos(-direction.y / length(direction));
		  float phi = atan(direction.x, -direction.z);
		  uv = vec2(0.5f + phi / (2.0f * PI), theta / PI);
		  return graphicsColor * (texture(lovrDiffuseTexture, uv) * (1.0f-ratio) + texture(otherImage, uv) * (ratio));
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
