// Rietmon: ������� �� ������ ���. � ���� �� ���, �� ���� ���������� ��� �������, ��� ��� ����������� ���� :)
#pragma once
#include "xrGame/Torch.h"

using namespace System;;

namespace XRay
{
	public ref class Torch
	{
	public:
		Torch(IntPtr InNativeObject);

		property bool IsEnabled
		{
			bool get()
			{
				return pNativeObject->torch_active();
			}

			void set(bool value)
			{
				torch->Switch(value);
			}
		}

	public:
	
	CTorch* pNativeObject;
	
	};
}