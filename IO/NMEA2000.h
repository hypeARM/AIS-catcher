/*
	Copyright(c) 2021-2024 jvde.github@gmail.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <fstream>
#include <iostream>
#include <iomanip>

#include "IO.h"
#include "AIS.h"
#include "JSONAIS.h"


// Work in progress. This code is heavily based on the work of CanBoat and others.... 
// Sources:
// https://github.com/AK-Homberger/NMEA2000-AIS-Gateway
// https://github.com/ttlappalainen/NMEA2000
// https://github.com/canboat/canboat


#ifdef HASCANSOCKET
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#endif

class Receiver;

namespace IO {


	class NMEA2000 : public OutputJSON {
#ifdef HASCANSOCKET
		int s;
		struct can_frame frame;
		char buffer[255] = { 0 };
		int length = 0;

	public:
		void Start() {
			struct sockaddr_can addr;
			struct ifreq ifr;

			std::cerr << "NMEA2000 output started" << std::endl;

			if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
				perror("Socket");
				throw std::runtime_error("Error opening CAN socket");
			}

			strcpy(ifr.ifr_name, "vcan0");
			ioctl(s, SIOCGIFINDEX, &ifr);
			memset(&addr, 0, sizeof(addr));
			addr.can_family = AF_CAN;
			addr.can_ifindex = ifr.ifr_ifindex;

			if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
				perror("Bind");
				throw std::runtime_error("Error binding to CAN interface");
			}
		}
		void Stop() {
			std::cerr << "NMEA2000 output stopped" << std::endl;
			close(s);
		}

		// from CanBoat - https://github.com/canboat/canboat

		unsigned int getCanIdFromISO11783Bits(unsigned int prio, unsigned int pgn, unsigned int src, unsigned int dst) {
			unsigned int canId = (src & 0xff) | 0x80000000U;

			unsigned int PF = (pgn >> 8) & 0xff;

			if (PF < 240) { // PDU 1
				canId |= (dst & 0xff) << 8;
				canId |= (pgn << 8);
			}
			else { // PDU 2
				canId |= pgn << 8;
			}
			canId |= prio << 26;

			return canId;
		}

		void sendFrame(struct can_frame* frame, int socket) {
			if (write(socket, frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
				perror("write");
				StopRequest();
			}
		}

		void addByte(uint8_t byte) {
			buffer[length++] = byte;
		}

		void addInt32(int32_t value) {
			std::memcpy(buffer + length, &value, 4);
			length += 4;
		}

		void addUint32(uint32_t value) {
			std::memcpy(buffer + length, &value, 4);
			length += 4;
		}

		void addFloat4(double value, double precision) {
			addInt32(value / precision);
		}

		void addFloat2(double value, double precision) {
			*((int16_t*)(buffer + length)) = value / precision;
			length += 2;
		}

		void addFloat2Undefined() {
			*((uint16_t*)(buffer + length)) = (uint16_t)0x7fff;
			length += 2;
		}

		void addFloat4Undefined() {
			*((uint32_t*)(buffer + length)) = (uint32_t)0x7fffffff;
			length += 4;
		}

		void SendData() {
			int index = 0;

			frame.data[0] = index;
			frame.data[1] = length;

			std::memcpy(&frame.data[2], buffer, 6);

			frame.can_dlc = 8;
			sendFrame(&frame, s);
			length -= 6;

			while (length > 0) {
				frame.data[0] = ++index;
				int sendlen = std::min(length, 7);

				std::memcpy(&frame.data[1], buffer + 6 + (index - 1) * 7, sendlen);

				frame.can_dlc = sendlen + 1;
				sendFrame(&frame, s);
				length -= sendlen;
			}
		}

		// Recipe from: https://github.com/AK-Homberger/NMEA2000-AIS-Gateway

		void ClassAPositionReport(const AIS::Message& ais, const JSON::JSON* data) {

			frame.can_id = getCanIdFromISO11783Bits(4, 129038, 240, 255);

			double lat = LAT_UNDEFINED;
			double lon = LON_UNDEFINED;
			double cog = COG_UNDEFINED;
			double speed = SPEED_UNDEFINED;
			int heading = HEADING_UNDEFINED;
			double turn = 0;
			int status = 0;
			int second = 0;
			int raim = 0;
			int accuracy = 0;

			for (const JSON::Property& p : data[0].getProperties()) {
				switch (p.Key()) {
				case AIS::KEY_LAT:
					lat = p.Get().getFloat();
					break;
				case AIS::KEY_LON:
					lon = p.Get().getFloat();
					break;
				case AIS::KEY_STATUS:
					status = p.Get().getInt();
					break;
				case AIS::KEY_TURN:
					turn = p.Get().getFloat();
					break;
				case AIS::KEY_HEADING:
					heading = p.Get().getInt();
					break;
				case AIS::KEY_SECOND:
					second = p.Get().getInt();
					break;
				case AIS::KEY_RAIM:
					raim = p.Get().getBool() ? 1 : 0;
					break;
				case AIS::KEY_ACCURACY:
					accuracy = p.Get().getBool() ? 1 : 0;
					break;
				case AIS::KEY_COURSE:
					cog = p.Get().getFloat();
					break;
				case AIS::KEY_SPEED:
					speed = p.Get().getFloat();
					break;
				}
			}

			length = 0;

			addByte(ais.type());
			addUint32(ais.mmsi());
			if (lon != LON_UNDEFINED && lat != LAT_UNDEFINED) {
				std::cerr << length << std::endl;

				addFloat4(lon, 1e-7);
				addFloat4(lat, 1e-7);
			}
			else {
				addFloat4Undefined();
				addFloat4Undefined();
			}
			addByte(second << 2 | raim << 1 | accuracy);

			if (cog != COG_UNDEFINED)
				addFloat2(cog * (2.0f * PI) / 360.0f, 1e-4);
			else
				addFloat2Undefined();

			if (speed != SPEED_UNDEFINED)
				addFloat2(speed * 1852.0f / 3600.0f, 1e-2);
			else
				addFloat2Undefined();

			addByte(0);
			addByte(0);
			addByte(0);

			if (heading != HEADING_UNDEFINED)
				addFloat2(heading * (2.0f * PI) / 360.0f, 1e-04); // heading
			else
				addFloat2Undefined();

			addFloat2(turn * (2.0f * PI) / 360.0f / 60.0f, 3.125E-05); // incorrect probably
			addByte(0xf0 | status);
			addByte(0xf0);
			addByte(0xff);
			SendData();
		}

		void Receive(const JSON::JSON* data, int ln, TAG& tag) {
			AIS::Message& ais = *((AIS::Message*)data[0].binary);

			switch (ais.type()) {
			case 1:
			case 2:
			case 3:
				ClassAPositionReport(ais, data);
				break;
			default:
				break;
			}
			return;
		}
	};
#endif
}
