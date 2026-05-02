#pragma once

class IView
{
public:
    virtual ~IView() {}

    // ── Lifecycle (Must implement) ────────────────────────────────────────────
    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void render() = 0;

    // ── Input (Optional: Defaults to doing nothing) ──────────────────────────
    // Removing the "= 0" allows subclasses to ignore these if they don't need them
    virtual void onTurnRight() {}
    virtual void onTurnLeft() {}
    virtual void onPress() {}
    virtual void onLongPress() {}
    virtual void onDoublePress() {}

    // ── Logic ─────────────────────────────────────────────────────────────────
    virtual void tick() {}
};