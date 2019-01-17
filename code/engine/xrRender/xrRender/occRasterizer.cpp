// occRasterizer.cpp: implementation of the occRasterizer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "occRasterizer.h"

#if DEBUG 
#include "dxRenderDeviceRender.h"
#include "xrRender_console.h"
#endif

occRasterizer	Raster;

void __stdcall fillDW_8x	(void* _p, u32 size, u32 value)
{
	LPDWORD ptr = LPDWORD	(_p);
	LPDWORD end = ptr+size;
	for (; ptr!=end; ptr+=2)
	{
		ptr[0]	= value;
		ptr[1]	= value;
	}
}

inline void propagade_depth			(LPVOID p_dest, LPVOID p_src, int dim)
{
	occD*	dest = (occD*)p_dest;
	occD*	src	 = (occD*)p_src;

	for (int y=0; y<dim; y++)
	{
		for (int x=0; x<dim; x++)
		{
			occD*	base0		= src + (y*2+0)*(dim*2) + (x*2);
			occD*	base1		= src + (y*2+1)*(dim*2) + (x*2);
			occD	f1			= base0[0];
			occD	f2			= base0[1];
			occD	f3			= base1[0];
			occD	f4			= base1[1];
			occD	f			= f1;
			if (f2>f)	f		= f2;
			if (f3>f)	f		= f3;
			if (f4>f)	f		= f4;
			dest[y*dim+x]		= f;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

occRasterizer::occRasterizer	()
#if DEBUG
:dbg_HOM_draw_initialized(false)
#endif
{
	
}

occRasterizer::~occRasterizer()
{
	
}

void occRasterizer::clear()
{
	for (u32 mit = 0; mit < occ_dim; mit++)
	{
		for (u32 it = 0; it < occ_dim; it++)
		{
			bufFrame[mit][it] = nullptr;
		}
	}

	float f = 1.f;
    u32 fillValue = *LPDWORD(&f);

    for (std::size_t i = 0; i < occ_dim * occ_dim; i++) // fill32 TODO: SSE optimize
    {
        std::memcpy(reinterpret_cast<u8*>(bufDepth) + (i * sizeof(u32)), &fillValue, sizeof(u32));
    }
}

inline bool shared(occTri* T1, occTri* T2)
{
	if (T1==T2) return true;
	for (u32 it = 0; it < 3; it++)
	{
		if (T1->adjacent[it] == T2)
		{
			return true;
		}
	}
	return false;
}

void occRasterizer::propagade	()
{
	// Clip-and-propagade zero level
	occTri**	pFrame	= get_frame	();
	float*		pDepth	= get_depth	();
	for (u32 y = 0; y < occ_dim_0; y++)
	{
		for (u32 x = 0; x < occ_dim_0; x++)
		{
			int ox = x + 2, 
				oy = y + 2;

			// Y2-connect
			u32	pos = oy * occ_dim + ox;
			u32	pos_up = pos - occ_dim;
			u32	pos_down = pos + occ_dim; 

			if (pos_down >= occ_dim_0 * occ_dim_0)
			{
				pos_down = pos;
			}

			u32	pos_down2 = pos_down + occ_dim;

			if (pos_down2 >= occ_dim_0 * occ_dim_0)
			{
				pos_down2 = pos_down;
			}

			occTri* Tu1 = pFrame[pos_up];
			if (Tu1) 
			{
				// We has pixel 1scan up
				if (pFrame[pos_down] && shared(Tu1, pFrame[pos_down]))
				{
					// We has pixel 1scan down
					float ZR = (pDepth[pos_up] + pDepth[pos_down]) / 2;
					if (ZR < pDepth[pos]) { pFrame[pos] = Tu1; pDepth[pos] = ZR; }
				}
				else if (pFrame[pos_down2] && shared(Tu1, pFrame[pos_down2]))
				{
					// We has pixel 2scan down
					float ZR = (pDepth[pos_up] + pDepth[pos_down2]) / 2;
					if (ZR < pDepth[pos]) { pFrame[pos] = Tu1; pDepth[pos] = ZR; }
				}
			}

			//
			float d = pDepth[pos];
			clamp(d, -1.99f, 1.99f);
			bufDepth_0[y][x] = df_2_s32(d);
		}
	}
	
	// Propagate other levels
	propagade_depth	(bufDepth_1,bufDepth_0,occ_dim_1);
	propagade_depth	(bufDepth_2,bufDepth_1,occ_dim_2);
	propagade_depth	(bufDepth_3,bufDepth_2,occ_dim_3);
}

void occRasterizer::on_dbg_render()
{
#if DEBUG
	if (!ps_r_flags.is(R_FLAG_HOM_DEPTH_DRAW))
	{
		dbg_HOM_draw_initialized = false;
		return;
	}

	for ( int i = 0; i< occ_dim_0; ++i)
	{
		for ( int j = 0; j< occ_dim_0; ++j)
		{
			if( bDebug )
			{
				Fvector quad,left_top,right_bottom,box_center,box_r;
				quad.set( (float)j-occ_dim_0/2.f, -((float)i-occ_dim_0/2.f), (float)bufDepth_0[i][j]/occQ_s32);
				Device.mProject;

				float z = -CastToGSCMatrix(Device.mProject)._43/(float)(CastToGSCMatrix(Device.mProject)._33-quad.z);
				left_top.set		( quad.x*z/ CastToGSCMatrix(Device.mProject)._11/(occ_dim_0/2.f),		quad.y*z/ CastToGSCMatrix(Device.mProject)._22/(occ_dim_0/2.f), z);
				right_bottom.set	( (quad.x+1)*z/ CastToGSCMatrix(Device.mProject)._11/(occ_dim_0/2.f), (quad.y+1)*z/ CastToGSCMatrix(Device.mProject)._22/(occ_dim_0/2.f), z);

				box_center.set		((right_bottom.x + left_top.x)/2, (right_bottom.y + left_top.y)/2, z);
				box_r = right_bottom;
				box_r.sub(box_center);

				Fmatrix inv;
				inv.invert(CastToGSCMatrix(Device.mView));
				inv.transform( box_center );
				inv.transform_dir( box_r );

				pixel_box& tmp = dbg_pixel_boxes[ i*occ_dim_0+j];
				tmp.center	= box_center;
				tmp.radius	= box_r;
				tmp.z 		= quad.z;
				dbg_HOM_draw_initialized = true;
			}

			if( !dbg_HOM_draw_initialized )
				return;

			pixel_box& tmp = dbg_pixel_boxes[ i*occ_dim_0+j];
			Fmatrix Transform;
			Transform.identity();
			Transform.translate(tmp.center);

			// draw wire
			Device.SetNearer(TRUE);

			RCache.set_Shader	(dxRenderDeviceRender::Instance().m_SelectionShader);
			RCache.dbg_DrawOBB( Transform, tmp.radius, D3DCOLOR_XRGB(u32(255*pow(tmp.z,20.f)),u32(255*(1-pow(tmp.z,20.f))),0) );
			Device.SetNearer(FALSE);
		}
	}
#endif
}


inline bool test_Level(occD* depth, int dim, float _x0, float _y0, float _x1, float _y1, occD z)
{
	int x0 = iFloor	(_x0*dim+.5f);	clamp(x0,0,		dim-1);
	int x1 = iFloor	(_x1*dim+.5f);	clamp(x1,x0,	dim-1);
	int y0 = iFloor	(_y0*dim+.5f);	clamp(y0,0,		dim-1);
	int y1 = iFloor	(_y1*dim+.5f);	clamp(y1,y0,	dim-1);
	
	// MT-Sync (delayed as possible)
	RImplementation.HOM.MT_SYNC	();

	for (u32 y = y0; y <= y1; y++)
	{
		occD* base = depth + y * dim;
		for (occD* it = base + x0; it <= base + x1; it++)
		{
			if (z < *it)
			{
				return true;
			}
		}
	}
	return false;
}

BOOL occRasterizer::test(float _x0, float _y0, float _x1, float _y1, float _z)
{
	occD z = df_2_s32up(_z) + 1;
	return test_Level(get_depth_level(0), occ_dim_0, _x0, _y0, _x1, _y1, z);
}