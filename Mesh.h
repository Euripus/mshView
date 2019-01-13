#ifndef MESH_H
#define MESH_H

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include "AABB.h"
#include "Controller.h"
#include "ImageData.h"

class Mesh
{
    friend class GL2Widget;
private:
    struct SubMesh
    {
        struct Weight
        {
            uint32_t jnt_index;
            float    w;
        };
        
        std::string                                _tex_name;
        std::vector<glm::vec3>                     _positions;
        std::vector<glm::vec3>                     _normals;
        std::vector<glm::vec3>                     _tangents;
        std::vector<glm::vec3>                     _bitangents;
        std::vector<std::vector<glm::vec2>>        _uvs;
        std::vector<std::pair<uint32_t, uint32_t>> _wght_inds;     // start and end indicies for vertex
        std::vector<Weight>                        _weights;
        
        std::vector<unsigned int> _indices;
        
        AABB          _base_bbox;
        
        SubMesh() {}
    };

    struct AnimSequence
    {
        struct JointNode
        {
            AABB     bbox;

            std::vector<glm::quat> rot;                 // absolute transform matrix for animation
            std::vector<glm::vec3> trans;
        };
        
        std::vector<JointNode> frames;
        float                  frameRate;
        
        AnimSequence() : frameRate(0.0f) {}
    };

    glm::mat4                 _modelMatrix;
    std::vector<SubMesh>      _meshes;
    std::vector<AnimSequence> _anims;
    ImageData                 _texData;
    
    AABB          _base_bbox;
    bool          _draw_bbox;

    Controller    _controller;

public:
    Mesh();
    virtual ~Mesh();
    
    //Mesh(const Mesh& ms);
    //Mesh& operator=(const Mesh& ms);

    Mesh(Mesh&& ms) = default;
    Mesh& operator=(Mesh&& ms) = default;
    
    bool LoadFromMsh(const char * fname);
    bool LoadFromAnm(const char * fname);
    bool LoadTexture(const char * fname);
    
    const glm::mat4& GetModelMatrix() const { return _modelMatrix; }
    void TranslateMesh(glm::vec3 translate);
    void RotateMesh(glm::vec3 euler_angles);           // angles in degrees
    void DrawBBox(bool val) { _draw_bbox = val; }
    bool isDrawBBox() const { return _draw_bbox; }
};

#endif // MESH_H
