/*
 * Copyright (c) 2017-2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2017-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef CADSCENE_H__
#define CADSCENE_H__

#include <nvmath/nvmath.h>
#include <cstdint>
#include <vector>

#include "config.h"

typedef unsigned short half;

class CadScene
{

public:
  struct BBox
  {
    nvmath::vec4f min;
    nvmath::vec4f max;

    BBox()
        : min(FLT_MAX)
        , max(-FLT_MAX)
    {
    }

    inline void merge(const nvmath::vec4f& point)
    {
      min = nvmath::nv_min(min, point);
      max = nvmath::nv_max(max, point);
    }

    inline void merge(const BBox& bbox)
    {
      min = nvmath::nv_min(min, bbox.min);
      max = nvmath::nv_max(max, bbox.max);
    }

    [[nodiscard]] inline BBox transformed(const nvmath::mat4f& matrix, int dim = 3) const
    {
      int           i;
      nvmath::vec4f box[16];
      // create box corners
      box[0] = nvmath::vec4f(min.x, min.y, min.z, min.w);
      box[1] = nvmath::vec4f(max.x, min.y, min.z, min.w);
      box[2] = nvmath::vec4f(min.x, max.y, min.z, min.w);
      box[3] = nvmath::vec4f(max.x, max.y, min.z, min.w);
      box[4] = nvmath::vec4f(min.x, min.y, max.z, min.w);
      box[5] = nvmath::vec4f(max.x, min.y, max.z, min.w);
      box[6] = nvmath::vec4f(min.x, max.y, max.z, min.w);
      box[7] = nvmath::vec4f(max.x, max.y, max.z, min.w);

      box[8]  = nvmath::vec4f(min.x, min.y, min.z, max.w);
      box[9]  = nvmath::vec4f(max.x, min.y, min.z, max.w);
      box[10] = nvmath::vec4f(min.x, max.y, min.z, max.w);
      box[11] = nvmath::vec4f(max.x, max.y, min.z, max.w);
      box[12] = nvmath::vec4f(min.x, min.y, max.z, max.w);
      box[13] = nvmath::vec4f(max.x, min.y, max.z, max.w);
      box[14] = nvmath::vec4f(min.x, max.y, max.z, max.w);
      box[15] = nvmath::vec4f(max.x, max.y, max.z, max.w);

      // transform box corners
      // and find new mins,maxs
      BBox bbox;

      for(i = 0; i < (1 << dim); i++)
      {
        nvmath::vec4f point = matrix * box[i];
        bbox.merge(point);
      }

      return bbox;
    }
  };

  struct MaterialSide
  {
    nvmath::vec4f ambient;
    nvmath::vec4f diffuse;
    nvmath::vec4f specular;
    nvmath::vec4f emissive;
  };

  // need to keep this 256 byte aligned (UBO range)
  struct Material
  {
    MaterialSide sides[2];
    uint32_t     _pad[8 * 4];

    Material()
        : _pad{}
    {
      memset((void*)this, 0, sizeof(Material));
    }
  };

  // need to keep this 256 byte aligned (UBO range)
  struct MatrixNode
  {
    nvmath::mat4f worldMatrix;
    nvmath::mat4f worldMatrixIT;
    nvmath::mat4f objectMatrix;
    nvmath::vec4f bboxMin;
    nvmath::vec4f bboxMax;
    nvmath::vec3f _pad0;
    float         winding;
    nvmath::vec4f color;
  };

  struct Vertex
  {
    nvmath::vec4f position;
  };

  struct VertexAttributes
  {
    nvmath::vec4f normal;
  };

  struct VertexFP16
  {
    half position[4];
  };

  struct VertexAttributesFP16
  {
    half normal[4];
  };

  struct DrawIndirectElements
  {
    uint32_t count;
    uint32_t primCount;
    uint32_t firstIndex;
    int32_t  baseVertex;
    uint32_t baseInstance;

    DrawIndirectElements()
        : count(0)
        , primCount(1)
        , firstIndex(0)
        , baseVertex(0)
        , baseInstance(0)
    {
    }
  };

  struct DrawRange
  {
    size_t offset;
    int    count;

    DrawRange()
        : offset(0)
        , count(0)
    {
    }
  };

  struct MeshletRange
  {
    uint32_t offset;
    uint32_t count;
  };

  struct GeometryPart
  {
    DrawRange    indexSolid;
    MeshletRange meshSolid{};
  };

  struct MeshletTopology
  {
    size_t primSize;
    size_t descSize;

    int numMeshlets = 0;

    // may not be used
    void* primData = nullptr;
    void* descData = nullptr;

    ~MeshletTopology()
    {
      if(primData)
      {
        free(primData);
      }
      if(descData)
      {
        free(descData);
      }
    }
  };


  struct Geometry
  {
    int partOffset;
    int useShorts;
    int partBboxOffset;

    size_t vboSize;
    size_t aboSize;
    size_t iboSize;
    size_t meshSize;
    size_t meshIndicesSize;

    MeshletTopology meshlet;

    std::vector<GeometryPart> parts;

    int numVertices;
    int numIndexSolid;

    void* vboData = nullptr;
    void* aboData = nullptr;
    void* iboData = nullptr;

    ~Geometry()
    {
      if(vboData)
      {
        free(vboData);
      }
      if(aboData)
      {
        free(aboData);
      }
      if(iboData)
      {
        free(iboData);
      }
    }
  };

  struct ObjectPart
  {
    int active;
    int materialIndex;
    int matrixIndex;
  };

  struct Object
  {
    int partOffset;
    int matrixIndex;
    int geometryIndex;
    int faceCCW;

    std::vector<ObjectPart> parts;
  };

  struct LoadConfig
  {
    float    scale           = 1.0f;
    bool     verbose         = true;
    bool     fp16            = false;
    bool     allowShorts     = true;
    bool     colorizeExtra   = false;
    uint32_t extraAttributes = 0;

    // must not change order
    uint32_t meshVertexCount    = 64;
    uint32_t meshPrimitiveCount = 126;

    MeshletBuilderType meshBuilder = MESHLET_BUILDER_PACKBASIC;
  };

  std::vector<Material>   m_materials;
  std::vector<BBox>       m_bboxes;
  std::vector<Geometry>   m_geometry;
  std::vector<MatrixNode> m_matrices;
  std::vector<Object>     m_objects;

  size_t   m_vboSize          = 0;
  size_t   m_iboSize          = 0;
  size_t   m_meshSize         = 0;
  uint32_t m_numGeometryParts = 0;
  uint32_t m_numObjectParts   = 0;

  LoadConfig m_cfg;
  BBox       m_bbox;
  BBox       m_bboxInstanced;

  size_t m_numOrigGeometries;
  size_t m_numOrigMatrices;
  size_t m_numOrigObjects;

  bool loadCSF(const char* filename, const LoadConfig& cfg, int clones = 0, int cloneaxis = 3);
  void unload();


  [[nodiscard]] size_t getVertexSize() const { return m_cfg.fp16 ? sizeof(VertexFP16) : sizeof(Vertex); }

  [[nodiscard]] size_t getVertexAttributeSize() const
  {
    return m_cfg.fp16 ? (sizeof(VertexAttributesFP16) + sizeof(half) * 4 * m_cfg.extraAttributes) :
                        (sizeof(VertexAttributes) + sizeof(float) * 4 * m_cfg.extraAttributes);
  }

  void* getVertex(void* data, size_t index) const { return ((uint8_t*)data) + (getVertexSize() * index); }

  void* getVertexAttribute(void* data, size_t index) const
  {
    return ((uint8_t*)data) + (getVertexAttributeSize() * index);
  }

private:
  void buildMeshletTopology(const struct _CSFile* csf);
};


#endif
