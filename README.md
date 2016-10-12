# SimpleG
Ultra fast windows console based gCode sender for grbl

# Usage
* Copy SimpleG.exe whereever you want to have it.
* Generate your g-Code file you want to stream to the grbl controller.
* Check the SimpleG-end.gcode file and define what you want to happen when you abort the program by hitting CTRL+C
* Start the program by typing: 
```SimpleG.exe -f test.gcode -F SimpleG-end.gcode -p com6

# Parameters
* -p followed by the desired COM-port. For using COM-ports over COM9 you have to type \\\\\\\\.\\\\COM10
* -f followed by the desired gCode file to stream
* -F followed by the abort gCode file you want to stream in case you hit CTRL+C

#TODO:
* Understand and optimize timouts for receiving single bytes
* Further testing and debugging
* Evaluate why my personal laser and grbl configuration sometimes does not turn of the laser after M5 S0 command followed by G0 commands. It works flawless with following G1 commands.
* Implement into GUI for users who like single button GUIs... this will likely never happen.