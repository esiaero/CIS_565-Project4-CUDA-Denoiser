#pragma once

#include <string>
#include <vector>
#include <cuda_runtime.h>
#include "glm/glm.hpp"

// --- toggleable things ---
#define BACKGROUND_COLOR (glm::vec3(0.0f))

#define _STREAM_COMPACTION_			0
#define _GROUP_RAYS_BY_MATERIAL_	0
#define _CACHE_FIRST_BOUNCE_		0
#define _STRATIFIED_SAMPLING_		0
#define _ADAPTIVE_SAMPLING_         0
#if _ADAPTIVE_SAMPLING_
    #define _MIN_SPP_           64
    #define _ADAPTIVE_DEBUG_    1
    #define _PIX_COV_TO_SKIP_   1
#endif

#define _TIME_ATROUS_DENOISER_       0
// --- end toggleable things ---

enum GeomType {
    SPHERE,
    CUBE,
};

struct GBufferPixel {
    glm::vec3 posn;
    glm::vec3 norm;
};

enum class MatType {
    DIFFUSE,
    SPECULAR,
    DIELECTRIC,
    TRANSLUCENT
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float time;
};

struct Geom {
    enum GeomType type;
    int materialid;
    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::vec3 velocity;
    glm::mat4 transform;
    glm::mat4 inverseTransform;
    glm::mat4 invTranspose;
};

struct Material {
    glm::vec3 color;
    glm::vec3 transColor;
    struct {
        float exponent;
    } specular;
    MatType matType;
    float indexOfRefraction;
    float emittance;
    float absorption;
};

struct Camera {
    glm::ivec2 resolution;
    glm::vec3 position;
    glm::vec3 lookAt;
    glm::vec3 view;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec2 fov;
    glm::vec2 pixelLength;
    float lensRad;
    float focalDist;
    bool antialias;
};

struct RenderState {
    Camera camera;
    unsigned int iterations;
    int traceDepth;
    std::vector<glm::vec3> image;
    std::string imageName;
};

struct PathSegment {
    Ray ray;
    glm::vec3 color;
    int pixelIndex;
    int remainingBounces;

    glm::vec3 colorSum;
    float magColorSumSq;
    int spp;
    bool skip;
    bool terminate;
};

// Use with a corresponding PathSegment to do:
// 1) color contribution computation
// 2) BSDF evaluation: generate a new ray
struct ShadeableIntersection {
  float t;
  glm::vec3 surfaceNormal;
  int materialId;
  bool outside;
};

#if _STREAM_COMPACTION_
// predicate for thrust::remove_if stream compaction
struct partition_terminated_paths {
    __host__ __device__
        bool operator()(const PathSegment& p) { return p.remainingBounces > 0; }
};
#endif

#if _GROUP_RAYS_BY_MATERIAL_
// _GROUP_RAYS_BY_MATERIAL_ sorting comparison
struct compare_intersection_mat {
    __host__ __device__
        bool operator()(const ShadeableIntersection& i1, const ShadeableIntersection& i2) {
        return i1.materialId < i2.materialId;
    }
};
#endif

#if _ADAPTIVE_DEBUG_
struct compare_path_spp {
    __host__ __device__
        bool operator()(const PathSegment& p1, const PathSegment p2) {
        return p1.spp < p2.spp;
    }
};
#endif