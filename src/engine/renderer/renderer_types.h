#ifndef RENDERER_TYPES_H
#define RENDERER_TYPES_H

#include <string>
#include <tuple>
#include <vector>

namespace eng {

enum Primitive {
  kPrimitive_Triangles,
  kPrimitive_TriangleStrip,
  kPrimitive_Max
};

enum AttribType {
  kAttribType_Invalid = -1,
  kAttribType_Color,
  kAttribType_Normal,
  kAttribType_Position,
  kAttribType_TexCoord,
  kAttribType_Max
};

enum DataType {
  kDataType_Invalid = -1,
  kDataType_Byte,
  kDataType_Float,
  kDataType_Int,
  kDataType_Short,
  kDataType_UInt,
  kDataType_UShort,
  kDataType_Max
};

// Make tuple elements verbose.
using ElementCount = size_t;
using DataTypeSize = size_t;

using VertexDescripton =
    std::vector<std::tuple<AttribType, DataType, ElementCount, DataTypeSize>>;

bool ParseVertexDescription(std::string vd_str, VertexDescripton& out);

}  // namespace eng

#endif  // RENDERER_TYPES_H
