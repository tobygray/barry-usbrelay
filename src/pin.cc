///
/// \file	pin.cc
///		class for device PIN notation
///

/*
    Copyright (C) 2007-2010, Net Direct Inc. (http://www.netdirect.ca/)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License in the COPYING file at the
    root directory of this project for more details.
*/

#include <sstream>
#include "pin.h"

namespace Barry {

std::ostream& operator<<(std::ostream &os, const Pin &pin)
{
	os << pin.str();
	return os;
}

std::istream& operator>>(std::istream &is, Pin &pin)
{
	uint32_t newpin;
	is >> std::hex >> newpin;
	if( is )
		pin = newpin;
	return is;
}

std::string Pin::str() const
{
	std::ostringstream oss;
	oss << std::hex << pin;
	return oss.str();
}

}

