# Unreal Public Source Distribution

This is OldUnreal's updated version of the original Unreal Public Source
Distribution (PubSrc224v).

## What is included in this SDK?

* UnrealScript source code for the following original UPackages: Core, Editor,
  Engine, Fire, IpServer, Render, UBrowser, UDSDemo, UMenu, UPak, UWindow,
  UnrealI, UnrealShare. This source code is included merely for convenience as
  it can also be exported directly from the corresponding .u files.

* C++ headers and Windows .lib/resource files for native packages that were
  included in PubSrc224v: Core, Engine

* C++ headers and Windows .lib/resource files for native packages that were
  previously released by Ion Storm: Editor, IpDrv, Fire

* C++ headers and Windows .lib files required to build other packages:
  Render

* Full C++ source code for video renderers that were included in Unreal
  Tournament's PubSrc432: D3DDrv

* Full C++ source code for some of the audio drivers and video renderers
  developed for or contributed to the OldUnreal Community Patches: ALAudio
  (OldUnreal version), D3D8Drv (OldUnreal version) D3D9Drv (OldUnreal version),
  OpenGLDrv (OldUnreal version), XOpenGLDrv, and ICBINDx11Drv. Note that
  OldUnreal's D3D8Drv, D3D9Drv, and OpenGLDrv are based on Chris Dohnal's UTGLR
  Renderers.

* Full C++ source code for native packages that were included in PubSrc224v:
  Setup, Launch, UCC

* Full C++ source code for native packages that were included in OpenUT: SDLDrv
  (OldUnreal/UTPG version), SDLLaunch (OldUnreal/UTPG version)

* Full C++ source code for packages OldUnreal got permission to release:
  UnrealEd, Window, WinDrv

## What is not included in this SDK?

* C++ source code for core engine packages: Core, Engine, Editor, Fire, Render,
  IpDrv, UPak

* C++ source code for older audio drivers and renderers: Galaxy, SoftDrv,
  GlideDrv, MetalDrv, ...

## Building C++ packages included in the SDK

Before building, you should copy the contents of this repository into your
Unreal root folder. To build on Windows, you will need a fairly recent version
of Visual Studio (we recommend version 2022) with the CMake tools installed. On
Linux, you will need clang and CMake.

#### Windows

This SDK works for Windows 7 and up. To build the packages included in the SDK,
you should open a Visual Studio command prompt and set up a CMake build folder
as follows:

```
cd /path/to/Unreal/
mkdir cmake-build
cd cmake-build
cmake -A Win32 -DCMAKE_INSTALL_PREFIX=C:\Path\To\Unreal\System\ -DCMAKE_TOOLCHAIN_FILE=C:\Path\To\Unreal\cmake\MSVC.cmake ..
```

Note:
- If you want to target the 64-bit x86 version of Unreal, you should drop the
"-A Win32" parameter and point your install prefix to the System64 folder.
- Our CMakeLists.txt automatically looks for 3rd party build dependencies in
the build folder that is included in the SDK. If you want to manually specify the
dependencies path, you should add the following option to your cmake configure
command:
```
-DOLDUNREAL_DEPENDENCIES_PATH=C:\Path\To\Unreal\Dependencies\
```

After configuring the CMake build folder, you can build as follows:
```
cmake --build . --config Release --target install --parallel 8
```

#### Linux

On Linux, you can set up a CMake build folder as follows:
```
cd /path/to/Unreal/
mkdir cmake-build
cd cmake-build
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/Unreal/<compiler>-<arch>-linux-gnu.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/Unreal/<System> -DOLDUNREAL_DEPENDENCIES_PATH=/path/to/Unreal/build/Linux/<arch>/Release/ ..
```

In the cmake command above, you should replace <System> with the name of the System folder on your platform. This means:
* System for x86/i386
* System64 for amd64/x86_64
* SystemARM64 for aarch64/arm64

You should also replace <arch> with the CPU architecture of your platform. Valid options are x86, amd64, and arm64.

Finally, you should replace <compiler> by clang (for x86 and amd64) or gcc (for arm64).

You can then build as follows:
```
cmake --build . --config Release -j 8 --target install
```

## Building UnrealScript packages included in the SDK

The UnrealScript packages included in this SDK are core packages. They should
not be rebuilt, as doing so **WILL** break network compatibility.

## License/Copyright

See LICENSE.md