# YAlpao

[Yorick](http://yorick.github.com/) interface to Alpao deformable mirrors.


## Usage

If the plug-in has been properly installed (see [Installation](#installation)
below), it is sufficient to use any of its functions to automatically load the
plug-in.  You may force the loading of the plug-in by something like:

```cpp
#include "alpao.i"
```

or

```cpp
require, "alpao.i";
```

in your code.

To open an Alpao deformable mirror in Yorick, call:

```cpp
dm = alpao_open(config);
```

where `config` is the path to the device configuration file (with an optional
`.acfg` extension).  The result, `dm`, is a Yorick object representing the
deformable mirror.  The device is automatically closed when `dm` is no longer
referenced.

The object `dm` can be used to retrieve parameters as follows:

```cpp
val = dm.key;
val = dm("key");
```

where `key` is the symbolic name of the parameter while `"key"` is the name of
the parameter as a string.  The case of the characters does not matter.  For
instance, any of:

```cpp
dm.NbOfActuator
dm("NbOfActuator")
dm.nbofactuator
dm("nbofactuator")
```

yields the number of actuators of `dm`.

The deformable mirror object `dm` can also be used as a function or a
subroutine to send a command to the deformable mirror:

```cpp
dm, x;      // send command x
xp = dm(x); // send command x and return actual command
```

where `x` is a vector of `dm.NbOfActuator` floating-point values and `xp` is
`x` clipped to the limits imposed by the deformable mirror.

The last commands sent to the deformable mirror are retrieved by:

```cpp
xp = dm();
```

To reset the shape of the deformable mirror, call:

```cpp
alpao_reset, dm;
```

To stop controlling the deformable mirror, call:

```cpp
alpao_stop, dm;
```

Finally, you may call:

```cpp
val = alpao_get(dm, key);
alpao_set, dm, key, val;
```

to repectively get or set the value `val` of a parameter `key` of the
deformable mirror `dm`.  Here, `key` is a string with the name of the parameter
(ignoring the case of characters).


## Installation

In short, building and installing the plug-in can be as quick as:

```sh
cd $BUILD_DIR
$SRC_DIR/configure --cflags="-I$PREFIX/include" --deplibs="-L$PREFIX/lib64 -lasdk"
make
make install
```

where `$BUILD_DIR` is the build directory (at your convenience), `$SRC_DIR` is
the source directory of the plug-in code and `$PREFIX` is the installation
directory of the Alpao Software Development Kit (SDK).  The build and source
directories can be the same in which case, call `./configure ...` to configure
for building.

More detailled installation explanations are given below.

1. You must have Yorick installed on your machine.

2. Unpack the plug-in code somewhere.

3. Configure for compilation.  There are two possibilities:

   For an **in-place build**, go to the source directory of the plug-in code
   and run the configuration script:
   ```sh
   cd SRC_DIR
   ./configure
   ```
   To see the configuration options, type:
   ```sh
   ./configure --help
   ```

   To compile in a **different build directory**, say `$BUILD_DIR`, create the
   build directory, go to the build directory, and run the configuration
   script:
   ```sh
   mkdir -p $BUILD_DIR
   cd $BUILD_DIR
   $SRC_DIR/configure
   ```
   where `$SRC_DIR` is the path to the source directory of the plug-in code.
   To see the configuration options, type:
   ```sh
   $SRC_DIR/configure --help
   ```

4. Compile the code:
   ```sh
   make clean
   make
   ```

5. Install the plug-in in Yorick directories:
   ```sh
   make install
   ```
