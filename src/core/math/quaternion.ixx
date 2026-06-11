export module nekomata2:core.math.quaternion;
import std;
import :core.math.matrix_types;
import :core.math.consts;

export namespace nekomata2::math {

class Quaternion {
public:
    float m_x, m_y, m_z, m_w;

    Quaternion() = default;
    Quaternion(float x, float y, float z, float w) : m_x(x), m_y(y), m_z(z), m_w(w) {}

    constexpr static auto identity() -> Quaternion {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }

    static auto fromAxisAngle(Vector3f axis, float angleRadians) -> Quaternion {
        auto normAxis = axis.normalize();
        float s = std::sin(0.5f * angleRadians);
        return {normAxis.x() * s, normAxis.y() * s, normAxis.z() * s, std::cos(0.5f * angleRadians)};
    }

    static auto fromEulerAngles(float pitchRadians, float yawRadians, float rollRadians) -> Quaternion {
        float cp = std::cos(0.5f * pitchRadians);
        float sp = std::sin(0.5f * pitchRadians);
        float cy = std::cos(0.5f * yawRadians);
        float sy = std::sin(0.5f * yawRadians);
        float cr = std::cos(0.5f * rollRadians);
        float sr = std::sin(0.5f * rollRadians);
    
        return {
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        };
    }

    static auto fromAngleToVector(Vector3f from, Vector3f to) -> Quaternion {
        auto normFrom = from.normalize();
        auto normTo = to.normalize();

        float d = normFrom.dot(normTo);
        if (d >= 1.0f - consts::EPSILON) return Quaternion::identity(); // if already aligned
        if (d <= -1.0f - consts::EPSILON) {
            Vector3f perpendicular = Vector3f(1.0f, 0.0f, 0.0f).cross(normFrom);
            if (perpendicular.lengthSquared() < consts::EPSILON) perpendicular = Vector3f(0.0f, 1.0f, 0.0f).cross(normFrom);
            return Quaternion::fromAxisAngle(perpendicular, consts::PI);
        }

        Vector3f axis = normFrom.cross(normTo);
        float s = std::sqrt(2.0f * (1.0f + d));
        return Quaternion(axis.x() / s, axis.y() / s, axis.z() / s, 0.5f * s).normalize();
    }

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Core Arithmetic

    auto operator*(const Quaternion& b) const -> Quaternion {
        return {
            m_w*b.m_x + m_x*b.m_w + m_y*b.m_z - m_z*b.m_y,
            m_w*b.m_y - m_x*b.m_z + m_y*b.m_w + m_z*b.m_x,
            m_w*b.m_z + m_x*b.m_y - m_y*b.m_x + m_z*b.m_w,
            m_w*b.m_w - m_x*b.m_x - m_y*b.m_y - m_z*b.m_z
        };
    }
 
    auto operator*=(const Quaternion& b) -> Quaternion& { *this = *this * b; return *this; }

    auto operator*(float s)             const -> Quaternion { return {m_x*s,     m_y*s,     m_z*s,     m_w*s    }; }
    auto operator*=(float s) -> Quaternion& { *this = *this * s; return *this; }
 
    auto operator+(const Quaternion& b) const -> Quaternion { return {m_x+b.m_x, m_y+b.m_y, m_z+b.m_z, m_w+b.m_w}; }
 
    [[nodiscard]] auto dot(const Quaternion& b)  const -> float { return m_x*b.m_x + m_y*b.m_y + m_z*b.m_z + m_w*b.m_w; }
    [[nodiscard]] auto lengthSquared()          const -> float { return dot(*this); }
    [[nodiscard]] auto length()                  const -> float { return std::sqrt(lengthSquared()); }
    [[nodiscard]] auto conjugate()               const -> Quaternion { return {-m_x, -m_y, -m_z, m_w}; }
    [[nodiscard]] auto inverse()                 const -> Quaternion { return conjugate() * (1.f / lengthSquared()); }

    [[nodiscard]] auto normalize() const -> Quaternion {
        const float l = length();
        return l > 1e-8f ? (*this)*(1.0f/l) : Quaternion{};
    }

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Rotation

    [[nodiscard]] auto rotateVector3f(Vector3f vec) const -> Vector3f {
        Vector3f im = Vector3f(m_x, m_y, m_z);
        Vector3f t = 2.0f * im.cross(vec);
        return vec + t * m_w + im.cross(t);
    }

    [[nodiscard]] auto toRotationMatrix() const -> Matrix4x4f {
        auto q = normalize();
        float xx = q.m_x*q.m_x, yy = q.m_y*q.m_y, zz = q.m_z*q.m_z,
              xy = q.m_x*q.m_y, xz = q.m_x*q.m_z, yz = q.m_y*q.m_z,
              wx = q.m_w*q.m_x, wy = q.m_w*q.m_y, wz = q.m_w*q.m_z;

        auto m = Matrix4x4f::identity();
        m[0, 0] = 1.0f - 2.0f * (yy + zz);
        m[0, 1] =        2.0f * (xy - wz);
        m[0, 2] =        2.0f * (xz + wy);

        m[1, 0] =        2.0f * (xy + wz);
        m[1, 1] = 1.0f - 2.0f * (xx + zz);
        m[1, 2] =        2.0f * (yz - wx);

        m[2, 0] =        2.0f * (xz - wy);
        m[2, 1] =        2.0f * (yz + wx);
        m[2, 2] = 1.0f - 2.0f * (xx + yy);
        return m;
    }

    // --------------------------------------------------------------------------------------------------------------------------------------------------------
    // Interpolation

    /// Performs linear interpolation between this quaternion and the given quaternion.
    ///
    /// It is straightforward, very fast, and depending on your use case it might be just good enough; however, it does not provide a constant angular velocity
    /// given a linear `t`, and it might not interpolate across the shortest arc on the quaternion sphere between the two.
    [[nodiscard]] auto linearInterpTo(Quaternion other, float t) const -> Quaternion {
        if (dot(other) < 0.f) other *= -1.0f;
        return (*this * (1.0f - t) + other * t).normalize();
    }

    /// Performs spherical interpolation between this quaternion and the given quaternion.
    ///
    /// This interpolation is geometrically correct; it follows the shortest arc on the quaternion sphere, interpolates smoothly, but is more expensive to
    /// compute (in contrast to `LinearInterpTo`).
    [[nodiscard]] auto sphericalInterpTo(Quaternion other, float t) const -> Quaternion {
        float cosTheta = dot(other);

        if (cosTheta < 0.0f) { other *= -1.0f; cosTheta *= -1.0f; }
        if (cosTheta > 1.0f - consts::EPSILON) return linearInterpTo(other, t);

        float theta = std::acos(cosTheta);
        float sinTheta = std::sin(theta);
        float wa = std::sin((1.0f - t) * theta) / sinTheta;
        float wb = std::sin(         t * theta) / sinTheta;
        return (*this * wa + other * wb).normalize();
    }
};

}