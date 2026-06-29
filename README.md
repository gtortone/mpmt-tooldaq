
# ToolDAQ mPMT Application 

This ToolDAQ application can easily build for ARMhf using Docker image available here:

   https://github.com/gtortone/debian-cross-armhf

## Usage

### Build with CMake

   ```
   cmake -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-linux-gnueabihf.cmake

   make -j -C build-arm
   ```

### Output files

   - executable dynamic linked:  ```build-arm/mpmt-tooldaq```
   - executable statically linked:  ```build-arm/mpmt-tooldaq.static```
   - configuration files: ```build-arm/configfiles```
