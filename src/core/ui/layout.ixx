export module projnekomata:core.ui.layout;
import std;

export namespace projnekomata::ui {

// ---- Absolute Layout ----------------------------------------------------------------------------------------------------------------------------------------

/// The absolute layout is the simplest layout there is; it does not influence the layout computation of the element's children in any way.
/// It is also the hardest to work with at scale.
class AbsoluteLayout {
public:
    AbsoluteLayout() = default;
};

// ---- Stack Layout -------------------------------------------------------------------------------------------------------------------------------------------

enum class StackDirection {
    VerticalTopToBottom,
    // TODO: add support for custom anchors/pivots to make this work
    //VerticalBottomToTop,
    HorizontalLeftToRight,
    //HorizontalRightToLeft,
};

/// The stack layout is a layout that arranges its children in a stack.
class StackLayout {
public:
    StackLayout() = default;
    StackLayout(StackDirection direction, float spacing) : direction(direction), spacing(spacing) {}

    StackDirection direction = StackDirection::VerticalTopToBottom;
    float spacing = 0.0f;
};

using Layout = std::variant<AbsoluteLayout, StackLayout>;

}