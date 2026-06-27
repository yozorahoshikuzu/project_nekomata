module;
#include <SDL3/SDL.h>
module nekomata2;
import :core.input.inputmanager;
#include <string.h>

namespace nekomata2::core::input {

auto Input::create() -> std::unique_ptr<Input> {
    debug_assert(g_input == nullptr, "only one Input may live at any given time");
    auto input = std::make_unique<Input>();
    g_input = input.get();
    return input;
}
auto Input::handleNewFrame(SdlWindow& window) -> void {
    memset(m_thisFramePressedKeyCounts, 0, sizeof(m_thisFramePressedKeyCounts));
    memset(m_thisFrameReleasedKeyCounts, 0, sizeof(m_thisFrameReleasedKeyCounts));
    m_keyEventsThisFrame.clear();

    if (m_systemCurrentMouseMode != m_mouseMode) {
        switch (m_mouseMode) {
        case MouseMode::Normal: {
            SDL_SetWindowRelativeMouseMode(window.handle(), false);
            break;
        }
        case MouseMode::Captured: {
            SDL_SetWindowRelativeMouseMode(window.handle(), true);
            break;
        }
        }

        m_systemCurrentMouseMode = m_mouseMode;
    }
}
auto Input::insertInputKeyEvent(InputKeyEvent&& event) -> void {
    m_keyEventsThisFrame.emplace(std::move(event));
}

auto Input::setKeyState(Key x, bool state) -> void {
    u32 code = static_cast<u32>(x);
    m_keyStates[code] = state;

    if (state) {
        m_thisFramePressedKeyCounts[code]++;
    } else {
        m_thisFrameReleasedKeyCounts[code]++;
    }
}
auto Input::setMousePosition(math::Vector2f pos) -> void {
    m_mousePosition = pos;
}
auto Input::setMouseDelta(math::Vector2f delta) -> void {
    m_mouseDelta = delta;
}

auto Input::setMouseMode(MouseMode mode) -> void {
    m_mouseMode = mode;
}

auto Input::isKeyPressed(Key x) const -> bool {
    u32 code = static_cast<u32>(x);
    return m_thisFramePressedKeyCounts[code] > 0;
}

auto Input::isKeyReleased(Key x) const -> bool {
    u32 code = static_cast<u32>(x);
    return m_thisFrameReleasedKeyCounts[code] > 0;
}

auto Input::isKeyDown(Key x) const -> bool {
    u32 code = static_cast<u32>(x);
    return m_keyStates[code];
}

auto Input::mousePosition() const -> math::Vector2f {
    return m_mousePosition;
}
auto Input::mouseDelta() const -> math::Vector2f {
    return m_mouseDelta;
}

} // namespace nekomata2::core::input