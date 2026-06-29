export module projnekomata:core.ui.components.ui_texture;
import :core.math;
import :graphics.texturesystem.texture_manager;

export namespace projnekomata::ui {

struct UiTexture {
    UiTexture() = default;
    explicit UiTexture(graphics::texturesystem::Texture texture) : texture(texture), texcoordStart(math::Vector2f(0.f)), texcoordEnd(math::Vector2f(1.f)) {}
    explicit UiTexture(graphics::texturesystem::Texture texture, math::Vector2f texcoordStart, math::Vector2f texcoordEnd)
        : texture(texture), texcoordStart(texcoordStart), texcoordEnd(texcoordEnd) {}

    graphics::texturesystem::Texture texture;

    math::Vector2f texcoordStart;
    math::Vector2f texcoordEnd;
};

}