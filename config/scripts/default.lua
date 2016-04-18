
--[[
    RESERVED VARIABLES
    THAT SHOULD NOT BE USED in the WebPlan
    If you don'w know that they are for.

        "package"

        "req_method"
        "req_host"
        "req_port"
        "req_url"

        "resp_errno"
        "resp_cause"
        "resp_detail"

        "custom_cause"
        "custom_detail"
        "custom_template"
        "additional_headers"
        "auth_username"
--]]

scripts_dir    = debug.getinfo(1,"S").source:match[[^@?(.*[\/])[^\/]-$]]
path_separator = scripts_dir:sub(-1)
package.path   = scripts_dir.."?.lua;"..package.path
package.cpath  = "/usr/lib/x86_64-linux-gnu/lua/5.1/?.so;"..package.cpath

require "_strict"
require "_utils"

-- ***************************************************************************
-- *** The main HOOKS
-- ***************************************************************************

-- The return code of on_connect is not important (and, infact, ignored)
function on_connect (conn, ip)
    -- every request pipelined through this connection 
    -- is shaped to max 250Kb/sec, by default
    nproxy.connection_set_traffic_shaper(conn, 0, 250 * 1024);


    -- Do you prefer IPv6 or IPv4 ?
    -- Here you can set your choice.
    --nproxy.connection_prefer_ipv4(conn);
    --nproxy.connection_prefer_ipv6(conn);


    -- be safe. Disallow connections if unconfigured.
    if ( configured == false ) then
        nproxy.logmsg("notice", "-----------------------------------")
        nproxy.logmsg("notice", " Please configure the script file.")
        nproxy.logmsg("notice", "-----------------------------------")
        nproxy.connection_set_variable(conn, "custom_cause", "Unconfigured proxy");
        nproxy.connection_set_variable(conn, "custom_detail", "Please configure NProxy before using it.");
        nproxy.connection_set_variable(conn, "custom_template", "unconfigured.html");
        nproxy.connection_set_variable(conn, "force_immediate_response", 500);
        return 0
    end

    -- Authorize connections from localhost
    if ip == "127.0.0.1" then
        return 0
    end

    -- This matches some subnets
    if (
           string.find(ip, "10.10.10.%d") 
        or string.find(ip, "10.10.9.%d") 
        or string.find(ip, "10.10.8.%d") 
       ) then
        return 0
    end

    -- This matches a single host
    --[[
    if ip == "1.2.3.4" then
        return 0
    end
    --]]

    -- If we haven't returned, yet, then the client is not authorized to connect.
    -- Force an HTTP UNAUTHORIZED response as soon as the 1st request comes in
    nproxy.connection_set_variable(conn, "custom_detail", "Your host is not authorized to use this proxy");
    nproxy.connection_set_variable(conn, "force_immediate_response", 401);

    return 0
end


-- This is called every time the proxy receives a connection with a
-- basic authentication scheme. We must analythe the username and the password.
function on_auth_basic (conn, username, password)
    --logdbg("debug", "BASIC auth for: "..username.." - "..password.."\n")

    if ( username == 'nproxy' and password == 'changeme' ) then
        nproxy.profile_store_credentials(conn, username, password, 3600)
        nproxy.connection_set_authuser(conn, username)
        return 0
    end

    return 0
end


-- Every time we receive a request, this is the hook that is called.
-- The return code, is returned if not zero and not 407.
-- If the return code is 407, then an auth request is generated.
function on_request(conn)
    local host   = nproxy.request_get_host(conn)
    local url    = nproxy.request_get_url(conn)
    local method = nproxy.request_get_method(conn)
    local port   = nproxy.request_get_port(conn)

    -- If this variable exists (see on_connect), immediately return a
    -- 401 UnAuthorized response.
    local force = nproxy.connection_get_variable(conn, "force_immediate_response");
    if ( force ~=nil ) then
        return force
    end

    -- This is a way to disconnect users when they use
    -- this proxy as transparent.
    if ( (not transparent_enable) and (nproxy.request_is_transparent(conn) == 1) ) then
        nproxy.connection_set_variable(conn, "custom_detail", "You cannot use this proxy as transparent.");
        return 401
    end

    -- Filter the allowed methods
    -- Notice that REST methods are not included here.
    if not in_table( method, { 'HEAD', 'GET', 'POST', 'CONNECT'} ) then
        return 400 -- Bad request
    end

    if ( method == "CONNECT" ) then
        -- No connect to weird ports
        if ( port ~= 443 ) then
            return 403
        end

        -- Most probably, Skype. Let it work.
        if ( string.find(host, "%d.%d.%d.%d") ) then
            nproxy.connection_set_inactivity_timeout(conn, 60);
            nproxy.connection_set_max_duration(conn, 0);
        end

	-- Prevent people from opening private tunnels.
        nproxy.connection_set_inactivity_timeout(conn, 10);
        nproxy.connection_set_max_duration(conn, 60);
    end

    -- don't send any cookie to facebook, basically
    -- avoiding any access to the website.
    -- (does not work with TLS)
    --[[
    if ( string.find(host, "(%a+.facebook.com)") ) then
        nproxy.request_del_header(conn, "Cookie");
    end
    --]]

    -- Block facebook chats
    -- Does not work with SSL
    --[[
    if ( string.find(host, "(%a+.facebook.com)") ) then
        if ( string.find(url, "chat") ) then
            if (
                   string.find(url, "send%.php") 
                or string.find(url, "typ%.php") 
            ) then
                return 500
            end
        end
    end
    --]]


    -- limit facebook bandwidth to 5Kb / sec
    -- and drop long connections (basically most of them).
    --[[
    if (
            string.find(host, "(%a+.facebook.com)")
         or string.find(host, "(channel(%d+)%.facebook.com)") 
         or string.find(host, "(%a+.fbcdn.net)") 
       ) then
        nproxy.connection_set_traffic_shaper(conn, 0, 5*1024);
        nproxy.connection_set_inactivity_timeout(conn, 10);
        nproxy.connection_set_max_duration(conn, 15);
        return 0
    end
    --]]


    -- Avoid some ad servers and statistic hosts returning
    -- a 404 (Not found) response.
    if (
           ( host=="asdasd" )
        or ( host=="google-analytics.com" )
        or ( host=="www.google-analytics.com" )
        or ( host=="ads.rcs.it" )
        or ( host=="adv.ilsole24ore.it" )
        or ( host=="server-it.imrworldwide.com" )
        or ( host=="secure-it.imrworldwide.com" )
        or ( host=="ad.doubleclick.net" )
        or ( host=="ad.it.doubleclick.net" )
        or ( host=="ad-emea.doubleclick.net" )
        or ( host=="ad.doubleclick.net" )
        or ( host=="pagead2.googlesyndication.com" )
        or ( host=="oas.rcsadv.it" )
        or ( host=="oas.repubblica.it" )
        or ( host=="feed.4wnet.com" )
        or ( host=="images.feed.4wnet.com" )
        or ( host=="adagiof3.repubblica.it" )
        or ( host=="scripts.kataweb.it" )
        or ( host=="ads.arcuspubblicita.it" )
        or ( host=="ad46.neodatagroup.com" )
        or ( host=="statse.webtrendslive.com" )
        or ( host=="widget.criteo.com" )
        or ( host=="static.atgsvcs.com" )
        or ( host=="data.coremetrics.com" )
        or ( host=="metrics.sun.com" )
        or ( host=="nsm.dell.com" )
        or ( host=="oas.iltempo.it" )
        or ( host=="creative.ak.fbcdn.net" )
        or ( host=="adlev.neodatagroup.com" )
        or ( host=="adserver.adtech.de" )
        or ( host=="b.scorecardresearch.com" )
        or ( host=="metrics.rcsmetrics.it" )
        or ( host=="a.visualrevenue.com" )
        or string.find(host, "g.doubleclick.net" )
        or string.find(host, ".googleadservices.com" )
       ) then
        nproxy.connection_set_variable(conn, "custom_detail", "Avoiding bandwidth loss");
        return 404;
    end

    -- example to show how to redirect a website to another page
    if ( string.find(host, ".search.yahoo.com") ) then
        -- Yahoo search results are not that usefull, after all...
        -- Marissa it's just an example!
        nproxy.connection_set_variable(conn, "additional_headers", "Location: http://www.google.it\r\n");
        return 302;
    end

    -- an example to block completely a website
    if ( host=="www.sportal.it" ) then
        nproxy.connection_set_variable(conn, "custom_cause", "Non autorizzato");
        nproxy.connection_set_variable(conn, "custom_detail", "Il sito è bloccato");
        return 401;
    end

    -- an example to restrict the access to particular websites only
    -- to authenticated users
    --[[
    if (
            host=="www.youtube.com"
         or host=="news.google.com"
       ) then
        user = nproxy.connection_get_authuser(conn);
        if ( user == nil ) then
            nproxy.connection_set_variable(conn, "custom_detail", "Il sito richiede l'accesso con password");
            nproxy.request_require_auth(conn, "Basic", "Proxy authentication");
        end
        return 0
    end
    --]]

    -- With XML-RPC you could send data to a control Host and
    -- receive back informations about what to do
    --[[
    local params = {}
    params["user"]              = nproxy.connection_get_authuser(conn);
    params["transparent"]       = nproxy.request_is_transparent(conn);
    params["host"]              = nproxy.request_get_host(conn)
    params["port"]              = nproxy.request_get_port(conn)
    params["method"]            = nproxy.request_get_method(conn)
    params["url"]               = nproxy.request_get_url(conn)
    params["protocol"]          = nproxy.request_get_protocol(conn)
    params["headers"] = {}
    params["headers"]["host"]   = nproxy.request_get_header(conn, "Host")
    params["headers"]["referer"]= nproxy.request_get_header(conn, "Referer")

    local ok, res = xmlrpc.http.call (rpc_url, "on_request", params)

    for i, v in pairs(res) do 
        print ('\t', i, v) 
    end
    --]]

    return 0
end


-- Every time the proxy receives a response, this hook is launched.
-- This allows us to apply some nice hacks to the connection.
-- Whatever return code is run, if not zero is returned to the client.
function on_response (conn, code)
    local host   = nproxy.request_get_host(conn)

    -- Shape debian downloads, we don't want to slow down our ADSL
    if ( string.find(host, "(%a+.debian.org)") ) then
        nproxy.request_set_traffic_shaper(conn, 0, 300*1024);
        return 0
    end

    -- If it is a flash video, simply return "404 not found"
    -- If the file is a Flash object, shape the incoming data to
    -- 15Kb/sec if it's longer than 100Kb
    -- Speedup something by adding a cache-control header
    -- where it's not set.
    local ctype = nproxy.response_get_header(conn, "Content-type");
    if ( ctype ) then
        if ( ctype == "video/x-flv" ) then
            return 403
        end
        if ( ctype == "text/plain" ) then
            local url    = nproxy.request_get_url(conn)
            if ( string.find(url, "%.flv") ) then
                return 403
            end
        end

        if ( ctype == "application/x-shockwave-flash" ) then
            nproxy.request_set_traffic_shaper(conn, 100*1024, 15*1024);
        end

        if (
                string.find(ctype, "image/(%a+)")
             or string.find(ctype, "text/css")
             or string.find(ctype, "application/x-javascript")
           ) then
            local cachectl = nproxy.response_get_header(conn, "Cache-Control");
            if cachectl == nil then
                nproxy.response_add_header(conn, "Cache-Control: max-age="..(3600*48)..", public")
            end
        end
    end

    -- If the response is larger than 5Mb, then
    -- shape the incoming data at 20Kb/sec after the 1st 250Kb and set the
    -- max duration of the connection to 300 seconds (5 minutes)
    --[[
    local clen = nproxy.response_get_header(conn, "Content-length")
    if ( (clen ~= nil) and ( tonumber(clen) > 5*1024*1024) ) then
        nproxy.request_set_traffic_shaper(conn, 0, 25*1024);
        nproxy.connection_set_inactivity_timeout(conn, 10);
        nproxy.connection_set_max_duration(conn, 300);
    end
    --]]

    return 0
end


-- Returning -1 doesn't write the log to file/syslog
function on_log_request (conn)
    local user = nproxy.connection_get_authuser(conn);

    -- don't log TheBoss Activity!
    if ( user == "theboss" ) then
	return -1
    end

    return 0
end

-- ***************************************************************************
-- *** WARNING
-- *** Set the 'configured' variable to 'true' to start accepting connections.
-- ***
-- *** Please understand this script, first!
-- ***************************************************************************

verbose = false
transparent_enable = false

configured = false
-- configured = true
