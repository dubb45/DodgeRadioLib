#include "LXInterface.h"

Thread thread;
Timer t;

LXInterface::LXInterface(PinName tx, PinName rx) :
   CANDevice(tx, rx),
   m_displayed_position(0),
   m_displayed_length(0),
   m_update_volume(false)
{
   printf("LXInterface Initializing\r\n");

   memset(&status, 0, sizeof(status));

   m_muted = false;

   m_HU._volume = 10;
   m_HU._bass = 10;
   m_HU._mid = 15;
   m_HU._treble = 15;
   m_HU._balance = 10;
   m_HU._fade = 10;

   timed = false;
   SWITimerRunning = false;
   prevSWC = 0;
   SWITickCount = 0;

   PowerUp();
   printf("LXInterface initialized\n\r");

   strcpy(m_displayed_text, "");
}

void LXInterface::PowerUp(void)
{
   ReceivedCANMsg = false;
   LPC_CAN2->BTR = 0x52001C;

   writeCANFlag = 0;

   thread.start(callback(Operate, this));
}

void LXInterface::MuteAction()
{
   if (m_muted)
   {
      SetVolume(m_unmuted_volume);
   }
   else
   {
      SetVolume(0);
   }
}

void LXInterface::SetVolume(char new_volume)
{
   m_HU._volume = new_volume;

   m_update_volume = true;

   // trigger the new volume setting and display
}

char LXInterface::GetVolume()
{
   return(m_HU._volume);
}

void LXInterface::Operate(void const *argument)
{
   LXInterface* lx = (LXInterface*)argument;
   t.start();

   while (1)
   {
      lx->writeCANFlag++;

      if (lx->writeCANFlag == 1000)
      {
         lx->writeCANFlag = 0;
         lx->SendSiriusMessage();
      }

      if (((lx->writeCANFlag % 100) == 0) || (lx->m_update_volume == true))
      {
         lx->SendEVICMsg();
         lx->SendStereoSettingsMsg();

         lx->m_update_volume = false;
      }

      if ((lx->writeCANFlag % 50) == 0)
      {
         lx->SendRadioModeMsg();
      }

      if ((lx->writeCANFlag % 50) == 0)
      {
         lx->UpdateText();
      }

      lx->readCANbus();

      Thread::wait(1);
   }
}

void LXInterface::InternalSWI(void const *argument)
{
   LXInterface* lx = (LXInterface*)argument;
   lx->m_HU.SWCAction = 0;

   if (!lx->SWITimerRunning && (lx->m_HU.SWCButtons != 0))
   {
      lx->SWITickCount = 0;
      lx->stacksize = 0;
      lx->SWITimerRunning = true;
   }

   if (lx->SWITimerRunning)
   {
      if (lx->timed)
      {
         lx->SWITickCount++;
      }

      if (((lx->m_HU.SWCButtons == 0) && (lx->prevSWC != 0)) || ((lx->m_HU.SWCButtons != 0) && (lx->m_HU.SWCButtons != lx->prevSWC)))
      {
         // either a button was just pressed, or just released... time to do something
         lx->buttonstack[lx->stacksize] = lx->m_HU.SWCButtons;
         lx->timestack[lx->stacksize] = lx->SWITickCount;
         lx->stacksize++;
      }

//        printf("%d %d %x %x %x\r\n", lx->stacksize, lx->SWITickCount, lx->m_HU.SWCButtons, lx->timed ? 1 : 0, lx->timestack[1]);

//        printf("Stack = %d\r\n", stacksize);
      if (lx->stacksize == 6)
      {
         // triple click?
         lx->m_HU.SWCAction = 0x80000000 + (lx->buttonstack[0] << 5) + kTripleClick;

         // reset everything because pressed and released 3 times.... this stops the triple hold
         lx->SWITimerRunning = false;
         lx->stacksize = 0;
         lx->SWITickCount = 0;
      }
      else if (lx->stacksize == 5)
      {
         if (lx->SWITickCount >= 2)
         {
            // triple click and held
            lx->m_HU.SWCAction = 0x80000000 + (lx->buttonstack[0] << 5) + kTripleHeld;
         }
      }
      else if (lx->stacksize == 4)
      {
         // double click?
         if (lx->timestack[3] > 2)
         {
            // reset everything because double click and held was released
            lx->SWITimerRunning = false;
            lx->stacksize = 0;
            lx->SWITickCount = 0;
         }
         else if (lx->SWITickCount >= 4)
         {
            lx->m_HU.SWCAction = 0x80000000 + (lx->buttonstack[0] << 5) + kDoubleClick;
            // reset everything because double click in < 600 ms
            lx->SWITimerRunning = false;
            lx->stacksize = 0;
            lx->SWITickCount = 0;
         }
      }
      else if (lx->stacksize == 3)
      {
         if (lx->SWITickCount >= 2)
         {
            // double click and held
            lx->m_HU.SWCAction = 0x80000000 + (lx->buttonstack[0] << 5) + kDoubleHeld;
         }
      }
      else if (lx->stacksize == 2)
      {
         // single click
         if (lx->timestack[1] > 2)
         {
            // reset everything because single click and held was released
            lx->SWITimerRunning = false;
            lx->stacksize = 0;
            lx->SWITickCount = 0;
         }
         else if (lx->SWITickCount >= 2)
         {
            lx->m_HU.SWCAction = 0x80000000 + (lx->buttonstack[0] << 5) + kSingleClick;
            // reset everything because only a single click
            lx->SWITimerRunning = false;
            lx->stacksize = 0;
            lx->SWITickCount = 0;
         }
      }
      else if (lx->stacksize == 1)
      {
         if (lx->SWITickCount >= 2)
         {
            // single click and hold
            lx->m_HU.SWCAction = 0x80000000 + (lx->buttonstack[0] << 5) + kSingleHeld;
         }
      }

      if (!lx->SWITimerRunning)
      {
         lx->queue.cancel(lx->m_swi_event);
      }
   }

   lx->prevSWC = lx->m_HU.SWCButtons;




   // start timer at first press....
   // if released, and no press in < 250 ms..... single click
   // if released, pressed in < 250 ms, and not released in < 500.... then single click and hold
   // if released, pressed in < 250 ms, and released again in < 500 ms... then double click
   // if released, pressed in < 250 ms, released, then pressed and not released < 500 ms.... then double click and hold
   // if released, pressed in < 250 ms, released, then pressed and released < 500 ms.... then triple click
}

void LXInterface::SendOnMsg()
{
   static CANMessage msg;
   msg.id = 0x416;
   msg.len = 8;
   static char temp[8] = {0xfd, 0x16, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff};
   memcpy(msg.data, temp, 8);
   CANDevice.write(msg);
}

void LXInterface::SendRadioModeMsg()
{
   static CANMessage msg;
   msg.id = 0x09F;
   msg.len = 8;

   msg.data[7] = 0x0f;
   msg.data[6] = 0xff;
   msg.data[5] = 0xff;
   msg.data[4] = 0xff;
   msg.data[3] = 0x07;
   msg.data[2] = 0x00;
   msg.data[1] = 0x00;
   msg.data[0] = 0x00;

   msg.data[0] |= 0x04;
   msg.data[1] = 0;
   msg.data[2] = m_HU._volume; //sirius_status.channel;
   // msg.data[2] = 2;
   CANDevice.write(msg);
}

void LXInterface::SetText(const char* text)
{
   memset(m_displayed_text, 0, 128);
   m_displayed_length = strlen(text);// >= 64 ? 64 : (strlen(text) % 64);
   m_displayed_position = 0;
   strncpy(m_displayed_text, text, m_displayed_length);
}

void LXInterface::UpdateText()
{
   static int count = 0;
   if (count == 0)
   {
      if (strlen(m_displayed_text) != 0)
      {
         SetEVICText(m_displayed_text, m_displayed_position, m_displayed_length - m_displayed_position > 8 ? 8 : m_displayed_length - m_displayed_position);
         if (m_displayed_position + 8 >= m_displayed_length)
         {
            m_displayed_position = 0;
         }
         else
         {
            m_displayed_position += 8;
         }
      }
   }
   count++;
   if (count > 20)
   {
      count = 0;
   }
}

void LXInterface::SendEVICMsg()
{
   static CANMessage msg;
   msg.id = 0x394;
   msg.len = 6;

   memset(msg.data, 0, 8);

   msg.data[0] = 0x61;
   msg.data[1] = 0x03;
   msg.data[2] = 0xDB;

   CANDevice.write(msg);
}

void LXInterface::SendStereoSettingsMsg()
{
   static CANMessage msg;
   msg.id = 0x3D0;
   msg.len = 7;

   msg.data[0] = m_HU._volume;
   msg.data[1] = m_HU._balance;
   msg.data[2] = m_HU._fade;
   msg.data[3] = m_HU._bass;
   msg.data[4] = m_HU._mid;
   msg.data[5] = m_HU._treble;

   CANDevice.write(msg);
}

void LXInterface::SetEVICText(const char *text, const int start_index, const int length)
{
   static CANMessage msg;
   msg.id = 0x1BF;
   msg.len = 8;
   memset(msg.data, 0, 8);

   memcpy(msg.data, text + start_index, length);
   // printf("EVIC Text = %s\r\n", msg.data);

   CANDevice.write(msg);
}

void LXInterface::SendSiriusMessage()
{
   static CANMessage msg;
   msg.id = 0x1bd;
   msg.len = 4;
   msg.data[0] = 0x09;
   msg.data[1] = m_HU._volume;
   msg.data[2] = 0x6D;
   msg.data[3] = 0x83;
   CANDevice.write(msg);

   msg.id = 0x1be;
   msg.len = 8;
   msg.data[0] = 'R';
   msg.data[1] = 'O';
   msg.data[2] = 'C';
   msg.data[3] = 'K';
   msg.data[4] = 0;
   msg.data[5] = 0;
   msg.data[6] = 0;
   msg.data[7] = 0;
   CANDevice.write(msg);

   msg.id = 0x3b0;
   msg.len = 6;
   msg.data[0] = 23;
   msg.data[1] = m_HU._volume;
   msg.data[2] = 0;
   msg.data[3] = 0;
   msg.data[4] = 0;
   msg.data[5] = 0;
   msg.data[6] = 0;
   msg.data[7] = 0;
   CANDevice.write(msg);
}

void LXInterface::ParseCANMessage(CANMessage can_MsgRx)
{
   // this message seems to be a message requesting all other devices
   // to start announcing their presence
   if ((can_MsgRx.id >= 0x400) && (can_MsgRx.data[0] == 0xfd))
   {}

   if (can_MsgRx.id == 0x000)
   {
/*
        if (can_MsgRx.data[0] > 1)
        {
            radioOn = true;
        }
        else
        {
            radioOn = false;
        }
 */
      status._keyPosition = can_MsgRx.data[0];
   }
   else if (can_MsgRx.id == 0x002)
   {
      status._rpm = (can_MsgRx.data[0] << 8) + can_MsgRx.data[1];
      status._speed = ((can_MsgRx.data[2] << 8) + can_MsgRx.data[3]) >> 7;

      // what are the other 4 bytes?
   }
   else if (can_MsgRx.id == 0x003)
   {
      status._brake = can_MsgRx.data[3] & 0x01;
      status._gear = can_MsgRx.data[4];
   }
   else if (can_MsgRx.id == 0x012)
   {}
   else if (can_MsgRx.id == 0x14)
   {
      status._odometer = (can_MsgRx.data[0] << 16) + (can_MsgRx.data[1] << 8) + can_MsgRx.data[2];
      // what are the other 4 bytes?
   }
   else if (can_MsgRx.id == 0x15)
   {
      status._batteryVoltage = (float)(can_MsgRx.data[1]) / 10;
   }
   else if (can_MsgRx.id == 0x01b)
   {
      // vin number
      int part = can_MsgRx.data[0];
      if ((part >= 0) && (part < 3))
      {
         for (int i = 1; i < 8; i++)
         {
            status._vin[(part*7) + i-1] = can_MsgRx.data[i];
         }
      }
   }
   else if (can_MsgRx.id == 0x0d0)
   {
      if (can_MsgRx.data[0] == 0x80)
      {
         status._parkingBrake = true;
      }
      else
      {
         status._parkingBrake = false;
      }
   }
   else if (can_MsgRx.id == 0x0EC)
   {
      if ((can_MsgRx.data[0] & 0x40) == 0x40)
      {
         status._fanRequested = true;
      }
      else
      {
         status._fanRequested = false;
      }

      if ((can_MsgRx.data[0] & 0x01) == 0x01)
      {
         status._fanOn = true;
      }
      else
      {
         status._fanOn = false;
      }
   }
   else if (can_MsgRx.id == 0x159)
   {
      status._fuel = can_MsgRx.data[5];
   }
   else if (can_MsgRx.id == 0x1a2)
   {
      if ((can_MsgRx.data[0] & 0x80) == 0x80)
      {
         status._rearDefrost = true;
      }
      else
      {
         status._rearDefrost = false;
      }

      if ((can_MsgRx.data[0] & 0x40) == 0x40)
      {
         status._fanRequested = true;
      }
      else
      {
         status._fanRequested = false;
      }

      if ((can_MsgRx.data[0] & 0x01) == 0x01)
      {
         status._fanOn = true;
      }
      else
      {
         status._fanOn = false;
      }
   }
   else if (can_MsgRx.id == 0x1bd)
   {
      // SDAR status

      // if (sirius_status_channel == 0)
      // {
      //    sirius_status_channel = can_MsgRx.data[1];
      // }
      //
      // if (can_MsgRx.data[0] == 0x85)
      // {
      //    ChangeSiriusStation(sirius_status_channel, true);
      // }
      //
      // m_HU._siriusChan = can_MsgRx.data[1];
   }
   else if (can_MsgRx.id == 0x1bf)
   {
      for (int i = 0; i < 8; i++)
      {
         printf("%02x ", can_MsgRx.data[i]);
      }
      printf("\r\n");
   }
   else if (can_MsgRx.id == 0x1c8)
   {
      status._headlights = can_MsgRx.data[0];
   }
   else if (can_MsgRx.id == 0x210)
   {
      status._dimmerMode = can_MsgRx.data[0];
      if (can_MsgRx.data[0] == 0x03)
      {
         status._dimmer = 255;
      }
      else if (can_MsgRx.data[0] == 0x02)
      {
         status._dimmer = can_MsgRx.data[1];
      }
   }
   else if (can_MsgRx.id == 0x3a0)
   {
      // note = 0x01
      // volume up = 0x02
      // volume down = 0x04
      // up arrow = 0x08
      // down arrow = 0x10
      // right arrow = 0x20

      // if (can_MsgRx.data[0] != m_HU.SWCButtons)
      // {
      //    printf("can: swi data = %x\r\n", can_MsgRx.data[0]);
      //    m_HU.SWCButtons = can_MsgRx.data[0];
      //    timed = false;
      //    printf("timed = %x\r\n", timed ? 1 : 0);
      //    InternalSWI(this);
      //    timed = true;
      //    printf("timed = %x\r\n", timed ? 1 : 0);
      //    if (!SWITimerRunning)
      //    {
      //       m_swi_event = queue.call_every(200, InternalSWI, this);
      //    }
      // }


      if (can_MsgRx.data[0] != m_HU.SWCButtons)
      {
         if (can_MsgRx.data[0] == 0)
         {
            // just released....
            m_HU.SWCAction = 0x80000000 + (m_HU.SWCButtons << 5) + kSingleClick;
         }
      }
      else if (can_MsgRx.data[0] != 0)
      {
         // held
         m_HU.SWCAction = 0x80000000 + (m_HU.SWCButtons << 5) + kSingleHeld;
      }

      m_HU.SWCButtons = can_MsgRx.data[0];
   }
/*
    else if (can_MsgRx.id == 0x3bd)
    {
        ReadSiriusText((char *)can_MsgRx.data);
    }
 */
}

/*
   void LXInterface::ReadSiriusText(char *data)
   {
    int num = (data[0] & 0xF0) >> 4;
    if ((num > 7) || (num < 0))
    {
        return;
    }

    int part = (data[0] & 0x0E) >> 1;
    if ((part > 7) || (part < 0))
    {
        return;
    }

    if ((data[0] & 0x01) != 0)
    {
        memset(sirius_status.text[num], 0, 64);
    }

    memset(&sirius_status.text[num][(part * 7)], 0, 7);

    for (int i = 1; i < 8; i++)
    {
        sirius_status.text[num][(part * 7) + (i-1)] = data[i];
    }
   }
 */

void LXInterface::readCANbus(void)
{
   if (CANDevice.read(can_MsgRx))
   {
      ReceivedCANMsg = true;
      ParseCANMessage(can_MsgRx);
   }
}
