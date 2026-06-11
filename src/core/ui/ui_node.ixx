export module nekomata2:core.ui.ui_node;
import :core.math;
import :core.log;
import :core.ui.components.ui_rect;
import :core.ui.components.ui_text;
import :core.ui.components.ui_texture;
import :core.overloaded;
import :core.ui.ui_drawcmds;

export namespace nekomata2::ui {

inline float unormToNdc(float x) {
    return x * 2.0f - 1.0f;
}

inline math::Vector2f unormToNdc(math::Vector2f x) {
    return x * 2.0f - math::Vector2f(1.0f);
}

using UiElement = std::variant<UiRect, UiText, UiTexture>;

struct UiNode {
    static auto create() -> std::unique_ptr<UiNode> {
        return std::make_unique<UiNode>();
    }
    math::Vector2f position;
    math::Vector2f extent;

    bool visible = true;

    UiElement element = UiRect();

    std::vector<std::unique_ptr<UiNode>> children;
    UiNode* parent = nullptr;

    UiNode& addChild(std::unique_ptr<UiNode>&& child) {
        child->parent = this;
        children.emplace_back(std::move(child));
        return *children.back();
    }

    void buildDrawCmds(std::vector<UiDrawCmd>& list, math::Vector2f screenLogicalSize, math::Vector2f parentPosition, math::Vector2f parentExtent) {
        if (!visible)
            return;

        auto position = this->position + parentPosition;
        auto endPos = position + this->extent;

        std::visit(overloaded{
            [&](const UiRect& rect) {
                auto drawCmd = UiRectDrawCmd{
                    .ndcBegin = unormToNdc(position.componentWiseDivide(screenLogicalSize)),
                    .ndcEnd   = unormToNdc(endPos.componentWiseDivide(screenLogicalSize)),
                    .color    = rect.color
                };
                list.emplace_back(drawCmd);
            },
            [&](const UiTexture& texture) {
                auto drawCmd = UiTextureDrawCmd{
                    .ndcBegin      = unormToNdc(position.componentWiseDivide(screenLogicalSize)),
                    .ndcEnd        = unormToNdc(endPos.componentWiseDivide(screenLogicalSize)),
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
            child->buildDrawCmds(list, screenLogicalSize, this->position, this->extent);
    }
};

}