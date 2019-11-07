
namespace = require "engine.namespace"

-- Load namespace basics
do
	local space = namespace.space("standard")

	-- PL classes missing? add here:
	for _,v in ipairs{"class", "pretty", "stringx", "tablex"} do
		space[v] = require("pl." .. v)
	end

	require "engine.types"
	require "engine.ent"
	space.ent.strictInsert = true -- Do not allow insert() to default to ent.root
	require "engine.common_ent"
	require "engine.lovr"
	require "engine.mode"

	space.cpml = require "cpml" -- CPML classes missing? Add here:
	for _,v in ipairs{"bound2", "bound3", "vec2", "vec3", "quat", "mat4", "color", "utils"} do
		space[v] = space.cpml[v]
	end
end

namespace.prepare("midi", "standard", function(space)
	space.midi = require "ext.midi"
	require "midi.types"
end)

-- Ent driver
-- Pass an app load point or a list of them as cmdline args or defaultApp will run

namespace "standard"

local defaultApp = "app/test/cube"

function lovr.load()
	ent.root = LoaderEnt(#arg > 0 and arg or {defaultApp})
	ent.root:route("onBoot") -- This will only be sent once
	ent.root:insert()
end

function lovr.update(dt)
	ent.root:route("onUpdate", dt)
	entity_cleanup()
end

function lovr.draw()
	drawMode()
	ent.root:route("onDraw")
end

local mirror = lovr.mirror
function lovr.mirror()
	mirror()
	ent.root:route("onMirror")
end
