///
/// \file	ConfigFile.cc
///		BarryBackup GUI configuraion class
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

#include "ConfigFile.h"
#include "util.h"
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <fstream>
#include <sstream>

bool ConfigFile::DBListType::IsSelected(const std::string &dbname) const
{
	const_iterator i = begin();
	for( ; i != end(); ++i ) {
		if( *i == dbname ) {
			return true;
		}
	}
	return false;
}

ConfigFile::ConfigFileError::ConfigFileError(const char *msg, int err)
	: std::runtime_error(std::string(msg) + ": " + strerror(err))
{
}

/// Opens and loads config file for given pin, and calls Enlighten
/// Throws ConfigFileError on error.  Should never fail unless
/// passed a bad pin.
ConfigFile::ConfigFile(const std::string &pin,
		       const Barry::DatabaseDatabase &db)
	: m_pin(pin)
	, m_loaded(false)
{
	if( m_pin.size() == 0 )
		throw ConfigFileError("Configfile: empty pin");

	BuildFilename();
	Load();
	Enlighten(db);
}

ConfigFile::~ConfigFile()
{
}


void ConfigFile::BuildFilename()
{
	struct passwd *pw = getpwuid(getuid());
	if( !pw )
		throw ConfigFileError("BuildFilename: getpwuid failed", errno);

	m_path = pw->pw_dir;
	m_path += "/.barry/backup/";
	m_path += m_pin;
	m_filename = m_path + "/config";
}

void ConfigFile::Clear()
{
	m_loaded = false;
	m_backupList.clear();
	m_restoreList.clear();
}

/// Attempt to load the configuration file, but do not fail if not available
void ConfigFile::Load()
{
	// start fresh
	Clear();

	// open input file
	std::ifstream in(m_filename.c_str(), std::ios::in | std::ios::binary);
	if( !in )
		return;

	std::string line;
	DBListType *pList = 0;

	while( std::getline(in, line) ) {
		std::string keyword;
		std::istringstream iss(line);
		iss >> keyword;

		if( keyword == "backup_list" ) {
			pList = &m_backupList;
		}
		else if( keyword == "restore_list" ) {
			pList = &m_restoreList;
		}
		else if( line[0] == ' ' && pList ) {
			pList->push_back(line.c_str() + 1);
		}
		else {
			pList = 0;

			// add all remaining keyword checks here
		}
	}

	m_loaded = true;
}

/// Checks that the path in m_path exists, and if not, creates it.
/// Returns false if unable to create path, true if ok.
bool ConfigFile::CheckPath()
{
	return ::CheckPath(m_path, &m_last_error);
}

/// Saves current config, overwriting or creating a config file
bool ConfigFile::Save()
{
	if( !CheckPath() )
		return false;

	std::ofstream out(m_filename.c_str(), std::ios::out | std::ios::binary);
	if( !out ) {
		m_last_error = "Unable to open " + m_filename + " for writing.";
		return false;
	}

	out << "backup_list" << std::endl;
	for( DBListType::iterator i = m_backupList.begin(); i != m_backupList.end(); ++i ) {
		out << " " << *i << std::endl;
	}

	out << "restore_list" << std::endl;
	for( DBListType::iterator i = m_restoreList.begin(); i != m_restoreList.end(); ++i ) {
		out << " " << *i << std::endl;
	}

	if( !out ) {
		m_last_error = "Error during write.  Config may be incomplete.";
		return false;
	}
	return true;
}

/// Compares a given databasedatabase from a real device with the
/// current config.  If not yet configured, initialize with valid
/// defaults.
void ConfigFile::Enlighten(const Barry::DatabaseDatabase &db)
{
	if( !m_loaded ) {
		// if not fully loaded, we use db as our default list
		// our defaults are: backup everything, restore everything
		// except email

		m_backupList.clear();
		m_restoreList.clear();

		Barry::DatabaseDatabase::DatabaseArrayType::const_iterator i =
			db.Databases.begin();
		for( ; i != db.Databases.end(); ++i ) {
			// backup everything
			m_backupList.push_back(i->Name);

			// restore everything except email
			if( i->Name != Barry::Message::GetDBName() ) {
				m_restoreList.push_back(i->Name);
			}
		}
	}
}

/// Sets list with new config
void ConfigFile::SetBackupList(const DBListType &list)
{
	m_backupList = list;
	m_loaded = true;
}

void ConfigFile::SetRestoreList(const DBListType &list)
{
	m_restoreList = list;
	m_loaded = true;
}
