# Sound-RT

Ray-traced sound rendering framework and demos

## Software Requirements

* CMake 3.12 minimum
* Compiler MSVC v143
* All the necessary external libraries are packaged with the project

## Hardware Requirements
* CPU with support for sse
* GPU with support for OpenGL 4.0 or newer

## How to Build and Run
1. Generate CMake files in build folder
2. Build one of the two demos(one of them is in git branch 'main', other is in 'audio-scene'). It will appear under ./bin/audio-proj/{target}/audio-proj.exe
3. It will run from Visual Studio, however to run it as an executable, the executable itself should be placed in the 'bin' folder of the project
4. When switching between git branches CMake files should be recompiled. To be extra safe, deleting contents of the 'build' folder is advised