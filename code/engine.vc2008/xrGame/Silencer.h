///////////////////////////////////////////////////////////////
// Silencer.h
// Silencer - ������� ������ ��������� 
///////////////////////////////////////////////////////////////
#pragma once
#include "inventory_item_object.h"

class CSilencer : public CInventoryItemObject 
{
private:
	using inherited = CInventoryItemObject;
public:
	CSilencer ();
	virtual ~CSilencer();
};