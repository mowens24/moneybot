#include "gui/base_window.h"

namespace moneybot {
namespace gui {

BaseWindow::BaseWindow(const std::string& title, const std::string& icon)
    : title_(title)
    , icon_(icon)
    , is_visible_(false)
    , position_(ImVec2(0, 0))
    , size_(ImVec2(400, 300))
    , min_size_(ImVec2(200, 150))
    , window_id_(next_window_id_++) {
}

// Initialize static member
int BaseWindow::next_window_id_ = 1;

} // namespace gui
} // namespace moneybot
