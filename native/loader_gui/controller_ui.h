#pragma once

#include "controller_model.h"

#include <windows.h>


#include <objidl.h>
#include <gdiplus.h>

#include <memory>
#include <string>
#include <chrono>
#include <vector>
#include <unordered_map>

class ControllerUi {
public:
    struct SettingsItem {
        const wchar_t* label;
        const wchar_t* description; 
        bool value;
        int focus; 
        bool advanced;
    };

    struct SettingsSection {
        const wchar_t* title;
        std::vector<SettingsItem> items;
    };

    ControllerUi(HINSTANCE instance, ControllerModel& model);
    ~ControllerUi();

    int run(int showCommand);

private:
    enum class Focus { None, Username, Password, SettingsAutoInject,
        SettingsCosmetics, SettingsBadges, SettingsEmotes, SettingsSprays,
        SettingsJams, SettingsLunarPlus, SettingsDebug, SettingsLanguage };

    static constexpr float GearX = 770.0f, GearY = 14.0f, GearSize = 36.0f;
    
    static constexpr float ContentX = 62.0f;
    static constexpr float ContentW = 700.0f;
    static constexpr float CardPad = 28.0f;
    static constexpr float RowH = 52.0f;
    static constexpr float HeaderBottom = 92.0f;

    static LRESULT CALLBACK windowProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void paint();
    void drawLogin(Gdiplus::Graphics& graphics);
    void drawBrowserAuth(Gdiplus::Graphics& graphics);
    void drawMinecraftSelection(Gdiplus::Graphics& graphics);
    void drawLoading(Gdiplus::Graphics& graphics);
    void drawCachePrompt(Gdiplus::Graphics& graphics);
    void drawLoadingComplete(Gdiplus::Graphics& graphics);
    void drawOutdated(Gdiplus::Graphics& graphics);
    void drawError(Gdiplus::Graphics& graphics);
    void drawSettings(Gdiplus::Graphics& graphics);
    void drawGearButton(Gdiplus::Graphics& graphics);
    void drawSwitch(Gdiplus::Graphics& graphics, float x, float y, float on);
    void drawSelect(Gdiplus::Graphics& graphics, float x, float y, float w,
        float h, const std::wstring& text, float hover);
    void drawIdentity(Gdiplus::Graphics& graphics, float y = 100.0f);
    void drawText(Gdiplus::Graphics& graphics, const std::wstring& text,
        float x, float y, float width, float height, float size,
        Gdiplus::Color color, bool semibold = false,
        Gdiplus::StringAlignment alignment = Gdiplus::StringAlignmentNear);
    void drawHeading(Gdiplus::Graphics& graphics, const std::wstring& text,
        float x, float y, float width, float height, float size,
        Gdiplus::Color color,
        Gdiplus::StringAlignment alignment = Gdiplus::StringAlignmentCenter);
    std::wstring ellipsize(Gdiplus::Graphics& graphics,
        const std::wstring& text, float fontSize, float maxWidth) const;
    void drawRoundedRect(Gdiplus::Graphics& graphics, float x, float y,
        float width, float height, float radius, Gdiplus::Color fill,
        Gdiplus::Color border = Gdiplus::Color(0, 0, 0, 0));
    bool hit(float logicalX, float logicalY, float x, float y,
        float width, float height) const;
    void updateFrame();
    void drawTransitionMasks(Gdiplus::Graphics& graphics);
    bool pointerIn(float x, float y, float width, float height) const;
    std::vector<SettingsSection> settingsSections() const;
    void cycleLanguage();
    void toggleSettingsItem(Focus focus);
    const Gdiplus::FontFamily* uiFont(bool semibold) const;

    HINSTANCE instance_{};
    HWND window_{};
    ControllerModel& model_;
    ULONG_PTR gdiplusToken_{};
    std::unique_ptr<Gdiplus::PrivateFontCollection> fonts_;
    std::unique_ptr<Gdiplus::FontFamily> proximaFamily_;
    std::unique_ptr<Gdiplus::FontFamily> proximaSemiboldFamily_;
    std::unique_ptr<Gdiplus::FontFamily> segoeFamily_;
    std::unique_ptr<Gdiplus::FontFamily> segoeSemiboldFamily_;
    Focus focus_{Focus::None};
    ControllerPage settingsReturnPage_{ControllerPage::Login};
    float settingsScroll_{};
    float settingsMaxScroll_{};
    float scaleX_{1.0f};
    float scaleY_{1.0f};
    float mouseX_{-1.0f};
    float mouseY_{-1.0f};
    bool trackingMouse_{};
    ControllerPage lastPage_{ControllerPage::Login};
    std::chrono::steady_clock::time_point lastFrame_{};
    std::chrono::steady_clock::time_point pageChanged_{};
    double spinnerAccumulator_{};
    int spinnerIndex_{};
    float spinnerAlpha_[4]{1.0f, 0.55f, 0.1f, 0.0f};
    float logoPosition_{};
    float loadingProgress_{0.05f};
    int previousLoadingStage_{};
    
    std::unordered_map<int, float> switchAnim_{}; 
    float gearHover_{};
    float backHover_{};
    std::unordered_map<int, float> rowHoverAnim_{}; 
    std::unordered_map<int, float> rowHoverTarget_{}; 
    float selectHover_{};   
    float selectHoverTarget_{};
    float settingsOpen_{};  
    float settingsScrollTarget_{};
};
