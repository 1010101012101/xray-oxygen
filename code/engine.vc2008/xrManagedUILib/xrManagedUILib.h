#pragma once
#include "../xrUICore/xrUIXmlParser.h"

using namespace System;
namespace XRay
{
	namespace xrManagedUILib
	{
		public ref class UIXMLParser
		{
		internal:
			CUIXml* NativeParserObject;

		public:
			UIXMLParser();
			virtual ~UIXMLParser();
		};

		public ref class Test sealed
		{
		public:
			Test();
			virtual ~Test();
		};
	}
}