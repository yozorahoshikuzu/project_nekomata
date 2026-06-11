export module nekomata2.graphics.fontsystem.font_face;
import nekomata2.core.platform.int_def;

export namespace nekomata2::graphics::fonts {

struct FontFace {
    u32 handleIndex;

    bool operator==(const FontFace& other) const {
        return handleIndex == other.handleIndex;
    }
};

}