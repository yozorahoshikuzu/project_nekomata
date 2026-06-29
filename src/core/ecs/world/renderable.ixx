export module projnekomata:core.ecs.world.renderable;
import :graphics.texturesystem.texture_manager;
import :graphics.meshsystem.mesh_asset_storage;

export namespace projnekomata::ecs::components {

struct Renderable {
    meshsystem::MeshAsset meshAsset;
    graphics::texturesystem::Texture texture;

    Renderable() = default;
    Renderable(meshsystem::MeshAsset meshAsset, graphics::texturesystem::Texture texture) : meshAsset(meshAsset), texture(texture) {}
};

}