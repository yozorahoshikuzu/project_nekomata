export module projnekomata:core.ecs.world.transform;
import :core.math;

using namespace projnekomata::math;

export namespace projnekomata::ecs::components {

class Transform {
public:
    Transform() = default;
    Transform(math::Vector3f position, math::Quaternion rotation, math::Vector3f scale) : m_transform3d(position, rotation, scale) {}

    Transform3D m_transform3d;
    Matrix4x4f m_modelMatrix;
    bool m_modelMatrixIsDirty{};

    
};

}