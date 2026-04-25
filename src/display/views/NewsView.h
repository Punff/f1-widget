#pragma once
#ifndef NEWS_VIEW_H
#define NEWS_VIEW_H

#include "../IView.h"
#include "../../../include/LGFX_Config.h"

class DisplayManager;

class NewsView : public IView
{
public:
    NewsView(LGFX *tft, DisplayManager *dm);

    void onEnter() override;
    void onExit() override;
    void render() override;
    void onTurnRight() override; // next article
    void onTurnLeft() override;  // previous article
    void onPress() override;     // TODO: expand article or open URL via serial
    void onLongPress() override;
    void onDoublePress() override; // TODO: TBD

    static constexpr int TITLE_Y = 10;
    static constexpr int DIVIDER_Y = 50;
    static constexpr int BODY_Y = 60;
    static constexpr int LINE_HEIGHT = 22;
    static constexpr int MARGIN_X = 10;
    static constexpr int MAX_BODY_LINES = 10;

private:
    LGFX *_tft;
    DisplayManager *_dm;
    int _articleIndex;

    void _renderTitle(const char *title);
    void _renderBody(const char *body);
};

#endif