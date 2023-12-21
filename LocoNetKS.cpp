/*
||
|| @file LocoNetKS.cpp
|| @version 1.1
|| @author Michael Zimmermann
|| @contact michael.zimmermann.sg@t-online.de
||
|| @description
|| | additional functions for class 'LocoNet'
|| #
||
|| @license
|| |	Copyright (c) 2021 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
|| |	All rights reserved.
|| |	
|| |	This program is free software: you can redistribute it and/or modify
|| |	it under the terms of the GNU General Public License as published by
|| |	the Free Software Foundation, either version 3 of the License, or
|| |	(at your option) any later version.
|| |	
|| |	This program is distributed in the hope that it will be useful,
|| |	but WITHOUT ANY WARRANTY; without even the implied warranty of
|| |	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|| |	GNU General Public License for more details.
|| |	
|| |	You should have received a copy of the GNU General Public License
|| |	along with this program. If not, see <http://www.gnu.org/licenses/>.
|| #
||
*/

#include <LocoNet.h>
#include "LocoNetKS.h"

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//=== LocoNetClassKS ===
LN_STATUS LocoNetClassKS::send(uint8_t OpCode)
{
  lnMsg SendPacket;

  SendPacket.data[0] = OpCode;

  return LocoNet.send(&SendPacket);
}

LN_STATUS LocoNetClassKS::sendSwitchState(uint16_t ui16_swAdr, boolean b_swOn, boolean b_swDirection, uint8_t ui8_OPC)
{
  if (!ui16_swAdr || (ui16_swAdr > 2048) || (ui8_OPC < OPC_SW_REQ) || (ui8_OPC > OPC_INPUT_REP))
    return LN_UNKNOWN_ERROR;

  switch (ui8_OPC)
  {
    case OPC_SW_REQ: // B0
      return LocoNet.requestSwitch(ui16_swAdr, b_swOn, b_swDirection);
    case OPC_SW_REP: // B1
      return sendSwitchReport(ui16_swAdr, b_swOn, b_swDirection);
    case OPC_INPUT_REP: // B2
      return sendReportSensor(ui16_swAdr, b_swOn);
  }
  return LN_UNKNOWN_ERROR;
}

LN_STATUS LocoNetClassKS::sendSwitchReport(uint16_t Address, uint8_t Output, uint8_t Direction)
{ // B1
  uint8_t AddrH = (--Address >> 7) & 0x0F;
  uint8_t AddrL = Address & 0x7F;

  if (Output)
    AddrH |= OPC_SW_REQ_OUT;

  if (Direction)
    AddrH |= OPC_SW_REQ_DIR;

  return LocoNet.send(OPC_SW_REP, AddrL, AddrH);
}

LN_STATUS LocoNetClassKS::sendReportSensor(uint16_t Address, uint8_t State)
{ // B2
  byte AddrH = ((--Address >> 8) & 0x0F);
  byte AddrL = (Address >> 1) & 0x7F;
  if (Address % 2)
    AddrH |= OPC_INPUT_REP_SW;

  if (State)
    AddrH |= OPC_INPUT_REP_HI;

  return LocoNet.send(OPC_INPUT_REP, AddrL, AddrH);
}
//=== end LocoNetClassKS ===
