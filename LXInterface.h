#ifndef LXINTERFACE_H
#define LXINtERFACE_H

#include "mbed.h"
#include "rtos.h"
#include "mbed_events.h"


#include "LXState.h"

class LXInterface
{
public:
   LXInterface(PinName tx, PinName rx);
   ~LXInterface() {}

   void SetText(const char* text);

   LXHeadUnit m_HU;
   LXState status;


   void MuteAction();
   void SetVolume(char new_volume);
   char GetVolume();
   bool m_muted;

private:

   CAN CANDevice;
   CANMessage can_MsgRx;

   void PowerUp(void);

   void readCANbus(void);

   void SendOnMsg();
   void SendEVICMsg();
   void SendRadioModeMsg();
   void SendStereoSettingsMsg();
   void SetEVICText(const char *text, const int start_index, const int length);
   void UpdateText();
   char m_displayed_text[128];
   int m_displayed_position;
   int m_displayed_length;


   void ParseCANMessage(CANMessage can_MsgRx);

   int writeCANFlag;
   bool ReceivedCANMsg;

   bool timed;
   static void InternalSWI(void const *argument);
   EventQueue queue;

   int m_swi_event;
   bool SWITimerRunning;
   int prevSWC;
   int timestack[6];
   int buttonstack[6];
   int stacksize;
   int SWITickCount;

   static void Operate(void const *argument);



   void SendSiriusMessage();

   bool m_update_volume;
   char m_unmuted_volume;
};

#endif
