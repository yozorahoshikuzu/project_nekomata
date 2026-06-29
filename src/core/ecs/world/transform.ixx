export module projnekomata:core.ecs.world.transform;
import :core.math;

using namespace projnekomata::math;

export namespace projnekomata::ecs::components {

class Transform {
public:
    Transform() = default;

    Transform3D m_transform3d;
    Matrix4x4f m_modelMatrix;
    bool m_modelMatrixIsDirty{};

    
};

};