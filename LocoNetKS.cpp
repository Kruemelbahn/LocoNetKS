/*
||
|| @file LocoNetKS.cpp
|| @version 1.0
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

//=== LocoNetFastClockClassKS ===
/* this class is copied from original LocoNetFastClockClass in file loconet.cpp (Version 1.1.13) 
*  modification is made for new flag "FC_FLAG_NOTIFY_JMRI" (see also commenst "added")
*/
#define FC_FLAG_DCS100_COMPATIBLE_SPEED	0x01
#define FC_FLAG_MINUTE_ROLLOVER_SYNC		0x02
#define FC_FLAG_NOTIFY_FRAC_MINS_TICK		0x04
#define FC_FLAG_NOTIFY_JMRI							0x08			// new for JMRI
#define FC_FRAC_MIN_BASE   							0x3FFF
#define FC_FRAC_RESET_HIGH	 						0x78
#define FC_FRAC_RESET_LOW 	 						0x6D
#define FC_TIMER_TICKS         					65        // 65ms ticks
#define FC_TIMER_TICKS_REQ	  					250       // 250ms waiting for Response to FC Req

void LocoNetFastClockClassKS::init(uint8_t DCS100CompatibleSpeed, uint8_t CorrectDCS100Clock, uint8_t NotifyFracMin)
{
	fcState = FC_ST_IDLE;

	fcFlags = 0;
	if (DCS100CompatibleSpeed)
		fcFlags |= FC_FLAG_DCS100_COMPATIBLE_SPEED;

	if (CorrectDCS100Clock)
		fcFlags |= FC_FLAG_MINUTE_ROLLOVER_SYNC;

	if (NotifyFracMin)
		fcFlags |= FC_FLAG_NOTIFY_FRAC_MINS_TICK;
}

// new method
void LocoNetFastClockClassKS::initJMRI(uint8_t IsJMRI)
{
	fcLastfrac_minsh = 0;
	if (IsJMRI)
		fcFlags |= FC_FLAG_NOTIFY_JMRI;
}

void LocoNetFastClockClassKS::poll(void)
{
	LocoNet.send(OPC_RQ_SL_DATA, FC_SLOT, 0);
}

void LocoNetFastClockClassKS::doNotify(uint8_t Sync)
{
	if (notifyFastClock)
		notifyFastClock(fcSlotData.clk_rate, fcSlotData.days,
			(fcSlotData.hours_24 >= (128 - 24)) ? fcSlotData.hours_24 - (128 - 24) : fcSlotData.hours_24 % 24,
			fcSlotData.mins_60 - (127 - 60), Sync);
}

// changed method
void LocoNetFastClockClassKS::processMessage(lnMsg* LnPacket)
{
	if ((LnPacket->fc.slot == FC_SLOT) && ((LnPacket->fc.command == OPC_WR_SL_DATA) || (LnPacket->fc.command == OPC_SL_RD_DATA)))
	{
		if ((LnPacket->fc.clk_cntrl & 0x40)
// +++ added because JMRI is sending 'fc.clk_cntrl = 0x00' (Bit 6 not set)
			|| (fcFlags & FC_FLAG_NOTIFY_JMRI))
// added end
		{
			if (fcState >= FC_ST_REQ_TIME)
			{
				memcpy(&fcSlotData, &LnPacket->fc, sizeof(fastClockMsg));
// ++ added for JMRI and use in "process66msActions"
				fcLastfrac_minsh = fcSlotData.frac_minsh;
// added end
				doNotify(1);

				if (notifyFastClockFracMins && fcFlags & FC_FLAG_NOTIFY_FRAC_MINS_TICK)
					notifyFastClockFracMins(FC_FRAC_MIN_BASE - ((fcSlotData.frac_minsh << 7) + fcSlotData.frac_minsl));

				fcState = FC_ST_READY;
			}
		}
		else
			fcState = FC_ST_DISABLED;
	}
}

// changed method
void LocoNetFastClockClassKS::process66msActions(void)
{
	// If we are all initialised and ready then increment accumulators
	if (fcState == FC_ST_READY)
	{
		fcSlotData.frac_minsl += fcSlotData.clk_rate;
		if (fcSlotData.frac_minsl & 0x80)
		{
			fcSlotData.frac_minsl &= ~0x80;

			fcSlotData.frac_minsh++;
			if ((fcSlotData.frac_minsh & 0x80)
// +++ added because JMRI starts with 'fcSlotData.frac_minsh = 3' instead of FC_FRAC_RESET_HIGH (0x78)
//                                 therefore fcSlotData.frac_minsh is always less than 0x80
//															   and fcSlotData.frac_minsh comes in maximum up to 10, independant from fcSlotdata.clk_rate
				|| ((fcFlags & FC_FLAG_NOTIFY_JMRI) &&
					  (fcSlotData.frac_minsh > (fcLastfrac_minsh + 6)) &&
						(fcSlotData.frac_minsh < (FC_FRAC_RESET_HIGH + (fcFlags & FC_FLAG_DCS100_COMPATIBLE_SPEED)))
					 ) 
				 )
// added end
			{
				// For the next cycle prime the fraction of a minute accumulators
				fcSlotData.frac_minsl = FC_FRAC_RESET_LOW;

				// If we are in FC_FLAG_DCS100_COMPATIBLE_SPEED mode we need to run faster
				// by reducong the FRAC_MINS duration count by 128
				fcSlotData.frac_minsh = FC_FRAC_RESET_HIGH + (fcFlags & FC_FLAG_DCS100_COMPATIBLE_SPEED);

				fcSlotData.mins_60++;
				if (fcSlotData.mins_60 >= 0x7F)
				{
					fcSlotData.mins_60 = 127 - 60;

					fcSlotData.hours_24++;
					if (fcSlotData.hours_24 & 0x80)
					{
						fcSlotData.hours_24 = 128 - 24;

						fcSlotData.days++;
					}
				}

				// We either send a message out onto the LocoNet to change the time,
				// which we will also see and act on or just notify our user
				// function that our internal time has changed.
				if (fcFlags & FC_FLAG_MINUTE_ROLLOVER_SYNC)
				{
					fcSlotData.command = OPC_WR_SL_DATA;
					LocoNet.send((lnMsg*)&fcSlotData);
				}
				else
					doNotify(0);
			}
		}

		if (notifyFastClockFracMins && (fcFlags & FC_FLAG_NOTIFY_FRAC_MINS_TICK))
			notifyFastClockFracMins(FC_FRAC_MIN_BASE - ((fcSlotData.frac_minsh << 7) + fcSlotData.frac_minsl));
	}

	if (fcState == FC_ST_IDLE)
	{
		LocoNet.send(OPC_RQ_SL_DATA, FC_SLOT, 0);
		fcState = FC_ST_REQ_TIME;
	}
}
//=== end LocoNetFastClockClassKS ===
