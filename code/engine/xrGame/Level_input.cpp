#include "stdafx.h"
#include <dinput.h>
#include "../xrEngine/xr_ioconsole.h"
#include "entity_alive.h"
#include "alife_simulator.h"
#include "alife_simulator_header.h"
#include "level_graph.h"
#include "../xrEngine/fdemorecord.h"
#include "level.h"
#include "..\xrEngine\xr_level_controller.h"

#include "stalker_movement_manager_smart_cover.h"
#include "Inventory.h"
#include "xrServer.h"
#include "autosave_manager.h"

#include "actor.h"
#include "huditem.h"
#include "UIGame.h"
#include "../xrEngine/xr_input.h"
#include "saved_game_wrapper.h"

#include "../Include/xrRender/DebugRender.h"

#ifdef DEBUG
#	include "ai/monsters/BaseMonster/base_monster.h"
// Lain: add
#   include "level_debug.h"
#endif

#ifdef DEBUG
	extern void try_change_current_entity();
	extern void restore_actor();
#endif

bool g_bDisableAllInput = false;
extern	float	g_fTimeFactor;

#define CURRENT_ENTITY()	(CurrentEntity() ? CurrentEntity() : nullptr)

void CLevel::IR_OnMouseWheel( int direction )
{
	if(	g_bDisableAllInput	) return;

	if (GameUI()->IR_UIOnMouseWheel(direction)) 
		return;

	if( Device.Paused()

		&& !psActorFlags.test(AF_NO_CLIP) 

		) return;

	if (CURRENT_ENTITY())		
	{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnMouseWheel(direction);
	}
}

static int mouse_button_2_key []	=	{MOUSE_1,MOUSE_2,MOUSE_3};

void CLevel::IR_OnMousePress(int btn)
{	IR_OnKeyboardPress(mouse_button_2_key[btn]);}
void CLevel::IR_OnMouseRelease(int btn)
{	IR_OnKeyboardRelease(mouse_button_2_key[btn]);}
void CLevel::IR_OnMouseHold(int btn)
{	IR_OnKeyboardHold(mouse_button_2_key[btn]);}

void CLevel::IR_OnMouseMove( int dx, int dy )
{
	if(g_bDisableAllInput)							return;
	if (GameUI()->IR_UIOnMouseMove(dx,dy))		return;
	if (Device.Paused() && !psActorFlags.test(AF_NO_CLIP))	return;

	if (CURRENT_ENTITY())		
	{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnMouseMove					(dx,dy);
	}
}

// Обработка нажатия клавиш
extern bool g_block_pause;

// Lain: added TEMP!!!
extern float g_separate_factor;
extern float g_separate_radius;

#include <luabind/functor.hpp>
#include "script_engine.h"
#include "ai_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"

void CLevel::IR_OnKeyboardPress(int key)
{
	if (Device.dwPrecacheFrame)
		return;

#ifdef INGAME_EDITOR
	if (Device.editor() && (pInput->iGetAsyncKeyState(DIK_LALT) || pInput->iGetAsyncKeyState(DIK_RALT)))
		return;
#endif // #ifdef INGAME_EDITOR

	bool b_ui_exist = (!!GameUI());

	EGameActions _curr = get_binded_action(key);

	if (_curr == kPAUSE)
		return;

	if (g_bDisableAllInput)
		return;

	switch (_curr)
	{
		case kSCREENSHOT:
			Render->Screenshot();
			return;

		case kCONSOLE:
			Console->Show();
			return;

		case kQUIT:
		{
			if (b_ui_exist && GameUI()->TopInputReceiver())
			{
				if (GameUI()->IR_UIOnKeyboardPress(key))	
					return; //special case for mp and main_menu

				GameUI()->TopInputReceiver()->HideDialog();
			}
			else
			{
				Console->Execute("main_menu");
			} 
			return;
		}
		break;
	}

	if (!bReady || !b_ui_exist)			
		return;

	if (b_ui_exist && GameUI()->IR_UIOnKeyboardPress(key)) 
		return;

	if (Device.Paused() && !psActorFlags.test(AF_NO_CLIP))	
		return;

    // developer actions
    if (GamePersistent().IsDeveloperMode())
    {
        switch (_curr)
        {
        case kDEV_NOCLIP:
            if (!pInput->iGetAsyncKeyState(DIK_LSHIFT))
            {
                Console->Hide();
                Console->Execute("demo_record 1");
            }
            break;
        default:
            break;
        }
    }

#ifdef DEBUG
	switch (key) {
	case DIK_DIVIDE:
	{
		if (!Server)
			break;

		SetGameTimeFactor(g_fTimeFactor);

		if (!m_bEnvPaused)
			SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), g_fTimeFactor);

		break;
	}
	case DIK_MULTIPLY:
	{
		if (!Server)
			break;

		SetGameTimeFactor(1000.f);
		if (!m_bEnvPaused)
			SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), 1000.f);

		break;
	}
	case DIK_SUBTRACT: {
		if (!Server)
			break;
		if (m_bEnvPaused)
			SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), g_fTimeFactor);
		else
			SetEnvironmentGameTimeFactor(GetEnvironmentGameTime(), 0.00001f);

		m_bEnvPaused = !m_bEnvPaused;
		break;
	}

	case DIK_RETURN: {
		bDebug = !bDebug;
		return;
	}
	case DIK_BACK:
		DRender->NextSceneMode();
		return;

	case DIK_F4: {
		if (pInput->iGetAsyncKeyState(DIK_LALT))
			break;

		if (pInput->iGetAsyncKeyState(DIK_RALT))
			break;

		bool bOk = false;
		u32 i = 0, j, n = Objects.o_count();
		if (pCurrentEntity)
			for (; i < n; ++i)
				if (Objects.o_get_by_iterator(i) == pCurrentEntity)
					break;
		if (i < n) {
			j = i;
			bOk = false;
			for (++i; i < n; ++i) {
				CEntityAlive* tpEntityAlive = smart_cast<CEntityAlive*>(Objects.o_get_by_iterator(i));
				if (tpEntityAlive) {
					bOk = true;
					break;
				}
			}
			if (!bOk)
				for (i = 0; i < j; ++i) {
					CEntityAlive* tpEntityAlive = smart_cast<CEntityAlive*>(Objects.o_get_by_iterator(i));
					if (tpEntityAlive) {
						bOk = true;
						break;
					}
				}
			if (bOk) {
				CObject *tpObject = CurrentEntity();
				CObject *__I = Objects.o_get_by_iterator(i);
				CObject **I = &__I;

				SetEntity(*I);
				if (tpObject != *I)
				{
					CActor* pActor = smart_cast<CActor*> (tpObject);
					if (pActor)
						pActor->inventory().Items_SetCurrentEntityHud(false);
				}
				if (tpObject)
				{
					Engine.Sheduler.Unregister(tpObject);
					Engine.Sheduler.Register(tpObject, TRUE);
				};
				Engine.Sheduler.Unregister(*I);
				Engine.Sheduler.Register(*I, TRUE);

				CActor* pActor = smart_cast<CActor*> (*I);
				if (pActor)
				{
					pActor->inventory().Items_SetCurrentEntityHud(true);

					CHudItem* pHudItem = smart_cast<CHudItem*>(pActor->inventory().ActiveItem());
					if (pHudItem)
					{
						pHudItem->OnStateSwitch(pHudItem->GetState(), pHudItem->GetState());
					}
				}
			}
		}
		return;
	}
				 // Lain: added
	case DIK_F5:
	{
		if (CBaseMonster* pBM = smart_cast<CBaseMonster*>(CurrentEntity()))
		{
			DBG().log_debug_info();
		}
		break;
	}

	case MOUSE_1:
	{
		if (pInput->iGetAsyncKeyState(DIK_LALT)) {
			if (smart_cast<CActor*>(CurrentEntity()))
				try_change_current_entity();
			else
				restore_actor();
			return;
		}
		break;
	}
	/**/
	}
#endif // DEBUG

	if (bindConsoleCmds.execute(key))
		return;

	if (CURRENT_ENTITY())
	{
		IInputReceiver*	IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				
			IR->IR_OnKeyboardPress(get_binded_action(key));
	}


#ifdef DEBUG
	CObject *obj = CurrentEntity();
	if (obj) 
	{
		CBaseMonster *monster = smart_cast<CBaseMonster *>(obj);
		if (monster)
			monster->debug_on_key(key);
	}
#endif
}

void CLevel::IR_OnKeyboardRelease(int key)
{
	if (!bReady || g_bDisableAllInput)								
		return;

	if (GameUI() && GameUI()->IR_UIOnKeyboardRelease(key)) 
		return;

	if (Device.Paused() && !psActorFlags.test(AF_NO_CLIP))
		return;

	if (CURRENT_ENTITY())
	{
		IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				
			IR->IR_OnKeyboardRelease(get_binded_action(key));
	}
}

void CLevel::IR_OnKeyboardHold(int key)
{
	if (g_bDisableAllInput) 
		return;

#ifdef DEBUG
	// Lain: added
	if (key == DIK_UP)
	{
		static u32 time = Device.dwTimeGlobal;
		if (Device.dwTimeGlobal - time > 20)
		{
			if (CBaseMonster* pBM = smart_cast<CBaseMonster*>(CurrentEntity()))
			{
				DBG().debug_info_up();
				time = Device.dwTimeGlobal;
			}
		}
	}
	else if (key == DIK_DOWN)
	{
		static u32 time = Device.dwTimeGlobal;
		if (Device.dwTimeGlobal - time > 20)
		{
			if (CBaseMonster* pBM = smart_cast<CBaseMonster*>(CurrentEntity()))
			{
				DBG().debug_info_down();
				time = Device.dwTimeGlobal;
			}
		}
	}

#endif // DEBUG

	if (GameUI() && GameUI()->IR_UIOnKeyboardHold(key)) 
		return;

	if (Device.Paused() && !psActorFlags.test(AF_NO_CLIP)) 
		return;

	if (CURRENT_ENTITY()) 
	{
		IInputReceiver*	IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)			
			IR->IR_OnKeyboardHold(get_binded_action(key));
	}
}

void CLevel::IR_OnMouseStop( int /**axis/**/, int /**value/**/)
{
}

void CLevel::IR_OnActivate()
{
	if (pInput)
	{
		for (u32 i = 0; i < CInput::COUNT_KB_BUTTONS; i++)
		{
			if (IR_GetKeyState(i))
			{

				EGameActions action = get_binded_action(i);
				switch (action) 
				{
					case kFWD:
					case kBACK:
					case kL_STRAFE:
					case kR_STRAFE:
					case kLEFT:
					case kRIGHT:
					case kUP:
					case kDOWN:
					case kCROUCH:
					case kACCEL:
					case kL_LOOKOUT:
					case kR_LOOKOUT:
					case kWPN_FIRE:
					{
						IR_OnKeyboardPress(i);
					} break;
				}
			}
		}
	}
}