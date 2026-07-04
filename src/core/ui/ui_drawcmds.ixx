export module projnekomata:core.ui.ui_drawcmds;
import std;
import :core.math;
import :graphics.texturesystem.texture_manager;
import :graphics.fontsystem.font_face;

export namespace projnekomata::ui {

struct UiRectDrawCmd {
    math::Vector2f ndcBegin;
    math::Vector2f ndcEnd;
    math::Vector4f color;
};

struct UiTextureDrawCmd {
    math::Vector2f ndcBegin;
    math::Vector2f ndcEnd;
    math::Vector2f texcoordBegin;
    math::Vector2f texcoordEnd;
    graphics::texturesystem::Texture texture;
};

struct UiTextDrawCmd {
    math::Vector2f baselinePos;
    std::string text;
    graphics::fonts::FontFace face;
    float size;
    math::Vector4f color;
};

using UiDrawCmd = std::variant<UiRectDrawCmd, UiTextureDrawCmd, UiTextDrawCmd>;

}