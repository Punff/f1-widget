#pragma once
#include "../IView.h"
#include "UI.h"
#include "../../data/DataCache.h"
#include "../../../include/LGFX_Config.h"

class DisplayManager;

class NewsView : public IView
{
public:
    NewsView(LGFX *tft, DisplayManager *dm);

    void onEnter() override;
    void onExit() override {}
    void render() override;
    void tick() override {}

    void onTurnRight() override;
    void onTurnLeft() override;
    void onPress() override;
    void onLongPress() override;
    void onDoublePress() override {}

private:
    LGFX *_tft;
    DisplayManager *_dm;
    int _current;
    bool _showOverlay;

    void fetchNews();
    void showLoading();
    void renderArticle();
    void renderNormal(const NewsArticle &a);
    void renderOverlay(const NewsArticle &a);
    int drawWrapped(const char *text, int x, int y, int maxW, int maxH,
                    const lgfx::v1::IFont *font, int lineH, uint32_t color, int scrollOff);
};
