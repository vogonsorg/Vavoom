//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	$Id$
//**
//**	Copyright (C) 1999-2006 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	Datagram driver, handles all LAN drivers
//**
//**************************************************************************
//
//	This is the network info/connection protocol. It is used to find Vavoom
// servers, get info about them, and connect to them. Once connected, the
// Vavoom game protocol (documented elsewhere) is used.
//
//
// CCREQ_CONNECT
//		string		"VAVOOM"
//		vuint8		net_protocol_version	NET_PROTOCOL_VERSION
//
// CCREQ_SERVER_INFO
//		string		"VAVOOM"
//		vuint8		net_protocol_version	NET_PROTOCOL_VERSION
//
//
//
// CCREP_ACCEPT
//		long		port
//
// CCREP_REJECT
//		string		reason
//
// CCREP_SERVER_INFO
//		string		host_name
//		string		level_name
//		vuint8		current_players
//		vuint8		max_players
//		vuint8		protocol_version		NET_PROTOCOL_VERSION
//		string[]	wad_files				empty string terminated
//
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "gamedefs.h"
#include "net_local.h"
#include "zlib.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

class VDatagramSocket : public VSocket
{
public:
	VNetLanDriver*	LanDriver;
	int				LanSocket;
	sockaddr_t		Addr;
	bool			Invalid;

	VDatagramSocket(VNetDriver* Drv)
	: VSocket(Drv)
	, LanDriver(NULL)
	, LanSocket(0)
	, Invalid(false)
	{}
	~VDatagramSocket();

	int GetMessage(TArray<vuint8>&);
	int SendMessage(vuint8*, vuint32);
	bool IsLocalConnection();
};

class VDatagramDriver : public VNetDriver
{
public:
	// I don't think that communication protocol will change, but just in a
	// case
	enum { NET_PROTOCOL_VERSION = 1 };

	enum { MASTER_SERVER_PORT = 26001 };

	//	Client request
	enum
	{
		CCREQ_CONNECT			= 1,
		CCREQ_SERVER_INFO		= 2
	};

	//	Server reply
	enum
	{
		CCREP_ACCEPT			= 11,
		CCREP_REJECT			= 12,
		CCREP_SERVER_INFO		= 13
	};

	//	Master server request
	enum
	{
		MCREQ_JOIN				= 1,
		MCREQ_QUIT				= 2,
		MCREQ_LIST				= 3,
	};

	//	Master server reply
	enum
	{
		MCREP_LIST				= 1,
	};

	struct
	{
		vuint8		data[MAX_MSGLEN];
	} packetBuffer;

	VDatagramDriver();
	int Init();
	void Listen(bool);
	void SearchForHosts(bool, bool);
	VSocket* Connect(const char*);
	VSocket* CheckNewConnections();
	void UpdateMaster();
	void QuitMaster();
	bool QueryMaster(bool);
	void EndQueryMaster();
	void Shutdown();

	void SearchForHosts(VNetLanDriver*, bool, bool);
	VSocket* Connect(VNetLanDriver*, const char*);
	VSocket* CheckNewConnections(VNetLanDriver*);
	void UpdateMaster(VNetLanDriver*);
	void QuitMaster(VNetLanDriver*);
	bool QueryMaster(VNetLanDriver*, bool);
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int			num_connected;
extern TArray<VStr>	wadfiles;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static VCvarI			UseMaster("use_master", "0", CVAR_Archive);
static VCvarS			MasterSrv("master_srv", "127.0.0.1", CVAR_Archive);

static VDatagramDriver	Impl;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	VDatagramDriver::VDatagramDriver
//
//==========================================================================

VDatagramDriver::VDatagramDriver()
: VNetDriver(1, "Datagram")
{
	memset(&packetBuffer, 0, sizeof(packetBuffer));
}

//==========================================================================
//
//	VDatagramDriver::Init
//
//==========================================================================

int VDatagramDriver::Init()
{
	guard(Datagram_Init);
	if (GArgs.CheckParm("-nolan"))
		return -1;

	for (int i = 0; i < VNetworkLocal::NumLanDrivers; i++)
	{
		VNetworkLocal::LanDrivers[i]->Net = Net;
		int csock = VNetworkLocal::LanDrivers[i]->Init();
		if (csock == -1)
			continue;
		VNetworkLocal::LanDrivers[i]->initialised = true;
		VNetworkLocal::LanDrivers[i]->controlSock = csock;
	}

	return 0;
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::Listen
//
//==========================================================================

void VDatagramDriver::Listen(bool state)
{
	guard(VDatagramDriver::Listen);
	for (int i = 0; i < VNetworkLocal::NumLanDrivers; i++)
		if (VNetworkLocal::LanDrivers[i]->initialised)
			VNetworkLocal::LanDrivers[i]->Listen(state);
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::SearchForHosts
//
//==========================================================================

void VDatagramDriver::SearchForHosts(VNetLanDriver* Drv, bool xmit,
	bool ForMaster)
{
	guard(VDatagramDriver::SearchForHosts);
	sockaddr_t	myaddr;
	sockaddr_t	readaddr;
	int			len;
	vuint8		control;
	vuint8		msgtype;
	int			n;
	int			i;
	vuint8		TmpByte;

	Drv->GetSocketAddr(Drv->controlSock, &myaddr);
	if (xmit)
	{
		VBitStreamWriter Reply(256 << 3);
		TmpByte = NETPACKET_CTL;
		Reply << TmpByte;
		TmpByte = CCREQ_SERVER_INFO;
		Reply << TmpByte;
		VStr GameName("VAVOOM");
		Reply << GameName;
		TmpByte = NET_PROTOCOL_VERSION;
		Reply << TmpByte;
		Drv->Broadcast(Drv->controlSock, Reply.GetData(), Reply.GetNumBytes());
	}

	while ((len = Drv->Read(Drv->controlSock, packetBuffer.data, MAX_MSGLEN, &readaddr)) > 0)
	{
		if (len < (int)sizeof(int))
		{
			continue;
		}

		// don't answer our own query
		if (!ForMaster && Drv->AddrCompare(&readaddr, &myaddr) >= 0)
		{
			continue;
		}

		// is the cache full?
		if (Net->HostCacheCount == HOSTCACHESIZE)
		{
			continue;
		}

		VBitStreamReader msg(packetBuffer.data, len << 3);
		msg << control;
		if (control != NETPACKET_CTL)
		{
			continue;
		}

		msg << msgtype;
		if (msgtype != CCREP_SERVER_INFO)
		{
			continue;
		}

		char*			addr;
		VStr			str;

		addr = Drv->AddrToString(&readaddr);

		// search the cache for this server
		for (n = 0; n < Net->HostCacheCount; n++)
		{
			if (Net->HostCache[n].CName == addr)
			{
				break;
			}
		}

		// is it already there?
		if (n < Net->HostCacheCount)
		{
			continue;
		}

		// add it
		Net->HostCacheCount++;
		msg << str;
		Net->HostCache[n].Name = str;
		msg << str;
		Net->HostCache[n].Map = str;
		msg << TmpByte;
		Net->HostCache[n].Users = TmpByte;
		msg << TmpByte;
		Net->HostCache[n].MaxUsers = TmpByte;
		msg << TmpByte;
		if (TmpByte != NET_PROTOCOL_VERSION)
		{
			Net->HostCache[n].Name = VStr("*") + Net->HostCache[n].Name;
		}
		Net->HostCache[n].CName = addr;
		i = 0;
		do
		{
			msg << str;
			Net->HostCache[n].WadFiles[i++] = str;
		}  while (str.IsNotEmpty());

		// check for a name conflict
		for (i = 0; i < Net->HostCacheCount; i++)
		{
			if (i == n)
			{
				continue;
			}
			if (Net->HostCache[n].Name.ICmp(Net->HostCache[i].Name) == 0)
			{
				i = Net->HostCache[n].Name.Length();
				if (i < 15 && Net->HostCache[n].Name[i - 1] > '8')
				{
					Net->HostCache[n].Name += '0';
				}
				else
				{
					Net->HostCache[n].Name[i - 1]++;
				}
				i = -1;
			}
		}
	}
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::SearchForHosts
//
//==========================================================================

void VDatagramDriver::SearchForHosts(bool xmit, bool ForMaster)
{
	guard(Datagram_SearchForHosts);
	for (int i = 0; i < VNetworkLocal::NumLanDrivers; i++)
	{
		if (Net->HostCacheCount == HOSTCACHESIZE)
			break;
		if (VNetworkLocal::LanDrivers[i]->initialised)
			SearchForHosts(VNetworkLocal::LanDrivers[i], xmit, ForMaster);
	}
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::Connect
//
//==========================================================================

VSocket* VDatagramDriver::Connect(VNetLanDriver* Drv, const char* host)
{
	guard(VDatagramDriver::Connect);
#ifdef CLIENT
	sockaddr_t			sendaddr;
	sockaddr_t			readaddr;
	VDatagramSocket*	sock;
	int					newsock;
	double				start_time;
	int					reps;
	int					ret;
	vuint8				control;
	VStr				reason;
	vuint8				msgtype;
	int					newport;
	VBitStreamReader*	msg = NULL;
	vuint8				TmpByte;

	// see if we can resolve the host name
	if (Drv->GetAddrFromName(host, &sendaddr, Net->HostPort) == -1)
		return NULL;

	newsock = Drv->OpenSocket(0);
	if (newsock == -1)
		return NULL;

	sock = new VDatagramSocket(this);
	sock->LanSocket = newsock;
	sock->LanDriver = Drv;

	// connect to the host
	if (Drv->Connect(newsock, &sendaddr) == -1)
		goto ErrorReturn;

	// send the connection request
	GCon->Log("trying..."); SCR_Update();
	start_time = Net->NetTime;

	for (reps = 0; reps < 3; reps++)
	{
		VBitStreamWriter MsgOut(256 << 3);
		// save space for the header, filled in later
		TmpByte = NETPACKET_CTL;
		MsgOut << TmpByte;
		TmpByte = CCREQ_CONNECT;
		MsgOut << TmpByte;
		VStr GameName("VAVOOM");
		MsgOut << GameName;
		TmpByte = NET_PROTOCOL_VERSION;
		MsgOut << TmpByte;
		Drv->Write(newsock, MsgOut.GetData(), MsgOut.GetNumBytes(), &sendaddr);

		do
		{
			ret = Drv->Read(newsock, packetBuffer.data, MAX_MSGLEN, &readaddr);
			// if we got something, validate it
			if (ret > 0)
			{
				// is it from the right place?
				if (sock->LanDriver->AddrCompare(&readaddr, &sendaddr) != 0)
				{
					ret = 0;
					continue;
				}

				if (ret < (int)sizeof(vint32))
				{
					ret = 0;
					continue;
				}

				msg = new VBitStreamReader(packetBuffer.data, ret << 3);

				*msg << control;
				if (control !=  NETPACKET_CTL)
				{
					ret = 0;
					delete msg;
					msg = NULL;
					continue;
				}
			}
		}
		while (ret == 0 && (Net->SetNetTime() - start_time) < 2.5);
		if (ret)
			break;
		GCon->Log("still trying..."); SCR_Update();
		start_time = Net->SetNetTime();
	}

	if (ret == 0)
	{
		reason = "No Response";
		GCon->Log(reason);
		VStr::Cpy(Net->ReturnReason, *reason);
		goto ErrorReturn;
	}

	if (ret == -1)
	{
		reason = "Network Error";
		GCon->Log(reason);
		VStr::Cpy(Net->ReturnReason, *reason);
		goto ErrorReturn;
	}

	*msg << msgtype;
	if (msgtype == CCREP_REJECT)
	{
		*msg << reason;
		GCon->Log(reason);
		VStr::NCpy(Net->ReturnReason, *reason, 31);
		goto ErrorReturn;
	}

	if (msgtype != CCREP_ACCEPT)
	{
		reason = "Bad Response";
		GCon->Log(reason);
		VStr::Cpy(Net->ReturnReason, *reason);
		goto ErrorReturn;
	}

	*msg << newport;

	memcpy(&sock->Addr, &readaddr, sizeof(sockaddr_t));
	Drv->SetSocketPort(&sock->Addr, newport);

	sock->Address = Drv->GetNameFromAddr(&sendaddr);

	GCon->Log("Connection accepted");
	sock->LastMessageTime = Net->SetNetTime();

	// switch the connection to the specified address
	if (Drv->Connect(newsock, &sock->Addr) == -1)
	{
		reason = "Connect to Game failed";
		GCon->Log(reason);
		VStr::Cpy(Net->ReturnReason, *reason);
		goto ErrorReturn;
	}

	delete msg;
	msg = NULL;
//	m_return_onerror = false;
	return sock;

ErrorReturn:
	delete sock;
	sock = NULL;
	Drv->CloseSocket(newsock);
	if (msg)
	{
		delete msg;
		msg = NULL;
	}
//	if (m_return_onerror)
//	{
//		key_dest = key_menu;
//		m_state = m_return_state;
//		m_return_onerror = false;
//	}
#endif
	return NULL;
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::Connect
//
//==========================================================================

VSocket* VDatagramDriver::Connect(const char* host)
{
	guard(Datagram_Connect);
	for (int i = 0; i < VNetworkLocal::NumLanDrivers; i++)
	{
		if (VNetworkLocal::LanDrivers[i]->initialised)
		{
			VSocket* ret = Connect(VNetworkLocal::LanDrivers[i], host);
			if (ret)
			{
				return ret;
			}
		}
	}
	return NULL;
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::CheckNewConnections
//
//==========================================================================

VSocket* VDatagramDriver::CheckNewConnections(VNetLanDriver* Drv)
{
	guard(VDatagramDriver::CheckNewConnections);
#ifdef SERVER
	sockaddr_t	clientaddr;
	sockaddr_t	newaddr;
	int			acceptsock;
	int			newsock;
	int			len;
	vuint8		control;
	vuint8		command;
	VDatagramSocket*	sock;
	int			ret;
	VStr		gamename;
	vuint8		TmpByte;
	VStr		TmpStr;

	acceptsock = Drv->CheckNewConnections();
	if (acceptsock == -1)
		return NULL;

	len = Drv->Read(acceptsock, packetBuffer.data, MAX_MSGLEN, &clientaddr);
	if (len < (int)sizeof(vint32))
		return NULL;
	VBitStreamReader msg(packetBuffer.data, len << 3);

	msg << control;
	if (control != NETPACKET_CTL)
		return NULL;

	msg << command;
	if (command == CCREQ_SERVER_INFO)
	{
		msg << gamename;
		if (gamename != "VAVOOM")
			return NULL;

		VBitStreamWriter MsgOut(MAX_MSGLEN << 3);
		TmpByte = NETPACKET_CTL;
		MsgOut << TmpByte;
		TmpByte = CCREP_SERVER_INFO;
		MsgOut << TmpByte;
		TmpStr = VNetworkLocal::HostName;
		MsgOut << TmpStr;
		TmpStr = *GLevel->MapName;
		MsgOut << TmpStr;
		TmpByte = svs.num_connected;
		MsgOut << TmpByte;
		TmpByte = svs.max_clients;
		MsgOut << TmpByte;
		TmpByte = NET_PROTOCOL_VERSION;
		MsgOut << TmpByte;
		for (int i = 0; i < wadfiles.Num(); i++)
		{
			TmpStr = wadfiles[i];
			MsgOut << TmpStr;
		}
		TmpStr = "";
		MsgOut << TmpStr;

		Drv->Write(acceptsock, MsgOut.GetData(), MsgOut.GetNumBytes(), &clientaddr);
		return NULL;
	}

	if (command != CCREQ_CONNECT)
		return NULL;

	msg << gamename;
	if (gamename != "VAVOOM")
		return NULL;

/*	if (MSG_ReadByte() != NET_PROTOCOL_VERSION)
	{
		SZ_Clear(&net_message);
		// save space for the header, filled in later
		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREP_REJECT);
		MSG_WriteString(&net_message, "Incompatible version.\n");
		*((int *)net_message.data) = BigLong(NETPACKET_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		Drv.Write (acceptsock, net_message.data, net_message.cursize, &clientaddr);
		SZ_Clear(&net_message);
		return NULL;
	}
*/
	// see if this guy is already connected
	for (VSocket* as = Net->ActiveSockets; as; as = as->Next)
	{
		if (as->Driver != this)
			continue;
		VDatagramSocket* s = (VDatagramSocket*)as;
		ret = Drv->AddrCompare(&clientaddr, &s->Addr);
		if (ret >= 0)
		{
			// is this a duplicate connection reqeust?
			if (ret == 0 && Net->NetTime - s->ConnectTime < 2.0)
			{
				// yes, so send a duplicate reply
				VBitStreamWriter MsgOut(32 << 3);
				Drv->GetSocketAddr(s->LanSocket, &newaddr);
				TmpByte = NETPACKET_CTL;
				MsgOut << TmpByte;
				TmpByte = CCREP_ACCEPT;
				MsgOut << TmpByte;
				vint32 TmpPort = Drv->GetSocketPort(&newaddr);
				MsgOut << TmpPort;
				Drv->Write(acceptsock, MsgOut.GetData(), MsgOut.GetNumBytes(), &clientaddr);
				return NULL;
			}
			// it's somebody coming back in from a crash/disconnect
			// so close the old socket and let their retry get them back in
			s->Invalid = true;
			return NULL;
		}
	}

	if (svs.num_connected >= svs.max_clients)
	{
		// no room; try to let him know
		VBitStreamWriter MsgOut(256 << 3);
		TmpByte = NETPACKET_CTL;
		MsgOut << TmpByte;
		TmpByte = CCREP_REJECT;
		MsgOut << TmpByte;
		TmpStr = "Server is full.\n";
		MsgOut << TmpStr;
		Drv->Write(acceptsock, MsgOut.GetData(), MsgOut.GetNumBytes(), &clientaddr);
		return NULL;
	}

	// allocate a network socket
	newsock = Drv->OpenSocket(0);
	if (newsock == -1)
	{
		return NULL;
	}

	// connect to the client
	if (Drv->Connect(newsock, &clientaddr) == -1)
	{
		Drv->CloseSocket(newsock);
		return NULL;
	}

	// allocate a VSocket
	// everything is allocated, just fill in the details	
	sock = new VDatagramSocket(this);
	sock->LanSocket = newsock;
	sock->LanDriver = Drv;
	sock->Addr = clientaddr;
	sock->Address = Drv->AddrToString(&clientaddr);

	Drv->GetSocketAddr(newsock, &newaddr);

	// send him back the info about the server connection he has been allocated
	VBitStreamWriter MsgOut(32 << 3);
	TmpByte = NETPACKET_CTL;
	MsgOut << TmpByte;
	TmpByte = CCREP_ACCEPT;
	MsgOut << TmpByte;
	vint32 TmpPort = Drv->GetSocketPort(&newaddr);
	MsgOut << TmpPort;
	Drv->Write(acceptsock, MsgOut.GetData(), MsgOut.GetNumBytes(), &clientaddr);

	return sock;
#else
	return NULL;
#endif
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::CheckNewConnections
//
//==========================================================================

VSocket* VDatagramDriver::CheckNewConnections()
{
	guard(VDatagramDriver::CheckNewConnections);
	for (int i = 0; i < Net->NumLanDrivers; i++)
	{
		if (Net->LanDrivers[i]->initialised)
		{
			VSocket* ret = CheckNewConnections(Net->LanDrivers[i]);
			if (ret != NULL)
				return ret;
		}
	}
	return NULL;
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::UpdateMaster
//
//==========================================================================

void VDatagramDriver::UpdateMaster(VNetLanDriver* Drv)
{
	guard(VDatagramDriver::UpdateMaster);
	sockaddr_t			sendaddr;

	// see if we can resolve the host name
	if (Drv->GetAddrFromName(MasterSrv, &sendaddr, MASTER_SERVER_PORT) == -1)
	{
		GCon->Logf("Could not resolve server name");
		return;
	}

	if (Drv->net_acceptsocket == -1)
	{
		GCon->Logf("Listen socket not open");
		return;
	}

	// send the connection request
	VBitStreamWriter MsgOut(256 << 3);
	vuint8 TmpByte = MCREQ_JOIN;
	MsgOut << TmpByte;
	Drv->Write(Drv->net_acceptsocket, MsgOut.GetData(), MsgOut.GetNumBytes(),
		&sendaddr);
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::UpdateMaster
//
//==========================================================================

void VDatagramDriver::UpdateMaster()
{
	guard(VDatagramDriver::UpdateMaster);
	if (!UseMaster)
	{
		return;
	}

	for (int i = 0; i < VNetworkLocal::NumLanDrivers; i++)
	{
		if (VNetworkLocal::LanDrivers[i]->initialised)
		{
			UpdateMaster(VNetworkLocal::LanDrivers[i]);
		}
	}
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::QuitMaster
//
//==========================================================================

void VDatagramDriver::QuitMaster(VNetLanDriver* Drv)
{
	guard(VDatagramDriver::QuitMaster);
	sockaddr_t			sendaddr;

	// see if we can resolve the host name
	if (Drv->GetAddrFromName(MasterSrv, &sendaddr, MASTER_SERVER_PORT) == -1)
	{
		GCon->Logf("Could not resolve server name");
		return;
	}

	if (Drv->net_acceptsocket == -1)
	{
		GCon->Logf("Listen socket not open");
		return;
	}

	// send the quit request
	VBitStreamWriter MsgOut(256 << 3);
	vuint8 TmpByte = MCREQ_QUIT;
	MsgOut << TmpByte;
	Drv->Write(Drv->net_acceptsocket, MsgOut.GetData(), MsgOut.GetNumBytes(),
		&sendaddr);
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::QuitMaster
//
//==========================================================================

void VDatagramDriver::QuitMaster()
{
	guard(VDatagramDriver::QuitMaster);
	if (!UseMaster)
	{
		return;
	}

	for (int i = 0; i < VNetworkLocal::NumLanDrivers; i++)
	{
		if (VNetworkLocal::LanDrivers[i]->initialised)
		{
			QuitMaster(VNetworkLocal::LanDrivers[i]);
		}
	}
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::QueryMaster
//
//==========================================================================

bool VDatagramDriver::QueryMaster(VNetLanDriver* Drv, bool xmit)
{
	guard(VDatagramDriver::QueryMaster);
	sockaddr_t	myaddr;
	sockaddr_t	readaddr;
	sockaddr_t	tmpaddr;
	int			len;
	vuint8		control;
	vuint8		TmpByte;

	if (Drv->MasterQuerySocket < 0)
	{
		Drv->MasterQuerySocket = Drv->OpenSocket(0);
	}
	Drv->GetSocketAddr(Drv->MasterQuerySocket, &myaddr);
	if (xmit)
	{
		sockaddr_t			sendaddr;
		// see if we can resolve the host name
		if (Drv->GetAddrFromName(MasterSrv, &sendaddr, MASTER_SERVER_PORT) == -1)
		{
			GCon->Logf("Could not resolve server name");
			return false;
		}
		// send the query request
		VBitStreamWriter MsgOut(256 << 3);
		vuint8 TmpByte = MCREQ_LIST;
		MsgOut << TmpByte;
		Drv->Write(Drv->MasterQuerySocket, MsgOut.GetData(),
			MsgOut.GetNumBytes(), &sendaddr);
		return false;
	}

	while ((len = Drv->Read(Drv->MasterQuerySocket, packetBuffer.data,
		MAX_MSGLEN, &readaddr)) > 0)
	{
		if (len < 1)
		{
			continue;
		}

		// is the cache full?
		if (Net->HostCacheCount == HOSTCACHESIZE)
		{
			continue;
		}

		VBitStreamReader msg(packetBuffer.data, len << 3);
		msg << control;
		if (control != MCREP_LIST)
		{
			continue;
		}

		while (!msg.AtEnd())
		{
			tmpaddr = readaddr;
			msg.Serialise(tmpaddr.sa_data + 2, 4);
			msg.Serialise(tmpaddr.sa_data, 2);

			VBitStreamWriter Reply(256 << 3);
			TmpByte = NETPACKET_CTL;
			Reply << TmpByte;
			TmpByte = CCREQ_SERVER_INFO;
			Reply << TmpByte;
			VStr GameName("VAVOOM");
			Reply << GameName;
			TmpByte = NET_PROTOCOL_VERSION;
			Reply << TmpByte;
			Drv->Write(Drv->controlSock, Reply.GetData(),
				Reply.GetNumBytes(), &tmpaddr);
		}
		return true;
	}
	return false;
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::QueryMaster
//
//==========================================================================

bool VDatagramDriver::QueryMaster(bool xmit)
{
	guard(VDatagramDriver::QueryMaster);
	for (int i = 0; i < VNetworkLocal::NumLanDrivers; i++)
	{
		if (Net->HostCacheCount == HOSTCACHESIZE)
		{
			break;
		}
		if (VNetworkLocal::LanDrivers[i]->initialised)
		{
			if (QueryMaster(VNetworkLocal::LanDrivers[i], xmit))
			{
				return true;
			}
		}
	}
	return false;
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::EndQueryMaster
//
//==========================================================================

void VDatagramDriver::EndQueryMaster()
{
	guard(VDatagramDriver::EndQueryMaster);
	for (int i = 0; i < VNetworkLocal::NumLanDrivers; i++)
	{
		if (VNetworkLocal::LanDrivers[i]->initialised &&
			VNetworkLocal::LanDrivers[i]->MasterQuerySocket > 0)
		{
			VNetworkLocal::LanDrivers[i]->CloseSocket(
				VNetworkLocal::LanDrivers[i]->MasterQuerySocket);
			VNetworkLocal::LanDrivers[i]->MasterQuerySocket = -1;
		}
	}
	unguard;
}

//==========================================================================
//
//	VDatagramDriver::Shutdown
//
//==========================================================================

void VDatagramDriver::Shutdown()
{
	guard(VDatagramDriver::Shutdown);
	//
	// shutdown the lan drivers
	//
	for (int i = 0; i < Net->NumLanDrivers; i++)
	{
		if (Net->LanDrivers[i]->initialised)
		{
			Net->LanDrivers[i]->Shutdown();
			Net->LanDrivers[i]->initialised = false;
		}
	}
	unguard;
}

//==========================================================================
//
//	VDatagramSocket::~VDatagramSocket
//
//==========================================================================

VDatagramSocket::~VDatagramSocket()
{
	guard(VDatagramSocket::~VDatagramSocket);
	LanDriver->CloseSocket(LanSocket);
	unguard;
}

//==========================================================================
//
//	VDatagramSocket::GetMessage
//
//	If there is a packet, return it.
//
//	returns 0 if no data is waiting
//	returns 1 if a packet was received
//	returns -1 if connection is invalid
//
//==========================================================================

int VDatagramSocket::GetMessage(TArray<vuint8>& Data)
{
	guard(VDatagramSocket::GetMessage);
	vuint32		length;
	sockaddr_t	readaddr;
	int			ret = 0;

	if (Invalid)
	{
		return -1;
	}

	struct
	{
		vuint8		data[MAX_MSGLEN];
	} packetBuffer;

	while(1)
	{
		//	Read message.
		length = LanDriver->Read(LanSocket, (vuint8*)&packetBuffer,
			NET_DATAGRAMSIZE, &readaddr);

		if (length == 0)
			break;

		if ((int)length == -1)
		{
			GCon->Log(NAME_DevNet, "Read error");
			return -1;
		}

		if (LanDriver->AddrCompare(&readaddr, &Addr) != 0)
		{
			continue;
		}

		Data.SetNum(length);
		memcpy(Data.Ptr(), packetBuffer.data, length);

		ret = 1;
		break;
	}

	return ret;
	unguard;
}

//==========================================================================
//
//	VDatagramSocket::SendMessage
//
//	Send a packet over the net connection.
//	returns 1 if the packet was sent properly
//	returns -1 if the connection died
//
//==========================================================================

int VDatagramSocket::SendMessage(vuint8* Data, vuint32 Length)
{
	guard(VDatagramSocket::SendMessage);
	checkSlow(Length > 0);
	checkSlow(Length <= MAX_MSGLEN);
	if (Invalid)
	{
		return -1;
	}
	return LanDriver->Write(LanSocket, Data, Length, &Addr) == -1 ? -1 : 1;
	unguard;
}

//==========================================================================
//
//	VDatagramSocket::IsLocalConnection
//
//==========================================================================

bool VDatagramSocket::IsLocalConnection()
{
	return false;
}

//==========================================================================
//
//	PrintStats
//
//==========================================================================

static void PrintStats(VSocket*)
{
	GCon->Logf("");
}

//==========================================================================
//
//	COMMAND NetStats
//
//==========================================================================

COMMAND(NetStats)
{
	guard(COMMAND NetStats);
	VSocket	*s;

	VNetworkLocal* Net = (VNetworkLocal*)GNet;
	if (Args.Num() == 1)
	{
		GCon->Logf("unreliable messages sent   = %d", Net->UnreliableMessagesSent);
		GCon->Logf("unreliable messages recv   = %d", Net->UnreliableMessagesReceived);
		GCon->Logf("reliable messages sent     = %d", Net->MessagesSent);
		GCon->Logf("reliable messages received = %d", Net->MessagesReceived);
		GCon->Logf("packetsSent                = %d", Net->packetsSent);
		GCon->Logf("packetsReSent              = %d", Net->packetsReSent);
		GCon->Logf("packetsReceived            = %d", Net->packetsReceived);
		GCon->Logf("receivedDuplicateCount     = %d", Net->receivedDuplicateCount);
		GCon->Logf("shortPacketCount           = %d", Net->shortPacketCount);
		GCon->Logf("droppedDatagrams           = %d", Net->droppedDatagrams);
	}
	else if (Args[1] == "*")
	{
		for (s = Net->ActiveSockets; s; s = s->Next)
			PrintStats(s);
	}
	else
	{
		for (s = Net->ActiveSockets; s; s = s->Next)
			if (Args[1].ICmp(s->Address) == 0)
				break;
		if (s == NULL)
			return;
		PrintStats(s);
	}
	unguard;
}
