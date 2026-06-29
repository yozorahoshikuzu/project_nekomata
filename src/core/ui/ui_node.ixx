export module projnekomata:core.ui.ui_node;
import :core.math;
import :core.log;
import :core.ui.components.ui_rect;
import :core.ui.components.ui_text;
import :core.ui.components.ui_texture;
import :core.overloaded;
import :core.ui.ui_drawcmds;

export namespace projnekomata::ui {

inline float unormToNdc(float x) {
    return x * 2.0f - 1.0f;
}

inline math::Vector2f unormToNdc(math::Vector2f x) {
    return x * 2.0f - math::Vector2f(1.0f);
}

struct ExtentPx { float pixels; };
struct ExtentPercent { float percent; };
using Extent = std::variant<ExtentPx, ExtentPercent>;

constexpr auto resolveExtent(Extent e, float extentBounds) -> float {
    return match(e,
        [](const ExtentPx& px) { return px.pixels; },
        [&](const ExtentPercent& percent) { return (percent.percent / 100.f) * extentBounds; }
    );
}

constexpr auto resolveExtent2D(Extent x, Extent y, math::Vector2f extentBounds) -> math::Vector2f {
    return math::Vector2f(resolveExtent(x, extentBounds.x()), resolveExtent(y, extentBounds.y()));
}

using UiElement = std::variant<std::monostate, UiRect, UiText, UiTexture>;

class UiNodeBuilder;

struct UiNode {
    constexpr static auto builder() -> UiNodeBuilder;
    static auto create() -> std::unique_ptr<UiNode> {
        return std::make_unique<UiNode>();
    }
    Extent posX;
    Extent posY;
    Extent extentX;
    Extent extentY;

    bool visible = true;

    UiElement element = std::monostate{};

    Vec<std::unique_ptr<UiNode>> children;
    UiNode* parent = nullptr;

    UiNode& addChild(std::unique_ptr<UiNode>&& child) {
        child->parent = this;
        children.emplace(std::move(child));
        return *children.last();
    }

    void buildDrawCmds(Vec<UiDrawCmd>& list, math::Vector2f screenLogicalSize, math::Vector2f parentPosition, math::Vector2f parentExtent) {
        if (!visible)
            return;

        auto position = resolveExtent2D(this->posX, this->posY, parentExtent) + parentPosition;
        auto extent = resolveExtent2D(this->extentX, this->extentY, parentExtent);
        auto endPos = position + extent;

        match(element,
            [&](const UiRect& rect) {
                auto drawCmd = UiRectDrawCmd{
                    .ndcBegin = unormToNdc(position.componentWiseDivide(screenLogicalSize)),
                    .ndcEnd   = unormToNdc(endPos.componentWiseDivide(screenLogicalSize)),
                    .color    = rect.color
                };
                list.emplace(drawCmd);
            },
            [&](const UiTexture& texture) {
                auto drawCmd = UiTextureDrawCmd{
                    .ndcBegin      = unormToNdc(position.componentWiseDivide(screenLogicalSize)),
                    .ndcEnd        = unormToNdc(endPos.componentWiseDivide(screenLogicalSize)),
                    .texcoordBegin = texture.texcoordStart,
                    .texcoordEnd = texture.texcoordEnd,
                    .texture = texture.texture
                };
                list.emplace(drawCmd);
            },
            [&](const UiText& text) {
                if (text.text.empty()) return;
                auto drawCmd = UiTextDrawCmd{
                    .baselinePos = position,
                    .text = text.text,
                    .size = text.size
                };
                list.emplace(drawCmd);
            },
            [](const std::monostate&) {}
        );

        for (auto& child : children)
            child->buildDrawCmds(list, screenLogicalSize, position, extent);
    }
};

class UiNodeBuilder {
public:
    constexpr UiNodeBuilder() = default;

    constexpr auto positionX(float pixels) -> UiNodeBuilder& { m_posX = ExtentPx{pixels}; return *this; }
    constexpr auto positionY(float pixels) -> UiNodeBuilder& { m_posY = ExtentPx{pixels}; return *this; }
    constexpr auto position(math::Vector2f pos) -> UiNodeBuilder& { m_posX = ExtentPx{pos.x()}; m_posY = ExtentPx{pos.y()}; return *this; }
    constexpr auto positionPercX(float percent) -> UiNodeBuilder& { m_posX = ExtentPercent{percent}; return *this; }
    constexpr auto positionPercY(float percent) -> UiNodeBuilder& { m_posY = ExtentPercent{percent}; return *this; }
    constexpr auto positionPercent(math::Vector2f percent) -> UiNodeBuilder& { m_posX = ExtentPercent{percent.x()}; m_posY = ExtentPercent{percent.y()}; return *this; }

    constexpr auto extentX(float pixels) -> UiNodeBuilder& { m_extX = ExtentPx{pixels}; return *this; }
    constexpr auto extentY(float pixels) -> UiNodeBuilder& { m_extY = ExtentPx{pixels}; return *this; }
    constexpr auto extent(math::Vector2f extent) -> UiNodeBuilder& { m_extX = ExtentPx{extent.x()}; m_extY = ExtentPx{extent.y()}; return *this; }
    constexpr auto extentPercentX(float percent) -> UiNodeBuilder& { m_extX = ExtentPercent{percent}; return *this; }
    constexpr auto extentPercentY(float percent) -> UiNodeBuilder& { m_extY = ExtentPercent{percent}; return *this; }
    constexpr auto extentPercent(math::Vector2f percent) -> UiNodeBuilder& { m_extX = ExtentPercent{percent.x()}; m_extY = ExtentPercent{percent.y()}; return *this; }

    constexpr auto visible(bool visible) -> UiNodeBuilder& { m_visible = visible; return *this; }
    template <typename... Args> constexpr auto rect(Args&&... args) -> UiNodeBuilder& { m_element = UiRect{std::forward<Args>(args)...}; return *this; }
    template <typename... Args> constexpr auto text(Args&&... args) -> UiNodeBuilder& { m_element = UiText{std::forward<Args>(args)...}; return *this; }
    template <typename... Args> constexpr auto texture(Args&&... args) -> UiNodeBuilder& { m_element = UiTexture{std::forward<Args>(args)...}; return *this; }
    template <typename... Ts> requires ((std::same_as<std::remove_cvref_t<Ts>, std::unique_ptr<UiNode>> && ...))
    constexpr auto children(Ts&&... children) -> UiNodeBuilder& {
        (m_children.emplace(std::forward<Ts>(children)), ...);
        return *this;
    }

    constexpr auto build() -> std::unique_ptr<UiNode> {
        auto node = UiNode::create();
        node->posX = m_posX;
        node->posY = m_posY;
        node->extentX = m_extX;
        node->extentY = m_extY;
        node->visible = m_visible;
        node->element = m_element;
        node->children = std::move(m_children);
        return node;
    }

private:
    Extent m_posX     = ExtentPx{0.f};
    Extent m_posY     = ExtentPx{0.f};
    Extent m_extX     = ExtentPercent{100.f};
    Extent m_extY     = ExtentPercent{100.f};
    bool m_visible    = true;


    UiElement m_element = std::monostate{};

    Vec<std::unique_ptr<UiNode>> m_children = Vec<std::unique_ptr<UiNode>>::create();
};
constexpr auto UiNode::builder() -> UiNodeBuilder { return UiNodeBuilder(); }

}