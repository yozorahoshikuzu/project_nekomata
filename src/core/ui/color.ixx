export module projnekomata:core.color;
import :core.math;

// TODO: Move this file somewhere else

export namespace projnekomata {

class Color {
public:
    Color() = default;

    // ---- To Color Conversions -------------------------------------------------------------------------------------------------------------------------------

    constexpr static auto fromRgb8Uint(u8 r, u8 g, u8 b) -> Color {
        f32 rh = static_cast<float>(r) / 255.0_f32;
        f32 gh = static_cast<float>(g) / 255.0_f32;
        f32 bh = static_cast<float>(b) / 255.0_f32;
        return Color{math::Vector4f(rh, gh, bh, 1.0_f32)};
    }

    constexpr static auto fromRgba8Uint(u8 r, u8 g, u8 b, u8 a) -> Color {
        f32 rh = static_cast<float>(r) / 255.0_f32;
        f32 gh = static_cast<float>(g) / 255.0_f32;
        f32 bh = static_cast<float>(b) / 255.0_f32;
        f32 ah = static_cast<float>(a) / 255.0_f32;
        return Color{math::Vector4f(rh, gh, bh, ah)};
    }

    constexpr static auto fromRgb8Hexcode(u32 hexcode) -> Color {
        return fromRgb8Uint(static_cast<u8>((hexcode >> 16) & 0xff), static_cast<u8>((hexcode >> 8) & 0xff), static_cast<u8>(hexcode & 0xff));
    }

    constexpr static auto fromRgba8Hexcode(u32 hexcode) -> Color {
        return fromRgba8Uint(static_cast<u8>((hexcode >> 24) & 0xff), static_cast<u8>((hexcode >> 16) & 0xff), static_cast<u8>((hexcode >> 8) & 0xff), static_cast<u8>(hexcode & 0xff));
    }

    constexpr static auto fromRgb32Float(f32 r, f32 g, f32 b) -> Color {
        return Color{math::Vector4f(r, g, b, 1.0_f32)};
    }

    constexpr static auto fromRgba32Float(f32 r, f32 g, f32 b, f32 a) -> Color {
        return Color{math::Vector4f(r, g, b, a)};
    }

    // ---- Getters --------------------------------------------------------------------------------------------------------------------------------------------

    [[nodiscard]] constexpr auto asRgba32Float() const -> math::Vector4f { return m_color; }

private:
    constexpr explicit Color(math::Vector4f color) : m_color(color) {}

    math::Vector4f m_color = math::Vector4f(1.0_f32);
};

}