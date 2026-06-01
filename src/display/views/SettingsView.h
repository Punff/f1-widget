#pragma once
#include "ScrollListView.h"
#include <cstdint>

struct SettingsData {
    uint32_t magic;
    uint8_t brightness;
    int8_t utcOffset;
    char favTeamId[32];
};

class SettingsView : public ScrollListView
{
public:
    SettingsView(LGFX *tft, DisplayManager *dm);
    static void loadSettings(SettingsData &s);
    static void saveSettings(const SettingsData &s);
    static void applyFavTeamColor(const SettingsData &s);

protected:
    int dataSize() const override;
    void onEnter() override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;
    void onPress() override;
    void onTurnRight() override;
    void onTurnLeft() override;
    void onLongPress() override;

private:
    enum SettingIdx {
        SET_BRIGHTNESS = 0,
        SET_UTC_OFFSET,
        SET_FAV_TEAM,
        SET_SYSINFO,
        SET_CLEAR_CACHE,
        SET_ABOUT,
        SET_COUNT
    };

    bool _editing;
    SettingsData _settings;

    void modifyValue(int delta);
    void applyBrightness();
};
