///
/// \file	brecsum.cc
///		Generate SHA1 sums of raw Blackberry database records.
///		This is mostly useful for data verification during testing.
///

/*
    Copyright (C) 2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <barry/barry.h>
#include <openssl/sha.h>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <getopt.h>

using namespace std;
using namespace Barry;

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "brecsum - Generate SHA1 sums of raw Blackberry database records.\n"
   << "        Copyright 2008, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "        Using: " << Version << "\n"
   << "\n"
   << "   -d db     Read database 'db' and sum all its records.\n"
   << "             Can be used multiple times to fetch more than one DB\n"
   << "   -h        This help\n"
   << "   -p pin    PIN of device to talk with\n"
   << "             If only one device is plugged in, this flag is optional\n"
   << "   -P pass   Simplistic method to specify device password\n"
   << "   -v        Dump protocol data during operation\n"
   << endl;
}

class ChecksumParser : public Barry::Parser
{
public:
	virtual void ParseFields(const Barry::Data &data, size_t &offset)
	{
		unsigned char sha1[SHA_DIGEST_LENGTH];
		SHA1(data.GetData() + offset, data.GetSize() - offset, sha1);

		for( int i = 0; i < SHA_DIGEST_LENGTH; i++ ) {
			cout << hex << setfill('0') << setw(2)
				<< (unsigned int) sha1[i];
		}
		cout << endl;
	}
};

int main(int argc, char *argv[])
{
	cout.sync_with_stdio(true);	// leave this on, since libusb uses
					// stdio for debug messages

	try {

		uint32_t pin = 0;
		bool
			data_dump = false;
		string password;
		vector<string> dbNames;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "d:hp:P:v");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'd':	// show dbname
				dbNames.push_back(string(optarg));
				break;

			case 'p':	// Blackberry PIN
				pin = strtoul(optarg, NULL, 16);
				break;

			case 'P':	// Device password
				password = optarg;
				break;

			case 'v':	// data dump on
				data_dump = true;
				break;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		// Initialize the barry library.  Must be called before
		// anything else.
		Barry::Init(data_dump);

		// Probe the USB bus for Blackberry devices and display.
		Barry::Probe probe;
		int activeDevice = probe.FindActive(pin);
		if( activeDevice == -1 ) {
			cerr << "No device selected, or PIN not found" << endl;
			return 1;
		}

		// Create our controller object
		Barry::Controller con(probe.Get(activeDevice));
		Barry::Mode::Desktop desktop(con);

		// Sum all specified databases
		if( dbNames.size() ) {
			vector<string>::iterator b = dbNames.begin();
			ChecksumParser parser;

			desktop.Open(password.c_str());
			for( ; b != dbNames.end(); b++ ) {
				unsigned int id = desktop.GetDBID(*b);
				desktop.LoadDatabase(id, parser);
			}
		}

	}
	catch( std::exception &e ) {
		std::cerr << e.what() << endl;
		return 1;
	}

	return 0;
}
