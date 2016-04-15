/*
 *   Copyright (C) 2016 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "HD44780.h"
#include "Log.h"

#include <wiringPi.h>
#include <lcd.h>

#include <cstdio>
#include <cassert>

const char* LISTENING = "Listening                                      ";

CHD44780::CHD44780(unsigned int rows, unsigned int cols, const char* callsign, unsigned int dmrid, const std::vector<unsigned int>& pins) :
m_rows(rows),
m_cols(cols),
m_callsign(callsign),
m_dmrid(dmrid),
m_rb(pins.at(0U)),
m_strb(pins.at(1U)),
m_d0(pins.at(2U)),
m_d1(pins.at(3U)),
m_d2(pins.at(4U)),
m_d3(pins.at(5U)),
m_fd(-1),
m_dmr(false)
{
	assert(rows > 1U);
	assert(cols > 15U);
}

CHD44780::~CHD44780()
{
}

bool CHD44780::open()
{
	::wiringPiSetup();

	m_fd = ::lcdInit(m_rows, m_cols, 4, m_rb, m_strb, m_d0, m_d1, m_d2, m_d3, 0, 0, 0, 0);
	if (m_fd == -1) {
		LogError("Unable to open the HD44780");
		return false;
	}

	::lcdDisplay(m_fd, 1);
	::lcdCursor(m_fd, 0);
	::lcdCursorBlink(m_fd, 0);

	return true;
}

void CHD44780::setIdle()
{
	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPrintf(m_fd, "%-6s / %u", m_callsign, m_dmrid);

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "MMDVM Idle");

	m_dmr = false;
}

void CHD44780::setError(const char* text)
{
	assert(text != NULL);

	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "MMDVM");

	::lcdPosition(m_fd, 0, 1);
	::lcdPrintf(m_fd, "%s ERROR", text);

	m_dmr = false;
}

void CHD44780::setLockout()
{
	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "MMDVM");

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "Lockout");

	m_dmr = false;
}

void CHD44780::writeDStar(const char* my1, const char* my2, const char* your, const char* type)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);

	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "D-Star");

	if (m_rows > 2U) {
		char buffer[40U];

		::sprintf(buffer, "%s %.8s/%.4s >", type, my1, my2);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);

		::sprintf(buffer, "%.8s", your);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	} else {
		char buffer[40U];
		::sprintf(buffer, "%s %.8s/%.4s > %.8s", type, my1, my2, your);

		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	}

	m_dmr = false;
}

void CHD44780::clearDStar()
{
	if (m_rows > 2U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "        ");
	} else {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	}
}

void CHD44780::writeDMR(unsigned int slotNo, const char* src, bool group, const char* dst, const char* type)
{
	assert(src != NULL);
	assert(type != NULL);

	if (!m_dmr) {
		::lcdClear(m_fd);

		if (m_rows > 2U) {
			::lcdPosition(m_fd, 0, 0);
			::lcdPuts(m_fd, "DMR");
		}

		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, m_rows > 2U ? 2 : 1);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
		} else {
			::lcdPosition(m_fd, 0, m_rows > 2U ? 1 : 0);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
		}
	}

	if (slotNo == 1U) {
		char buffer[40U];
		if (m_cols > 16U)
			::sprintf(buffer, "%s %s > %s%s", type, src, group ? "TG" : "", dst);
		else
			::sprintf(buffer, "%s > %s%s", src, group ? "TG" : "", dst);

		::lcdPosition(m_fd, 0, m_rows > 2U ? 1 : 0);
		::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, buffer);
	} else {
		char buffer[40U];
		if (m_cols > 16U)
			::sprintf(buffer, "%s %s > %s%s", type, src, group ? "TG" : "", dst);
		else
			::sprintf(buffer, "%s > %s%s", src, group ? "TG" : "", dst);

		::lcdPosition(m_fd, 0, m_rows > 2U ? 2 : 1);
		::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, buffer);
	}

	m_dmr = true;
}

void CHD44780::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U) {
		::lcdPosition(m_fd, 0, m_rows > 2U ? 1 : 0);
		::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
	} else {
		::lcdPosition(m_fd, 0, m_rows > 2U ? 2 : 1);
		::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
	}
}

void CHD44780::writeFusion(const char* source, const char* dest)
{
	assert(source != NULL);
	assert(dest != NULL);

	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "System Fusion");

	if (m_rows > 2U) {
		char buffer[40U];

		::sprintf(buffer, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);

		::sprintf(buffer, "%.10s", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	} else {
		char buffer[40U];
		::sprintf(buffer, "%.10s > %.10s", source, dest);

		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	}

	m_dmr = false;
}

void CHD44780::clearFusion()
{
	if (m_rows > 2U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "");
	} else {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	}
}

void CHD44780::close()
{
}
