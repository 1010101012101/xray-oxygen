// Rietmon: ������� �� ������ ���. � ���� �� ���, �� ���� ���������� ��� �������, ��� ��� ����������� ���� :)
#include "stdafx.h"
#include "API/Torch.h"

XRay::Torch::Torch(IntPtr InNativeObject)
{
	CAST_TO_NATIVE_OBJECT(CTorch, InNativeObject);
}
