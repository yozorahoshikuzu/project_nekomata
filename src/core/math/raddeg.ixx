export module projnekomata:core.math.raddeg;
import :core.math.consts;

export namespace projnekomata::math {

inline float degreesToRadians(float degrees) {
    return degrees * (consts::PI / 180.0f);
}

}
