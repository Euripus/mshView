#include "Mesh.h"
#include "ImageData.h"
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <cassert>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

Mesh::Mesh() : _modelMatrix(1.0f),
               _draw_bbox(false)
{
}

Mesh::~Mesh()
{
}

bool Mesh::LoadFromMsh(const char* fname)
{
    std::ifstream in(fname, std::ios::in);
    if(!in)
    {
        std::cerr << "Cannot open: " << fname << std::endl;
        return false;
    }
    
    std::string line;
    SubMesh* cur_mesh = nullptr;
    uint32_t num_wgts = 0;
    while(std::getline(in, line))
    {
        if(line.substr(0, 6) == "meshes")
        {
            uint32_t num_meshes;
            std::istringstream s(line.substr(6));
            s >> num_meshes;
            
            _meshes.resize(num_meshes);
        }
        if(line.substr(0, 4) == "mesh")
        {
            uint32_t num_mesh;
            std::istringstream s(line.substr(4));
            s >> num_mesh;
            
            cur_mesh = &_meshes[num_mesh];
            num_wgts = 0;
        }
        if(line.substr(0, 7) == "weights")
        {
            std::istringstream s(line.substr(7));
            s >> num_wgts;
        }
        else if(line.substr(0, 4) == "bbox")
        {
            std::istringstream s(line.substr(4));
            float mnx(0), mny(0), mnz(0), mxx(0), mxy(0), mxz(0);
            s >> mnx >> mny >> mnz
              >> mxx >> mxy >> mxz;
            cur_mesh->_base_bbox = AABB(mnx, mny, mnz, mxx, mxy, mxz);
        }
        if(line.substr(0, 8) == "material")
        {
            std::string tex_name;
            std::istringstream s(line.substr(8));
            s >> tex_name;
            cur_mesh->_tex_name = tex_name;
        }
        else if(line.substr(0, 3) == "vtx")
        {
            std::istringstream s(line.substr(3));
            glm::vec3 v;
            s >> v.x;
            s >> v.y;
            s >> v.z;
            cur_mesh->_positions.push_back(std::move(v));
        }
        else if(line.substr(0, 3) == "vnr")
        {
            std::istringstream s(line.substr(3));
            glm::vec3 v;
            s >> v.x;
            s >> v.y;
            s >> v.z;
            cur_mesh->_normals.push_back(glm::normalize(v));
        }
        else if(line.substr(0, 3) == "vtg")
        {
            std::istringstream s(line.substr(3));
            glm::vec3 v;
            s >> v.x;
            s >> v.y;
            s >> v.z;
            cur_mesh->_tangents.push_back(glm::normalize(v));
        }
        else if(line.substr(0, 3) == "vbt")
        {
            std::istringstream s(line.substr(3));
            glm::vec3 v;
            s >> v.x;
            s >> v.y;
            s >> v.z;
            cur_mesh->_bitangents.push_back(glm::normalize(v));
        }
        else if(line.substr(0, 12) == "tex_channels")
        {
            uint32_t num_texchannels;
            std::istringstream s(line.substr(12));
            s >> num_texchannels;

            cur_mesh->_uvs.resize(num_texchannels);
        }
        else if(line.substr(0, 2) == "tx")
        {
            std::istringstream s(line.substr(2));
            uint32_t chn;
            glm::vec2 v;
            s >> chn;
            s >> v.x >> v.y;
            cur_mesh->_uvs[chn].push_back(v);
        }
        else if(line.substr(0, 3) == "fcx")
        {
            std::istringstream s(line.substr(3));
            uint32_t v1(0), v2(0), v3(0);
            s >> v1;
            s >> v2;
            s >> v3;
            cur_mesh->_indices.push_back(v1);
            cur_mesh->_indices.push_back(v2);
            cur_mesh->_indices.push_back(v3);
        }
        else if(line.substr(0, 3) == "wgi")
        {
            std::istringstream s(line.substr(3));
            uint32_t end_wght_ind = 0;

            s >> end_wght_ind;
            std::pair<uint32_t, uint32_t> wgh_ind;
            wgh_ind.second = end_wght_ind;
            if(!cur_mesh->_wght_inds.empty())
                wgh_ind.first = cur_mesh->_wght_inds.back().second;
            else
                wgh_ind.first = 0;

            cur_mesh->_wght_inds.push_back(wgh_ind);
        }
        else if(line.substr(0, 3) == "wgh")
        {
            std::istringstream s(line.substr(3));
            SubMesh::Weight w;
            s >> w.jnt_index;
            s >> w.w;
            cur_mesh->_weights.push_back(w);
        }
    }   
    in.close();
    
    AABB bbox;
    for(auto & msh : _meshes)
    {
        bbox.expandBy(msh._base_bbox);
    }
    _base_bbox = bbox;
    
    return true;
}

bool Mesh::LoadFromAnm(const char * fname)
{
    std::ifstream in(fname, std::ios::in);
    if(!in)
    {
        std::cerr << "Cannot open: " << fname << std::endl;
        return false;
    }
    
    std::string line;
    AnimSequence::JointNode * cur_frame = nullptr;
    AnimSequence seq;
    uint32_t     jnt_ind = 0;
    uint32_t     num_bones = 0;
    while(std::getline(in, line))
    {
        if(line.substr(0, 5) == "bones")
        {
            std::istringstream s(line.substr(5));
            s >> num_bones;
        }
        else if(line.substr(0, 6) == "frames")
        {
            uint32_t num_frames;
            std::istringstream s(line.substr(6));
            s >> num_frames;
            
            seq.frames.resize(num_frames);
            for(auto & jnt : seq.frames)
            {
                jnt.rot.resize(num_bones);
                jnt.trans.resize(num_bones);
            }
        }
        else if(line.substr(0, 9) == "framerate")
        {
            float framerate;
            std::istringstream s(line.substr(9));
            s >> framerate;
            
            seq.frameRate = framerate;
        }
        else if(line.substr(0, 5) == "frame")
        {
            uint32_t frame;
            std::istringstream s(line.substr(5));
            s >> frame;
            
            cur_frame = &seq.frames[frame];
            jnt_ind = 0;
        }
        else if(line.substr(0, 4) == "bbox")
        {
            std::istringstream s(line.substr(4));
            float mnx(0), mny(0), mnz(0), mxx(0), mxy(0), mxz(0);
            s >> mnx >> mny >> mnz;
            s >> mxx >> mxy >> mxz;
            
            cur_frame->bbox = AABB(mnx, mny, mnz, mxx, mxy, mxz);
        }
        else if(line.substr(0, 3) == "jtr")
        {
            std::istringstream s(line.substr(3));
            float qtx(0), qty(0), qtz(0), qtw(0), 
                  tr_x(0), tr_y(0), tr_z(0);
            s >> qtx >> qty >> qtz >> qtw;
            s >> tr_x >> tr_y >> tr_z;
            
            cur_frame->rot[jnt_ind] = glm::quat(qtw, qtx, qty, qtz);
            cur_frame->trans[jnt_ind] = glm::vec3(tr_x, tr_y, tr_z);
            jnt_ind++;
        }
    }
    
    in.close();
    _controller = Controller(Controller::RepeatType::RT_WRAP, 0.0,
                             seq.frames.size()/seq.frameRate);
    _anims.push_back(std::move(seq));
    return true;
}

bool Mesh::LoadTexture(const char* fname)
{
    std::string fn(fname);
    if(fn.substr(fn.find_last_of(".") + 1) == "tga"
       || fn.substr(fn.find_last_of(".") + 1) == "TGA")
    {
        return ReadTGA(fn, _texData);
    }
    else if(fn.substr(fn.find_last_of(".") + 1) == "bmp"
            || fn.substr(fn.find_last_of(".") + 1) == "BMP")
    {
        return ReadBMP(fn, _texData);
    }
    else
    {
        return false;
    }
}

void Mesh::RotateMesh(glm::vec3 euler_angles)
{
    glm::mat4 rotate = glm::orientate4(euler_angles);
    _modelMatrix = _modelMatrix * rotate;
}

void Mesh::TranslateMesh(glm::vec3 translate)
{
    _modelMatrix = glm::translate(_modelMatrix, translate);
}
