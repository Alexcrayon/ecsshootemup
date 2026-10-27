#include "destruction.hpp"
#include "entity_manager.hpp"
#include "component_storage.hpp"

void destroy_marked_entities(EntityManager& em, ComponentStorage& storage) {
    // Get a snapshot (vector copy) of all entities marked for destruction.
    // Iterating a copy is safe even though we modify the maps during the loop.
    auto marked = storage.entities_with_component<DestroyRequest>();

    for (Entity entity : marked) {
        // Remove ALL 24 component types — each call is a no-op if the
        // entity doesn't have that particular component.
        //
        // IMPORTANT: this list must include EVERY component type in
        // components.hpp. There is no compiler check — a type omitted here leaks
        // silently: the entity ID is recycled by EntityManager while the stale
        // component stays in its map forever. That caused a real bug where leaked
        // BulletTags made entities_with_component<BulletTag>() grow without bound,
        // so BulletSpawnSystem's live-bullet cap permanently blocked firing.
        storage.remove_component<Position>(entity);
        storage.remove_component<Size>(entity);
        storage.remove_component<Color>(entity);
        storage.remove_component<Input>(entity);
        storage.remove_component<Velocity>(entity);
        storage.remove_component<Images>(entity);
        storage.remove_component<Text>(entity);
        storage.remove_component<ScreenPosition>(entity);
        storage.remove_component<Rotation>(entity);
        storage.remove_component<Collider>(entity);
        storage.remove_component<CircleCollider>(entity);
        storage.remove_component<OBBCollider>(entity);
        storage.remove_component<Lifetime>(entity);
        storage.remove_component<WrapAround>(entity);
        storage.remove_component<CollidedWith>(entity);
        storage.remove_component<Script>(entity);
        storage.remove_component<DestroyRequest>(entity);

        // add bullet-hell components. 
        storage.remove_component<BulletTag>(entity);
        storage.remove_component<ShipTag>(entity);
        storage.remove_component<EnemyTag>(entity);
        storage.remove_component<Health>(entity);
        storage.remove_component<IgnoreWorldBounds>(entity);
        storage.remove_component<Flash>(entity);
        storage.remove_component<AudioSource>(entity);

        em.destroy_entity(entity);
    }
}
