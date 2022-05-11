#include "libs/fast_obj/fast_obj.h"
#include "libs/tinyobjloader/tiny_obj_loader.h"
#include "libs/rapidobj/include/rapidobj/rapidobj.hpp"
#include "libs/xxHash/xxhash.h"

#include "libs/assimp/include/assimp/Importer.hpp"
#include "libs/assimp/include/assimp/scene.h"
#include "libs/assimp/include/assimp/postprocess.h"

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
    uint32_t vertex_hash = 0;
    uint32_t normal_hash = 0;
    uint32_t uv_hash = 0;

    void print(const char* title) const
    {
        printf("%-15s ok=%i t=%6.3f s num (v=%i vn=%i vt=%i o=%i mat=%i) hash (v=%08x vn=%08x vt=%08x)\n",
            title, ok, time, vertex_count, normal_count, uv_count, shape_count, material_count,
            vertex_hash, normal_hash, uv_hash);
    }
};

static void parse_tinyobjloader(const char* filename)
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

    if (res.ok)
    {
        res.vertex_count = (int)(attrib.vertices.size() / 3);
        res.normal_count = (int)(attrib.normals.size() / 3);
        res.uv_count = (int)(attrib.texcoords.size() / 2);
        res.vertex_hash = XXH3_64bits(attrib.vertices.data(), attrib.vertices.size() * 4) & 0xFFFFFFFF;
        res.normal_hash = XXH3_64bits(attrib.normals.data(), attrib.normals.size() * 4) & 0xFFFFFFFF;
        res.uv_hash = XXH3_64bits(attrib.texcoords.data(), attrib.texcoords.size() * 4) & 0xFFFFFFFF;
        res.shape_count = (int)shapes.size();
        res.material_count = (int)materials.size();
    }

    res.print("tinyobjloader");
}

static void parse_fast_obj(const char* filename)
{
    ObjParseStats res;
    auto t0 = get_time();

    fastObjMesh* m = fast_obj_read(filename);
    res.ok = m != nullptr;

    res.time = get_duration(t0);

    if (res.ok)
    {
        res.vertex_count = m->position_count - 1;
        res.normal_count = m->normal_count - 1;
        res.uv_count = m->texcoord_count - 1;
        res.vertex_hash = XXH3_64bits(m->positions + 3, (m->position_count-1) * 12) & 0xFFFFFFFF;
        res.normal_hash = XXH3_64bits(m->normals + 3, (m->normal_count-1) * 12) & 0xFFFFFFFF;
        res.uv_hash = XXH3_64bits(m->texcoords + 2, (m->texcoord_count-1) * 8) & 0xFFFFFFFF;
        res.shape_count = m->group_count;
        res.material_count = m->material_count;
        fast_obj_destroy(m);
    }

    res.print("fast_obj");
}

static void parse_rapidobj(const char* filename)
{
    ObjParseStats res;
    auto t0 = get_time();

    auto m = rapidobj::ParseFile(filename);
    res.ok = !m.error;

    res.time = get_duration(t0);

    if (res.ok)
    {
        res.vertex_count = (int)(m.attributes.positions.size() / 3);
        res.normal_count = (int)(m.attributes.normals.size() / 3);
        res.uv_count = (int)(m.attributes.texcoords.size() / 2);
        res.vertex_hash = XXH3_64bits(m.attributes.positions.data(), m.attributes.positions.size() * 4) & 0xFFFFFFFF;
        res.normal_hash = XXH3_64bits(m.attributes.normals.data(), m.attributes.normals.size() * 4) & 0xFFFFFFFF;
        res.uv_hash = XXH3_64bits(m.attributes.texcoords.data(), m.attributes.texcoords.size() * 4) & 0xFFFFFFFF;
        res.shape_count = (int)m.shapes.size();
        res.material_count = (int)m.materials.size();
    }

    res.print("rapidobj");
}

static void parse_assimp(const char* filename)
{
    ObjParseStats res;
    auto t0 = get_time();

    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(filename, 0);

    res.ok = scene != nullptr;

    res.time = get_duration(t0);

    if (res.ok)
    {
        // assimp "cooks" the imported data into a rendering-friendly
        // format where vertices/normals/uvs are no longer separate etc.
        // So the counting/hashing is not following the other libraries.
        res.vertex_count = 0;
        res.normal_count = 0;
        res.uv_count = 0;
        for (int i = 0; i < scene->mNumMeshes; ++i)
        {
            const aiMesh* m = scene->mMeshes[i];
            res.vertex_count += m->mNumVertices;
            res.normal_count += m->mNumVertices;
            res.uv_count += m->mNumVertices;
        }
        res.shape_count = scene->mNumMeshes;
        res.material_count = scene->mNumMaterials;
    }

    res.print("assimp");
}

static bool read_file(const char* filename)
{
    FILE* f = fopen(filename, "rb");
    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    auto size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = new char[size];
    fread(buf, 1, size, f);
    fclose(f);
    delete[] buf;
    return true;
}


int main(int argc, const char* argv[])
{
    if (argc != 2)
    {
        printf("USAGE: obj_parse_tester <obj file>\n");
        return -1;
    }
    const char* filename = argv[1];
    printf("File: %s\n", filename);
    // just read the file in, to prewarm OS file caches
    if (!read_file(filename))
    {
        printf("Can't read the file!\n");
        return 1;
    }
    parse_tinyobjloader(filename);
    parse_fast_obj(filename);
    parse_rapidobj(filename);
    parse_assimp(filename);
    return 0;
}
