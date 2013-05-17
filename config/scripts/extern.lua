
scripts_dir    = debug.getinfo(1,"S").source:match[[^@?(.*[\/])[^\/]-$]]
path_separator = scripts_dir:sub(-1)
package.path   = scripts_dir.."?.lua;"..package.path
package.cpath  = "/usr/lib/x86_64-linux-gnu/lua/5.1/?.so;"..package.cpath

require "_strict"
require "_utils"

-- ***************************************************************************
-- ***************************************************************************
-- ***************************************************************************

-- This is the name of the cookie that eventually will be used to track connections.
magic       = "nproxy_magic"

-- ***************************************************************************
-- ***************************************************************************
-- ***************************************************************************

-- The return code of on_connect is not important (and, infact, ignored)
function on_connect (conn, ip)
    -- Allow all clients to connect.
    return 0
end

function on_auth_basic (conn, username, password)
    -- No auth possible for transparent proxying...
    return 0
end

-- The return code, is returned if not zero and not 407.
-- If the return code is 407, then an auth request is generated.
function on_request(conn)

    host = nproxy.request_get_host(conn)
    url = nproxy.request_get_url(conn)
    method = nproxy.request_get_method(conn)
    port   = nproxy.request_get_port(conn)

    -- If the host of the request doesn't match the DNS name of the proxy,
    -- then return 404 NOT FOUND
    if ( host ~= "proxy.navynet.it" ) then
        return 404
    end

    -- Setup loop detection headers
    -- This is only needed to prevent loops
    xreq = nproxy.request_get_header(conn, "X-NProxy-Host");
    if ( xreq ~= nil ) then
        if xreq == "dev" then
            nproxy.connection_set_variable(conn, "custom_cause", "Loop detected");
            nproxy.connection_set_variable(conn, "custom_detail", "Loop detected within this request.");
            return 500
        end
    end
    nproxy.request_add_header(conn, "X-NProxy-Host: dev");


    -- Let's find if a magic tracking cookie exists...
    magic       = "nproxy_magic"
    magic_value = nil
    cookie      = nproxy.request_get_header(conn, "Cookie")
    while ( cookie ~= nil ) do
        s, e, val = string.find(cookie, "nproxy_magic=([^;]*);*.*")
        if ( val ~= nil ) then
            magic_value = val
            break
        else
            cookie = nproxy.request_get_header_next(conn, "Cookie")
        end
    end

    -- Now let's see if we have a mapping defined and, if so,
    -- remove the mapping from the url and eventually
    -- rewrite the url and prepare to send another magic cookie
    s, e, new_url = string.find(url, "^/fow/(.*)")
    if ( new_url ~= nil ) then
        nproxy.request_change_url(conn, "/"..new_url)
        magic_value = "/fow/"
        nproxy.connection_set_variable(conn, "add_magic_cookie", magic_value);
    end

    -- let's check where the magic cookie maps to
    -- Add as needed.
    if ( magic_value == "/fow/" ) then
        new_host = "192.168.1.5"
        new_port = 80
        new_hhost = "fow.example.com" -- hhost stands for heaader host
    else
        return 404
    end

    -- Eventually bind the outgoing request to a specific IP if you're on a 
    -- multihomed machine.
    --nproxy.request_force_bind(conn, "192.168.1.1")

    -- Set the upstream server you need to connect to.
    nproxy.request_force_upstream(conn, new_host, new_port)

    -- Shape the bandwidth not to saturate our ADSL line
    nproxy.connection_set_traffic_shaper(conn, 0, 25*1024);

    -- Disconnect all clients if inactive for more than 15 seconds
    nproxy.connection_set_inactivity_timeout(conn, 15);

    -- If the request has a Host header, then replace it
    -- Otherwise add it to the outgoing request
    tmp = nproxy.request_get_header(conn, "Host")
    if ( tmp ~= nil ) then
        nproxy.request_replace_header_current(conn, "Host: "..new_hhost)
    else
        nproxy.request_add_header(conn, "Host: "..new_hhost)
    end

    return 0
end


-- Whatever return code is run, if not zero is returned to the client.
function on_response (conn, code)

    mc = nproxy.connection_get_variable(conn, "add_magic_cookie");
    if ( mc ~= nil ) then
        nproxy.response_add_header(conn, "Set-Cookie: "..magic.."=/fow/; path=/");
        mc = nproxy.connection_set_variable(conn, "add_magic_cookie", "");
    end

    return 0
end


-- Returning -1 doesn't write the log to file/syslog
function on_log_request ()
    return 0
end
