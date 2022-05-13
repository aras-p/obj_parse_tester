# Testing various Wafefront .OBJ parsing libraries

Windows 10, AMD Ryzen 5950X, VS2022 17.1:

| Library           |sponza|Monkey-6|rungholt|Blender3Splash|
| :---              |  ---:|    ---:|    ---:|          ---:|
| tinyobjloader     | 0.17 | 2.75   | 2.83   | 19.57        |
| tinyobjloader_opt | 0.04 | 0.41   | 0.40   |  3.12        |
| fast_obj          | 0.04 | 0.70   | 0.58   |  5.16        |
| rapidobj          | 0.02 | 0.19   | 0.19   |  1.25        |
| blender           | 0.07 | 0.94   | 0.83   |  6.92        |
| assimp            | 0.20 | 2.98   | 3.88   | 21.98        |

Windows 10, AMD Ryzen 5950X, clang 13:

| Library           |sponza|Monkey-6|rungholt|Blender3Splash|
| :---              |  ---:|    ---:|    ---:|          ---:|
| tinyobjloader     |      | 2.65   | 2.42   | 16.47        |
| tinyobjloader_opt |      | 0.38   | 0.38   |  2.69        |
| fast_obj          |      | 0.71   | 0.58   |  5.25        |
| rapidobj          |      | 0.19   | 0.17   |  1.62        |
| blender           |      | 0.80   | 0.74   |  5.98        |
| assimp            |      | 2.74   | 3.53   | 19.90        |

macOS 12.3, Apple M1 Max, clang 13:

| Library           |sponza|Monkey-6|rungholt|Blender3Splash|
| :---              |  ---:|    ---:|    ---:|          ---:|
| tinyobjloader     | 0.14 | 2.09   | 2.12   | 14.72        |
| tinyobjloader_opt | 0.03 | 0.47   | 0.38   |  5.07        |
| fast_obj          | 0.02 | 0.33   | 0.30   |  2.40        |
| blender           |      | 0.73   | 0.76   |  5.53        |
| assimp            | 0.13 | 1.89   | 2.17   | 14.26        |

Memory usage in MB (peak/end), Windows/VS2022:
| Library           |rungholt  |Blender3Splash|
| :---              |      ---:|          ---:|
| tinyobjloader     | 395/248  |1505/1438     |
| tinyobjloader_opt | 1662/469 |13850/2272    |
| fast_obj          | 319/214  |1895/1237     |
| rapidobj          | 428/218  |2667/1265     |
| blender           | 272/253  |1621/1614     |
| assimp            | 1341/640 |6097/2788     |



* `sponza`: 20MB, 0.15M verts, 381 objects, 25 materials. "Crytek Sponza" from [McGuire Computer Graphics Archive](https://casual-effects.com/data/).
* `Monkey-6`: 330MB, 2.0M verts, 1 object, 1 material. Blender's Monkey mesh, subdivided to level 6.
* `rungholt`: 270MB, 2.5M verts, 1 object, 84 materials. "Rungholt" Minecraft map from [McGuire Computer Graphics Archive](https://casual-effects.com/data/).
* `Blender3Splash`: 2.5GB, 14.4M verts, 24k objects, 113 materials. Blender 3.0 splash scene "[Sprite Fright](https://cloud.blender.org/p/gallery/617933e9b7b35ce1e1c01066)", exported as OBJ.
