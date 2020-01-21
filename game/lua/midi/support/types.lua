-- Midi project lua utilities
-- Assumes pl "class", cpml in namespace
-- IMPORTS ALL ON REQUIRE

namespace "midi"

function tableScroll(t, max)
	local n = #t
	local overage = n - max
	if overage > 0 then
		for i=1,n do
			t[i] = t[i+overage]
		end
	end
end
