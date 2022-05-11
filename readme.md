﻿# Testing various Wafefront .OBJ parsing libraries

Windows 10, AMD Ryzen 5950X, VS2022:

| Library           |sponza |Monkey-6|rungholt|Blender3Splash|
| :---              |   ---:|    ---:|    ---:|          ---:|
| tinyobjloader     | 0.155 | 2.40   | 2.50   | 17.00        |
| tinyobjloader_opt | 0.040 | 0.49   | 0.42   |  3.98        |
| fast_obj          | 0.024 | 0.37   | 0.31   |  2.89        |
| rapidobj          | 0.020 | 0.21   | 0.18   |  0.88        |
| blender           | 0.069 | 0.89   | 0.84   |  6.63        |
| assimp            | 0.196 | 2.90   | 3.75   | 21.17        |

macOS 12.3, Apple M1 Max, clang 13:

| Library           |sponza |Monkey-6|rungholt|Blender3Splash|
| :---              |   ---:|    ---:|    ---:|          ---:|
| tinyobjloader     | 0.142 | 2.11   | 2.15   | 14.83        |
| tinyobjloader_opt | 0.030 | 0.49   | 0.39   |  6.06        |
| fast_obj          | 0.023 | 0.33   | 0.29   |  2.42        |
| blender           | 0.048 | 0.73   | 0.74   |  5.52        |
| assimp            | 0.129 | 1.91   | 2.21   | 14.27        |


* sponza: 20MB, 0.15M verts, 381 objects, 25 materials.
* Monkey-6: 330MB, 2.0M verts, 1 object, 1 material.
* rungholt: 270MB, 2.5M verts, 1 object, 84 materials.
* Blender3Splash: 2.5GB, 14.4M verts, 24k objects, 113 materials.
