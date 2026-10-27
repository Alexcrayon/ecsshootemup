#ifndef COMPONENT_STORAGE_HPP
#define COMPONENT_STORAGE_HPP

#include "components.hpp"
#include <unordered_map>
#include <optional>
#include <functional>
#include <vector>

/**
 * ComponentStorage class
 * 
 * Manages the storage and retrieval of components in the ECS architecture.
 * This class demonstrates a key ECS principle: components (data) are stored
 * separately from entities (IDs) and systems (logic).
 * 
 * The ComponentStorage associates component data with entity IDs using a
 * "table per component type" approach. Each component type (Position, Size, Color)
 * has its own std::unordered_map that maps Entity IDs to component instances.
 * 
 * Key Design Decisions:
 * - Template methods allow type-safe component operations
 * - std::optional<std::reference_wrapper<T>> for get_component() avoids copying
 *   and clearly indicates when a component doesn't exist
 * - Each entity can have at most one component of each type
 * - Adding a component when one already exists replaces the old value
 * 
 * Component Lifecycle:
 * - add_component<T>() associates a component with an entity
 * - get_component<T>() retrieves a reference to the component (if it exists)
 * - remove_component<T>() removes the association
 * - has_component<T>() checks if the association exists
 * - entities_with_component<T>() returns all entities that have that component type
 */
class ComponentStorage {
public:
    /**
     * Adds a component to an entity.
     * 
     * If the entity already has a component of this type, the old value is replaced.
     * This is the expected behavior for component updates - you don't need to remove
     * the old component first.
     * 
     * The component is copied into storage, so the original can be safely discarded.
     * 
     * @tparam T The component type (Position, Size, or Color)
     * @param entity The entity ID to attach the component to
     * @param component The component data to store
     */
    template<typename T>
    void add_component(Entity entity, const T& component);
    
    /**
     * Retrieves a component for an entity (mutable version).
     * 
     * Returns a reference wrapper to the component if it exists, allowing
     * modification of the component data. Returns std::nullopt if the entity
     * doesn't have this component type.
     * 
     * Using std::optional<std::reference_wrapper<T>> avoids copying the component
     * and clearly indicates the "not found" case without exceptions or null pointers.
     * 
     * Example usage:
     *   auto pos = storage.get_component<Position>(entity);
     *   if (pos.has_value()) {
     *       pos->get().x += 10;  // Modify the component
     *   }
     * 
     * @tparam T The component type (Position, Size, or Color)
     * @param entity The entity ID to retrieve the component from
     * @return std::optional containing a reference to the component, or std::nullopt
     */
    template<typename T>
    std::optional<std::reference_wrapper<T>> get_component(Entity entity);
    
    /**
     * Retrieves a component for an entity (const version).
     * 
     * Same as the mutable version, but returns a const reference for read-only access.
     * Used when you need to read component data without modifying it.
     * 
     * @tparam T The component type (Position, Size, or Color)
     * @param entity The entity ID to retrieve the component from
     * @return std::optional containing a const reference to the component, or std::nullopt
     */
    template<typename T>
    std::optional<std::reference_wrapper<const T>> get_component(Entity entity) const;
    
    /**
     * Removes a component from an entity.
     * 
     * After calling this method, has_component<T>(entity) will return false
     * and get_component<T>(entity) will return std::nullopt.
     * 
     * If the entity doesn't have this component type, this is a no-op (safe to call).
     * This simplifies cleanup code - you don't need to check has_component() first.
     * 
     * @tparam T The component type (Position, Size, or Color)
     * @param entity The entity ID to remove the component from
     */
    template<typename T>
    void remove_component(Entity entity);
    
    /**
     * Checks if an entity has a specific component type.
     * 
     * This is a convenience method that's equivalent to checking if
     * get_component<T>(entity).has_value(), but more readable.
     * 
     * @tparam T The component type (Position, Size, or Color)
     * @param entity The entity ID to check
     * @return true if the entity has this component type, false otherwise
     */
    template<typename T>
    bool has_component(Entity entity) const;
    
    /**
     * Returns all entities that have a specific component type.
     * 
     * This is used by systems to iterate over entities with required components.
     * For example, the RenderSystem uses this to find all entities with Position,
     * Size, and Color components.
     * 
     * The returned vector contains entity IDs in an unspecified but consistent order.
     * 
     * @tparam T The component type (Position, Size, or Color)
     * @return A vector of entity IDs that have this component type
     */
    template<typename T>
    std::vector<Entity> entities_with_component() const;

private:
    /**
     * Storage for Position components.
     * Maps entity IDs to Position data. Only entities that have a Position
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Position> positions_;
    
    /**
     * Storage for Size components.
     * Maps entity IDs to Size data. Only entities that have a Size
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Size> sizes_;
    
    /**
     * Storage for Color components.
     * Maps entity IDs to Color data. Only entities that have a Color
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Color> colors_;
    
    /**
     * Storage for Input components.
     * Maps entity IDs to Input data. Only entities that have an Input
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Input> inputs_;
    
    /**
     * Storage for Velocity components.
     * Maps entity IDs to Velocity data. Only entities that have a Velocity
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Velocity> velocities_;
    
    /**
     * Storage for Images components.
     * Maps entity IDs to Images data. Only entities that have an Images
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Images> images_;
    
    /**
     * Storage for Text components.
     * Maps entity IDs to Text data. Only entities that have a Text
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Text> texts_;
    
    /**
     * Storage for ScreenPosition components.
     * Maps entity IDs to ScreenPosition data. Only entities that have a ScreenPosition
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, ScreenPosition> screen_positions_;
    
    /**
     * Storage for Rotation components.
     * Maps entity IDs to Rotation data. Only entities that have a Rotation
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Rotation> rotations_;
    
    /**
     * Storage for Collider components.
     * Maps entity IDs to Collider data. Only entities that have a Collider
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Collider> colliders_;

    /**
     * Storage for CircleCollider components.
     * Maps entity IDs to CircleCollider data. Only entities that have a CircleCollider
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, CircleCollider> circle_colliders_;

    /**
     * Storage for OBBCollider components.
     * Maps entity IDs to OBBCollider data. Only entities that have an OBBCollider
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, OBBCollider> obb_colliders_;

    /**
     * Storage for Lifetime components.
     * Maps entity IDs to Lifetime data. Only entities that have a Lifetime
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Lifetime> lifetimes_;
    
    /**
     * Storage for WrapAround components.
     * Maps entity IDs to WrapAround data. Only entities that have a WrapAround
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, WrapAround> wrap_arounds_;
    
    /**
     * Storage for DestroyRequest components.
     * Maps entity IDs to DestroyRequest data. Only entities that have a DestroyRequest
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, DestroyRequest> destroy_requests_;
    
    /**
     * Storage for CollidedWith components.
     * Maps entity IDs to CollidedWith data. Only entities that have a CollidedWith
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, CollidedWith> collided_withs_;
    
    /**
     * Storage for Script components.
     * Maps entity IDs to Script data. Only entities that have a Script
     * component will have an entry in this map.
     */
    std::unordered_map<Entity, Script> scripts_;

    // ===== NEW (Class-110): bullet-hell component storage =====
    /** Storage for BulletTag components (marks bullets for the live cap/queries). */
    std::unordered_map<Entity, BulletTag> bullet_tags_;
    /** Storage for ShipTag components (marks the player entity). */
    std::unordered_map<Entity, ShipTag> ship_tags_;
    /** Storage for EnemyTag components (marks enemies for wave-clear detection). */
    std::unordered_map<Entity, EnemyTag> enemy_tags_;
    /** Storage for Health components (player, enemies, and boss). */
    std::unordered_map<Entity, Health> healths_;
    /** Storage for IgnoreWorldBounds components (projectiles leave the arena). */
    std::unordered_map<Entity, IgnoreWorldBounds> ignore_world_bounds_;
    /** Storage for Flash components (brief hit flash). */
    std::unordered_map<Entity, Flash> flashes_;
    /** Storage for AudioSource components (ported Class-070 audio). */
    std::unordered_map<Entity, AudioSource> audio_sources_;

    /**
     * Helper method to get the storage map for a specific component type.
     * This allows the template methods to work uniformly across all component types.
     * 
     * @tparam T The component type
     * @return Reference to the storage map for that type
     */
    template<typename T>
    std::unordered_map<Entity, T>& get_storage();
    
    /**
     * Const version of get_storage() for read-only access.
     * 
     * @tparam T The component type
     * @return Const reference to the storage map for that type
     */
    template<typename T>
    const std::unordered_map<Entity, T>& get_storage() const;
};

// Declare explicit specializations of get_storage
template<> std::unordered_map<Entity, Position>& ComponentStorage::get_storage<Position>();
template<> std::unordered_map<Entity, Size>& ComponentStorage::get_storage<Size>();
template<> std::unordered_map<Entity, Color>& ComponentStorage::get_storage<Color>();
template<> std::unordered_map<Entity, Input>& ComponentStorage::get_storage<Input>();
template<> std::unordered_map<Entity, Velocity>& ComponentStorage::get_storage<Velocity>();
template<> std::unordered_map<Entity, Images>& ComponentStorage::get_storage<Images>();
template<> std::unordered_map<Entity, Text>& ComponentStorage::get_storage<Text>();
template<> std::unordered_map<Entity, ScreenPosition>& ComponentStorage::get_storage<ScreenPosition>();
template<> const std::unordered_map<Entity, Position>& ComponentStorage::get_storage<Position>() const;
template<> const std::unordered_map<Entity, Size>& ComponentStorage::get_storage<Size>() const;
template<> const std::unordered_map<Entity, Color>& ComponentStorage::get_storage<Color>() const;
template<> const std::unordered_map<Entity, Input>& ComponentStorage::get_storage<Input>() const;
template<> const std::unordered_map<Entity, Velocity>& ComponentStorage::get_storage<Velocity>() const;
template<> const std::unordered_map<Entity, Images>& ComponentStorage::get_storage<Images>() const;
template<> const std::unordered_map<Entity, Text>& ComponentStorage::get_storage<Text>() const;
template<> const std::unordered_map<Entity, ScreenPosition>& ComponentStorage::get_storage<ScreenPosition>() const;
template<> std::unordered_map<Entity, Rotation>& ComponentStorage::get_storage<Rotation>();
template<> std::unordered_map<Entity, Collider>& ComponentStorage::get_storage<Collider>();
template<> std::unordered_map<Entity, CircleCollider>& ComponentStorage::get_storage<CircleCollider>();
template<> std::unordered_map<Entity, OBBCollider>& ComponentStorage::get_storage<OBBCollider>();
template<> std::unordered_map<Entity, Lifetime>& ComponentStorage::get_storage<Lifetime>();
template<> std::unordered_map<Entity, WrapAround>& ComponentStorage::get_storage<WrapAround>();
template<> std::unordered_map<Entity, DestroyRequest>& ComponentStorage::get_storage<DestroyRequest>();
template<> std::unordered_map<Entity, CollidedWith>& ComponentStorage::get_storage<CollidedWith>();
template<> const std::unordered_map<Entity, Rotation>& ComponentStorage::get_storage<Rotation>() const;
template<> const std::unordered_map<Entity, Collider>& ComponentStorage::get_storage<Collider>() const;
template<> const std::unordered_map<Entity, CircleCollider>& ComponentStorage::get_storage<CircleCollider>() const;
template<> const std::unordered_map<Entity, OBBCollider>& ComponentStorage::get_storage<OBBCollider>() const;
template<> const std::unordered_map<Entity, Lifetime>& ComponentStorage::get_storage<Lifetime>() const;
template<> const std::unordered_map<Entity, WrapAround>& ComponentStorage::get_storage<WrapAround>() const;
template<> const std::unordered_map<Entity, DestroyRequest>& ComponentStorage::get_storage<DestroyRequest>() const;
template<> const std::unordered_map<Entity, CollidedWith>& ComponentStorage::get_storage<CollidedWith>() const;
template<> std::unordered_map<Entity, Script>& ComponentStorage::get_storage<Script>();
template<> const std::unordered_map<Entity, Script>& ComponentStorage::get_storage<Script>() const;

// NEW (Class-110): get_storage specializations for the bullet-hell components.
template<> std::unordered_map<Entity, BulletTag>& ComponentStorage::get_storage<BulletTag>();
template<> const std::unordered_map<Entity, BulletTag>& ComponentStorage::get_storage<BulletTag>() const;
template<> std::unordered_map<Entity, ShipTag>& ComponentStorage::get_storage<ShipTag>();
template<> const std::unordered_map<Entity, ShipTag>& ComponentStorage::get_storage<ShipTag>() const;
template<> std::unordered_map<Entity, EnemyTag>& ComponentStorage::get_storage<EnemyTag>();
template<> const std::unordered_map<Entity, EnemyTag>& ComponentStorage::get_storage<EnemyTag>() const;
template<> std::unordered_map<Entity, Health>& ComponentStorage::get_storage<Health>();
template<> const std::unordered_map<Entity, Health>& ComponentStorage::get_storage<Health>() const;
template<> std::unordered_map<Entity, IgnoreWorldBounds>& ComponentStorage::get_storage<IgnoreWorldBounds>();
template<> const std::unordered_map<Entity, IgnoreWorldBounds>& ComponentStorage::get_storage<IgnoreWorldBounds>() const;
template<> std::unordered_map<Entity, Flash>& ComponentStorage::get_storage<Flash>();
template<> const std::unordered_map<Entity, Flash>& ComponentStorage::get_storage<Flash>() const;
template<> std::unordered_map<Entity, AudioSource>& ComponentStorage::get_storage<AudioSource>();
template<> const std::unordered_map<Entity, AudioSource>& ComponentStorage::get_storage<AudioSource>() const;

// Prevent implicit instantiation — definitions are in component_storage.cpp
extern template void ComponentStorage::add_component<Position>(Entity, const Position&);
extern template void ComponentStorage::add_component<Size>(Entity, const Size&);
extern template void ComponentStorage::add_component<Color>(Entity, const Color&);
extern template void ComponentStorage::add_component<Input>(Entity, const Input&);
extern template void ComponentStorage::add_component<Velocity>(Entity, const Velocity&);
extern template void ComponentStorage::add_component<Images>(Entity, const Images&);
extern template void ComponentStorage::add_component<Text>(Entity, const Text&);
extern template void ComponentStorage::add_component<ScreenPosition>(Entity, const ScreenPosition&);

extern template std::optional<std::reference_wrapper<Position>> ComponentStorage::get_component<Position>(Entity);
extern template std::optional<std::reference_wrapper<Size>> ComponentStorage::get_component<Size>(Entity);
extern template std::optional<std::reference_wrapper<Color>> ComponentStorage::get_component<Color>(Entity);
extern template std::optional<std::reference_wrapper<Input>> ComponentStorage::get_component<Input>(Entity);
extern template std::optional<std::reference_wrapper<Velocity>> ComponentStorage::get_component<Velocity>(Entity);
extern template std::optional<std::reference_wrapper<Images>> ComponentStorage::get_component<Images>(Entity);
extern template std::optional<std::reference_wrapper<Text>> ComponentStorage::get_component<Text>(Entity);
extern template std::optional<std::reference_wrapper<ScreenPosition>> ComponentStorage::get_component<ScreenPosition>(Entity);

extern template std::optional<std::reference_wrapper<const Position>> ComponentStorage::get_component<Position>(Entity) const;
extern template std::optional<std::reference_wrapper<const Size>> ComponentStorage::get_component<Size>(Entity) const;
extern template std::optional<std::reference_wrapper<const Color>> ComponentStorage::get_component<Color>(Entity) const;
extern template std::optional<std::reference_wrapper<const Input>> ComponentStorage::get_component<Input>(Entity) const;
extern template std::optional<std::reference_wrapper<const Velocity>> ComponentStorage::get_component<Velocity>(Entity) const;
extern template std::optional<std::reference_wrapper<const Images>> ComponentStorage::get_component<Images>(Entity) const;
extern template std::optional<std::reference_wrapper<const Text>> ComponentStorage::get_component<Text>(Entity) const;
extern template std::optional<std::reference_wrapper<const ScreenPosition>> ComponentStorage::get_component<ScreenPosition>(Entity) const;

extern template void ComponentStorage::remove_component<Position>(Entity);
extern template void ComponentStorage::remove_component<Size>(Entity);
extern template void ComponentStorage::remove_component<Color>(Entity);
extern template void ComponentStorage::remove_component<Input>(Entity);
extern template void ComponentStorage::remove_component<Velocity>(Entity);
extern template void ComponentStorage::remove_component<Images>(Entity);
extern template void ComponentStorage::remove_component<Text>(Entity);
extern template void ComponentStorage::remove_component<ScreenPosition>(Entity);

extern template bool ComponentStorage::has_component<Position>(Entity) const;
extern template bool ComponentStorage::has_component<Size>(Entity) const;
extern template bool ComponentStorage::has_component<Color>(Entity) const;
extern template bool ComponentStorage::has_component<Input>(Entity) const;
extern template bool ComponentStorage::has_component<Velocity>(Entity) const;
extern template bool ComponentStorage::has_component<Images>(Entity) const;
extern template bool ComponentStorage::has_component<Text>(Entity) const;
extern template bool ComponentStorage::has_component<ScreenPosition>(Entity) const;

extern template std::vector<Entity> ComponentStorage::entities_with_component<Position>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Size>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Color>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Input>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Velocity>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Images>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Text>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<ScreenPosition>() const;

extern template void ComponentStorage::add_component<Rotation>(Entity, const Rotation&);
extern template void ComponentStorage::add_component<Collider>(Entity, const Collider&);
extern template void ComponentStorage::add_component<CircleCollider>(Entity, const CircleCollider&);
extern template void ComponentStorage::add_component<OBBCollider>(Entity, const OBBCollider&);
extern template void ComponentStorage::add_component<Lifetime>(Entity, const Lifetime&);
extern template void ComponentStorage::add_component<WrapAround>(Entity, const WrapAround&);
extern template void ComponentStorage::add_component<DestroyRequest>(Entity, const DestroyRequest&);
extern template void ComponentStorage::add_component<CollidedWith>(Entity, const CollidedWith&);

extern template std::optional<std::reference_wrapper<Rotation>> ComponentStorage::get_component<Rotation>(Entity);
extern template std::optional<std::reference_wrapper<Collider>> ComponentStorage::get_component<Collider>(Entity);
extern template std::optional<std::reference_wrapper<CircleCollider>> ComponentStorage::get_component<CircleCollider>(Entity);
extern template std::optional<std::reference_wrapper<OBBCollider>> ComponentStorage::get_component<OBBCollider>(Entity);
extern template std::optional<std::reference_wrapper<Lifetime>> ComponentStorage::get_component<Lifetime>(Entity);
extern template std::optional<std::reference_wrapper<WrapAround>> ComponentStorage::get_component<WrapAround>(Entity);
extern template std::optional<std::reference_wrapper<DestroyRequest>> ComponentStorage::get_component<DestroyRequest>(Entity);
extern template std::optional<std::reference_wrapper<CollidedWith>> ComponentStorage::get_component<CollidedWith>(Entity);

extern template std::optional<std::reference_wrapper<const Rotation>> ComponentStorage::get_component<Rotation>(Entity) const;
extern template std::optional<std::reference_wrapper<const Collider>> ComponentStorage::get_component<Collider>(Entity) const;
extern template std::optional<std::reference_wrapper<const CircleCollider>> ComponentStorage::get_component<CircleCollider>(Entity) const;
extern template std::optional<std::reference_wrapper<const OBBCollider>> ComponentStorage::get_component<OBBCollider>(Entity) const;
extern template std::optional<std::reference_wrapper<const Lifetime>> ComponentStorage::get_component<Lifetime>(Entity) const;
extern template std::optional<std::reference_wrapper<const WrapAround>> ComponentStorage::get_component<WrapAround>(Entity) const;
extern template std::optional<std::reference_wrapper<const DestroyRequest>> ComponentStorage::get_component<DestroyRequest>(Entity) const;
extern template std::optional<std::reference_wrapper<const CollidedWith>> ComponentStorage::get_component<CollidedWith>(Entity) const;

extern template void ComponentStorage::remove_component<Rotation>(Entity);
extern template void ComponentStorage::remove_component<Collider>(Entity);
extern template void ComponentStorage::remove_component<CircleCollider>(Entity);
extern template void ComponentStorage::remove_component<OBBCollider>(Entity);
extern template void ComponentStorage::remove_component<Lifetime>(Entity);
extern template void ComponentStorage::remove_component<WrapAround>(Entity);
extern template void ComponentStorage::remove_component<DestroyRequest>(Entity);
extern template void ComponentStorage::remove_component<CollidedWith>(Entity);

extern template bool ComponentStorage::has_component<Rotation>(Entity) const;
extern template bool ComponentStorage::has_component<Collider>(Entity) const;
extern template bool ComponentStorage::has_component<CircleCollider>(Entity) const;
extern template bool ComponentStorage::has_component<OBBCollider>(Entity) const;
extern template bool ComponentStorage::has_component<Lifetime>(Entity) const;
extern template bool ComponentStorage::has_component<WrapAround>(Entity) const;
extern template bool ComponentStorage::has_component<DestroyRequest>(Entity) const;
extern template bool ComponentStorage::has_component<CollidedWith>(Entity) const;

extern template std::vector<Entity> ComponentStorage::entities_with_component<Rotation>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Collider>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<CircleCollider>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<OBBCollider>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Lifetime>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<WrapAround>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<DestroyRequest>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<CollidedWith>() const;

extern template void ComponentStorage::add_component<Script>(Entity, const Script&);

extern template std::optional<std::reference_wrapper<Script>> ComponentStorage::get_component<Script>(Entity);

extern template std::optional<std::reference_wrapper<const Script>> ComponentStorage::get_component<Script>(Entity) const;

extern template void ComponentStorage::remove_component<Script>(Entity);

extern template bool ComponentStorage::has_component<Script>(Entity) const;

extern template std::vector<Entity> ComponentStorage::entities_with_component<Script>() const;

// NEW (Class-110): extern template declarations for the bullet-hell components.
extern template void ComponentStorage::add_component<BulletTag>(Entity, const BulletTag&);
extern template void ComponentStorage::add_component<ShipTag>(Entity, const ShipTag&);
extern template void ComponentStorage::add_component<EnemyTag>(Entity, const EnemyTag&);
extern template void ComponentStorage::add_component<Health>(Entity, const Health&);

extern template std::optional<std::reference_wrapper<BulletTag>> ComponentStorage::get_component<BulletTag>(Entity);
extern template std::optional<std::reference_wrapper<ShipTag>> ComponentStorage::get_component<ShipTag>(Entity);
extern template std::optional<std::reference_wrapper<EnemyTag>> ComponentStorage::get_component<EnemyTag>(Entity);
extern template std::optional<std::reference_wrapper<Health>> ComponentStorage::get_component<Health>(Entity);

extern template std::optional<std::reference_wrapper<const BulletTag>> ComponentStorage::get_component<BulletTag>(Entity) const;
extern template std::optional<std::reference_wrapper<const ShipTag>> ComponentStorage::get_component<ShipTag>(Entity) const;
extern template std::optional<std::reference_wrapper<const EnemyTag>> ComponentStorage::get_component<EnemyTag>(Entity) const;
extern template std::optional<std::reference_wrapper<const Health>> ComponentStorage::get_component<Health>(Entity) const;

extern template void ComponentStorage::remove_component<BulletTag>(Entity);
extern template void ComponentStorage::remove_component<ShipTag>(Entity);
extern template void ComponentStorage::remove_component<EnemyTag>(Entity);
extern template void ComponentStorage::remove_component<Health>(Entity);

extern template bool ComponentStorage::has_component<BulletTag>(Entity) const;
extern template bool ComponentStorage::has_component<ShipTag>(Entity) const;
extern template bool ComponentStorage::has_component<EnemyTag>(Entity) const;
extern template bool ComponentStorage::has_component<Health>(Entity) const;

extern template std::vector<Entity> ComponentStorage::entities_with_component<BulletTag>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<ShipTag>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<EnemyTag>() const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Health>() const;

// NEW (Class-110): IgnoreWorldBounds — opt out of MovementSystem clamping.
extern template void ComponentStorage::add_component<IgnoreWorldBounds>(Entity, const IgnoreWorldBounds&);
extern template std::optional<std::reference_wrapper<IgnoreWorldBounds>> ComponentStorage::get_component<IgnoreWorldBounds>(Entity);
extern template std::optional<std::reference_wrapper<const IgnoreWorldBounds>> ComponentStorage::get_component<IgnoreWorldBounds>(Entity) const;
extern template void ComponentStorage::remove_component<IgnoreWorldBounds>(Entity);
extern template bool ComponentStorage::has_component<IgnoreWorldBounds>(Entity) const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<IgnoreWorldBounds>() const;

// NEW (Class-110): Flash — brief hit flash.
extern template void ComponentStorage::add_component<Flash>(Entity, const Flash&);
extern template std::optional<std::reference_wrapper<Flash>> ComponentStorage::get_component<Flash>(Entity);
extern template std::optional<std::reference_wrapper<const Flash>> ComponentStorage::get_component<Flash>(Entity) const;
extern template void ComponentStorage::remove_component<Flash>(Entity);
extern template bool ComponentStorage::has_component<Flash>(Entity) const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<Flash>() const;

// NEW (Class-110): AudioSource — ported Class-070 audio system.
extern template void ComponentStorage::add_component<AudioSource>(Entity, const AudioSource&);
extern template std::optional<std::reference_wrapper<AudioSource>> ComponentStorage::get_component<AudioSource>(Entity);
extern template std::optional<std::reference_wrapper<const AudioSource>> ComponentStorage::get_component<AudioSource>(Entity) const;
extern template void ComponentStorage::remove_component<AudioSource>(Entity);
extern template bool ComponentStorage::has_component<AudioSource>(Entity) const;
extern template std::vector<Entity> ComponentStorage::entities_with_component<AudioSource>() const;

#endif // COMPONENT_STORAGE_HPP
