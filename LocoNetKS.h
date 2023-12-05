/*
||
|| @file LocoNetKS.h
|| @version 1.0
|| @author Michael Zimmermann
|| @contact michael.zimmermann.sg@t-online.de
||
|| @description
|| | additional functions for 'LocoNetClass' and 'LocoNetFastClockClass' from Loconet-Library, published by MRRwA (previously EmbeddedLocoNet)
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
|| |
|| |  (see also: licenseinformations in file 'loconet.h' published by MRRwA (previously EmbeddedLocoNet))
|| #
||
*/

#ifndef _KS_LOCONETKS_H
#define _KS_LOCONETKS_H

class LocoNetClassKS {
	public:
		LocoNetClassKS() {};

		LN_STATUS send(uint8_t OpCode);

		LN_STATUS sendSwitchState(uint16_t ui16_swAdr, boolean b_swOn, boolean b_swDirection, uint8_t ui8_OPC);

protected:
		// B1
		LN_STATUS sendSwitchReport(uint16_t Address, uint8_t Output, uint8_t Direction);
		// B2
		LN_STATUS sendReportSensor(uint16_t Address, uint8_t State);
};

extern LocoNetClassKS LocoNetKS;

class LocoNetFastClockClassKS
{
private:
	FC_STATE			fcState;			// State of the Fast Clock Slave 
	uint8_t				fcFlags;			// Storage of the option flags passed into initFastClock()
	fastClockMsg 	fcSlotData;		// Primary storage for the Fast Clock slot data 
	uint8_t 			fcLastPeriod;	// Period of last tick so we can alternate between
	uint8_t       fcLastfrac_minsh; // new

	void doNotify(uint8_t Sync);

public:
	void init(uint8_t DCS100CompatibleSpeed, uint8_t CorrectDCS100Clock, uint8_t NotifyFracMin);
	void initJMRI(uint8_t IsJMRI);															// new
	void poll(void);
	void processMessage(lnMsg* LnPacket);												// modified
	void process66msActions(void);															// modified

	FC_STATE getFastClockState() { return fcState; }						// new, used in debug-cases
	fastClockMsg getFastClockSlotData() { return fcSlotData; }	// new, used in debug-cases
};

#endif
