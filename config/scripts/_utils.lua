
-- ***************************************************************************
-- *** Some global variables and helper functions
-- ***************************************************************************

function logdbg(level, msg)
    if ( verbose ) then
        nproxy.logmsg(level, msg)
    end
end

function in_table ( e, t )
    for _,v in pairs(t) do
        if (v==e) then
            return true
        end
    end
    return false
end

