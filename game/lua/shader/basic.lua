-- Common components for shaders

local basic = {}

basic.defaultVertex = [[
	vec4 lovrMain {
		return lovrProjection * lovrTransform * lovrVertex;
	}
]]

basic.defaultFragment = [[
	vec4 lovrMain {
		return lovrGraphicsColor * lovrVertexColor * lovrDiffuseColor * texture(lovrDiffuseTexture, lovrTexCoord);
	}
]]

basic.screenVertex = [[
	vec4 lovrMain {
		return lovrVertex;
	}
]]

basic.skyboxVertex = [[
	out vec3 texturePosition[2];
	vec4 lovrMain {
		texturePosition[lovrViewID] = inverse(mat3(lovrTransform)) * (inverse(lovrProjection) * lovrVertex).xyz;
		return lovrVertex;
	}
]]

return basic