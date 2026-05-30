#pragma once
#include "core/math/matrix_types.hpp"
#include "graphics/texturesystem/texture_manager.hpp"

#include <variant>

namespace nekomata2::ui {

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

using UiDrawCmd = std::variant<UiRectDrawCmd, UiTextureDrawCmd>;

}