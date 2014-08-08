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



## mk.request



## mk.response



## mk.cookie