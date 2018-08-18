#include "stdafx.h"
#pragma hdrstop

#include "blender_gamma.h"

CBlender_gamma::CBlender_gamma	()	{ description.CLS = 0; }
CBlender_gamma::~CBlender_gamma	()	{ }

void CBlender_gamma::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
	case 0: // gamma lut generation
		C.r_Pass		("stub_screen_space", "gamma_gen_lut", FALSE, FALSE, FALSE);
		C.r_End			();
		break;
    case 1: // applying
		C.r_Pass		("stub_screen_space", "gamma_apply", FALSE, FALSE, FALSE);
		C.r_dx10Texture	("s_image",			r2_RT_generic0);
		C.r_dx10Texture	("s_gamma_lut",		r2_RT_gamma_lut);

		C.r_dx10Sampler	("smp_nofilter");
		C.r_dx10Sampler	("smp_rtlinear");
		C.r_End			();
        break;
    }
}
