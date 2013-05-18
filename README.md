NPROXY
======

**An HTTP/1.1 High performance, programmable application proxy**

Introduction
------------

NProxy is a lightweight, fast, programmable HTTP/1.1 Proxy, driven by simple user-written LUA programs which allows a great flexibility.
NProxy runs on **Linux**, **BSD flavours** (tested on FreeBSD but should work on NetBSD and OpenBSD), **OSX** and (soon) **Windows**.

The Open source version is derived from a more complex and feature-complete version based on a proprietary Runtime Library. This version, based on APR (Apache Runtime Library) can run (eventually with minor fixes, so let me know) on all platforms supported by APR.

Use Cases
---------

Nproxy can be used in serveral cases. The following ones are only examples but there is actually no limit:

- As a **security measure**, in transparent mode, to filter all outgoing connections from a web server. In fact, sometimes, on shared hostings, you need a way to block outgoing HTTP connections to "strange" hosts (eventually generated by bots or malware) and allow trusted connections to other services (blog pings, for example).

- As a filter to **allow/deny access internet access** to your employees. The flexibility here is unbeatable because you can customize every single parameter of your filter, according to your needs.

- As a **filter for your house**, you prevent your childs to surf on unwanted websites

- As a **filter for adware**, banners and annoying or invasive contents (cookie tracking services)


Feel free to write me and describe your experience!

Installation
------------

The prerequisites are:

- you must have CMake 2.4 (or greater)

- you must have Lua (5.0 and 5.1 supported - 5.2 support is on the TODO list)

- you must have APR - Apache Runtime Library 1.4 (or greater) installed. Should probably work with 1.3 as well.


### Unix/Linux/[Free,Net,Open]BSD

- Install Cmake, Lua and APR. On Debian, for example, use:

    apt-get install cmake liblua5.1-0-dev libapr1-dev


- Create a directory to store your code. This is not the directory where you will be building NProxy:

    mkdir nproxy && cd nproxy


- Clone NProxy repository:

    git clone https://github.com/ctrix/nproxy trunk


- Create the directory to build NProxy:

    mkdir build && cd build


- Run CMake to configure the build:

    cmake ../trunk


- If no errors are reported, the compile and install. Everything should be quite fast:

    make install


### OSX

Try to follow the details for Unix.

### Windows

Instructions will be posted as soon as support will be ready.

### Debian Quick Howto

You can build your Debian Package in 2 ways.

Using pbuilder you have to:

- clone the repository (see above)

- create the source package

    cd trunk (the directory where you cloned nproxy code in)

    dpkg-buildpackage -S -uc


- you will find in the upper directory 2 files. The important one is the .dsc file. So switch to that directory 

    cd ..


- and run pbuilder (Google may help you to configure it but it should be quite simple)

    pbuilder --build nproxy_*.dsc (enter the proper file name)



Using a quicker way you should:

- clone the repository (see above) and switch to the directory where the code is

- Start building your package with

    dpkg-buildpackage



You will find the .deb files in the upper directory.

Configuration
-------------

The configuration file is **nproxyd.conf.xml**. Yes, it's XML. I don't like it too but there are good reasons to use it.
Anyway it's a few lines long and it's intended to be read and written by a human with vim. **Really, it's easy!** you shouldn't worry!

There are 2 zones.
The first refers to logging, the second refers to the profiles (i'll tell you in a while what a profile is).

### Logging

You can log to syslog and/or to a file.
For both methods you can specify the log level, which is a number. The bigger, the more details you will receive.

    1  CRITICAL

    2  ERROR

    3  WARNING

    4  NOTICE

    5  DEBUG


Leave empty the log file name or the syslog ident if you want to disable that method.

> NOTE: syslog does not work on windows.


### Profiles
A profile is a particular configuration of NProxy which links a listening address/port with some configuration parameters and with a **WebPlan**.

A **WebPlan**, as explained later, is the way to control NProxy logics.


You can setup as many profiles you want.

- **listen_address**

    The address on which the profile will listen. You can specify any IPv4 or IPv6 address.
    If you need to bind to any IPv4 address, use 0.0.0.0.
    If you need to bind to any IPv6 and IPv4 address, use ::
- **listen_port**

    The port on which the profile will listen. Usually 8080 or 3128.


- **bind_address** (optional)

    The address on which all outgoing connections will bind to.
    You can set it on-demand from the WebPlan.


- **inactivity_timeout**

    After how many seconds without data the connection should be dropped ?
    You can set it on-demand from the WebPlan.


- **max_duration**

    What's the maximum duration of a connection ? Useful to block tunnels or long downloads.
    You can set it on-demand from the WebPlan.


- **shape_after**

    If you want to shape a download, after how many bytes the limit should kick in ?
    You can set it on-demand from the WebPlan.


- **shape_bps**

    If you want to shape a download, how many bytes per second should be the download speed ?
    You can set it on-demand from the WebPlan.


- **template_dir**

    Templates are HTML pages sent to the client in case of errors. Where are those templates located ?


- **script_dir**

    What is the directory containing the LUA files of the WebPlan ?


- **script_file**
    What is the main file of your WebPlan ?


WebPlan Howto
-------------

(TODO)


Lua Command Syntax
------------------

All LUA command are in the nproxy package and namespace.

The list of command is the following:

- **nmail.logmsg(level, message)**

    does not return anything


- **nmail.profile_get_script_dir(connection)**

    returns a string


### Connection commands

- **nmail.connection_set_authuser(connection, name)**

    does not return anything


- **nmail.connection_get_authuser(connection)**

    returns a string


- **nmail.connection_set_variable(connection, name, value)**

    does not return anything


- **nmail.connection_get_variable(connection, name)**

    returns a string



- **nmail.connection_set_inactivity_timeout(connection, int secs)**

    does not return anything


- **nmail.connection_set_max_duration(connection, int secs)**

    does not return anything


- **nmail.connection_set_traffic_shaper(connection, size_t unshaped_bytes, size_t bytes_per_sec)**

    does not return anything



### Request commands

- **nmail.request_force_upstream(connection, host, int port)**

    does not return anything


- **nmail.request_force_bind(connection, host)**

    does not return anything


- **nmail.request_require_auth(connection, type, realm)**

    does not return anything


- **nmail.request_set_traffic_limit(connection, size_t in, size_t out)**

    does not return anything


- **nmail.request_set_traffic_shaper(connection, size_t unshaped_bytes, size_t bytes_per_sec)**

    does not return anything



- **nmail.request_change_url(connection, url)**

    does not return anything


- **nmail.request_get_header(connection, h)**

    returns a string


- **nmail.request_get_header_next(connection, h)**

    returns a string


- **nmail.request_del_header(connection, h)**

    does not return anything


- **nmail.request_del_header_current(connection)**

    does not return anything


- **nmail.request_add_header(connection, h)**

    does not return anything


- **nmail.request_replace_header_current(connection, h)**

    does not return anything



- **nmail.int request_is_transparent(connection)**

    returns 1, respectively, if the request is transparent, 0 otherwise


- **nmail.request_get_host(connection)**

    returns a string


- **nmail.int request_get_port(connection)**

    returns a number


- **nmail.request_get_method(connection)**

    returns a string


- **nmail.request_get_url(connection)**

    returns a string


- **nmail.int request_get_protocol(connection)**


# Response commands

- **nmail.response_get_header(connection, h)**


- **nmail.response_get_header_next(connection, h)**


- **nmail.response_del_header(connection, h)**

    does not return anything


- **nmail.response_del_header_current(connection)**

    does not return anything


- **nmail.response_add_header(connection, h)**

    does not return anything


- **nmail.response_replace_header_current(connection, h)**

    does not return anything


- **nmail.int response_get_code(connection)**




Developement
------------

- Source hosted at [GitHub](https://github.com/ctrix/nproxy)
- Report issues, questions, feature requests on [GitHub Issues](https://github.com/ctrix/nproxy/issues)

Authors
-------

[Massimo Cetra](http://www.ctrix.it/)

* * *

