-- Draw a basic floor plane just so the world isn't invisible
namespace "standard"

local Floor = classNamed("Floor", Ent)

local floorSize = 20

function Floor:onLoad()
	local floorPixels = 24
	local data = lovr.data.newTextureData(floorPixels, floorPixels, "rgba")
	for x=0,(floorPixels-1) do
		for y=0,(floorPixels-1) do
			local bright = (x+y)%2 == 0 and 0.5 or 0.75
			local function sq(x) return x*x end
			local alpha = math.min(1, (1-math.max(0, math.min(1, 2*math.sqrt(sq(0.5-x/(floorPixels-1))+sq(0.5-y/(floorPixels-1))))))*1.2)
			print(x,y,bright,alpha)
			data:setPixel(x,y,bright,bright,bright,alpha)
		end
	end
	local texture = lovr.graphics.newTexture(data, {mipmaps=false})
	texture:setFilter("nearest")
	self.floorMaterial = lovr.graphics.newMaterial(texture)
end

function Floor:onDraw()
	lovr.graphics.setBlendMode("alpha", "alphamultiply")
	lovr.graphics.setColor(1,1,1,1)
	lovr.graphics.plane(self.floorMaterial, 0,0,0, floorSize, floorSize, math.pi/2,1,0,0)
end

return Floor
