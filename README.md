# SimpleG
Ultra fast windows console based gCode sender for grbl

# Usage
* Copy SimpleG.exe whereever you want to have it.
* Generate your g-Code file you want to stream to the grbl controller.
* Check the SimpleG-end.gcode file and define what you want to happen when you abort the program by hitting CTRL+C
* Start the program by typing: 
```SimpleG.exe -f test.gcode -F SimpleG-end.gcode -p com6

# Comments
* Please change the parameters according to your configuration. Com-ports over COM9 you have to type \\\\\\\\.\\\\COM10
* All other parameters 