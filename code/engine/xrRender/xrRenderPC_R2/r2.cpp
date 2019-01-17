#include "stdafx.h"
#include "r2.h"
#include "../../xrCore/xrDelegate/xrDelegate.h"
#include "../xrRender/fbasicvisual.h"
#include "../../xrEngine/xr_object.h"
#include "../../xrEngine/CustomHUD.h"
#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/environment.h"
#include "../xrRender/SkeletonCustom.h"
#include "../xrRender/LightTrack.h"
#include "../xrRender/dxRenderDeviceRender.h"
#include "../xrRender/dxWallMarkArray.h"
#include "../xrRender/dxUIShader.h"
#include "../xrRender/dxGlowManager.h"

CRender RImplementation;

ENGINE_API BOOL isGraphicDebugging;

//////////////////////////////////////////////////////////////////////////
float		r_dtex_range		= 50.f;
//////////////////////////////////////////////////////////////////////////
ShaderElement*			CRender::rimp_select_sh_dynamic	(dxRender_Visual	*pVisual, float cdist_sq)
{
	int		id	= SE_R2_SHADOW;
	if	(CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq)-pVisual->vis.sphere.R)<r_dtex_range)?SE_R2_NORMAL_HQ:SE_R2_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();
}
//////////////////////////////////////////////////////////////////////////
ShaderElement*			CRender::rimp_select_sh_static	(dxRender_Visual	*pVisual, float cdist_sq)
{
	int		id	= SE_R2_SHADOW;
	if	(CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq)-pVisual->vis.sphere.R)<r_dtex_range)?SE_R2_NORMAL_HQ:SE_R2_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();
}

extern ENGINE_API BOOL r2_advanced_pp;	//	advanced post process and effects
//////////////////////////////////////////////////////////////////////////
// Just two static storage
void					CRender::create					()
{
	Device.seqFrame.Add	(this,REG_PRIORITY_HIGH+0x12345678);

	m_skinning			= -1;

	// hardware
	o.smapsize			= ps_r_smapsize;
	o.mrt				= (HW.Caps.raster.dwMRT_count >= 3);
	o.mrtmixdepth		= (HW.Caps.raster.b_MRT_mixdepth);

	// Check for NULL render target support
	D3DFORMAT	nullrt	= (D3DFORMAT)MAKEFOURCC('N','U','L','L');
	o.nullrt			= HW.IsFormatSupported(nullrt, D3DRTYPE_SURFACE, D3DUSAGE_RENDERTARGET);

	if (o.nullrt)		
	{
		Msg				("* NULLRT supported and used");
	};


	// SMAP / DST
	o.HW_smap_FETCH4	= FALSE;
	o.HW_smap			= HW.IsFormatSupported(D3DFMT_D24X8, D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL);
	o.HW_smap_PCF		= o.HW_smap		;
	if (o.HW_smap)		{
		o.HW_smap_FORMAT	= D3DFMT_D24X8;
		Msg				("* HWDST/PCF supported and used");
	}

	o.fp16_filter		= HW.IsFormatSupported(D3DFMT_A16B16G16R16F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_FILTER);
	o.fp16_blend		= HW.IsFormatSupported(D3DFMT_A16B16G16R16F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING);

	// search for ATI formats
	if (!o.HW_smap && (0==strstr(Core.Params,"-nodf24")) )		{
		o.HW_smap		= HW.IsFormatSupported((D3DFORMAT)(MAKEFOURCC('D','F','2','4')), D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL);
		if (o.HW_smap)	{
			o.HW_smap_FORMAT= MAKEFOURCC	('D','F','2','4');
			o.HW_smap_PCF	= FALSE			;
			o.HW_smap_FETCH4= TRUE			;
		}
		Msg				("* DF24/F4 supported and used [%X]", o.HW_smap_FORMAT);
	}

	// emulate ATI-R4xx series
	if (strstr(Core.Params,"-r4xx"))	{
		o.mrtmixdepth	= FALSE;
		o.HW_smap		= FALSE;
		o.HW_smap_PCF	= FALSE;
		o.fp16_filter	= FALSE;
		o.fp16_blend	= FALSE;
	}

	VERIFY2				(o.mrt && (HW.Caps.raster.dwInstructions>=256),"Hardware doesn't meet minimum feature-level");
	if (o.mrtmixdepth)		o.albedo_wo		= FALSE	;
	else if (o.fp16_blend)	o.albedo_wo		= FALSE	;
	else					o.albedo_wo		= TRUE	;

	// nvstencil on NV40 and up
	// nvstencil should be enabled only for GF 6xxx and GF 7xxx
	// if hardware support early stencil (>= GF 8xxx) stencil reset trick only
	// slows down.
	o.nvstencil			= FALSE;
	if ((HW.Caps.id_vendor==0x10DE)&&(HW.Caps.id_device>=0x40))	
	{
		o.nvstencil = ( S_OK==HW.pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8 , 0, D3DRTYPE_TEXTURE, (D3DFORMAT MAKEFOURCC('R','A','W','Z'))) );
	}

	if (strstr(Core.Params,"-nonvs"))		o.nvstencil	= FALSE;

	// nv-dbt
	o.nvdbt				= HW.IsFormatSupported((D3DFORMAT)MAKEFOURCC('N','V','D','B'), D3DRTYPE_SURFACE, 0);
	if (o.nvdbt)		Msg	("* NV-DBT supported and used");

	// gloss
	char*	g			= strstr(Core.Params,"-gloss ");
	o.forcegloss		= g?	TRUE	:FALSE	;
	if (g)				{
		o.forcegloss_v		= float	(atoi	(g+xr_strlen("-gloss ")))/255.f;
	}

	// options
	o.sunfilter			= (strstr(Core.Params,"-sunfilter"))?	TRUE	:FALSE	;
	o.advancedpp		= r2_advanced_pp;
	o.sjitter			= (strstr(Core.Params,"-sjitter"))?		TRUE	:FALSE	;
	o.depth16			= (strstr(Core.Params,"-depth16"))?		TRUE	:FALSE	;
	if (strstr(Core.Params,"-noshadows") || strstr(Core.Params,"-render_for_weak_systems"))
		o.noshadows = TRUE;
	else
		o.noshadows = FALSE;
	o.Tshadows			= (strstr(Core.Params,"-tsh"))?			TRUE	:FALSE	;
	o.distortion_enabled= (strstr(Core.Params,"-nodistort"))?	FALSE	:TRUE	;
	o.distortion		= o.distortion_enabled;
	o.disasm			= (strstr(Core.Params,"-disasm"))?		TRUE	:FALSE	;
	o.forceskinw		= (strstr(Core.Params,"-skinw"))?		TRUE	:FALSE	;

	o.ssao_blur_on		= ps_r_ssao_flags.test(R_FLAG_SSAO_BLUR) && ps_r_ssao != 0;
	o.ssao_opt_data		= ps_r_ssao_flags.test(R_FLAG_SSAO_OPT_DATA) && (ps_r_ssao != 0);
	o.ssao_half_data	= ps_r_ssao_flags.test(R_FLAG_SSAO_HALF_DATA) && o.ssao_opt_data && (ps_r_ssao != 0);
	o.ssao_hbao			= ps_r_ssao_flags.test(R_FLAG_SSAO_HBAO) && (ps_r_ssao != 0);
	
	if ((HW.Caps.id_vendor==0x1002)&&(HW.Caps.id_device<=0x72FF))	
	{
		o.ssao_opt_data = false;
		o.ssao_hbao = false;
	}

	c_lmaterial					= "L_material";
	c_sbase						= "s_base";

	Target						= xr_new<CRenderTarget>		();	// Main target

	Models						= xr_new<CModelPool>		();
	PSLibrary.OnCreate			();
	HWOCC.occq_create			(occq_size);

	marker						= 0;

    std::memset(q_sync_point,0,sizeof(q_sync_point));
	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		R_CHK						(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT,&q_sync_point[i]));

	::PortalTraverser.initialize();
}

void					CRender::destroy				()
{
	::PortalTraverser.destroy	();
	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		_RELEASE(q_sync_point[i]);
	HWOCC.occq_destroy			();
	xr_delete					(Models);
	xr_delete					(Target);
	PSLibrary.OnDestroy			();
	Device.seqFrame.Remove		(this);
	r_dsgraph_destroy			();
}

void CRender::reset_begin()
{
	// Update incremental shadowmap-visibility solver
	// BUG-ID: 10646
	{
		u32 it = 0;
		for (it = 0; it < Lights_LastFrame.size(); it++) 
		{
			if (!Lights_LastFrame[it])	continue;
			try 
			{
				Lights_LastFrame[it]->svis.resetoccq();
			}
			catch (...)
			{
				Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear	();
	}

	// KD: let's reload details while changed details options on vid_restart
	if (b_loaded && ((dm_current_size != dm_size) || (ps_r_Detail_density != ps_current_detail_density) 
												  || (ps_r_Detail_height != ps_current_detail_height)))
	{
		Details->Unload();
		xr_delete(Details);
	}	
	
	xr_delete					(Target);
	HWOCC.occq_destroy			();

	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		_RELEASE(q_sync_point[i]);
}

void CRender::reset_end()
{
	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		R_CHK					(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT,&q_sync_point[i]));
	HWOCC.occq_create			(occq_size);

	Target						=	xr_new<CRenderTarget>	();

	// KD: let's reload details while changed details options on vid_restart
	if (b_loaded && ((dm_current_size != dm_size) || (ps_r_Detail_density != ps_current_detail_density)
												  || (ps_r_Detail_height != ps_current_detail_height)))
	{
		Details = xr_new<CDetailManager>();
		Details->Load();
	}	

	// Set this flag true to skip the first render frame,
	// that some data is not ready in the first frame (for example device camera position)
	m_bFirstFrameAfterReset = true;
}

void CRender::OnFrame()
{
	Models->DeleteQueue();
	// MT-details (@front)
	Device.seqParallel.insert(Device.seqParallel.begin(),
		xrDelegate(BindDelegate(Details, &CDetailManager::MT_CALC)));

	// MT-HOM (@front)
	Device.seqParallel.insert(Device.seqParallel.begin(),
		xrDelegate(BindDelegate(&HOM, &CHOM::MT_RENDER)));
}


// Implementation
IRender_ObjectSpecific*	CRender::ros_create				(IRenderable* parent)				{ return xr_new<CROS_impl>();			}
void					CRender::ros_destroy			(IRender_ObjectSpecific* &p)		{ xr_delete(p);							}
IRenderVisual*			CRender::model_Create			(LPCSTR name, IReader* data)		{ return Models->Create(name,data);		}
IRenderVisual*			CRender::model_CreateChild		(LPCSTR name, IReader* data)		{ return Models->CreateChild(name,data);}
IRenderVisual*			CRender::model_Duplicate		(IRenderVisual* V)					{ return Models->Instance_Duplicate((dxRender_Visual*)V);	}
void					CRender::model_Delete			(IRenderVisual* &V, BOOL bDiscard)	
{ 
	dxRender_Visual* pVisual = (dxRender_Visual*)V;
	Models->Delete(pVisual, bDiscard);
	V = nullptr;
}
IRender_DetailModel*	CRender::model_CreateDM			(IReader*	F)
{
	CDetail*	D		= xr_new<CDetail> ();
	D->Load				(F);
	return D;
}
void					CRender::model_Delete			(IRender_DetailModel* & F)
{
	if (F)
	{
		CDetail*	D	= (CDetail*)F;
		D->Unload		();
		xr_delete		(D);
		F				= nullptr;
	}
}
IRenderVisual*			CRender::model_CreatePE			(LPCSTR name)	
{ 
	PS::CPEDef*	SE			= PSLibrary.FindPED	(name);		R_ASSERT3(SE,"Particle effect [%s] doesn't exist",name);
	return					Models->CreatePE	(SE);
}
IRenderVisual*			CRender::model_CreateParticles	(LPCSTR name)	
{ 
	PS::CPEDef*	SE			= PSLibrary.FindPED	(name);
	if (SE) return			Models->CreatePE	(SE);
	else{
		PS::CPGDef*	SG		= PSLibrary.FindPGD	(name);		R_ASSERT3(SG,"Particle effect or group [%s] doesn't exist",name);
		return				Models->CreatePG	(SG);
	}
}
void					CRender::models_Prefetch		()					{ Models->Prefetch	();}
void					CRender::models_Clear			(BOOL b_complete)	{ Models->ClearPool	(b_complete);}

ref_shader				CRender::getShader				(int id)			{ VERIFY(id<int(Shaders.size()));	return Shaders[id];	}
IRender_Portal*			CRender::getPortal				(int id)			{ VERIFY(id<int(Portals.size()));	return Portals[id];	}
IRender_Sector*			CRender::getSector				(int id)			{ VERIFY(id<int(Sectors.size()));	return Sectors[id];	}
IRender_Sector*			CRender::getSectorActive		()					{ return pLastSector;									}
IRenderVisual*			CRender::getVisual				(int id)			{ VERIFY(id<int(Visuals.size()));	return Visuals[id];	}
D3DVERTEXELEMENT9*		CRender::getVB_Format			(int id, BOOL	_alt)	{ 
	if (_alt)	{ VERIFY(id<int(xDC.size()));	return xDC[id].begin();	}
	else		{ VERIFY(id<int(nDC.size()));	return nDC[id].begin(); }
}
IDirect3DVertexBuffer9*	CRender::getVB					(int id, BOOL	_alt)	{
	if (_alt)	{ VERIFY(id<int(xVB.size()));	return xVB[id];		}
	else		{ VERIFY(id<int(nVB.size()));	return nVB[id];		}
}
IDirect3DIndexBuffer9*	CRender::getIB					(int id, BOOL	_alt)	{ 
	if (_alt)	{ VERIFY(id<int(xIB.size()));	return xIB[id];		}
	else		{ VERIFY(id<int(nIB.size()));	return nIB[id];		}
}
FSlideWindowItem*		CRender::getSWI					(int id)			{ VERIFY(id<int(SWIs.size()));		return &SWIs[id];	}
IRender_Target*			CRender::getTarget				()					{ return Target;										}

IRender_Light*			CRender::light_create			()					{ return Lights.Create();								}
IRender_Glow*			CRender::glow_create			()					{ return xr_new<CGlow>();								}

void					CRender::flush					()					{ r_dsgraph_render_graph	(0);						}

BOOL					CRender::occ_visible			(vis_data& P)		{ return HOM.visible(P);								}
BOOL					CRender::occ_visible			(sPoly& P)			{ return HOM.visible(P);								}
BOOL					CRender::occ_visible			(Fbox& P)			{ return HOM.visible(P);								}

void					CRender::add_Visual				(IRenderVisual*		V )	{ add_leafs_Dynamic((dxRender_Visual*)V);								}
void					CRender::add_Geometry			(IRenderVisual*		V )	{ add_Static((dxRender_Visual*)V,View->getMask());					}
void					CRender::add_StaticWallmark		(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (T->suppress_wm)	return;
	VERIFY2							(_valid(P) && _valid(s) && T && verts && (s>EPS_L), "Invalid static wallmark params");
	Wallmarks->AddStaticWallmark	(T,verts,P,&*S,s);
}

void CRender::add_StaticWallmark			(IWallMarkArray *pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
	dxWallMarkArray *pWMA = (dxWallMarkArray *)pArray;
	ref_shader *pShader = pWMA->dxGenerateWallmark();
	if (pShader) add_StaticWallmark		(*pShader, P, s, T, V);
}

void CRender::add_StaticWallmark			(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
	dxUIShader* pShader = (dxUIShader*)&*S;
	add_StaticWallmark		(pShader->hShader, P, s, T, V);
}

void					CRender::clear_static_wallmarks	()
{
	Wallmarks->clear				();
}

void					CRender::add_SkeletonWallmark	(intrusive_ptr<CSkeletonWallmark> wm)
{
	Wallmarks->AddSkeletonWallmark				(wm);
}
void					CRender::add_SkeletonWallmark	(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size)
{
	Wallmarks->AddSkeletonWallmark				(xf, obj, sh, start, dir, size);
}
void					CRender::add_SkeletonWallmark	(const Fmatrix* xf, IKinematics* obj, IWallMarkArray *pArray, const Fvector& start, const Fvector& dir, float size)
{
	dxWallMarkArray *pWMA = (dxWallMarkArray *)pArray;
	ref_shader *pShader = pWMA->dxGenerateWallmark();
	if (pShader) add_SkeletonWallmark(xf, (CKinematics*)obj, *pShader, start, dir, size);
}
void					CRender::add_Occluder			(Fbox2&	bb_screenspace	)
{
	HOM.occlude			(bb_screenspace);
}
void					CRender::set_Object				(IRenderable*	O )	
{ 
	val_pObject				= O;
}
void					CRender::rmNear				()
{
	IRender_Target* T	=	getTarget	();
	D3DVIEWPORT9 VP		=	{0,0,T->get_width(),T->get_height(),0,0.02f };
	CHK_DX				(HW.pDevice->SetViewport(&VP));
}
void					CRender::rmFar				()
{
	IRender_Target* T	=	getTarget	();
	D3DVIEWPORT9 VP		=	{0,0,T->get_width(),T->get_height(),0.99999f,1.f };
	CHK_DX				(HW.pDevice->SetViewport(&VP));
}
void					CRender::rmNormal			()
{
	IRender_Target* T	=	getTarget	();
	D3DVIEWPORT9 VP		= {0,0,T->get_width(),T->get_height(),0,1.f };
	CHK_DX				(HW.pDevice->SetViewport(&VP));
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender()
:m_bFirstFrameAfterReset(false)
{
	init_cacades();
}

CRender::~CRender()
{
	for (FSlideWindowItem it: SWIs)
	{
		xr_free(it.sw);
		it.sw = nullptr;
		it.count = 0;
	}
	SWIs.clear(); 
}

#include "../../xrEngine/GameFont.h"
void	CRender::Statistics	(CGameFont* _F)
{
	CGameFont&	F	= *_F;
	F.OutNext	(" **** LT:%2d,LV:%2d **** ",stats.l_total,stats.l_visible		);	stats.l_visible = 0;
	F.OutNext	("    S(%2d)   | (%2d)NS   ",stats.l_shadowed,stats.l_unshadowed);
	F.OutNext	("smap use[%2d], merge[%2d], finalclip[%2d]",stats.s_used,stats.s_merged-stats.s_used,stats.s_finalclip);
	stats.s_used = 0; stats.s_merged = 0; stats.s_finalclip = 0;
	F.OutSkip	();
	F.OutNext	(" **** Occ-Q(%03.1f) **** ",100.f*f32(stats.o_culled)/f32(stats.o_queries?stats.o_queries:1));
	F.OutNext	(" total  : %2d",	stats.o_queries	);	stats.o_queries = 0;
	F.OutNext	(" culled : %2d",	stats.o_culled	);	stats.o_culled	= 0;
	F.OutSkip	();
	u32	ict		= stats.ic_total + stats.ic_culled;
	F.OutNext	(" **** iCULL(%03.1f) **** ",100.f*f32(stats.ic_culled)/f32(ict?ict:1));
	F.OutNext	(" visible: %2d",	stats.ic_total	);	stats.ic_total	= 0;
	F.OutNext	(" culled : %2d",	stats.ic_culled	);	stats.ic_culled	= 0;
#ifdef DEBUG
	HOM.stats	();
#endif
}

/////////
#pragma comment(lib,"d3dx9.lib")

static HRESULT create_shader (
		LPCSTR name,
		LPCSTR const pTarget,
		DWORD const* buffer,
		u32 const buffer_size,
		LPCSTR const file_name,
		void*& result,
		bool const disasm
	)
{
	HRESULT		_result = E_FAIL;
	if (pTarget[0] == 'p') {
		SPS* sps_result = (SPS*)result;
		_result			= HW.pDevice->CreatePixelShader(buffer, &sps_result->ps);
		if ( !SUCCEEDED(_result) ) {
			Msg			("! PS: %s", file_name);
			Msg			("! CreatePixelShader hr == 0x%08x", _result);
			return		E_FAIL;
		}

		LPCVOID			data		= NULL;
		_result			= D3DXFindShaderComment	(buffer,MAKEFOURCC('C','T','A','B'),&data,NULL);
		if (SUCCEEDED(_result) && data)
		{
			LPD3DXSHADER_CONSTANTTABLE	pConstants	= LPD3DXSHADER_CONSTANTTABLE(data);
			sps_result->constants.parse	(pConstants,0x1);
		} 
		else
		{
			Msg			("! PS: %s", file_name);
			Msg			("! D3DXFindShaderComment hr == 0x%08x", _result);
		}
	}
	else {
		SVS* svs_result = (SVS*)result;
		_result			= HW.pDevice->CreateVertexShader(buffer, &svs_result->vs);
		if ( !SUCCEEDED(_result) ) {
			Msg			("! VS: %s", file_name);
			Msg			("! CreatePixelShader hr == 0x%08x", _result);
			return		E_FAIL;
		}

		LPCVOID			data		= NULL;
		_result			= D3DXFindShaderComment	(buffer,MAKEFOURCC('C','T','A','B'),&data,NULL);
		if (SUCCEEDED(_result) && data)
		{
			LPD3DXSHADER_CONSTANTTABLE	pConstants	= LPD3DXSHADER_CONSTANTTABLE(data);
			svs_result->constants.parse	(pConstants,0x2);
		} 
		else
		{
			Msg			("! VS: %s", file_name);
			Msg			("! D3DXFindShaderComment hr == 0x%08x", _result);
		}
	}

	if (disasm)
	{
		ID3DXBuffer*	pDisasm = 0;
		D3DXDisassembleShader(LPDWORD(buffer), FALSE, 0, &pDisasm);
		string_path		dname;
		xr_strconcat( dname, "disasm\\", file_name, ('v' == pTarget[0]) ? ".vs" : ".ps");
		IWriter*		W = FS.w_open("$logs$", dname);
		W->w(pDisasm->GetBufferPointer(), pDisasm->GetBufferSize());
		FS.w_close(W);
		_RELEASE(pDisasm);
	}

	return				_result;
}

static inline bool match_shader_id	( LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result );

class	includer				: public ID3DXInclude
{
public:
	HRESULT __stdcall	Open	(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
	{
		string_path				pname;
		xr_strconcat			(pname, ::Render->getShaderPath(), pFileName);
		IReader*		R		= FS.r_open	("$game_shaders$",pname);
		if (0==R)				{
			// possibly in shared directory or somewhere else - open directly
			R					= FS.r_open	("$game_shaders$",pFileName);
			if (0==R)			return			E_FAIL;
		}

		// duplicate and zero-terminate
		u32				size	= R->length();
		u8*				data	= xr_alloc<u8>	(size + 1);
        std::memcpy(data,R->pointer(),size);
		data[size]				= 0;
		FS.r_close				(R);

		*ppData					= data;
		*pBytes					= size;
		return	D3D_OK;
	}
	HRESULT __stdcall	Close	(LPCVOID	pData)
	{
		xr_free	(pData);
		return	D3D_OK;
	}
};

HRESULT	CRender::shader_compile(LPCSTR name, DWORD const* pSrcData, UINT SrcDataLen, LPCSTR pFunctionName, LPCSTR pTarget, DWORD Flags, void*& result)
{
	D3DXMACRO defines[128];
	int	def_it = 0;
	char c_smapsize[32];
	char c_gloss[32];
	char c_sun_shafts[32];
	char c_ssao[32];
	char c_sun_quality[32];
    char c_bokeh_quality[32];
	char c_pp_aa_quality[32];

	char sh_name[MAX_PATH] = "";
	u32 len	= 0;

	// options
	{
		xr_sprintf						(c_smapsize,"%04d",u32(o.smapsize));
		defines[def_it].Name		=	"SMAP_size";
		defines[def_it].Definition	=	c_smapsize;
		def_it						++	;
		VERIFY							( xr_strlen(c_smapsize) == 4 );
		xr_strcat(sh_name, c_smapsize); len+=4;
	}

	if (o.fp16_filter)		{
		defines[def_it].Name		=	"FP16_FILTER";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(o.fp16_filter); ++len;

	if (o.fp16_blend)		{
		defines[def_it].Name		=	"FP16_BLEND";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(o.fp16_blend); ++len;

	if (o.HW_smap)			{
		defines[def_it].Name		=	"USE_HWSMAP";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(o.HW_smap); ++len;

	if (o.HW_smap_PCF)			{
		defines[def_it].Name		=	"USE_HWSMAP_PCF";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(o.HW_smap_PCF); ++len;

	if (o.HW_smap_FETCH4)			{
		defines[def_it].Name		=	"USE_FETCH4";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(o.HW_smap_FETCH4); ++len;

	if (o.sjitter)			{
		defines[def_it].Name		=	"USE_SJITTER";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(o.sjitter); ++len;

	if (HW.Caps.raster_major >= 3)	{
		defines[def_it].Name		=	"USE_BRANCHING";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(HW.Caps.raster_major >= 3); ++len;

	if (HW.Caps.geometry.bVTF)	{
		defines[def_it].Name		=	"USE_VTF";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(HW.Caps.geometry.bVTF); ++len;

	if (o.Tshadows)			{
		defines[def_it].Name		=	"USE_TSHADOWS";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(o.Tshadows); ++len;

	if (ps_r_flags.test(R_FLAG_MBLUR)) {
		defines[def_it].Name		=	"USE_MBLUR";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(ps_r_flags.test(R_FLAG_MBLUR)); ++len;

	if (o.sunfilter)		{
		defines[def_it].Name		=	"USE_SUNFILTER";
		defines[def_it].Definition	=	"1";
		def_it						++	;
	}
	sh_name[len]='0'+char(o.sunfilter); ++len;

	if (o.forcegloss)		{
		xr_sprintf						(c_gloss,"%f",o.forcegloss_v);
		defines[def_it].Name		=	"FORCE_GLOSS";
		defines[def_it].Definition	=	c_gloss;
		def_it						++	;
	}
	sh_name[len]='0'+char(o.forcegloss); ++len;

	if (o.forceskinw)		{
		defines[def_it].Name		=	"SKIN_COLOR";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(o.forceskinw); ++len;

	if (o.ssao_blur_on)
	{
		defines[def_it].Name		=	"USE_SSAO_BLUR";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(o.ssao_blur_on); ++len;

	if (o.ssao_hbao)
	{
		defines[def_it].Name		=	"USE_HBAO";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(o.ssao_hbao); ++len;

	if (o.ssao_opt_data)
	{
		defines[def_it].Name		=	"SSAO_OPT_DATA";
		if (o.ssao_half_data)
			defines[def_it].Definition	=	"2";
		else
			defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(o.ssao_opt_data ? (o.ssao_half_data ? 2 : 1) : 0); ++len;

	// skinning
	if (m_skinning<0)		{
		defines[def_it].Name		=	"SKIN_NONE";
		defines[def_it].Definition	=	"1";
		def_it						++	;
		sh_name[len]='1'; ++len;
	}
	else
	{
		sh_name[len]='0'; ++len;
	}

	if (0==m_skinning)		{
		defines[def_it].Name		=	"SKIN_0";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(0==m_skinning); ++len;

	if (1==m_skinning)		{
		defines[def_it].Name		=	"SKIN_1";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(1==m_skinning); ++len;

	if (2==m_skinning)		{
		defines[def_it].Name		=	"SKIN_2";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(2==m_skinning); ++len;

	if (3==m_skinning)		{
		defines[def_it].Name		=	"SKIN_3";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(3==m_skinning); ++len;

	if (4==m_skinning)		{
		defines[def_it].Name		=	"SKIN_4";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len]='0'+char(4==m_skinning); ++len;
	
	//	Igor: need restart options
	if (RImplementation.o.advancedpp)
	{
		if (ps_r_flags.test(R_FLAG_SOFT_WATER))
		{
			defines[def_it].Name = "USE_SOFT_WATER";
			defines[def_it].Definition = "1";
			def_it++;
			sh_name[len] = '1'; ++len;
		}
		else
			sh_name[len] = '0'; ++len;

		if (ps_r_flags.test(R_FLAG_SOFT_PARTICLES))
		{
			defines[def_it].Name = "USE_SOFT_PARTICLES";
			defines[def_it].Definition = "1";
			def_it++;
			sh_name[len] = '1'; ++len;
		}
		else
			sh_name[len] = '0'; ++len;

		if (ps_r_bokeh_quality > 0)
		{
			defines[def_it].Name = "USE_DOF";
			defines[def_it].Definition = "1";
			def_it++;
			sh_name[len] = '1'; ++len;
		}
		else
			sh_name[len] = '0'; ++len;

		if (ps_r_sun_shafts > 0)
		{
			xr_sprintf(c_sun_shafts, "%d", ps_r_sun_shafts);
			defines[def_it].Name = "SUN_SHAFTS_QUALITY";
			defines[def_it].Definition = c_sun_shafts;
			def_it++;
			sh_name[len] = '0' + char(ps_r_sun_shafts); ++len;
		}
		else
			sh_name[len] = '0'; ++len;

		if (ps_r_ssao > 0)
		{
			xr_sprintf(c_ssao, "%d", ps_r_ssao);
			defines[def_it].Name = "SSAO_QUALITY";
			defines[def_it].Definition = c_ssao;
			def_it++;
			sh_name[len] = '0' + char(ps_r_ssao); ++len;
		}
		else
			sh_name[len] = '0'; ++len;

		if (ps_r_sun_quality)
		{
			xr_sprintf(c_sun_quality, "%d", ps_r_sun_quality);
			defines[def_it].Name = "SUN_QUALITY";
			defines[def_it].Definition = c_sun_quality;
			def_it++;
			sh_name[len] = '0' + char(ps_r_sun_quality); ++len;
		}
		else
			sh_name[len] = '0'; ++len;

		if (ps_r_flags.test(R_FLAG_STEEP_PARALLAX))
		{
			defines[def_it].Name = "ALLOW_STEEPPARALLAX";
			defines[def_it].Definition = "1";
			def_it++;
			sh_name[len] = '1'; ++len;
		}
		else
			sh_name[len] = '0'; ++len;

		if (ps_r_bokeh_quality > 0)
		{
			xr_sprintf(c_bokeh_quality, "%d", ps_r_bokeh_quality);
			defines[def_it].Name = "BOKEH_QUALITY";
			defines[def_it].Definition = c_bokeh_quality;
			def_it++;
			sh_name[len] = '0' + char(ps_r_bokeh_quality); ++len;
		}
		else
			sh_name[len] = '0'; ++len;

		if (ps_r_pp_aa_quality > 0)
		{
			xr_sprintf(c_pp_aa_quality, "%d", ps_r_pp_aa_quality);
			defines[def_it].Name = "PP_AA_QUALITY";
			defines[def_it].Definition = c_pp_aa_quality;
			def_it++;
			sh_name[len] = '0' + char(ps_r_pp_aa_quality); ++len;
		}
		else
			sh_name[len] = '0'; ++len;
	}

	// Puddles
	defines[def_it].Name = "USE_PUDDLES";
	defines[def_it].Definition = "1";
	def_it++;

	sh_name[len] = 0; // intorr: String must be null-terminated.

	// finish
	defines[def_it].Name			=	0;
	defines[def_it].Definition		=	0;
	def_it							++;

	HRESULT		_result = E_FAIL;

	string_path	folder_name, folder;
	xr_strcpy		( folder, "objects\\r2" );
    string512 shaderFilename;
    bool bGetFilenameResult = FS.getFileName(name, shaderFilename);
    VERIFY(bGetFilenameResult);
	xr_strcat		( folder, shaderFilename);
	xr_strcat		( folder, "." );

	char extension[3];
	strncpy_s		( extension, pTarget, 2 );
	xr_strcat		( folder, extension );

	FS.update_path	( folder_name, "$game_shaders$", folder );
	xr_strcat		( folder_name, "\\" );
	
	m_file_set.clear( );
	FS.file_list	( m_file_set, folder_name, FS_ListFiles | FS_RootOnly, "*");

	string_path temp_file_name, file_name;
	if ( !match_shader_id(name, sh_name, m_file_set, temp_file_name) ) {
		string_path file;
		xr_strcpy		( file, "shaders_cache\\r2\\" );
		xr_strcat		( file, name );
		xr_strcat		( file, "." );
		xr_strcat		( file, extension );
		xr_strcat		( file, "\\" );
		xr_strcat		( file, sh_name );
		FS.update_path	( file_name, "$app_data_root$", file);
	}
	else {
		xr_strcpy		( file_name, folder_name );
		xr_strcat		( file_name, temp_file_name );
	}

    //When graphical debugger enabled, we want to always compile shader from source, because they changing to frequently
    //#Giperion: Shader cache needs to be refactored
	if (false && FS.exist(file_name))
	{
//		Msg				( "opening library or cache shader..." );
		IReader* file = FS.r_open(file_name);
		if (file->length()>4)
		{
			u32 crc = 0;
			crc = file->r_u32();

			//boost::crc_32_type		processor;
			//processor.process_block	( file->pointer(), ((char*)file->pointer()) + file->elapsed() );
			u32 const real_crc = crc32(file->pointer(), file->elapsed());

			if ( real_crc == crc ) {
				_result				= create_shader(name, pTarget, (DWORD*)file->pointer(), file->elapsed(), file_name, result, o.disasm);
				//if ( !SUCCEEDED(_result) ) {
				//	Msg				("! create shader failed");
				//}
				//else {
				//	Msg				( "create shaders succeeded" );
				//}
			}
		}
		file->close();
	}

	if (FAILED(_result))
	{
		// 
		if (0==xr_strcmp(pFunctionName,"main"))	{
			if ('v'==pTarget[0])			pTarget = D3DXGetVertexShaderProfile	(HW.pDevice);	// vertex	"vs_2_a"; //	
			else							pTarget = D3DXGetPixelShaderProfile		(HW.pDevice);	// pixel	"ps_2_a"; //	
		}

		includer					Includer;
		LPD3DXBUFFER				pShaderBuf	= NULL;
		LPD3DXBUFFER				pErrorBuf	= NULL;
		LPD3DXCONSTANTTABLE			pConstants	= NULL;
		LPD3DXINCLUDE               pInclude	= (LPD3DXINCLUDE)&Includer;
		
		_result						= D3DXCompileShader((LPCSTR)pSrcData,SrcDataLen,defines,pInclude,pFunctionName,pTarget,Flags,&pShaderBuf,&pErrorBuf,&pConstants);
		if (SUCCEEDED(_result)) {
			IWriter* file = FS.w_open(file_name);

			u32 const crc = crc32(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize());

			file->w_u32				(crc);
			file->w					( pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
			FS.w_close				(file);

			_result					= create_shader(name, pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(), file_name, result, o.disasm);
		}
		else {
			Msg						("! %s", file_name);
			if ( pErrorBuf )
				Msg					("! error: %s",(LPCSTR)pErrorBuf->GetBufferPointer());
			else
				Msg					("Can't compile shader hr=0x%08x", _result);
		}
	}

	return							_result;
}

static inline bool match_shader		( LPCSTR const debug_shader_id, LPCSTR const full_shader_id, LPCSTR const mask, size_t const mask_length )
{
	u32 const full_shader_id_length	= xr_strlen( full_shader_id );
	R_ASSERT_FORMAT( full_shader_id_length == mask_length, "bad cache for shader %s, [%s], [%s]",
			debug_shader_id, mask,full_shader_id);

	char const* i			= full_shader_id;
	char const* const e		= full_shader_id + full_shader_id_length;
	char const* j			= mask;
	for ( ; i != e; ++i, ++j ) {
		if ( *i == *j )
			continue;

		if ( *j == '_' )
			continue;

		return				false;
	}

	return					true;
}

static inline bool match_shader_id	( LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result )
{
#if 1
#ifdef DEBUG
	LPCSTR temp					= "";
	bool found					= false;
	FS_FileSet::const_iterator	i = file_set.begin();
	FS_FileSet::const_iterator	const e = file_set.end();
	for ( ; i != e; ++i ) {
		if ( match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size() ) ) {
			VERIFY				( !found );
			found				= true;
			temp				= (*i).name.c_str();
		}
	}

	xr_strcpy					( result, temp );
	return						found;
#else // #ifdef DEBUG
	FS_FileSet::const_iterator	i = file_set.begin();
	FS_FileSet::const_iterator	const e = file_set.end();
	for ( ; i != e; ++i ) {
		if ( match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size() ) ) {
			xr_strcpy			( result, (*i).name.c_str() );
			return				true;
		}
	}

	return						false;
#endif // #ifdef DEBUG
#endif// #if 1
}

void CRender::Screenshot(ScreenshotMode mode, LPCSTR name)
{
	ScreenshotManager.MakeScreenshot(mode, name);
}