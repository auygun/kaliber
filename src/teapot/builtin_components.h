#ifndef TEAPOT_BUILTIN_COMPONENTS_H
#define TEAPOT_BUILTIN_COMPONENTS_H

#include "base/vecmath.h"
#include "teapot/ecs.h"

namespace eng {

// The component for storing parent-child relationships and transformations of
// world objects. This is the core of the scene graph.
struct CoreDataComponent {
  char name[8];

  base::Matrix4f local_transform{1};
  base::Matrix4f world_transform{1};
  bool is_dirty{true};

  // Hierarchy (Doubly-Linked Sibling List)
  Entity parent{NULL_ENTITY};
  Entity first_child{NULL_ENTITY};
  Entity next_sibling{NULL_ENTITY};
  Entity prev_sibling{NULL_ENTITY};
};

struct ModelComponent {
  size_t model_index{(size_t)-1};
};

}  // namespace eng

#endif  // TEAPOT_BUILTIN_COMPONENTS_H
