export module projnekomata:core.ui.ui_node;
import projnekomata.cs;
import :core.math;
import :core.ui.components.ui_rect;
import :core.ui.components.ui_text;
import :core.ui.components.ui_texture;
import :core.overloaded;
import :core.ui.ui_drawcmds;
import :core.ui.layout;
import :core.ui.element_style;

export namespace projnekomata::ui {
struct UiNode;

inline float unormToNdc(float x) {
    return x * 2.0f - 1.0f;
}

inline math::Vector2f unormToNdc(math::Vector2f x) {
    return x * 2.0f - math::Vector2f(1.0f);
}

struct ExtentPx { float pixels; };
struct ExtentPercent { float percent; };
using Extent = FlatVariant<ExtentPx, ExtentPercent>;

constexpr auto resolveExtent(Extent e, float extentBounds) -> float {
    return match(e,
        [](const ExtentPx& px) { return px.pixels; },
        [&](const ExtentPercent& percent) { return (percent.percent / 100.f) * extentBounds; }
    );
}

constexpr auto resolveExtent2D(Extent x, Extent y, math::Vector2f extentBounds) -> math::Vector2f {
    return math::Vector2f(resolveExtent(x, extentBounds.x()), resolveExtent(y, extentBounds.y()));
}

using UiElement = FlatVariant<std::monostate, UiRect, UiText, UiTexture>;

class UiNodeBuilder;

struct UiMouseHitRegion {
    math::Vector2f position;
    math::Vector2f extent;
    UiNode* ref;

    bool capturesClicks = false;
    bool capturesHover = false;

    Option<std::function_ref<auto(math::Vector2f) -> void>> clickCallback;
    Option<std::function_ref<auto(math::Vector2f) -> void>> hoverCallback;
};

struct UiNode {
    constexpr static auto builder() -> UiNodeBuilder;
    static auto create() -> std::unique_ptr<UiNode> {
        return std::make_unique<UiNode>();
    }

    // ---- Positioning ----------------------------------------------------------------------------------------------------------------------------------------

    Extent posX    = ExtentPx{0.f};
    Extent posY    = ExtentPx{0.f};
    Extent extentX = ExtentPercent{100.f};
    Extent extentY = ExtentPercent{100.f};

    Layout                       childrenLayout = AbsoluteLayout();
    Vec<std::unique_ptr<UiNode>> children;
    UiNode* parent = nullptr;

    // ---- Style ----------------------------------------------------------------------------------------------------------------------------------------------

    bool visible = true;
    ElementStyle style = ElementStyle();

    // ---- Behavior -------------------------------------------------------------------------------------------------------------------------------------------

    UiElement element = std::monostate{};

    bool capturesClicks = false;
    std::function<auto(math::Vector2f) -> void> clickCallback = nullptr;
    bool capturesHover = false;
    std::function<auto(math::Vector2f) -> void> hoverCallback = nullptr;


    UiNode& addChild(std::unique_ptr<UiNode>&& child) {
        child->parent = this;
        children.emplace(std::move(child));
        return *children.last();
    }

    auto buildDrawCmds(Vec<UiDrawCmd>& list, Vec<UiMouseHitRegion>& dstHitregions, math::Vector2f screenLogicalSize, math::Vector2f origin, math::Vector2f bounds,
        UiNode* currentClicked, UiNode* currentHovered, bool styleParentIsClicked, bool styleParentIsHovered) -> math::Vector2f
    {
        if (!visible)
            return math::Vector2f(0.0f);

        bool isPressed = (style.pressedStateInheritsParent && styleParentIsClicked) || (currentClicked == this);
        bool isHovered = (style.hoverStateInheritsParent && styleParentIsHovered) || (currentHovered == this);

        auto position = resolveExtent2D(this->posX, this->posY, bounds) + origin;
        auto extent = resolveExtent2D(this->extentX, this->extentY, bounds);
        auto endPos = position + extent;

        if (capturesClicks || capturesHover) {
            auto optClickCallback = Option<std::function_ref<auto(math::Vector2f) -> void>>::someIf(
                capturesClicks && (clickCallback != nullptr),
                [&] { return std::function_ref<auto(math::Vector2f) -> void>(clickCallback); }
            );
            auto optHoverCallback = Option<std::function_ref<auto(math::Vector2f) -> void>>::someIf(
                capturesHover && (hoverCallback != nullptr),
                [&] { return std::function_ref<auto(math::Vector2f) -> void>(hoverCallback); }
            );

            dstHitregions.emplace(UiMouseHitRegion{
                .position = position,
                .extent = extent,
                .ref = this,
                .capturesClicks = capturesClicks,
                .capturesHover = capturesHover,
                .clickCallback = optClickCallback,
                .hoverCallback = optHoverCallback
            });
        }

        match(element,
            [&](const UiRect& rect) {
                auto drawCmd = UiRectDrawCmd{
                    .ndcBegin = unormToNdc(position.componentWiseDivide(screenLogicalSize)),
                    .ndcEnd   = unormToNdc(endPos.componentWiseDivide(screenLogicalSize)),
                    .color    = style.getEndColor(isHovered, isPressed),
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
                    .face = text.fontFace.clone(),
                    .size = text.size,
                    .color = style.getEndColor(isHovered, isPressed)
                };
                list.emplace(drawCmd);
            },
            [](const std::monostate&) {}
        );

        match(childrenLayout,
            [&](const AbsoluteLayout& layout) {
                for (auto& child : children)
                    child->buildDrawCmds(list, dstHitregions, screenLogicalSize, position, extent, currentClicked, currentHovered, isPressed, isHovered);
            },
            [&](const StackLayout& layout) {
                auto axisCursor = 0.0_f32;
                switch (layout.direction) {
                case StackDirection::VerticalTopToBottom: {
                    for (auto& child : children) {
                        auto childExtent = child->buildDrawCmds(list, dstHitregions, screenLogicalSize, position + math::Vector2f(0.0f, axisCursor), extent, currentClicked, currentHovered, isPressed, isHovered);
                        axisCursor += childExtent.y() + layout.spacing;
                    }
                    break;
                }
                case StackDirection::HorizontalLeftToRight: {
                    for (auto& child : children) {
                        auto childExtent = child->buildDrawCmds(list, dstHitregions, screenLogicalSize, position + math::Vector2f(axisCursor, 0.0f), extent, currentClicked, currentHovered, isPressed, isHovered);
                        axisCursor += childExtent.x() + layout.spacing;
                    }
                }
                }
            }
        );

        return extent;
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
    constexpr auto childrenLayout(Layout layout) -> UiNodeBuilder& { m_childrenLayout = layout; return *this; }
    template <typename... Ts> requires ((std::same_as<std::remove_cvref_t<Ts>, std::unique_ptr<UiNode>> && ...))
    constexpr auto children(Ts&&... children) -> UiNodeBuilder& {
        (m_children.emplace(std::forward<Ts>(children)), ...);
        return *this;
    }

    constexpr auto style(const ElementStyle& style) -> UiNodeBuilder& { m_style = style; return *this; }

    constexpr auto capturesClicks(bool capturesClicks) -> UiNodeBuilder& { m_capturesClicks = capturesClicks; return *this; }
    constexpr auto capturesHover(bool capturesHover) -> UiNodeBuilder& { m_capturesHover = capturesHover; return *this; }
    constexpr auto onClick(std::function<auto(math::Vector2f) -> void>&& callback) -> UiNodeBuilder& { m_clickCallback = std::move(callback); return *this; }
    constexpr auto onHover(std::function<auto(math::Vector2f) -> void>&& callback) -> UiNodeBuilder& { m_hoverCallback = std::move(callback); return *this; }

    constexpr auto build() -> std::unique_ptr<UiNode> {
        auto node = UiNode::create();
        node->posX = m_posX;
        node->posY = m_posY;
        node->extentX = m_extX;
        node->extentY = m_extY;
        node->visible = m_visible;
        node->capturesClicks = m_capturesClicks;
        node->clickCallback = m_clickCallback;
        node->capturesHover = m_capturesHover;
        node->hoverCallback = m_hoverCallback;
        node->element = m_element;
        node->style = m_style;
        node->childrenLayout = std::move(m_childrenLayout);
        node->children = std::move(m_children);
        return node;
    }

private:
    Extent m_posX     = ExtentPx{0.f};
    Extent m_posY     = ExtentPx{0.f};
    Extent m_extX     = ExtentPercent{100.f};
    Extent m_extY     = ExtentPercent{100.f};
    bool m_visible    = true;
    bool m_capturesClicks = false;
    bool m_capturesHover = false;

    std::function<auto(math::Vector2f) -> void> m_clickCallback = nullptr;
    std::function<auto(math::Vector2f) -> void> m_hoverCallback = nullptr;

    ElementStyle m_style = ElementStyle();
    Layout m_childrenLayout = AbsoluteLayout();
    UiElement m_element = std::monostate{};

    Vec<std::unique_ptr<UiNode>> m_children = Vec<std::unique_ptr<UiNode>>::create();
};
constexpr auto UiNode::builder() -> UiNodeBuilder { return UiNodeBuilder(); }

}