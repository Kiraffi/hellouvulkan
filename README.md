# hellouvulkan

For Linux you can add: "cmake.debugConfig": { "cwd": "${workspaceFolder}" } into cmake tools settings to configure the working directory to be the same as workspace directory. Thus you don't need to copy assets over to built folder. https://github.com/microsoft/vscode-cmake-tools/issues/1395

In Windows with Visual studio you can add currentDir to configure the working directory per project. https://docs.microsoft.com/en-us/cpp/build/launch-vs-schema-reference-cpp?view=msvc-160
This can be done in visual studio 2019 using the solution explorer and pressing switch views button to change into CMake target views, then right click project and add debug configuration.

Also for windows, you need to copy sdl2 development libraries into external/sdl2 folder. Mostly the libs, the header includes should already be in there. https://www.libsdl.org/download-2.0.php


