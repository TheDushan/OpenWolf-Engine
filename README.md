OpenWolf game engine
====================================

## Description

------------------------------------------------------------------------------------
OpenWolf is an heavy updated id Tech 3 game engine. The project is a fork of ET:XreaL (ET:XreaL is a subproject bringing all XreaL enhancements to Return to Castle Wolfenstein: Enemy Territory) . This is just a hobby project and this is an effort to update the ET-Xreal mod with bug-fixes and some add modern conveniences. 

This is merely an attempt to improve the engine and to bring it to some standards in the new millennium.

Source code available at GitHub is under the GPL v3 or later, binaries are distributed under the GPL v3.
------------------------------------------------------------------------------------
------------------------------------------------------------------------------------
## How to compile

Use cMake to generate your `make` or `Visual Studio 2019` solution.
``On initial run, cMake will show error where user need to choose name of the application, the application stage of development and the application version. ``

![alt text](https://cdn.discordapp.com/attachments/489104059027030027/815608999553794058/Compiling.jpg)

![alt text](https://cdn.discordapp.com/attachments/489104059027030027/815609028985880576/compiling_1.jpg)

### *Linux*
* Instructions are only for Ubuntu based distributions
* Open a terminal window and change to the directory containing this readme
* Install necessary libraries: 
    - `sudo apt install cmake-qt-gui`
    - `sudo apt install g++`
    - `sudo apt install libsdl2-dev`
    - `sudo apt install libjpeg-turbo8-dev`
    - `sudo apt install libcurl4-openssl-dev`
    - `sudo apt install libpng-dev`
    - `sudo apt install libopenal-dev`
    - `sudo apt install libogg-dev`
    - `sudo apt install libvorbis-dev`
    - `sudo apt install libssl-dev`
    - `sudo apt install libncurses-dev`
    - `sudo apt install libbz2-dev`
    - `sudo apt install libgeoip-dev`
    - `sudo apt install mysql-client`
    - `sudo apt install libmysqlcient-dev`
    - `sudo apt install libfreetype-dev`
    - `sudo apt install gobjc++`
* Generate `Unix makefile`
  * NOTE: Only x64 build is supported.
* Switch to the your `build` directory where `make` file is located.
* Run `make`

### *Windows (Visual Studio 2019-compile)*

* Download and install `Visual Studio 2019 Community build` from https://www.visualstudio.microsoft.com/downloads/
* Download and install `cMake` from https://cmake.org/download/
    * Necessary libraries for the Windows building are already present in the `libs` folder in the repo
* Generate Visual Studio 16 2019 solution.
    * NOTE: Only x64 build is supported.
* Switch to the your `build` directory where `yourapp` file is located.
* Run `yourapp` vs solution
* Press F5 to build it and that is all.

## Supported platforms
* 64-bit Linux distributions based on Ubuntu.
* 64-bit Windows 10.

## Legal and extra information

Whole project inherited the license GNU GPL v3 from [Wolfenstein: Enemy Territory](https://github.com/id-Software/Enemy-Territory) and [ET:Xreal](https://sourceforge.net/p/xreal/ET-XreaL/ci/master/tree/) mod.

* For full list of [Credits](https://github.com/TheDushan/OpenWolf-Engine/wiki/Credits) 
* For full list of [Licenses](https://www.gnu.org/licenses/gpl-3.0.en.html) 
* For full list of [Additional Licenses](https://github.com/TheDushan/OpenWolf-Engine/wiki/Additional-Licenses) 
