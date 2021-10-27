#include "engine/renderer/renderer_types.h"

#include <cstring>

#include "base/log.h"

namespace {

// Used to parse the vertex layout,
// e.g. "p3f;c4b" for "position 3 floats, color 4 bytes".
const char kLayoutDelimiter[] = ";/ \t";

}  // namespace

namespace eng {

bool ParseVertexDescription(std::string vd_str, VertexDescripton& out) {
  // Parse the description.
  char buffer[32];
  strcpy(buffer, vd_str.c_str());
  char* token = strtok(buffer, kLayoutDelimiter);

  // Parse each encountered attribute.
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3) {
      LOG << "Invalid format: " << token;
      return false;
    }

    AttribType attrib_type = kAttribType_Invalid;
    ;
    switch (token[0]) {
      case 'c':
        attrib_type = kAttribType_Color;
        break;
      case 'n':
        attrib_type = kAttribType_Normal;
        break;
      case 'p':
        attrib_type = kAttribType_Position;
        break;
      case 't':
        attrib_type = kAttribType_TexCoord;
        break;
      default:
        LOG << "Unknown attribute: " << token;
        return false;
    }

    // There can be between 1 and 4 elements in an attribute.
    ElementCount num_elements = token[1] - '1' + 1;
    if (num_elements < 1 || num_elements > 4) {
      LOG << "Invalid number of elements: " << token;
      return false;
    }

    // The data type is needed, the most common ones are supported.
    DataType data_type = kDataType_Invalid;
    DataTypeSize type_size = 0;
    switch (token[2]) {
      case 'b':
        data_type = kDataType_Byte;
        type_size = sizeof(unsigned char);
        break;
      case 'f':
        data_type = kDataType_Float;
        type_size = sizeof(float);
        break;
      case 'i':
        data_type = kDataType_Int;
        type_size = sizeof(int);
        break;
      case 's':
        data_type = kDataType_Short;
        type_size = sizeof(short);
        break;
      case 'u':
        data_type = kDataType_UInt;
        type_size = sizeof(unsigned int);
        break;
      case 'w':
        data_type = kDataType_UShort;
        type_size = sizeof(unsigned short);
        break;
      default:
        LOG << "Unknown data type: " << token;
        return false;
    }

    out.push_back(
        std::make_tuple(attrib_type, data_type, num_elements, type_size));

    token = strtok(NULL, kLayoutDelimiter);
  }
  return true;
}

}  // namespace eng
