#ifndef LXSTATE_H
#define LXSTATE_H

enum radioMode { AM = 0, FM, CD, SAT, VES, MAX_MODE };

enum LXButtons
{
   kNoButtons = 0,
   kNote = 1,
   kVolumeUp = 2,
   kVolumeDown = 4,
   kUpArrow = 8,
   kDownArrow = 0x10,
   kRightArrow = 0x20
};

enum LXSWIAction
{
   kSingleClick = 0,
   kDoubleClick = 1,
   kTripleClick = 2,
   kSingleHeld = 3,
   kDoubleHeld = 4,
   kTripleHeld = 5
};

struct LXHeadUnit
{
    char _volume;
    char _balance;
    char _fade;
    char _bass;
    char _mid;
    char _treble;

    short SWCButtons;
    unsigned int SWCAction;
};


struct LXState
{
    float _batteryVoltage;
    char _driverHeatedSeatLevel;
    char _passHeatedSeatLevel;
    char _vin[24];
    char _headlights;
    char _dimmerMode;
    char _dimmer;
    char _gear;
    char _brake;
    char _parkingBrake;
    char _keyPosition;
    int _rpm;
    char _fanRequested;
    char _fanOn;
    char _rearDefrost;
    char _fuel;
    char _speed;
    int _odometer;
    int count ;
};

#endif
