export module projnekomata:core.ecs.world.renderable;
import :graphics.texturesystem.texture_manager;
import :graphics.meshsystem.mesh_asset_storage;
import :graphics.materialsystem.mat_manager;

export namespace projnekomata::ecs::components {

struct Renderable {
    meshsystem::MeshAsset meshAsset;
    Material material;

    Renderable() = default;
    Renderable(meshsystem::MeshAsset meshAsset, Material material) : meshAsset(meshAsset), material(material) {}
};

}
