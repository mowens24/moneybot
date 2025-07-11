#pragma once

#include <imgui.h>
#include <string>
#include <memory>

namespace moneybot {
namespace gui {

/**
 * @brief Base class for all GUI windows
 * 
 * Provides common functionality for window management, rendering,
 * and integration with the main application systems.
 */
class BaseWindow {
public:
    BaseWindow(const std::string& title, const std::string& icon = "");
    virtual ~BaseWindow() = default;
    
    // Core window interface
    virtual void render() = 0;
    virtual void update() {}
    virtual void onShow() {}
    virtual void onHide() {}
    
    // Window state management
    void show() { 
        if (!is_visible_) {
            is_visible_ = true;
            onShow();
        }
    }
    
    void hide() {
        if (is_visible_) {
            is_visible_ = false;
            onHide();
        }
    }
    
    void toggle() { is_visible_ ? hide() : show(); }
    
    // Getters/Setters
    bool isVisible() const { return is_visible_; }
    void setVisible(bool visible) { is_visible_ = visible; }
    
    const std::string& getTitle() const { return title_; }
    const std::string& getIcon() const { return icon_; }
    
    void setPosition(const ImVec2& pos) { position_ = pos; position_set_ = true; }
    void setSize(const ImVec2& size) { size_ = size; size_set_ = true; }
    void setSizeConstraints(const ImVec2& min_size, const ImVec2& max_size) {
        min_size_ = min_size;
        max_size_ = max_size;
        constraints_set_ = true;
    }
    
    // Window flags
    void setFlags(ImGuiWindowFlags flags) { window_flags_ = flags; }
    ImGuiWindowFlags getFlags() const { return window_flags_; }

protected:
    // Window properties
    std::string title_;
    std::string icon_;
    bool is_visible_ = false;
    
    // Window layout
    ImVec2 position_ = ImVec2(0, 0);
    ImVec2 size_ = ImVec2(400, 300);
    ImVec2 min_size_ = ImVec2(200, 150);
    ImVec2 max_size_ = ImVec2(FLT_MAX, FLT_MAX);
    
    bool position_set_ = false;
    bool size_set_ = false;
    bool constraints_set_ = false;
    
    ImGuiWindowFlags window_flags_ = ImGuiWindowFlags_None;
    
    // Helper methods
    void beginWindow();
    void endWindow();
    
    // Styling helpers
    void pushTheme();
    void popTheme();
    
private:
    static int next_window_id_;
    int window_id_;
};

}} // namespace moneybot::gui
