#pragma once

#include "../xrEngine/feel_touch.h"
#include "inventory_item_object.h"

#include "InfoPortionDefs.h"
#include "character_info_defs.h"

#include "PdaMsg.h"

class CInventoryOwner;

class GAME_API CPda : public CInventoryItemObject, public Feel::Touch
{
	typedef	CInventoryItemObject inherited;
public:
											CPda					();
	virtual									~CPda					();

	virtual BOOL 							net_Spawn				(CSE_Abstract* DC);
	virtual void 							Load					(LPCSTR section);
	virtual void 							net_Destroy				();

	virtual void 							OnH_A_Chield			();
	virtual void 							OnH_B_Independent		(bool just_before_destroy);

	virtual void 							shedule_Update			(u32 dt);

	virtual void 							feel_touch_new			(CObject* O);
	virtual void 							feel_touch_delete		(CObject* O);
	virtual BOOL 							feel_touch_contact		(CObject* O);


	virtual u16								GetOriginalOwnerID		() {return m_idOriginalOwner;}
	virtual CInventoryOwner*				GetOriginalOwner		();
	virtual CObject*						GetOwnerObject			();


	inline	void							TurnOn					() {m_bTurnedOff = false;}
	inline	void							TurnOff					() {m_bTurnedOff = true;}
	
	inline	bool 							IsActive				() {return IsOn();}
	inline	bool 							IsOn					() {return !m_bTurnedOff;}
	inline	bool 							IsOff					() {return m_bTurnedOff;}

			void							ActivePDAContacts		(xr_vector<CPda*>& res);
			CPda*							GetPdaFromOwner			(CObject* owner);
	inline	u32								ActiveContactsNum		()	{return u32(m_active_contacts.size());}
			void							PlayScriptFunction		();
	inline	bool							CanPlayScriptFunction	() { return bool(xr_strcmp(m_functor_str, "")); };


	virtual void							save					(NET_Packet &output_packet);
	virtual void							load					(IReader &input_packet);

	float									GetRadius() const			{ return m_fRadius; }
	void									SetRadius(float val)		{ m_fRadius = val; }
protected:
	void									UpdateActiveContacts	();

	xr_vector<CObject*>						m_active_contacts;
	float									m_fRadius;

	u16										m_idOriginalOwner;
	shared_str								m_SpecificChracterOwner;
	xr_string								m_sFullName;

	bool									m_bTurnedOff;
	shared_str								m_functor_str;
};

using PDA_LIST = xr_vector<CPda*>;