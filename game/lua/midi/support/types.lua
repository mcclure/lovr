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

local function ipairsReverseIter(t, i) -- (Helper for ichars)
	i = i - 1
	if i > 0 then
		return i, t[i]
	end
end

function ipairsReverse(t) -- ipairs() but for characters of an array
	return ipairsReverseIter, t, #t+1
end
