# SimpleG
Ultra fast Windows console based gCode sender for grbl

# Usage
* Copy SimpleG.exe whereever you want to have it.
* Generate your g-Code file you want to stream to the grbl controller.
* Check the SimpleG-end.gcode file and define what you want to happen when you abort the program by hitting CTRL+C
* Start the program by typing: 
```
SimpleG.exe -f test.gcode -F SimpleG-end.gcode -p com6
```

# Parameters
* -p followed by the desired COM-port. For using COM-ports over COM9 you have to type 
```
SimpleG.exe -f test.gcode -F SimpleG-end.gcode -p \\\\.\\COM10
```
* -f followed by the desired gCode file to stream
* -F followed by the abort gCode file you want to stream in case you hit CTRL+C

# Compiling
I developed this with the code:blocks IDE (http://www.codeblocks.org/) together with the mingW compiler (both in one package found here: http://sourceforge.net/projects/codeblocks/files/Binaries/16.01/Windows/codeblocks-16.01mingw-setup.exe)
I provided the project file so you can directly start developing under windows.
Therefore I didn't provide a makefile but that might follow later.

# Binaries
Under bin/Release and bin/Debug you can find precompiled binaries (exe-files) that can be used under Windows 10 64bit.
I didn't test those under any other Windows system. It might work. It might through an error!

#TODO:
* Understand and optimize timouts for receiving single bytes
* Further testing and debugging
* Evaluate why my personal laser and grbl configuration sometimes does not turn of the laser after M5 S0 command followed by G0 commands. It works well with following G1 commands.
* Implement into GUI for users who like single button GUIs... this will likely never happen.
* Provide a makefile for msys and cygwin users