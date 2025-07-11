# Optimisation Task #1

[![CMake desktop workflow](/../../actions/workflows/cmake-desktop.yml/badge.svg)](/../../actions/workflows/cmake-desktop.yml) [![CMake Emscripten workflow](/../../actions/workflows/cmake-emscripten.yml/badge.svg)](/../../actions/workflows/cmake-emscripten.yml)

This is a programming task given to candidates interviewing at [Numfum GmbH](//www.numfum.com/jobs_en/) to complete from home, taking as long as they need. The code is a snippet from [Basis Universal](//github.com/BinomialLLC/basis_universal) (copyright 2019-2021 Binomial LLC, released under an Apache 2.0 license). The [original code](//github.com/BinomialLLC/basis_universal/blob/77b7df8e5df3532a42ef3c76de0c14cc005d0f65/transcoder/basisu_transcoder.cpp#L1178-L1253) is extracted from the ETC1S to DXT transcoder and turned into this standalone sample.

### The Task

The task is to optimise the table generation in `create_etc1_to_dxt1_6_conversion_table()` (in [main.cpp](main.cpp)), showing timings before and after. Example timings from the unoptimised code for comparison:

| Machine                                                                                                      | OS             | CPU            | Compiler | Time  |
|--------------------------------------------------------------------------------------------------------------|----------------|----------------|----------|-------|
| Mac Pro (Late 2013)                                                                                          | Windows 10     | Intel Xeon E5  | MSVC 19  | 408ms |
| Mac Pro (Late 2013)                                                                                          | macOS Big Sur  | Intel Xeon E5  | Clang 13 | 326ms |
| Talos II                                                                                                     | Debian Buster  | IBM Power9     | GCC 10   | 289ms |
| MacBook Pro (2021)                                                                                           | macOS Monterey | Apple M1 Max   | Clang 13 | 158ms |
| [Flow X16 baseline](https://github.com/firestar99/task-opt1/commit/bc3df10185e0641c8aa7b9f757c6511ff195b187) | Ubuntu 24.10   | Ryzen 9 6900HS | Clang 19 | 179ms |
| [explicit indexing](https://github.com/firestar99/task-opt1/commit/dd3dab22034bec0e98700b6c850c28dc6540165b) | Ubuntu 24.10   | Ryzen 9 6900HS | Clang 19 | 178ms |
| [scatter writes](https://github.com/firestar99/task-opt1/commit/c4b3801dedca23c434fabeac67627f0bd8ac148c)    | Ubuntu 24.10   | Ryzen 9 6900HS | Clang 19 | 164ms |
| [rearrange loops](https://github.com/firestar99/task-opt1/commit/e0da2b0c5a21c5c2991fa31e901ba3d691f0f671)   | Ubuntu 24.10   | Ryzen 9 6900HS | Clang 19 | 92ms  |
| [cache errors](https://github.com/firestar99/task-opt1/commit/f84a3fd584fa53e105ad8738d77f3f1865e2141e)      | Ubuntu 24.10   | Ryzen 9 6900HS | Clang 19 | 37ms  |
| [cache squaring](https://github.com/firestar99/task-opt1/commit/81e4723acc9d196d182a917dd0efa825559d2bed)    | Ubuntu 24.10   | Ryzen 9 6900HS | Clang 19 | 34ms  |
| [write op function](https://github.com/firestar99/task-opt1/commit/90c26c5925168a427e9bd21a93ee497f5d07f381) | Ubuntu 24.10   | Ryzen 9 6900HS | Clang 19 | 35ms  |
| [unroll inner loop](https://github.com/firestar99/task-opt1/commit/539389e674d3dfdac77bcb6afc41f7f56f94a5cb) | Ubuntu 24.10   | Ryzen 9 6900HS | Clang 19 | 30ms  |

These numbers should give you an idea of what to expect on different architectures (and also compilers and OSes).

### Building

For macOS/Linux build with:
```
cc -Wall -Wextra -O3 -g0 main.cpp
```
For Windows, in a Visual Studio Command Prompt, build with:
```
cl /W3 /wd4576 /Ox main.cpp
```
For Emscripten build with:
```
emcc -Wall -Wextra -O3 -g0 -s SINGLE_FILE=1 main.cpp -o out.html
```
Feel free to tweak the compiler flags, `-flto` or `/GL` for example, or target a specific CPU with `-mcpu=power9`, `/arch:AVX2`, etc., but both optimised and unoptimised runs should be compared with the same flags.

(The Windows/MSVC warning `C4576` is for the initialiser list syntax.)

### CMake

Alternatively you can use CMake to perform command-line builds for macOS, Linux and Windows:
```
cmake -B out -DCMAKE_BUILD_TYPE=Release
cmake --build out --config Release
```
And also Emscripten:
```
emcmake cmake -B out -DCMAKE_BUILD_TYPE=Release
cmake --build out --config Release
```
Or generate a Visual Studio solution, for example:
```
mkdir out
cd out
cmake .. -G "Visual Studio 16 2019"
task-opt1.sln
```

### Limits

The following limits are imposed:
1. Use a single thread. Whilst the code can be parallelised with ease, the task is to see what optimisations can be applied to the table generation.
2. Generate the table. The ultimate unbeatable optimisation is to simply include the pre-generated table, but that defeats the task (BasisU's code already includes the pre-generated table).
3. It must compile with Clang/GCC/MSVC. Again, an easy route would be compiling with [`ispc`](//ispc.github.io) or similar SPMD compilers, but this would also defeat the task. It _would_ be interesting to compare the results though.
4. No using OpenMP's `#pragma omp parallel` or similar libraries or preprocessors.
5. The before and after comparison should be on the same hardware under the same conditions and with the same compiler (otherwise, from the table above, a 2.5x speed-up can simply be had by using a newer machine).

Any SIMD optimisations can be for an architecture of your choice, the interesting point being the before and after timings. We have implementations for SSE4.1, Neon and Wasm. Any questions, feel free to ask.

### Background

The original code is a greedy algorithm to [determine the best colour matches](//richg42.blogspot.com/2018/06/etc1s-texture-format-encoding.html) for a given ETC1S block when transcoding to DXT. In BasisU these matches are generated ahead of time and stored as tables (5- and 6-bit tables for DXT1), so this code is only ever run if the tables need regenerating. When assessing BasisU we looked at various ways of reducing the binary size, and generating the tables at runtime was considered. During the process it became a fun distraction for us in-house as a way of flexing optimisation skills amongst colleagues!
