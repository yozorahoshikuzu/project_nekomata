#pragma once

namespace nekomata2 {

template <class... Arms>
struct overloaded : Arms... {
    using Arms::operator()...;
};

} // namespace nekomata2