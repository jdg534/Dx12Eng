#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace RendererUnitTests
{		
	TEST_CLASS(DeviceCreation)
	{
	public:
		
		TEST_METHOD(DeviceCreate_d3d12DeviceCreationWorks)
		{
			Assert::AreEqual(1,2);
		}

	};
}