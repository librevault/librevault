Build notes
===========

Dependencies
------------

These dependencies are required:

| Library      | Minimum version |
|--------------|-----------------|
| GCC or Clang | 4.9, 3.4        |
| CMake        | 3.2             |
| Boost        | 1.58.0          |
| OpenSSL      | 1.0.1           |
| Crypto++     | 5.6.2           |
| Qt5          | 5.3             |
| Protobuf     | 3.0             |

Other dependencies are listed in [dependencies.md](dependencies.md). Most of them are included in the source tree as submodules or as amalgamated sources (jsoncpp and sqlite).

Building from source
--------------------

After all the dependencies are built and installed, invoke these commands in source directory, cloned from Git or unpacked from [tarball](https://releases.librevault.com/sources/librevault_latest.tar.gz):  
**Note**: If you are the cloning source code tree from Git, make sure, that you have cloned all the submodules by command `git submodule update --init`

```
mkdir build && cd build
cmake .. && cmake --build .
```

###Installing
You can perform installation using this command: 
```
sudo make install (or sudo checkinstall)
```

Dependency build instructions: Ubuntu & Debian
----------------------------------------------

* On at least Ubuntu 15.10+ and Debian 7+ you can install most of the dependencies by using this command:

```
sudo apt install build-essential cmake libboost-all-dev libssl-dev qtbase5-dev libqt5svg5-dev libqt5websockets5-dev
```

* If `libcrypto++-dev` version is < 5.2, then you should [download](http://www.cryptopp.com/#download), unpack and build it yourself:

```
sudo make install (or sudo checkinstall)
```

* Then, [download](https://github.com/google/protobuf/releases) Protobuf 3.0 source code (it has no Ubuntu or Debian packages now). Then, unpack and build it:

```
./autogen.sh && \
./configure && \
make && \
sudo make install (or sudo checkinstall)
```

### Ubuntu 16.10

All dependencies are available from the repository. Simply install them using the following command:

```
sudo apt install build-essential cmake libboost-all-dev libssl-dev qtbase5-dev libqt5svg5-dev libqt5websockets5-dev libcrypto++-dev qttools5-dev-tools protobuf-compiler libsqlite3-dev
```

### macOS Sierra

Make sure you have Homebrew (http://brew.sh) installed. Then:

1. `brew install cmake`
2. `brew install qt5`
3. `brew install boost`
4. `brew install openssl`
5. `brew install protobuf`
6. `brew install cryptopp`

7. Install Sparkle update framework dependency for the graphical interface:

	1. Download Sparkle from https://sparkle-project.org
  	2. Make sure a ~/Library/Frameworks folder exists: `mkdir ~/Libary/Frameworks`
  	3. From the downloaded Sparkle folder: `cp -R Sparkle.framework ~/Library/Frameworks`

8. Clone the repository and change to its directory:

	`git clone https://github.com/Librevault/librevault.git && cd librevault`

9. Install the submodules:

	`git submodule update --init`

10. Get an application package directory structure ready:

	`mkdir -p product/Librevault.app/Contents/MacOS`

11. Build Librevault:

	1. `mkdir build && cd build`
	2. `cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt5) -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) -DOPENSSL_INCLUDE_DIR=$(brew --prefix openssl)/include -DOPENSSL_LIBRARIES=$(brew --prefix openssl)/lib -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_INSTALL_PREFIX=$(pwd)/../product/Librevault.app/Contents/MacOS`
	3. `cmake --build .`

12. And generate the app bundle:

	`sudo make install`

You’ll find the generated Librevault.app bundle in the product folder.
