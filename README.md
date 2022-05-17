# hellouvulkan

Requires VulkanSDK 1.3 for using VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME and drivers that support it, I have only tested with win10 and linux mint.

I have used CMake Tools extension with vscode for linux: https://code.visualstudio.com/docs/cpp/cmake-linux

With windows10 + visual stdudio, the project should? work if vulkansdk is configured and Visual Studio has CMake projects
installed by just right click the folder and open with Visual Studio. It should compile with ninja without having
to manually generating Visual Studio solution with cmake.

Some problems I have encountered:

For Linux you can add: "cmake.debugConfig": { "cwd": "${workspaceFolder}" } into cmake tools settings to configure
the working directory to be the same as workspace directory. Thus you don't need to copy assets over to built folder.
https://github.com/microsoft/vscode-cmake-tools/issues/1395

In Windows with Visual studio you can add currentDir to configure the working directory per project. https://docs.microsoft.com/en-us/cpp/build/launch-vs-schema-reference-cpp?view=msvc-160
This can be done in visual studio 2019 using the solution explorer and pressing switch views button to change into CMake target views, then right click project and add debug configuration.
Adding "currentDir": "${workspaceRoot}" to every project manually is a bit pain though. It should use the the global launch.vs.json, so these should be configured for the projects mentioned in there.
Adding new project would require adding the currentDir manually, and preferably moving the configuration out from .vs-folder.

In case validation doesn't work, or tries to use wrong one: https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/master/LAYER_CONFIGURATION.md
In windows set environment value: VK_LAYER_PATH=C:\VulkanSDK\1.3.211.0\Bin or the version of vulkan thats being used.

I did get weird popup with Visual Studio when it is supposed to compile, after generating CMake cache.
It was due to having DWORDS in windows environment settings.
So checking registry for system variables: HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment and
local variables: HKEY_CURRENT_USER\Environment might be worth it.
