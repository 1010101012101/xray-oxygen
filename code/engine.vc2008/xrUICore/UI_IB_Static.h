// File:		UI_IB_Static.h
// Description:	Inheritance of UIInteractiveBackground template class with some
//				CUIStatic features
// Created:		09.02.2005
// Author:		Serge Vynnychenko
// Mail:		narrator@gsc-game.kiev.ua

// Copyright 2005 GSC Game World
#pragma once
#include "UIInteractiveBackground.h"
#include "UIStatic.h"

class UI_API CUI_IB_Static : public CUIInteractiveBackground<CUIStatic> 
{
public:
	virtual void SetTextureOffset(float x, float y);
	void			SetStretchTexture(bool stretch_texture);
};