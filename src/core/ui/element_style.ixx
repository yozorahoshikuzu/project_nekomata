export module projnekomata:core.ui.element_style;
import :core.math;
import :core.color;

export namespace projnekomata::ui {

class ElementStyleBuilder;
class ElementStyle {
public:
    ElementStyle() = default;
    static constexpr auto builder() -> ElementStyleBuilder;

    /// The default element color
    Color color = Color::fromRgb8Uint(255, 255, 255);
    Option<Color> colorHovered = None;
    Option<Color> colorPressed = None;

    bool hoverStateInheritsParent = true;
    bool pressedStateInheritsParent = true;

    auto getEndColor(bool isHovered, bool isPressed) const -> Color {
        if (isPressed && colorPressed.isSome()) return colorPressed.unwrap();
        if (isHovered && colorHovered.isSome()) return colorHovered.unwrap();
        return color;
    }
};

class ElementStyleBuilder {
public:
    constexpr auto color(Color color) -> ElementStyleBuilder& { m_style.color = color; return *this; }
    constexpr auto colorHovered(Color color) -> ElementStyleBuilder& { m_style.colorHovered = Some(color); return *this; }
    constexpr auto colorPressed(Color color) -> ElementStyleBuilder& { m_style.colorPressed = Some(color); return *this; }
    constexpr auto hoverStateInheritsParent(bool hoverStateInheritsParent) -> ElementStyleBuilder& { m_style.hoverStateInheritsParent = hoverStateInheritsParent; return *this; }
    constexpr auto pressedStateInheritsParent(bool pressedStateInheritsParent) -> ElementStyleBuilder& { m_style.pressedStateInheritsParent = pressedStateInheritsParent; return *this; }

    constexpr auto build() -> ElementStyle { return m_style; }

private:
    constexpr ElementStyleBuilder() = default;
    friend class ElementStyle;

    ElementStyle m_style;
};

constexpr auto ElementStyle::builder() -> ElementStyleBuilder { return ElementStyleBuilder(); }


}