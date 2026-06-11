export module nekomata2:core.math.raddeg;
import :core.math.consts;

export namespace nekomata2::math {

inline float degreesToRadians(float degrees) {
    return degrees * (consts::PI / 180.0f);
}

}
