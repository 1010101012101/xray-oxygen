// xrServer.h: interface for the xrServer class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "../xrEngine/ClS/net_server.h"
#include "game_sv_base.h"
#include "id_generator.h"

class CSE_Abstract;

const u32	NET_Latency		= 50;		// time in (ms)

// t-defs
typedef xr_hash_map<u16, CSE_Abstract*>	xrS_entities;

class xrClientData	: public IClient
{
public:
	CSE_Abstract*			owner;
	BOOL					net_Ready;
	BOOL					net_Accepted;
	
	u32						net_LastMoveUpdateTime;

							xrClientData			();
	virtual					~xrClientData			() = default;
	virtual void			Clear					();
};


// main
struct	svs_respawn
{
	u32		timestamp;
	u16		phantom;
};
IC bool operator < (const svs_respawn& A, const svs_respawn& B)	{ return A.timestamp<B.timestamp; }

class xrServer	: public IPureServer  
{
private:
	xrS_entities				entities;
	xr_multiset<svs_respawn>	q_respawn;
	xr_vector<u16>				conn_spawned_ids;
	
	struct DelayedPacket
	{
		ClientID		SenderID;
		NET_Packet		Packet;
		bool operator == (const DelayedPacket& other)
		{
			return SenderID == other.SenderID;
		}
	};

	std::recursive_mutex		DelayedPackestCS;
	xr_deque<DelayedPacket>		m_aDelayedPackets;
	void						ProceedDelayedPackets	();
	void						AddDelayedPacket		(NET_Packet& Packet, ClientID Sender);
	void						OnDelayedMessage		(NET_Packet& P, ClientID sender);

private:
	typedef 
		CID_Generator<
			u32,		// time identifier type
			u8,			// compressed id type 
			u16,		// id type
			u8,			// block id type
			u16,		// chunk id type
			0,			// min value
			u16(-2),	// max value
			256,		// block size
			u16(-1)		// invalid id
		> id_generator_type;

private:
	id_generator_type		m_tID_Generator;

public:
	game_sv_GameState*		game;

	void					Perform_game_export		();
	void					PerformMigration		(CSE_Abstract* E, xrClientData* from, xrClientData* to);
	
	IC void					clear_ids				()
	{
		m_tID_Generator		= id_generator_type();
	}
	IC u16					PerformIDgen			(u16 ID)
	{
		return				(m_tID_Generator.tfGetID(ID));
	}
	IC void					FreeID					(u16 ID, u32 time)
	{
		return				(m_tID_Generator.vfFreeID(ID, time));
	}

	void					Perform_connect_spawn	(CSE_Abstract* E, xrClientData* to, NET_Packet& P);
	void					Perform_transfer		(NET_Packet &PR, NET_Packet &PT, CSE_Abstract* what, CSE_Abstract* from, CSE_Abstract* to);
	void					Perform_reject			(CSE_Abstract* what, CSE_Abstract* from, int delta);
	void					Perform_destroy			(CSE_Abstract* tpSE_Abstract, u32 mode);

	CSE_Abstract*			Process_spawn			(NET_Packet& P, ClientID sender, BOOL bSpawnWithClientsMainEntityAsParent=FALSE, CSE_Abstract* tpExistedEntity=0);
	void					Process_update			(NET_Packet& P, ClientID sender);
	void					Process_save			(NET_Packet& P, ClientID sender);
	void					Process_event			(NET_Packet& P, ClientID sender);
	void					Process_event_ownership	(NET_Packet& P, ClientID sender, u32 time, u16 ID, BOOL bForced = FALSE);
	bool					Process_event_reject	(NET_Packet& P, const ClientID sender, const u32 time, const u16 id_parent, const u16 id_entity, bool send_message = true);
	void					Process_event_destroy	(NET_Packet& P, ClientID sender, u32 time, u16 ID, NET_Packet* pEPack);
	void					Process_event_activate	(NET_Packet& P, const ClientID sender, const u32 time, const u16 id_parent, const u16 id_entity, bool send_message = true);
	
	void					SendConnectResult		(IClient* CL, char* ResultStr);
	void	__stdcall		SendConfigFinished		(ClientID const & clientId);
	void					AttachNewClient			(IClient* CL);

protected:
			void			RequestClientDigest					(IClient* CL);
	virtual void			Check_BuildVersion_Success			(IClient* CL);

	void					SendConnectionData		(IClient* CL);
	void					OnProcessClientMapData	(NET_Packet& P, ClientID const & clientID);

public:
	// constr / destr
	xrServer				();
	virtual ~xrServer		();

	// extended functionality
	virtual u32				OnMessage			(NET_Packet& P, ClientID sender);	// Non-Zero means broadcasting with "flags" as returned
	virtual void			OnCL_Connected		(IClient* CL);

	virtual void			SendTo_LL			(void* data, u32 size);
    virtual void		    SendTo(ClientID ID, NET_Packet& P, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0);
	virtual	void			SendBroadcast		(ClientID exclude, NET_Packet& P, u32 dwFlags=DPNSEND_GUARANTEED);

	virtual void			client_Destroy		(IClient* C);					// destroy client info

	// utilities
	CSE_Abstract*			entity_Create		(LPCSTR name);
	void					entity_Destroy		(CSE_Abstract *&P);
	CSE_Abstract*			GetEntity			(u32 Num);

	xrClientData*			ID_to_client		(ClientID ID, bool ScanAll = false ) { return (xrClientData*)(IPureServer::ID_to_client( ID, ScanAll)); }
	CSE_Abstract*			ID_to_entity		(u16 ID);

	// main
	virtual EConnect		Connect				(shared_str& session_name);
	virtual void			Disconnect			();
	virtual void			Update				();
	void					SLS_Default			();
	void					SLS_Clear			();
	void					SLS_Save			(IWriter&	fs);
	void					SLS_Load			(IReader&	fs);

			shared_str		level_name			(const shared_str &server_options) const;
			shared_str		level_version		(const shared_str &server_options) const;

    void createClient(); // Create actor

#ifdef DEBUG
public:
			bool			verify_entities		() const;
			void			verify_entity		(const CSE_Abstract *entity) const;
#endif
};

#ifdef DEBUG
		enum e_dbg_net_Draw_Flags
		{
			dbg_draw_actor_alive			=(1<<0),	
			dbg_draw_actor_dead				=(1<<1),	
			dbg_draw_customzone				=(1<<2),	
			dbg_draw_teamzone				=(1<<3),	
			dbg_draw_invitem				=(1<<4),	
			dbg_draw_actor_phys				=(1<<5),	
			dbg_draw_customdetector			=(1<<6),	
			dbg_destroy						=(1<<7),	
			dbg_draw_autopickupbox			=(1<<8),	
			dbg_draw_rp						=(1<<9),	
			dbg_draw_climbable				=(1<<10),
			dbg_draw_skeleton				=(1<<11)
		};
extern	Flags32	dbg_net_Draw_Flags;
#endif

