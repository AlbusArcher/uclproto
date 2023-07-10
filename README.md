# uclproto
A protoc plugin, support proto message load from ucl(universal configuration language) configuration file.

---
## Build Plugin
1. Preparation
   Build [protobuf](https://github.com/protocolbuffers/protobuf) and [libucl](https://github.com/vstakhov/libucl), And install them to your project third-dir. The structure of third-dir show like this project third directory, Otherwise, The CMakeLists of this project should be changed.
   
2. Build Plugin
   Use THIRD_DIR specify you project third-dir path (The default path is this project's third directory).
   Use UCL_INCLUDE_DIR specify libucl's headers directory name in third-dir-include (The default dir-name is libucl).
   Example:
   > cmake -DTHIRD_DIR="/your_project/third" -DUCL_INCLUDE_DIR="your_ucl" -S . -B build && cmake --build build
   Plugin executable will output in this project's bin directory

3. Build Test
   Macros Like Build Plugin
   Example:
   > cd test && cmake -DTHIRD_DIR="/your_project/third" -DUCL_INCLUDE_DIR="your_ucl" -S . -B build && cmake --build build && cd build && ctest

---
## Use protoc plugin
> ${PROTOC_BIN} --plugin=protoc-gen-uclproto=${PLUGIN_BIN} --cpp_out=. --uclproto_out=. ${NAME}.proto
