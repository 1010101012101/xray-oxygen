#include "stdafx.h"
#pragma hdrstop

#include "../xrRenderDX10/dx10BufferUtils.h"

CBackend			RCache;

// Create Quad-IB
#ifdef USE_DX11

// Igor: is used to test bug with rain, particles corruption
void CBackend::RestoreQuadIBData()
{
	// Igor: never seen this corruption for DX10
	;
}

void CBackend::CreateQuadIB		()
{
	static const u32 dwTriCount	= 4*1024;
	static const u32 dwIdxCount	= dwTriCount*2*3;
	u16		IndexBuffer[dwIdxCount];
	u16		*Indices		= IndexBuffer;

	D3D_BUFFER_DESC desc;
	desc.ByteWidth = dwIdxCount*2;
	desc.Usage = D3D_USAGE_DEFAULT;
	desc.BindFlags = D3D_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D_SUBRESOURCE_DATA subData;
	subData.pSysMem = IndexBuffer;

	{
		int		Cnt = 0;
		int		ICnt= 0;
		for (int i=0; i<dwTriCount; i++)
		{
			Indices[ICnt++]=u16(Cnt+0);
			Indices[ICnt++]=u16(Cnt+1);
			Indices[ICnt++]=u16(Cnt+2);

			Indices[ICnt++]=u16(Cnt+3);
			Indices[ICnt++]=u16(Cnt+2);
			Indices[ICnt++]=u16(Cnt+1);

			Cnt+=4;
		}
	}

	R_CHK(HW.pDevice->CreateBuffer		( &desc, &subData, &QuadIB));
	HW.stats_manager.increment_stats_ib	( QuadIB);
}

#else

// Igor: is used to test bug with rain, particles corruption
void CBackend::RestoreQuadIBData()
{
	const u32 dwTriCount	= 4*1024;
	u16		*Indices		= 0;
	R_CHK(QuadIB->Lock(0,0,(void**)&Indices,0));
	{
		int		Cnt = 0;
		int		ICnt= 0;
		for (int i=0; i<dwTriCount; i++)
		{
			Indices[ICnt++]=u16(Cnt+0);
			Indices[ICnt++]=u16(Cnt+1);
			Indices[ICnt++]=u16(Cnt+2);

			Indices[ICnt++]=u16(Cnt+3);
			Indices[ICnt++]=u16(Cnt+2);
			Indices[ICnt++]=u16(Cnt+1);

			Cnt+=4;
		}
	}
	R_CHK(QuadIB->Unlock());
}


void CBackend::CreateQuadIB		()
{
	const u32 dwTriCount	= 4*1024;
	const u32 dwIdxCount	= dwTriCount*2*3;
	u16		*Indices		= 0;
	u32		dwUsage			= D3DUSAGE_WRITEONLY;
	if (HW.Caps.geometry.bSoftware)	dwUsage|=D3DUSAGE_SOFTWAREPROCESSING;
	R_CHK(HW.pDevice->CreateIndexBuffer	(dwIdxCount*2,dwUsage,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&QuadIB,NULL));
	HW.stats_manager.increment_stats_ib	(QuadIB);
	R_CHK(QuadIB->Lock(0,0,(void**)&Indices,0));
	{
		int		Cnt = 0;
		int		ICnt= 0;
		for (int i=0; i<dwTriCount; i++)
		{
			Indices[ICnt++]=u16(Cnt+0);
			Indices[ICnt++]=u16(Cnt+1);
			Indices[ICnt++]=u16(Cnt+2);

			Indices[ICnt++]=u16(Cnt+3);
			Indices[ICnt++]=u16(Cnt+2);
			Indices[ICnt++]=u16(Cnt+1);

			Cnt+=4;
		}
	}
	R_CHK(QuadIB->Unlock());
}

#endif

// Device dependance
void CBackend::OnDeviceCreate	()
{
	CreateQuadIB		();

	// streams
	Vertex.Create		();
	Index.Create		();

	// invalidate caching
	Invalidate			();
}

void CBackend::OnDeviceDestroy()
{
	// streams
	Index.Destroy		();
	Vertex.Destroy		();

	// Quad
	HW.stats_manager.decrement_stats_ib	(QuadIB);
	_RELEASE							(QuadIB);
}