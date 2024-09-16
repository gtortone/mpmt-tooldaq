
# ToolDAQ Application template

This template can be used to develop a brand new ToolDAQ toolchain with minimal effort.

## Usage

- Provides ToolFrameworkCore and ToolDAQFramework libraries in the same directory (e.g. /opt)

```
git clone https://github.com/ToolFramework/ToolFrameworkCore.git

cd ToolFrameworkCore
make clean
make
```

```
git clone https://github.com/ToolDAQ/ToolDAQFramework.git

cd ToolDAQFramework
make clean
make
```

- Install following libraries with your package manager utility

```
boost_date_time
boost_iostreams
boost_serialization
cppzmq
```

### Build with GNU-Make

- edit Makefile and modify `Dependencies` accordingly to your setup

- start build

  ```
  make
  ```

### Build with CMake

- edit CMakeLists.txt and modify `DEPENDENCIES_PATH` accordingly to your setup

- start build

  ```
  mkdir build
  cd build
  cmake ..
  make
  ```
