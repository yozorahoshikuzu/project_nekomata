export module projnekomata:graphics.fontsystem.font_face;
import :core.platform.int_def;

export namespace projnekomata::graphics::fonts {

struct FontFace {
    u32 handleIndex;

    bool operator==(const FontFace& other) const {
        return handleIndex == other.handleIndex;
    }
};

}