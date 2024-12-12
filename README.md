Going thru [Vulkan Tutorial](https://vulkan-tutorial.com).

# Build

Hardcoded:

 * Path to `VulkanSDK`, see CMakeLists.txt; install from <https://vulkan.lunarg.com/sdk/home#windows>.
 * Path to [`vcpkg`](https://github.com/microsoft/vcpkg) toolchain file, see build.cmd, `-DCMAKE_TOOLCHAIN_FILE`.

```
:: install VulkanSDK
:: install vcpkg, then:
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>\scripts\buildsystems\vcpkg.cmake
```
