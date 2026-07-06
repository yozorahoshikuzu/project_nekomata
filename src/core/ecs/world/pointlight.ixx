export module projnekomata:core.ecs.world.pointlight;
import std;
import :core.math;

using namespace projnekomata::math;

export namespace projnekomata::ecs::components {

struct PointLight {
    PointLight() = default;
    PointLight(Vector3f lightRadiance) : lightRadiance(lightRadiance) {}

    Vector3f lightRadiance;
};

}