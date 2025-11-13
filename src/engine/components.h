#ifndef ENGINE_COMPONENTS_H
#define ENGINE_COMPONENTS_H

#include <array>
#include <cstdint>

#include "base/vecmath.h"
#include "engine/ecs.h"
#include "engine/input_codes.h"

namespace eng {

const uint32_t NULL_INDEX = (uint32_t)-1;

//
// SceneGraph Components
//

// The component for storing parent-child relationships and transformations of
// world objects. This is the core of the scene graph.
struct SceneNodeComponent {
  char name[8]{0};

  // Hierarchy (Doubly-Linked Sibling List)
  Entity parent{NULL_ENTITY};
  Entity first_child{NULL_ENTITY};
  Entity next_sibling{NULL_ENTITY};
  Entity prev_sibling{NULL_ENTITY};

  // Depth in the hierarchy (root = 0, child = 1, grandchild = 2, etc.).
  uint32_t depth{NULL_INDEX};
  // The index of this entity inside its specific depth_bucket vector.
  uint32_t bucket_index{NULL_INDEX};
};

// The entity's local transformation. This is the component the user or game
// logic systems (like AI or player input) will typically modify.
struct LocalTransformComponent {
  base::Matrix4f transform{1};
};

// The final, calculated, absolute world-space transform. This is written to by
// the internal SceneGraph system and read from by the Render and Physics
// systems.User code should almost never write to this directly.
struct WorldTransformComponent {
  base::Matrix4f transform{1};
};

struct WorldBoundsComponent {
  base::OBBf obb;
};

struct WorldTransformDirtyTag {};

//
// Rendering Components
//

// A component that holds the data needed to draw an entity.
struct ModelComponent {
  uint32_t model_index{NULL_INDEX};
  base::Vector3f extents{0};  // Extents of the model
};

// A component that defines the lens of a camera. It does not store position or
// rotation (that's in LocalTransformComponent).
struct CameraComponent {
  float fov = 45.0f;  // In degrees (TODO: [1,0])
  float near_plane = 1.0f;
  float far_plane = 1000.0f;
};

// An empty tag component You add this to the one entity you want to be the
// active camera. The Camera system will look for the entity with this tag.
struct PrimaryCameraTag {};

// A component that stores the state for a fly-camera control logic.
struct FlyCameraComponent {
  base::Vector3f offset{0};
  float pitch = 0.0f;
  float yaw = -90.0f;
  // float mouse_sensitivity = 0.1f;
};

// A component that stores the control state for an orbit-camera control logic.
struct OrbitCameraComponent {
  base::Vector3f center_{0};
  float radius = 0;
  float polar = 0;
  float azimuthal = 0;
};

// A global resource that holds the data for the entire frame. The Camera system
// writes to this. The Render system reads from this.
struct RenderContext {
  base::Matrix4f view{1};
  base::Matrix4f proj{1};
  base::Matrix4f view_proj{1};
  base::Vector3f camera_world_pos{0};
};

// A global resource that provides viewport dimensions. The Camera system reads
// this to get the aspect ratio. Render system updates this on resize.
struct Viewport {
  float width = 1280.0f;
  float height = 720.0f;

  float GetAspectRatio() const {
    if (height == 0)
      return 1.0f;
    return width / height;
  }
};

//
// Input
//

// A global resource that stores the state of all player inputs for the current
// frame. It holds low-level keyboard and mouse state. Input system writes to
// this. Other game systems read from this.
struct PlayerInput {
  // Delta of the cursor for this frame.
  float mouse_x_delta = 0.0f;
  float mouse_y_delta = 0.0f;

  // Absolute position (in pixels) of the cursor on the screen/window.
  float mouse_x = 0.0f;
  float mouse_y = 0.0f;

  // Scroll wheel delta for this frame.
  // Positive values for scroll up/forward, negative for scroll down/backward.
  float mouse_scroll_delta = 0.0f;

  // True for the single frame it's pressed.
  bool mouse_left_pressed = false;
  bool mouse_right_pressed = false;
  bool mouse_middle_pressed = false;

  // True for *every frame* it's held.
  bool mouse_left_held = false;
  bool mouse_right_held = false;
  bool mouse_middle_held = false;

  // True for the *single frame* a key is pressed.
  std::array<bool, static_cast<int>(Key::MaxKeys)> keys_pressed = {};

  // True for *every frame* a key is held.
  std::array<bool, static_cast<int>(Key::MaxKeys)> keys_held = {};
};

}  // namespace eng

#endif  // ENGINE_COMPONENTS_H

#if 0

//
// EcsCoreComponents.h
//

// Assuming you have your own math library with these types
#include "YourMath/Matrix4.h"
#include "YourMath/Quaternion.h"
#include "YourMath/Vector3.h"

// Define a type for your entity IDs
using EntityID = uint32_t;


/**
 * @brief The entity's local position, rotation, and scale relative to its parent.
 * This is the component the user or game logic systems (like AI or player input)
 * will typically modify.
 */
struct TransformComponent {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Quaternion rotation = Quaternion::Identity();
    Vector3 scale = {1.0f, 1.0f, 1.0f};

/**
 * @brief Defines the parent-child hierarchy.
 * An entity with this component is a child of the 'parent' entity.
 * A system will traverse this hierarchy to compute world transforms.
 */
struct SceneNodeComponent {
    EntityID parent;
    // Note: Children are often managed by a separate SceneGraph system
    // or by having each parent store a std::vector<EntityID> children.
    // Keeping it simple here.
};

/**
 * @brief The final, calculated, absolute world-space transform.
 * This is written to by an internal "SceneGraphUpdateSystem" and
 * read from by the Render and Physics systems.
 * User code should almost never write to this directly.
 */
struct WorldTransformComponent {
    Matrix4 transform = Matrix4::Identity();
};

//
// EcsPhysicsComponents.h
//

#include "YourCollision/OBB.h"  // Assuming you have an OBB class
#include "YourMath/Vector3.h"

/**
 * @brief The "intent" for movement, set by user-facing systems.
 * This is what GameFixedUpdate (e.g., PlayerInputSystem) writes to.
 * The engine's EngineFixedUpdate (PhysicsSystem) reads this.
 */
struct DesiredPositionComponent {
    Vector3 position;
};

/**
 * @brief The physics body definition for an entity.
 * This tells the PhysicsSystem that this object can collide.
 */
struct PhysicsComponent {
    // The object's actual collision shape (Oriented Bounding Box)
    OBB obb; 

    // The velocity, which the PhysicsSystem will calculate
    // from (DesiredPosition - CurrentPosition).
    Vector3 velocity; 

    // You can use bitmasks for collision layers
    uint32_t layer = 0; // e.g., 0001 = "Player"
    uint32_t mask = 0;  // e.g., 0010 = "Collides with Scenery"
};

/**
 * @brief A "tag" component.
 * Tells the PhysicsSystem that this entity is static (like a wall or asteroid).
 * The system will use this to skip movement calculations.
 */
struct StaticBodyComponent {};

/**
 * @brief The "memory" of the object's last *resolved* position.
 * This is written by the PhysicsSystem at the *start* of its tick.
 * It is read by the RenderSystem (in Draw) for interpolation.
 */
struct PreviousPositionComponent {
    Vector3 position;
};

//
// EcsRenderingComponents.h
//

// Assuming you have handles or IDs for your graphics resources
using MeshID = uint32_t;
using MaterialID = uint32_t;

/**
 * @brief A component that holds the data needed to draw an entity.
 * The RenderSystem will query for entities that have BOTH
 * a WorldTransformComponent AND a MeshComponent.
 */
struct MeshComponent {
    MeshID mesh;
    MaterialID material;
    
    // Bounding box for this mesh, used for frustum culling
    // This is in *local* space and must be transformed by the
    // WorldTransformComponent before culling.
    AABB localBounds;
};

enum class ProjectionType
{
    PERSPECTIVE,
    ORTHOGRAPHIC
};

/**
 * @struct CameraComponent
 * @brief A data-only component that defines the "lens" of a camera.
 * It does NOT store position or rotation (that's in CoreDataComponent).
 */
struct CameraComponent
{
    ProjectionType projection = ProjectionType::PERSPECTIVE;

    // --- Perspective Data ---
    float fieldOfView = 45.0f; // In degrees

    // --- Orthographic Data ---
    float orthographicSize = 10.0f;
    
    // --- Shared Data ---
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

/**
 * @struct PrimaryCameraTag
 * @brief An empty "tag" component.
 * You add this to the ONE entity you want to be the active camera.
 * The CameraSystem will look for the entity with this tag.
 */
struct PrimaryCameraTag {};

struct FlyCameraInputComponent {
    float pitch = 0.0f;
    float yaw = -90.0f;
    float moveSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
};

/**
 * @struct RenderMatrices
 * @brief A global "Resource" (or singleton component) that holds the
 * matrices for the entire frame.
 * The CameraSystem WRITES to this.
 * The RenderSystem READS from this.
 */
struct RenderMatrices
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
    glm::vec3 cameraWorldPosition;
};

/**
 * @struct Viewport
 * @brief Another global resource that provides viewport dimensions.
 * The CameraSystem READS this to get the aspect ratio.
 * Your "WindowSystem" or "RenderSystem" should update this on resize.
 */
struct Viewport
{
    float width = 1280.0f;
    float height = 720.0f;
    
    float GetAspectRatio() const
    {
        if (height == 0) return 1.0f;
        return width / height;
    }
};

// In your engine's Init() function...
Entity globalsEntity = m_World->CreateEntity();

// Add a tag so you can find it easily
m_World->AddComponent(globalsEntity, GlobalContextTag{}); 

// Add all your singleton data components
m_World->AddComponent(globalsEntity, RenderMatrices{});
m_World->AddComponent(globalsEntity, Viewport{});
m_World->AddComponent(globalsEntity, PlayerInput{});
m_World->AddComponent(globalsEntity, GameState{});
// ...and so on

#endif