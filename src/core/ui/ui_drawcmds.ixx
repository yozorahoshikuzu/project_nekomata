export module projnekomata:core.ui.ui_drawcmds;
import std;
import :core.math;
import :graphics.texturesystem.texture_manager;
import :graphics.fontsystem.font_face;
import :core.color;

export namespace projnekomata::ui {

struct UiRectDrawCmd {
    math::Vector2f ssBegin;
    math::Vector2f ssEnd;
    Color color;
};

struct UiTextureDrawCmd {
    math::Vector2f ssBegin;
    math::Vector2f ssEnd;
    math::Vector2f texcoordBegin;
    math::Vector2f texcoordEnd;
    graphics::texturesystem::Texture texture;
};

struct UiTextDrawCmd {
    math::Vector2f ssPosition;
    std::string text;
    graphics::fonts::FontFace face;
    float size;
    Color color;
};

using UiDrawCmd = FlatVariant<UiRectDrawCmd, UiTextureDrawCmd, UiTextDrawCmd>;

}