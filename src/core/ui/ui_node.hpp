#pragma once
#include "components/ui_rect.hpp"
#include "components/ui_text.hpp"
#include "components/ui_texture.hpp"
#include "ui_drawcmds.hpp"

#include "core/math/matrix_types.hpp"
#include "core/overloaded/overloaded.hpp"

#include <variant>

namespace nekomata2::ui {

inline float unormToNdc(float x) {
    return x * 2.0f - 1.0f;
}

inline Vector2f unormToNdc(Vector2f x) {
    return x * 2.0f - Vector2f(1.0f);
}

using UiElement = std::variant<UiRect, UiText, UiTexture>;

struct UiNode {
    static auto create() -> std::unique_ptr<UiNode> {
        return std::make_unique<UiNode>();
    }
    math::Vector2f boundsBegin;
    math::Vector2f boundsEnd;

    bool visible = true;

    UiElement element = UiRect();

    std::vector<std::unique_ptr<UiNode>> children;
    UiNode* parent = nullptr;

    UiNode& addChild(std::unique_ptr<UiNode>&& child) {
        child->parent = this;
        children.emplace_back(std::move(child));
        return *children.back();
    }

    void buildDrawCmds(std::vector<UiDrawCmd>& list, Vector2f screenLogicalSize, Vector2f parentBoundsBegin, Vector2f parentBoundsEnd) {
        if (!visible)
            return;

        std::visit(overloaded{
            [&](const UiRect& rect) {
                auto drawCmd = UiRectDrawCmd{
                    .ndcBegin = unormToNdc(boundsBegin.componentWiseDivide(screenLogicalSize)),
                    .ndcEnd   = unormToNdc(boundsEnd.componentWiseDivide(screenLogicalSize)),
                    .color    = rect.color
                };
                list.emplace_back(drawCmd);
            },
            [&](const UiTexture& texture) {
                auto drawCmd = UiTextureDrawCmd{
                    .ndcBegin      = unormToNdc(boundsBegin.componentWiseDivide(screenLogicalSize)),
                    .ndcEnd        = unormToNdc(boundsEnd.componentWiseDivide(screenLogicalSize)),
                    .texcoordBegin = texture.texcoordStart,
                    .texcoordEnd = texture.texcoordEnd,
                    .texture = texture.texture
                };
                list.emplace_back(drawCmd);
            },
            [&](const UiText& text) {
                log::warn("Text not implemented");
            }
        }, element);

        for (auto& child : children)
            child->buildDrawCmds(list, screenLogicalSize, boundsBegin, boundsEnd);
    }
};

}