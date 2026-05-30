# nekomata2

Just a plain 3D game engine using modern Vulkan.

I don't expect it to be anywhere near ready to make actual stuff with; I mostly intend it to be a learning project.
Well, maybe in the future..

## Current Features

- Multithreading (Main and render thread split)
- ECS-based world
- Mesh rendering
- Mesh LODs
- Async insertion of mesh LODs
- Textures
- Async loading of KTX2 textures
- Bindless textures and samplers
- Font rendering with on-the-fly font atlas generation (currently kind of hardcoded and not exposed to user code yet)
- Basic script system

## Planned Features

- Lighting, deferred lighting, and PBR
- Shadow mapping
- A more full UI solution
- Model loading (most likely glTF)
- Input handling
- Physics
- `VK_EXT_descriptor_heap`-based texture and sampler tables
- Pipeline caching via both `vk::PipelineCache` and via `VK_KHR_pipeline_binary`
- Ray tracing
- Render graph
- Post-processing effects (SMAA, bloom, etc.)

## Building

Note that the version numbers provided are what the project works with. Earlier or later versions of a given dependency might or might not build/work.

To build the project, you need the following dependencies:
- Base development packages (e.g. `base-devel` on Arch Linux)
- CMake 4.0 or later
- Clang 22 or later (GCC may work but not tested and is not really supported in the long run)
- Vulkan SDK (the project typically requires the very latest version of the SDK but a couple versions back might work fine)
- Slang compiler v2026.9.1 or later
- SDL3 3.4.8 or later
- FreeType2 2.14.3 or later
- utfcpp 4.0.9 or later
- tcmalloc 4.6.5 or later (part of gperftools)

On CachyOS, you can install all of the dependencies as follows:
```shell
paru -S base-devel cmake clang sdl3 freetype2 gperftools utf8cpp vulkan-headers shader-slang 
```

Download the project's source code (e.g. using `git clone`). At that point you can either build and run it in your
favourite IDE (e.g. using the CMake integration in CLion) or run the following commands (assuming you're in the project's
root directory):
```shell
mkdir build
cd build
cmake ..
cmake --build .
./nekomata2
```

If you generate makefiles instead of a build.ninja file, add `-j(number of cores + 1)` to the `cmake --build .` command to parallelize the build.

## License

Copyright 2026 yoruhanigoubgataa

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.