#pragma once
#include "../IView.h"
#include "../../../include/LGFX_Config.h"

class DisplayManager;

enum class MenuItem
{
    DRIVER_STANDINGS = 0,
    CONSTRUCTOR_STANDINGS,
    NEWS,
    DRIVERS,
    CIRCUITS,
    CALENDAR,
    SETTINGS,
    COUNT // This will now automatically be 7
};

class MenuView : public IView
{
public:
    MenuView(LGFX *tft, DisplayManager *dm);
    void onEnter() override;
    void onExit() override;
    void render() override;
    void onTurnRight() override;
    void onTurnLeft() override;
    void onPress() override;
    void onLongPress() override;
    void onDoublePress() override;
    void tick() override;

    static constexpr int TEXT_RADIUS = 110;
    static constexpr int SAFE_W = 415;

private:
    LGFX *_tft;
    DisplayManager *_dm;

    static constexpr int ITEM_COUNT = static_cast<int>(MenuItem::COUNT);
    static constexpr float ITEM_STEP = 360.0f / ITEM_COUNT;

    float _faceAngle;
    int _activeIndex;
    bool _isMoving;

    struct ItemRect
    {
        int x, y, w, h;
        bool valid;
        ItemRect() : x(0), y(0), w(0), h(0), valid(false) {}
        void set(int nx, int ny, int nw, int nh)
        {
            x = nx;
            y = ny;
            w = nw;
            h = nh;
            valid = true;
        }
    };

    ItemRect _prevRects[ITEM_COUNT];
    ItemRect _prevLockIn;

    const char *_label(int index) const;
    float _targetAngle(int index) const;
    void _itemPos(int i, float faceAngle, int &x, int &y) const;
    uint32_t _lerpCol(uint32_t a, uint32_t b, float t) const;
    void _erasePrev();
    void _drawItems(float faceAngle);
    void _drawLockIn(int index);
    void _eraseLockIn();
};