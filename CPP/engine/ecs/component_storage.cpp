#include "component_storage.hpp"

// Template specializations for get_storage() - maps component types to storage maps

template<>
std::unordered_map<Entity, Position>& ComponentStorage::get_storage<Position>() {
    return positions_;
}

template<>
std::unordered_map<Entity, Size>& ComponentStorage::get_storage<Size>() {
    return sizes_;
}

template<>
std::unordered_map<Entity, Color>& ComponentStorage::get_storage<Color>() {
    return colors_;
}

template<>
const std::unordered_map<Entity, Position>& ComponentStorage::get_storage<Position>() const {
    return positions_;
}

template<>
const std::unordered_map<Entity, Size>& ComponentStorage::get_storage<Size>() const {
    return sizes_;
}

template<>
const std::unordered_map<Entity, Color>& ComponentStorage::get_storage<Color>() const {
    return colors_;
}

template<>
std::unordered_map<Entity, Input>& ComponentStorage::get_storage<Input>() {
    return inputs_;
}

template<>
const std::unordered_map<Entity, Input>& ComponentStorage::get_storage<Input>() const {
    return inputs_;
}

template<>
std::unordered_map<Entity, Velocity>& ComponentStorage::get_storage<Velocity>() {
    return velocities_;
}

template<>
const std::unordered_map<Entity, Velocity>& ComponentStorage::get_storage<Velocity>() const {
    return velocities_;
}

template<>
std::unordered_map<Entity, Images>& ComponentStorage::get_storage<Images>() {
    return images_;
}

template<>
const std::unordered_map<Entity, Images>& ComponentStorage::get_storage<Images>() const {
    return images_;
}

template<>
std::unordered_map<Entity, Text>& ComponentStorage::get_storage<Text>() {
    return texts_;
}

template<>
const std::unordered_map<Entity, Text>& ComponentStorage::get_storage<Text>() const {
    return texts_;
}

template<>
std::unordered_map<Entity, ScreenPosition>& ComponentStorage::get_storage<ScreenPosition>() {
    return screen_positions_;
}

template<>
const std::unordered_map<Entity, ScreenPosition>& ComponentStorage::get_storage<ScreenPosition>() const {
    return screen_positions_;
}

template<>
std::unordered_map<Entity, Rotation>& ComponentStorage::get_storage<Rotation>() {
    return rotations_;
}

template<>
const std::unordered_map<Entity, Rotation>& ComponentStorage::get_storage<Rotation>() const {
    return rotations_;
}

template<>
std::unordered_map<Entity, Collider>& ComponentStorage::get_storage<Collider>() {
    return colliders_;
}

template<>
const std::unordered_map<Entity, Collider>& ComponentStorage::get_storage<Collider>() const {
    return colliders_;
}

template<>
std::unordered_map<Entity, CircleCollider>& ComponentStorage::get_storage<CircleCollider>() {
    return circle_colliders_;
}

template<>
const std::unordered_map<Entity, CircleCollider>& ComponentStorage::get_storage<CircleCollider>() const {
    return circle_colliders_;
}

template<>
std::unordered_map<Entity, OBBCollider>& ComponentStorage::get_storage<OBBCollider>() {
    return obb_colliders_;
}

template<>
const std::unordered_map<Entity, OBBCollider>& ComponentStorage::get_storage<OBBCollider>() const {
    return obb_colliders_;
}

template<>
std::unordered_map<Entity, Lifetime>& ComponentStorage::get_storage<Lifetime>() {
    return lifetimes_;
}

template<>
const std::unordered_map<Entity, Lifetime>& ComponentStorage::get_storage<Lifetime>() const {
    return lifetimes_;
}

template<>
std::unordered_map<Entity, WrapAround>& ComponentStorage::get_storage<WrapAround>() {
    return wrap_arounds_;
}

template<>
const std::unordered_map<Entity, WrapAround>& ComponentStorage::get_storage<WrapAround>() const {
    return wrap_arounds_;
}

template<>
std::unordered_map<Entity, DestroyRequest>& ComponentStorage::get_storage<DestroyRequest>() {
    return destroy_requests_;
}

template<>
const std::unordered_map<Entity, DestroyRequest>& ComponentStorage::get_storage<DestroyRequest>() const {
    return destroy_requests_;
}

template<>
std::unordered_map<Entity, CollidedWith>& ComponentStorage::get_storage<CollidedWith>() {
    return collided_withs_;
}

template<>
const std::unordered_map<Entity, CollidedWith>& ComponentStorage::get_storage<CollidedWith>() const {
    return collided_withs_;
}

template<>
std::unordered_map<Entity, Script>& ComponentStorage::get_storage<Script>() {
    return scripts_;
}

template<>
const std::unordered_map<Entity, Script>& ComponentStorage::get_storage<Script>() const {
    return scripts_;
}

// NEW (Class-110): get_storage specializations for the bullet-hell components.
template<>
std::unordered_map<Entity, BulletTag>& ComponentStorage::get_storage<BulletTag>() {
    return bullet_tags_;
}
template<>
const std::unordered_map<Entity, BulletTag>& ComponentStorage::get_storage<BulletTag>() const {
    return bullet_tags_;
}
template<>
std::unordered_map<Entity, ShipTag>& ComponentStorage::get_storage<ShipTag>() {
    return ship_tags_;
}
template<>
const std::unordered_map<Entity, ShipTag>& ComponentStorage::get_storage<ShipTag>() const {
    return ship_tags_;
}
template<>
std::unordered_map<Entity, EnemyTag>& ComponentStorage::get_storage<EnemyTag>() {
    return enemy_tags_;
}
template<>
const std::unordered_map<Entity, EnemyTag>& ComponentStorage::get_storage<EnemyTag>() const {
    return enemy_tags_;
}
template<>
std::unordered_map<Entity, Health>& ComponentStorage::get_storage<Health>() {
    return healths_;
}
template<>
const std::unordered_map<Entity, Health>& ComponentStorage::get_storage<Health>() const {
    return healths_;
}
template<>
std::unordered_map<Entity, IgnoreWorldBounds>& ComponentStorage::get_storage<IgnoreWorldBounds>() {
    return ignore_world_bounds_;
}
template<>
const std::unordered_map<Entity, IgnoreWorldBounds>& ComponentStorage::get_storage<IgnoreWorldBounds>() const {
    return ignore_world_bounds_;
}
template<>
std::unordered_map<Entity, Flash>& ComponentStorage::get_storage<Flash>() {
    return flashes_;
}
template<>
const std::unordered_map<Entity, Flash>& ComponentStorage::get_storage<Flash>() const {
    return flashes_;
}
template<>
std::unordered_map<Entity, AudioSource>& ComponentStorage::get_storage<AudioSource>() {
    return audio_sources_;
}
template<>
const std::unordered_map<Entity, AudioSource>& ComponentStorage::get_storage<AudioSource>() const {
    return audio_sources_;
}

// Template method implementations

template<typename T>
void ComponentStorage::add_component(Entity entity, const T& component) {
    // Get the storage map for this component type
    auto& storage = get_storage<T>();
    
    // Insert or update the component
    // If the entity already has this component type, this replaces it
    storage[entity] = component;
}

template<typename T>
std::optional<std::reference_wrapper<T>> ComponentStorage::get_component(Entity entity) {
    // Get the storage map for this component type
    auto& storage = get_storage<T>();
    
    // Look up the entity in the storage map
    auto it = storage.find(entity);
    
    if (it != storage.end()) {
        // Component found - return a reference wrapper to it
        return std::ref(it->second);
    } else {
        // Component not found - return empty optional
        return std::nullopt;
    }
}

template<typename T>
std::optional<std::reference_wrapper<const T>> ComponentStorage::get_component(Entity entity) const {
    // Get the storage map for this component type
    const auto& storage = get_storage<T>();
    
    // Look up the entity in the storage map
    auto it = storage.find(entity);
    
    if (it != storage.end()) {
        // Component found - return a const reference wrapper to it
        return std::cref(it->second);
    } else {
        // Component not found - return empty optional
        return std::nullopt;
    }
}

template<typename T>
void ComponentStorage::remove_component(Entity entity) {
    // Get the storage map for this component type
    auto& storage = get_storage<T>();
    
    // Remove the entity from the storage map
    // If the entity doesn't have this component, erase() is a no-op
    storage.erase(entity);
}

template<typename T>
bool ComponentStorage::has_component(Entity entity) const {
    // Get the storage map for this component type
    const auto& storage = get_storage<T>();
    
    // Check if the entity exists in the storage map
    return storage.find(entity) != storage.end();
}

template<typename T>
std::vector<Entity> ComponentStorage::entities_with_component() const {
    // Get the storage map for this component type
    const auto& storage = get_storage<T>();
    
    // Collect all entity IDs from the storage map
    std::vector<Entity> entities;
    entities.reserve(storage.size());  // Pre-allocate for efficiency
    
    for (const auto& [entity, component] : storage) {
        entities.push_back(entity);
    }
    
    return entities;
}

// Explicit template instantiations for the three component types
// This ensures the template methods are compiled for Position, Size, and Color

template void ComponentStorage::add_component<Position>(Entity, const Position&);
template void ComponentStorage::add_component<Size>(Entity, const Size&);
template void ComponentStorage::add_component<Color>(Entity, const Color&);
template void ComponentStorage::add_component<Input>(Entity, const Input&);
template void ComponentStorage::add_component<Velocity>(Entity, const Velocity&);

template void ComponentStorage::add_component<Images>(Entity, const Images&);

template void ComponentStorage::add_component<Text>(Entity, const Text&);
template void ComponentStorage::add_component<ScreenPosition>(Entity, const ScreenPosition&);

template void ComponentStorage::add_component<Rotation>(Entity, const Rotation&);
template void ComponentStorage::add_component<Collider>(Entity, const Collider&);
template void ComponentStorage::add_component<CircleCollider>(Entity, const CircleCollider&);
template void ComponentStorage::add_component<OBBCollider>(Entity, const OBBCollider&);
template void ComponentStorage::add_component<Lifetime>(Entity, const Lifetime&);
template void ComponentStorage::add_component<WrapAround>(Entity, const WrapAround&);
template void ComponentStorage::add_component<DestroyRequest>(Entity, const DestroyRequest&);
template void ComponentStorage::add_component<CollidedWith>(Entity, const CollidedWith&);
template void ComponentStorage::add_component<Script>(Entity, const Script&);

template std::optional<std::reference_wrapper<Position>> ComponentStorage::get_component<Position>(Entity);
template std::optional<std::reference_wrapper<Size>> ComponentStorage::get_component<Size>(Entity);
template std::optional<std::reference_wrapper<Color>> ComponentStorage::get_component<Color>(Entity);
template std::optional<std::reference_wrapper<Input>> ComponentStorage::get_component<Input>(Entity);
template std::optional<std::reference_wrapper<Velocity>> ComponentStorage::get_component<Velocity>(Entity);

template std::optional<std::reference_wrapper<Images>> ComponentStorage::get_component<Images>(Entity);

template std::optional<std::reference_wrapper<Text>> ComponentStorage::get_component<Text>(Entity);
template std::optional<std::reference_wrapper<ScreenPosition>> ComponentStorage::get_component<ScreenPosition>(Entity);

template std::optional<std::reference_wrapper<Rotation>> ComponentStorage::get_component<Rotation>(Entity);
template std::optional<std::reference_wrapper<Collider>> ComponentStorage::get_component<Collider>(Entity);
template std::optional<std::reference_wrapper<CircleCollider>> ComponentStorage::get_component<CircleCollider>(Entity);
template std::optional<std::reference_wrapper<OBBCollider>> ComponentStorage::get_component<OBBCollider>(Entity);
template std::optional<std::reference_wrapper<Lifetime>> ComponentStorage::get_component<Lifetime>(Entity);
template std::optional<std::reference_wrapper<WrapAround>> ComponentStorage::get_component<WrapAround>(Entity);
template std::optional<std::reference_wrapper<DestroyRequest>> ComponentStorage::get_component<DestroyRequest>(Entity);
template std::optional<std::reference_wrapper<CollidedWith>> ComponentStorage::get_component<CollidedWith>(Entity);
template std::optional<std::reference_wrapper<Script>> ComponentStorage::get_component<Script>(Entity);

template std::optional<std::reference_wrapper<const Position>> ComponentStorage::get_component<Position>(Entity) const;
template std::optional<std::reference_wrapper<const Size>> ComponentStorage::get_component<Size>(Entity) const;
template std::optional<std::reference_wrapper<const Color>> ComponentStorage::get_component<Color>(Entity) const;
template std::optional<std::reference_wrapper<const Input>> ComponentStorage::get_component<Input>(Entity) const;
template std::optional<std::reference_wrapper<const Velocity>> ComponentStorage::get_component<Velocity>(Entity) const;

template std::optional<std::reference_wrapper<const Images>> ComponentStorage::get_component<Images>(Entity) const;

template std::optional<std::reference_wrapper<const Text>> ComponentStorage::get_component<Text>(Entity) const;
template std::optional<std::reference_wrapper<const ScreenPosition>> ComponentStorage::get_component<ScreenPosition>(Entity) const;

template std::optional<std::reference_wrapper<const Rotation>> ComponentStorage::get_component<Rotation>(Entity) const;
template std::optional<std::reference_wrapper<const Collider>> ComponentStorage::get_component<Collider>(Entity) const;
template std::optional<std::reference_wrapper<const CircleCollider>> ComponentStorage::get_component<CircleCollider>(Entity) const;
template std::optional<std::reference_wrapper<const OBBCollider>> ComponentStorage::get_component<OBBCollider>(Entity) const;
template std::optional<std::reference_wrapper<const Lifetime>> ComponentStorage::get_component<Lifetime>(Entity) const;
template std::optional<std::reference_wrapper<const WrapAround>> ComponentStorage::get_component<WrapAround>(Entity) const;
template std::optional<std::reference_wrapper<const DestroyRequest>> ComponentStorage::get_component<DestroyRequest>(Entity) const;
template std::optional<std::reference_wrapper<const CollidedWith>> ComponentStorage::get_component<CollidedWith>(Entity) const;
template std::optional<std::reference_wrapper<const Script>> ComponentStorage::get_component<Script>(Entity) const;

template void ComponentStorage::remove_component<Position>(Entity);
template void ComponentStorage::remove_component<Size>(Entity);
template void ComponentStorage::remove_component<Color>(Entity);
template void ComponentStorage::remove_component<Input>(Entity);
template void ComponentStorage::remove_component<Velocity>(Entity);

template void ComponentStorage::remove_component<Images>(Entity);

template void ComponentStorage::remove_component<Text>(Entity);
template void ComponentStorage::remove_component<ScreenPosition>(Entity);

template void ComponentStorage::remove_component<Rotation>(Entity);
template void ComponentStorage::remove_component<Collider>(Entity);
template void ComponentStorage::remove_component<CircleCollider>(Entity);
template void ComponentStorage::remove_component<OBBCollider>(Entity);
template void ComponentStorage::remove_component<Lifetime>(Entity);
template void ComponentStorage::remove_component<WrapAround>(Entity);
template void ComponentStorage::remove_component<DestroyRequest>(Entity);
template void ComponentStorage::remove_component<CollidedWith>(Entity);
template void ComponentStorage::remove_component<Script>(Entity);

template bool ComponentStorage::has_component<Position>(Entity) const;
template bool ComponentStorage::has_component<Size>(Entity) const;
template bool ComponentStorage::has_component<Color>(Entity) const;
template bool ComponentStorage::has_component<Input>(Entity) const;
template bool ComponentStorage::has_component<Velocity>(Entity) const;

template bool ComponentStorage::has_component<Images>(Entity) const;

template bool ComponentStorage::has_component<Text>(Entity) const;
template bool ComponentStorage::has_component<ScreenPosition>(Entity) const;

template bool ComponentStorage::has_component<Rotation>(Entity) const;
template bool ComponentStorage::has_component<Collider>(Entity) const;
template bool ComponentStorage::has_component<CircleCollider>(Entity) const;
template bool ComponentStorage::has_component<OBBCollider>(Entity) const;
template bool ComponentStorage::has_component<Lifetime>(Entity) const;
template bool ComponentStorage::has_component<WrapAround>(Entity) const;
template bool ComponentStorage::has_component<DestroyRequest>(Entity) const;
template bool ComponentStorage::has_component<CollidedWith>(Entity) const;
template bool ComponentStorage::has_component<Script>(Entity) const;

template std::vector<Entity> ComponentStorage::entities_with_component<Position>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<Size>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<Color>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<Input>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<Velocity>() const;

template std::vector<Entity> ComponentStorage::entities_with_component<Images>() const;

template std::vector<Entity> ComponentStorage::entities_with_component<Text>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<ScreenPosition>() const;

template std::vector<Entity> ComponentStorage::entities_with_component<Rotation>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<Collider>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<CircleCollider>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<OBBCollider>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<Lifetime>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<WrapAround>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<DestroyRequest>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<CollidedWith>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<Script>() const;

// NEW (Class-110): explicit instantiations for the bullet-hell components.
template void ComponentStorage::add_component<BulletTag>(Entity, const BulletTag&);
template void ComponentStorage::add_component<ShipTag>(Entity, const ShipTag&);
template void ComponentStorage::add_component<EnemyTag>(Entity, const EnemyTag&);
template void ComponentStorage::add_component<Health>(Entity, const Health&);

template std::optional<std::reference_wrapper<BulletTag>> ComponentStorage::get_component<BulletTag>(Entity);
template std::optional<std::reference_wrapper<ShipTag>> ComponentStorage::get_component<ShipTag>(Entity);
template std::optional<std::reference_wrapper<EnemyTag>> ComponentStorage::get_component<EnemyTag>(Entity);
template std::optional<std::reference_wrapper<Health>> ComponentStorage::get_component<Health>(Entity);

template std::optional<std::reference_wrapper<const BulletTag>> ComponentStorage::get_component<BulletTag>(Entity) const;
template std::optional<std::reference_wrapper<const ShipTag>> ComponentStorage::get_component<ShipTag>(Entity) const;
template std::optional<std::reference_wrapper<const EnemyTag>> ComponentStorage::get_component<EnemyTag>(Entity) const;
template std::optional<std::reference_wrapper<const Health>> ComponentStorage::get_component<Health>(Entity) const;

template void ComponentStorage::remove_component<BulletTag>(Entity);
template void ComponentStorage::remove_component<ShipTag>(Entity);
template void ComponentStorage::remove_component<EnemyTag>(Entity);
template void ComponentStorage::remove_component<Health>(Entity);

template bool ComponentStorage::has_component<BulletTag>(Entity) const;
template bool ComponentStorage::has_component<ShipTag>(Entity) const;
template bool ComponentStorage::has_component<EnemyTag>(Entity) const;
template bool ComponentStorage::has_component<Health>(Entity) const;

template std::vector<Entity> ComponentStorage::entities_with_component<BulletTag>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<ShipTag>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<EnemyTag>() const;
template std::vector<Entity> ComponentStorage::entities_with_component<Health>() const;

// NEW (Class-110): IgnoreWorldBounds — opt out of MovementSystem clamping.
template void ComponentStorage::add_component<IgnoreWorldBounds>(Entity, const IgnoreWorldBounds&);
template std::optional<std::reference_wrapper<IgnoreWorldBounds>> ComponentStorage::get_component<IgnoreWorldBounds>(Entity);
template std::optional<std::reference_wrapper<const IgnoreWorldBounds>> ComponentStorage::get_component<IgnoreWorldBounds>(Entity) const;
template void ComponentStorage::remove_component<IgnoreWorldBounds>(Entity);
template bool ComponentStorage::has_component<IgnoreWorldBounds>(Entity) const;
template std::vector<Entity> ComponentStorage::entities_with_component<IgnoreWorldBounds>() const;

// NEW (Class-110): Flash — brief hit flash.
template void ComponentStorage::add_component<Flash>(Entity, const Flash&);
template std::optional<std::reference_wrapper<Flash>> ComponentStorage::get_component<Flash>(Entity);
template std::optional<std::reference_wrapper<const Flash>> ComponentStorage::get_component<Flash>(Entity) const;
template void ComponentStorage::remove_component<Flash>(Entity);
template bool ComponentStorage::has_component<Flash>(Entity) const;
template std::vector<Entity> ComponentStorage::entities_with_component<Flash>() const;

// NEW (Class-110): AudioSource — ported Class-070 audio system.
template void ComponentStorage::add_component<AudioSource>(Entity, const AudioSource&);
template std::optional<std::reference_wrapper<AudioSource>> ComponentStorage::get_component<AudioSource>(Entity);
template std::optional<std::reference_wrapper<const AudioSource>> ComponentStorage::get_component<AudioSource>(Entity) const;
template void ComponentStorage::remove_component<AudioSource>(Entity);
template bool ComponentStorage::has_component<AudioSource>(Entity) const;
template std::vector<Entity> ComponentStorage::entities_with_component<AudioSource>() const;
