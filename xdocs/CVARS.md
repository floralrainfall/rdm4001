# CVar List

These can be found in settings.json, created by RDM4001 after running once (depending on what the specific game does)

## Types

### Bool

A boolean value. 0 corresponds to false, 1 corresponds to true. Any value that isnt 0 will resolve as a true.

### String

A string value.

### Float

A floating point number.

### Integer

An integer.

### Vec2/3/4

A vector composed of floating point numbers. The numbers are seperated with a space, e.g.
	0.0 10.0 20.0

## CVars

### cl_copyright

Shows the copyright value, Bool. Default is 1

### cl_loglevel

The log level. Log messages below the level will be hidden from the console. Integer. Default is 2

0. External
1. Debug
2. Info
3. Warning
4. Error
5. Fatal

### cl_showstats

Show scheduler status on the title bar. Boolean. Default is 0

### input_rate

The frame rate in which the GameEventJob task runs. Setting it too low can cause input delay. Float. Default is 20.0

### net_inbandwidth

The maximum incoming bandwidth that ENet will allow. Default is 0, which disables this feature. Integer

### net_outbandwidth

The maximum outcoming bandwidth that ENet will allow. Default is 0, which disables this feature. Integer.

### net_rate

The frame rate in which the Network task runs. Setting it too low can cause latency. Float. Default is 60.0

### net_service

The time that ENet is allowed to service the connection, in miliseconds. Integer. Default is 1

### r_bloomamount

The amount of times the Bloom effect will iterate. Integer. Default is 10

### r_gldebug

Enables GL debug output. Setting cl_loglevel to 0 will make the outputs of the debug more visible. Bool. Default is 0

### r_glvsync

Enables GL vsync. Bool. Default is 0

### r_rate

The framerate in which the Render job will run. Setting it to 0 will make it run at an unlimited speed. Float. Default is 60.0

### r_scale

The framebuffer scale of the rendered scene. Decreasing this will result in performance increases, but will sacrifice visual fidelity. Float. Default is 1.0

### sv_ansi

Allow the server thread to output ANSI title information to the console. Boolean. Default is 1

### sv_maxpeers

The maximum number of peers allowed to be connected to the server. Integer. Default is 32

# Warning

There might be more CVars defined by individual games. It is not the duty of this document to document them all.
