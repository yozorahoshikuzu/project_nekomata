import std;
import nekomata2;
#include <string.h>

using namespace nekomata2::math;
using namespace nekomata2::core::input;

class MovingScript : public nekomata2::ecs::ScriptBase {
public:
    MovingScript(float time, float spinRadius, float spinThetaSpeed, float spinPhiSpeed, float spinInitialTheta, float spinInitialPhi, float rotationConstX, float rotationConstY) :
        m_time(time), m_spinRadius(spinRadius), m_spinThetaSpeed(spinThetaSpeed), m_spinPhiSpeed(spinPhiSpeed), m_spinInitialTheta(spinInitialTheta), m_spinInitialPhi(spinInitialPhi), m_rotationConstX(rotationConstX), m_rotationConstY(rotationConstY) {}

    void onCreate() override {
        m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity)
            .m_transform3d = Transform3D::identity();
    }
    void onDestroy() override {}
    void onUpdate(float dt) override {
        m_time += dt;

        m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity).m_transform3d.m_position =
            Vector3f(
                m_spinRadius * std::cos(m_spinInitialTheta + m_spinThetaSpeed * m_time),
                m_spinRadius * std::sin(m_spinInitialTheta + m_spinThetaSpeed * m_time),
                -100.0f * std::cos(m_spinInitialPhi + m_spinPhiSpeed * m_time)
            );
        m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity)
            .m_transform3d.m_rotation = Quaternion::fromEulerAngles(0.8f * m_time * m_rotationConstX, 0.8f * m_time * m_rotationConstY, 0.4f * m_time * m_rotationConstX);
    }

    float m_time;

    float m_spinRadius;
    float m_spinThetaSpeed;
    float m_spinPhiSpeed;
    float m_spinInitialTheta;
    float m_spinInitialPhi;
    float m_rotationConstX;
    float m_rotationConstY;
};

class CameraScript : public nekomata2::ecs::ScriptBase {
public:
    CameraScript(nekomata2::graphics::fonts::FontFace face) : m_fontFace(face) {}

    void onCreate() override {
        m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity)
            .m_transform3d = Transform3D::identity();
        m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity)
            .m_transform3d.m_position = { 2.0f, 1.0f, 1.0f };

        auto posText = nekomata2::ui::UiNode::create();
        posText->position = { 10.0f, 220.0f };
        posText->extent = { 100.0f, 100.0f };
        posText->element = nekomata2::ui::UiText{"hai :3", 18.0f, std::move(m_fontFace)};
        m_text = posText.get();
        nekomata2::ui::UiSystem::get().getRoot().addChild(std::move(posText));
    }

    void onDestroy() override {}

    void onUpdate(float dt) override {
        if (m_handleMouseMovement) {
            auto mousedelta = Input::get().mouseDelta();
            m_rotationYaw += mousedelta.x() * 0.1f;
            m_rotationPitch -= mousedelta.y() * 0.1f;

            m_rotationYaw = std::fmod(m_rotationYaw, 360.0f);
            if (m_rotationYaw < 0.0f) m_rotationYaw += 360.0f;
            m_rotationPitch = std::clamp(m_rotationPitch, -90.0f, 90.0f);


            auto yawQuat = Quaternion::fromAxisAngle(Vector3f(0.0f, 1.0f, 0.0f), degreesToRadians(m_rotationYaw));
            auto pitchQuat = Quaternion::fromAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), degreesToRadians(m_rotationPitch));

            m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity).m_transform3d.m_rotation = yawQuat * pitchQuat;
        }

        float forwardVel = 0.0f;
        float sidewaysVel = 0.0f;
        float upVel = 0.0f;

        if (Input::get().isKeyDown(Key::W)) forwardVel -= 1.0f;
        if (Input::get().isKeyDown(Key::S)) forwardVel += 1.0f;
        if (Input::get().isKeyDown(Key::A)) sidewaysVel += 1.0f;
        if (Input::get().isKeyDown(Key::D)) sidewaysVel -= 1.0f;
        if (Input::get().isKeyDown(Key::Space)) upVel += 1.0f;
        if (Input::get().isKeyDown(Key::C)) upVel -= 1.0f;


        auto dp = Vector3f(sidewaysVel, upVel, forwardVel);

        if (dp != Vector3f(0.0f)) {
            auto factor = 5.0f;
            if (Input::get().isKeyDown(Key::LShift)) factor = 25.0f;

            auto rotation = m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity).m_transform3d.m_rotation;
            auto delta = dp.normalize() * dt * factor;
            delta = rotation.rotateVector3f(delta);

            m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity).m_transform3d.m_position += delta;
        }

        if (Input::get().isKeyDown(Key::Escape)) {
            Input::get().setMouseMode(MouseMode::Normal);
            m_handleMouseMovement = false;
        }

        if (Input::get().isKeyDown(Key::MouseLeft)) {
            Input::get().setMouseMode(MouseMode::Captured);
            m_handleMouseMovement = true;
        }

        auto camPos = m_workingWorld->get<nekomata2::ecs::components::Transform>(m_workingEntity).m_transform3d.m_position;
        std::get<nekomata2::ui::UiText>(m_text->element).text = std::format("pos: {:.2f}, {:.2f}, {:.2f}", camPos.x(), camPos.y(), camPos.z());
    }

    bool m_handleMouseMovement = true;
    float m_rotationPitch = 0.0f;
    float m_rotationYaw = 0.0f;

    nekomata2::graphics::fonts::FontFace m_fontFace;
    nekomata2::ui::UiNode* m_text = nullptr;
};


struct Vertex {
    Vector3f position;
    float uvX;
    Vector3f normal;
    float uvY;
    Vector4f color;
};
std::pair<std::vector<Vertex>, std::vector<u32>> generateSphere(u32 latSegments, u32 lonSegments, float radius) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    for (u32 lat = 0; lat <= latSegments; lat++) {
        float theta = static_cast<float>(lat) / static_cast<float>(latSegments) * consts::PI;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (u32 lon = 0; lon <= lonSegments; lon++) {
            float phi = static_cast<float>(lon) / static_cast<float>(lonSegments) * consts::PI * 2;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            vertices.push_back({Vector3f(x * radius, y * radius, z * radius), static_cast<float>(lon) / static_cast<float>(lonSegments),
                                Vector3f(x, y, z), static_cast<float>(lat) / static_cast<float>(latSegments), Vector4f(1.0f, 0.0f, 0.0f, 1.0f)});
        }
    }

    for (u32 lat = 0; lat < latSegments; lat++) {
        for (u32 lon = 0; lon < lonSegments; lon++) {
            u32 first = (lat * (lonSegments + 1)) + lon;
            u32 second = first + lonSegments + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
        }
    }

    return {std::move(vertices), std::move(indices)};
}


void onGameInit(std::unique_ptr<nekomata2::ecs::World>& world) {
    auto& ts = nekomata2::graphics::texturesystem::TextureManager::get();

    auto samplerSettings = nekomata2::graphics::texturesystem::SamplerParams::defaultValues()
        .setAnisotropy(16.0f);

    nekomata2::graphics::texturesystem::Texture ts1 = ts.loadKtx2TextureAsync("../Assets/abstractart.ktx2", samplerSettings);
    auto fnt = nekomata2::graphics::fonts::FontManager::get().loadFont("/usr/share/fonts/noto/NotoSans-Regular.ttf");


    auto& mas = nekomata2::meshsystem::MeshAssetStorage::get();
    auto mesh = mas.allocateMeshAsset();
    mas.getLodList(mesh).maxLodIndex = 3; // set all LOD levels used
    // initially bestLodIndex = ~0 => no LODs are ready (rendering thread will skip rendering).

    // LOD 3
    auto [l3verts, l3inds] = generateSphere(10, 10, 1.0f);
    mas.perpareLodSpace(mesh, 3, l3verts.size() * sizeof(Vertex), l3inds.size() * sizeof(u32), alignof(Vertex), 4);
    memcpy(mas.getLodList(mesh).lods[3].meshSuballocation.vertexBuffer.hostAddress, l3verts.data(), l3verts.size() * sizeof(Vertex));
    memcpy(mas.getLodList(mesh).lods[3].meshSuballocation.indexBuffer.hostAddress, l3inds.data(), l3inds.size() * sizeof(u32));
    mas.getLodList(mesh).bestLodIndex.store(3, std::memory_order_release);
    mas.getLodList(mesh).lods[3].screenSizeThreshold = 8.0f;

    // LOD 2
    auto [l2verts, l2inds] = generateSphere(16, 16, 1.0f);
    mas.perpareLodSpace(mesh, 2, l2verts.size() * sizeof(Vertex), l2inds.size() * sizeof(u32), alignof(Vertex), 4);
    memcpy(mas.getLodList(mesh).lods[2].meshSuballocation.vertexBuffer.hostAddress, l2verts.data(), l2verts.size() * sizeof(Vertex));
    memcpy(mas.getLodList(mesh).lods[2].meshSuballocation.indexBuffer.hostAddress, l2inds.data(), l2inds.size() * sizeof(u32));
    mas.getLodList(mesh).bestLodIndex.store(2, std::memory_order_release);
    mas.getLodList(mesh).lods[2].screenSizeThreshold = 20.0f;

    // LOD 1
    auto [l1verts, l1inds] = generateSphere(30, 30, 1.0f);
    mas.perpareLodSpace(mesh, 1, l1verts.size() * sizeof(Vertex), l1inds.size() * sizeof(u32), alignof(Vertex), 4);
    memcpy(mas.getLodList(mesh).lods[1].meshSuballocation.vertexBuffer.hostAddress, l1verts.data(), l1verts.size() * sizeof(Vertex));
    memcpy(mas.getLodList(mesh).lods[1].meshSuballocation.indexBuffer.hostAddress, l1inds.data(), l1inds.size() * sizeof(u32));
    mas.getLodList(mesh).bestLodIndex.store(1, std::memory_order_release);
    mas.getLodList(mesh).lods[1].screenSizeThreshold = 80.0f;

    // LOD 0
    auto [l0verts, l0inds] = generateSphere(100, 100, 1.0f);
    mas.perpareLodSpace(mesh, 0, l0verts.size() * sizeof(Vertex), l0inds.size() * sizeof(u32), alignof(Vertex), 4);
    memcpy(mas.getLodList(mesh).lods[0].meshSuballocation.vertexBuffer.hostAddress, l0verts.data(), l0verts.size() * sizeof(Vertex));
    memcpy(mas.getLodList(mesh).lods[0].meshSuballocation.indexBuffer.hostAddress, l0inds.data(), l0inds.size() * sizeof(u32));
    mas.getLodList(mesh).bestLodIndex.store(0, std::memory_order_release);
    mas.getLodList(mesh).lods[0].screenSizeThreshold = 200.0f;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> radiusDist(3.0f, 120.0f);
    std::uniform_real_distribution<float> thetaDist(0.0f, 2.0f * consts::PI);
    std::uniform_real_distribution<float> phiDist(0.1f, consts::PI - 1.0f);
    std::uniform_real_distribution<float> thetaSpeedDist(0.04f, 0.16f);
    std::uniform_real_distribution<float> phiSpeedDist(0.01f, 0.07f);
    std::uniform_real_distribution<float> rotationConstDist(0.05f, 0.15f);
    for (usize i = 0; i < 2000; i++) {
        auto ent = world->createEntity();
        world->emplace<nekomata2::ecs::components::Transform>(ent);
        world->emplace<nekomata2::ecs::components::Renderable>(ent, mesh, ts1);
        world->addScript<MovingScript>(ent, 0.0f, radiusDist(gen), thetaSpeedDist(gen), phiSpeedDist(gen), thetaDist(gen), phiDist(gen), rotationConstDist(gen), rotationConstDist(gen));
    }

    auto cameraEnt = world->createEntity();
    world->emplace<nekomata2::ecs::components::Camera>(cameraEnt, 0.01f, 1000.0f, 60.0f, true);
    world->emplace<nekomata2::ecs::components::Transform>(cameraEnt);
    world->addScript<CameraScript>(cameraEnt, fnt);

    Input::get().setMouseMode(MouseMode::Captured);
}

int main(int argc, char* argv[]) {
    nekomata2::log::info("haii :3");

    nekomata2::entry(onGameInit);

    return 0;
}

