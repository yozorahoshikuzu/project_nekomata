export module nekomata2:core.ecs.world.transform;
import :core.math;

using namespace nekomata2::math;

export namespace nekomata2::ecs::components {

class Transform {
public:
    Transform() = default;

    Transform3D m_transform3d;
    Matrix4x4f m_modelMatrix;
    bool m_modelMatrixIsDirty{};

    
};

};