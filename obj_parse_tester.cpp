#include "libs/fast_obj/fast_obj.h"
#include "libs/tinyobjloader/tiny_obj_loader.h"
//#include "blender/importer/obj_importer.hh"
#include "libs/rapidobj/include/rapidobj/rapidobj.hpp"
#include "libs/xxHash/xxhash.h"

#include <stdio.h>
#include <chrono>

struct Hasher
{
    Hasher()
    {
        state = XXH3_createState();
        XXH3_64bits_reset(state);
    }
    void feed(const void* data, size_t size)
    {
        XXH3_64bits_update(state, data, size);
    }
    uint64_t get_hash()
    {
        return XXH3_64bits_digest(state);
    }
    ~Hasher()
    {
        XXH3_freeState(state);
    }
    XXH3_state_t* state;
};

static std::chrono::steady_clock::time_point get_time()
{
    return std::chrono::high_resolution_clock::now();
}
static double get_duration(std::chrono::steady_clock::time_point since)
{
    std::chrono::duration<double> dur = get_time() - since;
    return dur.count();
}

struct ObjParseStats
{
    bool ok = false;
    double time = -1;
    int vertex_count = -1;
    int normal_count = -1;
    int uv_count = -1;
    int shape_count = -1;
    int material_count = -1;
    uint64_t vertex_hash = 0;
    uint64_t normal_hash = 0;
    uint64_t uv_hash = 0;
};

static ObjParseStats parse_tinyobjloader(const char* filename)
{
    ObjParseStats res;
    auto t0 = get_time();

    using namespace tinyobj;
    attrib_t attrib;
    std::vector<shape_t> shapes;
    std::vector<material_t> materials;
    std::string err;
    std::string warn;
    res.ok = LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, 0, false);

    res.time = get_duration(t0);

    res.vertex_count = (int)(attrib.vertices.size() / 3);
    res.normal_count = (int)(attrib.normals.size() / 3);
    res.uv_count = (int)(attrib.texcoords.size() / 2);
    res.vertex_hash = XXH3_64bits(attrib.vertices.data(), attrib.vertices.size() * 4);
    res.normal_hash = XXH3_64bits(attrib.normals.data(), attrib.normals.size() * 4);
    res.uv_hash = XXH3_64bits(attrib.texcoords.data(), attrib.texcoords.size() * 4);
    res.shape_count = (int)shapes.size();
    res.material_count = (int)materials.size();
}


int main(int argc, const char* argv[])
{
    if (argc != 2)
    {
        printf("USAGE: obj_parse_tester <obj file>\n");
        return -1;
    }
    printf("File: %s\n", argv[1]);
	return 0;
}
