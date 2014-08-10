LUA Scripting Plugin
====================


This is a Lua plugin for scipting purposes for the [Monkey HTTP daemon](http://monkey-project.com/).

You can execute a Lua script and return the response. You get access
to few of monkey's internals in the lua environment.

Requirements
============

* Latest [Monkey HTTP daemon](http://monkey-project.com/) git master.
* [Lua 5.2](http://www.lua.org/)

Installation
============

    cd monkey
    git clone https://github.com/diadara/monkey_lua.git plugins/lua
    ./configure
    make

    # Edit conf/plugins.load and uncomment line with monkey-lua.so
    # Edit conf/plugins/lua.conf
    # put your scripts in htdocs 
    ./bin/monkey

Usage
=======

The plugin exposes a table `mk` through which you can get the request
object and also set the response.

## mk.print

mk.print takes a string and prints it one the response body.



## mk.config
config table contains information about monkey server and the current configuration. This table consists of-

### kernel_features
    This table consists of 3 boolean fields TCP_FASTOPEN, SO_REUSEPORT, TCP_AUTOCORKING.

### worker_capacity
Monkey servers current worker capacity.

### workers
Number of running workers.

### max_load
Maximum load that can be handled by the server currently.

### manual_tcp_cork
Boolean indicating whether manual_tcp_cork.

### fdt
Boolean indicating whether fdt enabled.

### is_daemon
Boolean indicating whether monkey running in daemon mode.

### is_seteuid
Boolean indicating whether seteuid.

### scheduler_mode
String indicating the current scheduler algorithm.

### server_config
Current  server configuration.

### listen_addr
Current server listen address.

### server_addr
Current server address.

### server_software
Current server software string.

### user
Username under which server is running.

### user_dir
current user directory.

### pid_file_path
PID file path

### server_conf_file
Server configuration file.

### mimes_conf_file
Mimetypes configuration file.

### plugin_load_conf_file
Plugin load configuration file.

### sites_conf_dir
Sites configuration directory

### plugins_conf_dir
Plugin configuration directory

### transport
Current transport (http/https).

### transport_layer
Current transport layer.



## mk.request
Request table consists of following items:

### server_software
Server software string.

### document_root
Document root for the current request

### server_name
Server name for the current request.

### server_addr
Current server address.

### server_port
Current server port.

### script_filename
Script file name for the current request.

### request_method
Current request method.

### remote_addr
client address.

### remote port
client port.

### request_uri
Request url for the current request.

### query string
query string

### data
`POST` or `PUT` data.

### headers
Table with the http headers from the current request.

### parseargs()

Function which parses the query string and returns two tables one with
keys as the query string variable and value as it's value and other
with key and other with query string variable as the key and value as
an array of variable values.

    t1, t2 = parseargs()

### parsedata()

Function which parses the post data and returns two tables

    t1, t2 = parsedata()



## mk.response
Response table consists of following items:

### status
Set this variable to current request response status code. Default value set to 200.

### headers
Set http headers with this table

    headers["Content-type"] = "text/json"

    
## mk.cookies
The cookie table consists of following items.

### get_cookies()
The function returns a table of cookies the client has sent.

### mk.set_cookie()
The function takes a table as argument and sets the cookie accordingly.

    cookies.set_cookie{name="name", value="nithin", httponly = true, secure=true, domain="nithinsaji.in", expiry=os.time()+60*60*8}


