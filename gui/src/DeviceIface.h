///
/// \file	DeviceIface.h
///		Interface class for device backup and restore
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

#ifndef __BARRYBACKUP_DEVICEIFACE_H__
#define __BARRYBACKUP_DEVICEIFACE_H__

#include <barry/barry.h>
#include <string>
#include <memory>
#include <stdint.h>
#include "ConfigFile.h"
#include "tarfile.h"

#define DI_THREAD_DONE 100
#define DI_THREAD_PROGRESS 101

namespace Glib {
	class Dispatcher;
	class Mutex;
}

class DeviceInterface :
	public Barry::Parser,
	public Barry::Builder
{
public:
	struct AppComm				// app communication
	{
		Glib::Dispatcher *m_erase_db;	// to notify the app about the
						// db erase stage of restore
		Glib::Dispatcher *m_progress;
		Glib::Dispatcher *m_error_done;
		Glib::Dispatcher *m_done;

		AppComm() :
			m_erase_db(0),
			m_progress(0),
			m_error_done(0),
			m_done(0)
			{}
		AppComm(Glib::Dispatcher *progress,
			Glib::Dispatcher *error,
			Glib::Dispatcher *done,
			Glib::Dispatcher *erase_db) :
			m_erase_db(erase_db),
			m_progress(progress),
			m_error_done(error),
			m_done(done)
			{}
		bool IsValid() const
			{ return m_erase_db && m_progress && m_error_done && m_done; }
		void Invalidate()
			{ m_erase_db = m_progress = m_error_done = m_done = 0; }
	};

	class Quit	// quit exception to break out of upload/download
	{
	};

private:
	Barry::Controller *m_con;
	std::string m_last_error;
	std::string m_last_thread_error;

	AppComm m_AppComm;
	std::auto_ptr<reuse::TarFile> m_tar;

	// parser and builder data (only one side uses these at a time)
	ConfigFile::DBListType m_dbList;
	mutable Glib::Mutex *m_dbnameMutex;
	std::string m_current_dbname_not_thread_safe;
	std::string m_current_dbname;
	uint32_t m_unique_id;
	std::string m_unique_id_text;
	std::string m_record_data;
	bool m_end_of_tar;
	bool m_tar_record_loaded;

	// thread quit flag... not locked, only a byte
	volatile bool m_thread_quit;

protected:
	bool False(const std::string &msg);

	void BackupThread();
	void RestoreThread();

	// helpers
	std::string MakeFilename(const std::string &pin);
	int CountFiles(reuse::TarFile &tar, const ConfigFile::DBListType &restoreList);
	bool SplitTarPath(const std::string &tarpath, std::string &dbname,
		std::string &dbid_text, uint32_t &dbid);

	// Sets the name of the database the thread is currently working on
	void SetThreadDBName(const std::string &dbname);

public:
	DeviceInterface();
	~DeviceInterface();

	const std::string& get_last_error() const { return m_last_error; }
	const std::string& get_last_thread_error() const { return m_last_thread_error; }

	bool Connect(const Barry::ProbeResult &dev);
	void Disconnect();

	const Barry::DatabaseDatabase& GetDBDB() const { return m_con->GetDBDB(); }
	int GetDeviceRecordTotal(const ConfigFile::DBListType &backupList) const;

	void QuitThread()	{ m_thread_quit = true; }

	/// returns name of database the thread is currently working on
	std::string GetThreadDBName() const;

	bool StartBackup(AppComm comm,
		const ConfigFile::DBListType &backupList,
		const std::string &directory, const std::string &pin);
	bool StartRestore(AppComm comm,
		const ConfigFile::DBListType &backupList,
		const std::string &tarfilename, int *pRecordCount = 0);

	// Barry::Parser overrides
	virtual void SetUniqueId(uint32_t Id);
	virtual void ParseFields(const Barry::Data &data, size_t &offset);
	virtual void Store();

	// Barry::Builder overrides
	virtual bool Retrieve(unsigned int dbId);
	virtual uint32_t GetUniqueId() const;
	virtual void BuildHeader(Barry::Data &data, size_t &offset);
	virtual void BuildFields(Barry::Data &data, size_t &offset);
};

#endif
