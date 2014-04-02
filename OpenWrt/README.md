This guide aims to give a short introduction to compiling packages (specifically p2p-dprd) for an OpenWRT router using the OpenWRT build system.

### Limitations of scope:
* This guide is valid for programs written in gcc-compatible C, and does not handle compilation of packages in C++ etc. Most of the information should be valid for the case of C++ (using g++) as well, though.
* External libraries are not handled in this guide, but any library fulfilling the criteria above should be compilable and runnable. It is our experience, however, that there are several pitfalls of using libraries which have not been sufficiently tested and verified on more exotic platforms such as MIPS and ARM [1]. 

_[1] Several software breaking bugs were encountered trying to use [libmsgpack](http://msgpack.org/) for the initial version of p2p-dprd, most of which seems to be rooted in incompatibility with the target platform. OpenWrt does, however, ship with a [package manager](http://wiki.openwrt.org/doc/techref/opkg) which provides most common libraries as binaries for your platform, directly to the device. Neat!_

## Why is this useful? ##
In order to compile executables for a device (router) running OpenWrt, we are going to need to cross-compile the code on an x86 machine using a cross-compilation toolchain. OpenWrt provides this toolchain as a heavily modified and extended version of [buildroot](http://buildroot.uclibc.org/), which integrates a toolchain for OpenWrt with a custom package build system, allowing easy creation of [opkg](http://wiki.openwrt.org/doc/techref/opkg) compatible packages.

Sounds easy, right? Well, in theory it is. In practice, however, there are a number of quirks and incompatibilites, as well as a lot of outdated documentation which can prevent the smooth sailing of package creation which OpenWrt advertises. 

This guide aims to address the problems and quirks which we have **already battled**. It is not a [complete guide to OpenWrt](http://wiki.openwrt.org/doc/start) by a long shot, and should be read in conjunction with the [official documentation](http://wiki.openwrt.org/about/toolchain) on using the toolchain. We also focus on compiling our own code, and this is a such not a general purpose tutorial.

## Prerequisites ##
To get started you will first need to complete the following steps:

* If you haven't done so already: [install OpenWrt on you router](http://wiki.openwrt.org/doc/howto/generic.flashing). For development purposes, it is recommended to get the latest (trunk) build (if available). Depending on your hardware, you _might_ have to compile the firmware yourself (which can be achieved using the tools described in this guide). Set up the router as needed and make sure it works.

* Get the OpenWrt source code _which corresponds to the version running on your device_. In our case, we will get the trunk code (bleeding edge) using the snippet shown below.
  
```
#!bash
$ git clone git://git.openwrt.org/openwrt.git --depth 1
```
_Note: This creates a shallow copy, which is fine for our purposes, and saves a lot of time (and space)_

## Notes about the buildroot / SDK ##
The OpenWrt project provides __two__ main ways to get your software compiled for and running on an OpenWrt device. The first option is to download and build the complete toolchain (_buildroot_, which is explained in this guide), the second is to download and/or build the SDK, a stripped down version of the toolchain intended to _only_ build packages, and not firmware images. Even though the SDK seems ideal (being that we are only really interested in compiling packages), it has shown to be quirky, poorly documented and down-right broken in most cases. Therefore, we recommended doing it "the hard way" and compile the complete toolchain with all needed bells and whistles from source.

The drawbacks of the buildroot approach is of course that it will require a fair bit of compilation time, is not relocatable, and needs configuration for your specific system(s). The advantage is that __it works__, as opposed to the SDK (which might work, but hasn't so far).

## Installing the buildroot ##
The instructions for installing the OpenWrt buildroot are given [here](http://wiki.openwrt.org/doc/howto/buildroot.exigence), and can be easily followed. There are a few things you should know, though:

### Before building, go through the following steps:
1. Make sure your system has the required dependencies installed. There is a table given on the referenced guide on the official site which covers necessary prerequisites for most popular distros. There are instructions in the official docs to run make to discover missing dependencies, but this has not tended to work as expected, and is a waste of time.

2. If your program has any dependencies, these will need to be cross-compiled and installed in the buildroot alongside the program itself. To achieve this, we need to download the package definitions from the OpenWrt package repository. Thankfully, there's a script for that. The by far easiest way is to run (as described in the official docs) the following in the source root directory:

```
#!bash

$ ./scripts/feeds update -a
$ ./scripts/feeds install -a
```
### Configuring buildroot
When we're all as far as dependencies and prerequisites go, we need to configure our buildroot in accordance with our target hardware (router), and program (p2p-dprd). These steps are described [here](http://wiki.openwrt.org/doc/howto/build) under _Image configuration_, but a short version is given below.

1. Run `make menuconfig`. If all goes well, you will be presented with the configuration interface.
2. Configure buildroot for your target device. There are several ways to go about doing this. If you are using an officially supported device, chances are you will find a preconfigured profile under `Target profile` (e.g. for the Buffalo WZR-HP-AG300H which have been used during p2p-dprd development). If there is no profile, you will need to choose a target system and a subtarget from the menues to suit your device. Generally speaking, choosing the correct chipset of your device from `Target system` and the generic profile under `Subtarget` will do the trick.
3. In our case, we do not need the actual firmware image. Therefore, you can deselect any selected options under `Target image`. If you wish to generate a compatible firmware image along side the buildroot, leave this section as is.
4. If you need to use any particular std libs (openwrt uses uClibc by default), this can be configured under `Advanced configuration options`. Compilation of gdb and debugging symbols _should_ be selected by default, but can be configured there as well. (Note: You need to select the option to actually access the menu with relevant options).

5. Select `Build the OpenWrt based toolchain`. We do _not_ want the SDK (reasons given above).

6. If you need to compile packages with debugging symbols, gdb can be chosen under `Base system`. Any libraries which your program(s) depend(s) on can be selected from the `Libraries` menu.

7. Exit and save your configuration.

### Building the toolchain

You are now ready to build the toolchain.

```
#!bash

$ make -j 3 V=99
```
_Note: set the j-parameter to your number of cores + 1. This saves A LOT of time during compilation_

If everything works as expected and intended, the buildroot will compile in the current source folder.

## Using the buildroot
Your OpenWrt buildroot is now situated in the source code root directory, and contains a deeply nested system of source code, configuration files, compilers and package definitions. Luckily, we do not need to modify most of it, as we are only interested in compiling packages.

### Using the compiler (stand alone)
First, if you need to use the raw gcc compiler for your build (and are not interested in creating an opkg-compatible package), it can be found in `/staging_dir/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33.2/bin` Of course, the _toolchain*_ line will vary depending on platform and version. In this folder you should be able to use any of the executables to compile executable which are runnable on the target device.

### Using OpenWrt package build system
In the long run, it is easier to manage your executables as installable (and uninstallable/upgradable) packages using the `opkg` package manager on your OpenWrt device. Therefore, we wish to compile our code and wrap it as an opkg package. To achieve this, we use the OpenWrt package buildroot.

In short terms, the package building system requires you to create a specialized OpenWrt makefile which wraps your existing source code (and its makefile/build system), compiles it and creates an opkg package. Details on creating the OpenWrt-makefile needed can be found [here](http://wiki.openwrt.org/doc/devel/packages), but p2p-dprd has an included OpenWrt-makefile located in p2p-dprd/OpenWrt/Makefile.OpenWrt which does the job for our purposes.

We need to define our custom package in the buildroot package index, and then run build on it. The following steps have to be taken:

* Place Makefile.OpenWrt in `<buildroot_root>/package/<package_name>` -> In our case: `trunk/package/p2p-dprd` The file must be named `Makefile`.

* Place the p2p-dprd source code in the same folder.

_These two are best acheived by symlinking the makefile and source code to the corresponding locations, so that we have `package/p2p-dprd/Makefile` and `package/p2p-dprd/<source code dir>`_

* Now go to the buildroot root folder (trunk, most likely) and run `./scripts/feeds install p2p-dprd` (p2p-dprd can be substituted for any package name, of course). The p2p-dprd package should now be available in the package index of the buildroot.

* run make menuconfig and naviagate to the section which corresponds to the category given in the openwrt-makefile of the package (utilities for p2p-dprd). You package should now be selectable. Select it as <M>, which builds is as a stand-alone module (package). Any libraries (dependencies) must also be selected at this stage.

* Navigate back to your buildroot root and make with `make package/p2p-dprd/compile V=99`

* If the previous step was successful and the package compiled, it will now be located in `<buildroot root>/bin/<arch>/packages/<package-name>_<arch>.ipk`

### A few notes on OpenWrt makefile and compiling packages
As previously stated, OpenWrt wraps your existing code (and it's build system) within its own build system. This imposes a couple of requirements on your code:

#### Code requirements
1. Your code should build using make. You should also implement `make clean` - see the p2p-dprd makefile for examples.

2. In your OpenWrt makefile package definition (under `define Package/<package name>`) the _DEPENDS_ section __must__ include __all__ dependencies of your package. The exact name of the dependency must match the name of the dependency in your buildroot package index. For example, p2p-dprd has _libconfig_ as a dependency. If _libconfig_ had not been listed under _DEPENDS_ and was not available in the package index (either through the official repository or through adding it yourself), the p2p-dprd package would __not__ compile at all, and would probably not give a sane error message upon exiting the build process.

#### Easily automating the build->upload->install process
As long as the build system is set up correctly, automating the build process as much as possible is a potentially huge time saver during development and testing. This is especially true is the package needs to be uploaded and ran on a remote device running OpenWrt. In the p2p-dprd repository, under _OpenWrt_, there is an example of a simple bash script which can be used to clean, compile, upload and install the package in one go. The script is named _remote_install_p2p-dprd.sh_, and could be easily modified for use on your dev system.

#### Keep your source code clean!
Here's a typical example of a hard to recognise but very common problem: You've been hacking away on your code, making sure it compiles on your dev machine (running make on the source code and running it on your dev box). 

Eventually you want to try it on your OpenWrt device, so you try to build the package using `make package/<package name>/compile` in the OpenWrt buildroot. Your build fails horribly and the error messages make no sense, complaining about erroneous object files. You run `make package/<package name>/clean` to resolve the issue, and try to compile again. The problem persists. Why? 

Your source code folder is more than likely polluted by the x86 object files you've previously compiled. Since these object files are incompatible with your cross compiles, and are created with a completely separate instance of make, the buildroot doesn't recognize them during the clean process, but still tries to link the x86 binaries during the build. The result is a horrible mess and can be avoided by simply __making sure to run make clean in the source directory before trying to build the package.__

## Installing packages on the OpenWrt device
If you've successfully built your custom package, you can transfer it to your device using, for example, ´scp´ and manage it using the built-in package manager:

```
#!bash
# Install
$ opkg install <package_file_path.ipk>

# Run
$ <package name>

# Remove
$ opkg remove <package name>
```

## Resources / docs
[OpenWrt docs - creating packages](http://wiki.openwrt.org/doc/devel/packages)
[OpenWrt docs - buildroot](http://wiki.openwrt.org/about/toolchain)
[OpenWrt package hello world tutorial](http://www.ccs.neu.edu/home/noubir/Courses/CS6710/S12/material/OpenWrt_Dev_Tutorial.pdf)
