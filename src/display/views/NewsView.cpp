#include "NewsView.h"
#include "../DisplayManager.h"
#include "../../api/APIClient.h"
#include "../../../include/UI_Fonts.h"
#include <qrcode.h>

extern DataCache *cache;

// Content padding (matches project standard)

NewsView::NewsView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm), _current(0), _showOverlay(false) {}

void NewsView::onEnter()
{
    _current = 0;
    _showOverlay = false;

    if (cache->newsFeed.empty())
    {
        showLoading();
        fetchNews();
    }

    _tft->fillScreen(UI::COL_BG);
    render();
}

void NewsView::render()
{
    if (cache->newsFeed.empty())
    {
        _tft->setTextDatum(middle_center);
        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->drawString("No news available", UI::SCREEN_W / 2, UI::HEADER_H + UI::CONTENT_H / 2);
        return;
    }

    if (_current < 0) _current = 0;
    if (_current >= (int)cache->newsFeed.size()) _current = (int)cache->newsFeed.size() - 1;

    renderArticle();
}

void NewsView::onTurnRight()
{
    if (cache->newsFeed.empty()) return;
    if (_current < (int)cache->newsFeed.size() - 1)
    {
        _current++;
        render();
    }
}

void NewsView::onTurnLeft()
{
    if (cache->newsFeed.empty()) return;
    if (_current > 0)
    {
        _current--;
        render();
    }
}

void NewsView::onPress()
{
    if (cache->newsFeed.empty()) return;
    _showOverlay = !_showOverlay;
    render();
}

void NewsView::onLongPress()
{
    _dm->returnToMenu();
}

// ── Rendering ──────────────────────────────────────────────────────────

void NewsView::renderArticle()
{
    // No full screen fill to prevent flicker
    _dm->header()->draw("NEWS", nullptr, "F1");

    const auto &a = cache->newsFeed[_current];

    if (_showOverlay)
        renderOverlay(a);
    else
        renderNormal(a);

    char buf[32];
    snprintf(buf, sizeof(buf), "NEW \xc2\xb7 %d/%d", _current + 1, (int)cache->newsFeed.size());
    _dm->footer()->draw();
    _dm->footer()->drawCenter(buf, UI::COL_MUTED);
}

void NewsView::renderNormal(const NewsArticle &a)
{
    // Clear content area AND the hint area at the bottom
    _tft->fillRect(0, UI::HEADER_H + 2, UI::SCREEN_W, UI::SCREEN_H - UI::HEADER_H - 2, UI::COL_BG);

    int cx = UI::PAD_X;
    int cw = UI::SCREEN_W - UI::PAD_X * 2;

    _tft->setTextDatum(top_left);

    // Date line
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->setTextColor(UI::COL_MUTED);
    char dateLine[64];
    snprintf(dateLine, sizeof(dateLine), "%s  \xc2\xb7  motorsport.com", a.pubDate);
    _tft->drawString(dateLine, cx, UI::HEADER_H + 8);

    // Title
    int ty = UI::HEADER_H + 30;
    int titleLines = drawWrapped(a.title, cx, ty, cw, 72,
                                 UI::Fonts::BODY_MAIN, 22, UI::COL_TEXT, 0);

    // Divider
    int divY = ty + titleLines * 22 + 8;
    _tft->drawFastHLine(cx, divY, cw, UI::COL_DIVIDER);

    // Description excerpt
    int dy = divY + 12;
    int dh = UI::CONTENT_H - (dy - UI::HEADER_H) - 24;
    if (dh > 8)
        drawWrapped(a.description, cx, dy, cw, dh,
                    UI::Fonts::LABEL_SMALL, 15, UI::COL_TEXT_DIM, 0);

    // Press hint
    _tft->setTextDatum(bottom_center);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->setTextColor(UI::COL_MUTED);
    _tft->drawString("Press for QR Code", UI::SCREEN_W / 2, UI::FOOTER_Y - 4);
}

void NewsView::renderOverlay(const NewsArticle &a)
{
    // Clear with slightly different background for depth, including bottom hint area
    _tft->fillRect(0, UI::HEADER_H + 2, UI::SCREEN_W, UI::SCREEN_H - UI::HEADER_H - 2, 0x0A0A0A);

    // Generate QR Code
    QRCode qrcode;
    size_t qsize = qrcode_getBufferSize(10);
    uint8_t *qrcodeData = (uint8_t *)malloc(qsize);
    if (!qrcodeData) return;

    int8_t res = qrcode_initText(&qrcode, qrcodeData, 10, ECC_MEDIUM, a.url);

    if (res == 0)
    {
        const int quietZoneModules = 4;
        const int modules = qrcode.size + (quietZoneModules * 2);
        int scale = 2;
        int qrTotalSize = modules * scale;
        
        int qrX = (UI::SCREEN_W - qrTotalSize) / 2;
        int qrY = UI::HEADER_H + (UI::CONTENT_H - qrTotalSize) / 2 - 10;

        // Visual Improvement: Subtle frame/shadow
        _tft->drawRect(qrX - 2, qrY - 2, qrTotalSize + 4, qrTotalSize + 4, UI::COL_DIVIDER);
        
        // Use a sprite to draw QR once (scaled)
        LGFX_Sprite qrSprite(_tft);
        if (qrSprite.createSprite(qrTotalSize, qrTotalSize))
        {
            qrSprite.fillSprite(0xFFFFFF); // Background White

            for (uint8_t y = 0; y < qrcode.size; y++) {
                for (uint8_t x = 0; x < qrcode.size; x++) {
                    if (qrcode_getModule(&qrcode, x, y)) {
                        qrSprite.fillRect(
                            (x + quietZoneModules) * scale,
                            (y + quietZoneModules) * scale,
                            scale, scale, 0x000000
                        );
                    }
                }
            }
            _tft->waitDMA();
            _tft->pushImageDMA(qrX, qrY, qrSprite.width(), qrSprite.height(), (uint16_t*)qrSprite.getBuffer());
            // waitDMA will be called by subsequent draw calls or next frame
        }

        // URL Label below QR
        _tft->setTextDatum(top_center);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->setTextColor(UI::COL_MUTED);
        _tft->drawString("SCAN TO READ ARTICLE", UI::SCREEN_W / 2, qrY + qrTotalSize + 10);
    }
    else
    {
        _tft->setTextDatum(middle_center);
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->setTextColor(UI::COL_F1_RED);
        _tft->drawString("URL ERROR", UI::SCREEN_W / 2, UI::SCREEN_H / 2);
    }

    _tft->setTextDatum(bottom_center);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->setTextColor(UI::COL_MUTED);
    _tft->drawString("Press to return", UI::SCREEN_W / 2, UI::FOOTER_Y - 4);

    free(qrcodeData);
}

// ── Word-wrap helper ───────────────────────────────────────────────────

int NewsView::drawWrapped(const char *text, int x, int y, int maxW, int maxH,
                          const IFont *font, int lineH, uint32_t color, int scrollOff)
{
    if (!text || !*text) return 0;

    // F1 fonts are wider than average. Formula1_Display_Regular10pt7b is ~10-12px avg.
    // Formula1_Display_Regular7pt7b is ~7-8px.
    const int avgCharW = (font == UI::Fonts::LABEL_SMALL) ? 8 : 11;
    
    // Add a 10px safety margin to maxW to ensure no overflow
    int safeMaxW = maxW - 10;
    int maxChars = safeMaxW / avgCharW;
    if (maxChars < 6) maxChars = 6;

    _tft->setFont(font);
    _tft->setTextColor(color);
    _tft->setTextDatum(top_left);

    char line[64];
    char word[32];
    int li = 0, wi = 0;
    int lineNum = 0;

    for (const char *p = text;; p++)
    {
        char c = *p;
        bool end = (c == '\0');
        bool space = (c == ' ' || c == '\t' || end);

        if (space)
        {
            word[wi] = '\0';
            wi = 0;
            int wl = strlen(word);

            if (wl > 0)
            {
                bool wordLong = wl > maxChars;

                if (!wordLong && li + wl + (li > 0 ? 1 : 0) <= maxChars)
                {
                    if (li > 0) line[li++] = ' ';
                    for (int i = 0; word[i]; i++) line[li++] = word[i];
                    line[li] = '\0';
                }
                else
                {
                    if (li > 0)
                    {
                        if (lineNum >= scrollOff)
                        {
                            int sy = y + (lineNum - scrollOff) * lineH;
                            if (sy < y + maxH - lineH)
                                _tft->drawString(line, x, sy);
                        }
                        lineNum++;
                        li = 0;
                    }

                    if (wordLong)
                    {
                        for (int i = 0; word[i]; i++)
                        {
                            line[li++] = word[i];
                            if (li >= maxChars || !word[i + 1])
                            {
                                line[li] = '\0';
                                if (lineNum >= scrollOff)
                                {
                                    int sy = y + (lineNum - scrollOff) * lineH;
                                    if (sy < y + maxH - lineH)
                                        _tft->drawString(line, x, sy);
                                }
                                lineNum++;
                                li = 0;
                            }
                        }
                    }
                    else
                    {
                        for (int i = 0; word[i]; i++) line[li++] = word[i];
                        line[li] = '\0';
                    }
                }
            }

            if (end)
            {
                if (li > 0 && lineNum >= scrollOff)
                {
                    int sy = y + (lineNum - scrollOff) * lineH;
                    if (sy < y + maxH - lineH)
                        _tft->drawString(line, x, sy);
                }
                return lineNum + (li > 0 ? 1 : 0);
            }
        }
        else if (wi < (int)sizeof(word) - 2)
        {
            word[wi++] = c;
        }
    }

    return lineNum;
}

// ── Loading & Fetch ────────────────────────────────────────────────────

void NewsView::showLoading()
{
    _tft->fillScreen(UI::COL_BG);

    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_ACCENT);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("NEWS", UI::SCREEN_W / 2, 120);

    _tft->drawFastHLine(UI::SCREEN_W / 2 - 20, 152, 40, UI::COL_ACCENT);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("Loading...", UI::SCREEN_W / 2, 170);

    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString("motorsport.com", UI::SCREEN_W / 2, 200);
}

void NewsView::fetchNews()
{
    APIClient api(cache);
    bool ok = api.fetchNewsFeed();
    Serial.printf("[NEWS] RSS fetch %s\n", ok ? "OK" : "FAILED");
}
