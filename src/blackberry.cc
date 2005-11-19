///
/// \file	blackberry.cc
///		High level BlackBerry API class
///

#include "blackberry.h"
#include "common.h"
#include "protocol.h"
#include "error.h"
#include "data.h"

#define __DEBUG_MODE__
#include "debug.h"

#include <sstream>

#include <iomanip>

namespace Barry {

Blackberry::Blackberry(const ProbeResult &device)
	: m_dev(device.m_dev),
	m_iface(m_dev, BLACKBERRY_INTERFACE),
	m_pin(device.m_pin),
	m_socket(m_dev, WRITE_ENDPOINT, READ_ENDPOINT),
	m_mode(Unspecified)
{
	if( !m_dev.SetConfiguration(BLACKBERRY_CONFIGURATION) )
		throw SBError(m_dev.GetLastError(),
			"Blackberry: SetConfiguration failed");
}

Blackberry::~Blackberry()
{
}

void Blackberry::SelectMode(ModeType mode, uint16_t &socket, uint8_t &flag)
{
	// select mode
	Packet packet;
	packet.socket = 0;
	packet.size = SB_MODE_PACKET_COMMAND_SIZE;
	packet.command = SB_COMMAND_SELECT_MODE;
	packet.data.mode.socket = SB_MODE_REQUEST_SOCKET;
	packet.data.mode.flag = 0x05;	// FIXME
	memset(packet.data.mode.modeName, 0,
		sizeof(packet.data.mode.modeName));

	char *modeName = (char *) packet.data.mode.modeName;
	switch( mode )
	{
	case Bypass:
		strcpy(modeName, "RIM Bypass");
		break;

	case Desktop:
		strcpy(modeName, "RIM Desktop");
		break;

	case JavaLoader:
		strcpy(modeName, "RIM_JavaLoader");
		break;

	default:
		throw std::logic_error("Blackberry: Invalid mode in SelectMode");
		break;
	}

	// send mode command before we open, as a default socket is socket 0
	Data command(&packet, packet.size);
	Data response;
	if( !m_socket.Send(command, response) ) {
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error setting desktop mode");
	}

	// get the data socket number
	// indicates the socket number that
	// should be used below in the Open() call
	CheckSize(response, SB_MODE_PACKET_RESPONSE_SIZE);
	MAKE_PACKET(modepack, response);
	if( modepack->command != SB_COMMAND_MODE_SELECTED ) {
		eeout(command, response);
		throw SBError("Blackberry: mode not selected");
	}

	// return the socket and flag that the device is expecting us to use
	socket = modepack->data.mode.socket;
	flag = modepack->data.mode.flag + 1;
}

void Blackberry::OpenMode(ModeType mode)
{
	uint16_t socket;
	uint8_t flag;

	m_socket.Close();
	SelectMode(mode, socket, flag);
	m_socket.Open(socket, flag);
}

unsigned int Blackberry::GetCommand(CommandType ct)
{
	unsigned int cmd = 0;
	char *cmdName = "Unknown";

	switch( ct )
	{
	case DatabaseAccess:
		cmdName = "Database Access";
		cmd = m_commandTable.GetCommand(cmdName);
		break;
	default:
		throw std::logic_error("Blackberry: unknown command type");
	}

	if( cmd == 0 ) {
		std::ostringstream oss;
		oss << "Blackberry: unable to get command code: " << cmdName;
		throw SBError(oss.str());
	}

	return cmd;
}

template <class Record>
struct StoreFunc
{
	int count;
	StoreFunc() :count(0) {}
	void operator()(const Record &rec)
	{
		count++;
//		std::cout << "count so far: " << count << std::endl;
		std::cout << rec << std::endl;
	}
};

void Blackberry::Test()
{
	// open desktop mode socket
	OpenMode(Desktop);

	// get command table
	LoadCommandTable();

	// get database database
	LoadDBDB();

	// some databases
//	StoreFunc<Contact> store;
//	RecordParser<Contact, StoreFunc<Contact> > contactParser(store);
//	LoadDatabase(GetDBID("Address Book"), contactParser);

	StoreFunc<Message> storeMsg;
	RecordParser<Message, StoreFunc<Message> > messageParser(storeMsg);
	LoadDatabase(GetDBID("Messages"), messageParser);

	// cleanup... not strictly needed, but nice to "release" the device
	m_socket.Close();
}

void Blackberry::LoadCommandTable()
{
	char rawCommand[] = { 6, 0, 0x0a, 0, 0x40, 0, 0, 1, 0, 0 };
	*((uint16_t*) rawCommand) = m_socket.GetSocket();

	Data command(rawCommand, sizeof(rawCommand));
	Data response;
	if( !m_socket.Packet(command, response) ) {
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error getting command table");
	}

	MAKE_PACKET(firstpack, response);
	while( firstpack->command != SB_COMMAND_DB_DONE ) {
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw SBError(m_socket.GetLastStatus(),
				"Blackberry: error getting command table(next)");
		}

		MAKE_PACKET(rpack, response);
		if( rpack->command == SB_COMMAND_DB_DATA && rpack->size > 10 ) {
			// second packet is generally large, and contains
			// the command table
			m_commandTable.Clear();
			m_commandTable.Parse(response, 6);
		}
	}

	ddout(m_commandTable);
}

void Blackberry::LoadDBDB()
{
	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 7;
	packet.command = SB_COMMAND_DB_DATA;
	packet.data.db.tableCmd = GetCommand(DatabaseAccess);
//	packet.data.db.data.db.operation = SB_DBOP_GET_DBDB;
	packet.data.db.data.db.operation = SB_DBOP_OLD_GET_DBDB;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error getting database database");
	}

	MAKE_PACKET(rpack, response);
	while( rpack->command != SB_COMMAND_DB_DONE ) {
		if( rpack->command == SB_COMMAND_DB_DATA ) {
			m_dbdb.Clear();
			m_dbdb.Parse(response);
		}

		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw SBError(m_socket.GetLastStatus(),
				"Blackberry: error getting command table(next)");
		}
		rpack = (const Packet *) response.GetData();
	}

	ddout(m_dbdb);
}

unsigned int Blackberry::GetDBID(const char *name) const
{
	unsigned int ID = m_dbdb.GetDBNumber(name);
	// FIXME - this needs a better error handler... the dbdb needs one too!
	if( ID == 0 ) {
		throw SBError("Blackberry: Address Book not found");
	}
	return ID;
}

void Blackberry::LoadDatabase(unsigned int dbId, Parser &parser)
{
	Packet packet;
	packet.socket = m_socket.GetSocket();
	packet.size = 9;
	packet.command = SB_COMMAND_DB_DATA;
	packet.data.db.tableCmd = GetCommand(DatabaseAccess);
//	packet.data.db.data.db.operation = SB_DBOP_GET_RECORDS;
	packet.data.db.data.db.operation = SB_DBOP_OLD_GET_RECORDS;
	packet.data.db.data.db.databaseId = dbId;

	Data command(&packet, packet.size);
	Data response;

	if( !m_socket.Packet(command, response) ) {
		eout("Database ID: " << dbId);
		eeout(command, response);
		throw SBError(m_socket.GetLastStatus(),
			"Blackberry: error loading database");
	}

	MAKE_PACKET(rpack, response);
	while( rpack->command != SB_COMMAND_DB_DONE ) {
		if( rpack->command == SB_COMMAND_DB_DATA ) {
			parser(response);
		}

		// advance!
		if( !m_socket.NextRecord(response) ) {
			eout("Response packet:\n" << response);
			throw SBError(m_socket.GetLastStatus(),
				"Blackberry: error loading database (next)");
		}
		rpack = (const Packet *) response.GetData();
	}
}


} // namespace Barry

