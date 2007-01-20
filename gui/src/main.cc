///
/// \file	main.cc
///		Entry point for barrybackup GUI
///

/*
    Copyright (C) 2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <iostream>
#include <stdexcept>
#include <gtkmm.h>
#include <libglademm.h>
#include <barry/barry.h>
#include "BackupWindow.h"
#include "util.h"

void main_exception_handler()
{
	try {
		throw;
	}
	catch( Glib::Exception &e ) {
		std::cerr << "Glib::Exception caught in main: " << std::endl;
		std::cerr << e.what() << std::endl;
		Gtk::MessageDialog msg(e.what());
		msg.run();
	}
	catch( std::exception &e ) {
		std::cerr << "std::exception caught in main: " << std::endl;
		std::cerr << e.what() << std::endl;
		Gtk::MessageDialog msg(e.what());
		msg.run();
	}
}

int main(int argc, char *argv[])
{
	Barry::Init(true);
	Glib::thread_init();
	Gtk::Main app(argc, argv);
	Glib::add_exception_handler( sigc::ptr_fun(main_exception_handler) );

	try {

		Glib::RefPtr<Gnome::Glade::Xml> refXml = LoadXml("BackupWindow.glade");

		BackupWindow *pWnd = 0;
		refXml->get_widget_derived("BackupWindow", pWnd);
		std::auto_ptr<BackupWindow> apWnd(pWnd);

		Gtk::Main::run(*pWnd);

	}
	catch(...) {
		main_exception_handler();
		return 1;
	}
}
