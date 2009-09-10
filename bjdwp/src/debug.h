///
/// \file	debug.h
///		Debug output functions and macros
///

/*
    Copyright (C) 2009, Nicolas VIVIEN

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

#ifndef __BARRYJDWP_DEBUG_H__
#define __BARRYJDWP_DEBUG_H__


#define jdwplog(x)	if(Barry::LogVerbose()) { Barry::LogLock lock; (*Barry::GetLogStream()) << x << std::endl; }

#endif

