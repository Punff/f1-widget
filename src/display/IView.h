#pragma once

class IView
{
public:
    virtual ~IView() {}
    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void render() = 0;
    virtual void onTurnRight() = 0;
    virtual void onTurnLeft() = 0;
    virtual void onPress() = 0;
    virtual void onLongPress() = 0;
    virtual void onDoublePress() = 0;
    virtual void tick() {} // optional, override per view
};