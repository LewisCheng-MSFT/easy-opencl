#include "stdafx.h"
#include "Tests.hpp"
#include "SDKBitMap.hpp"

static void Test(streamsdk::uchar4* data, cl_uint width, cl_uint height)
{
	for (cl_uint i = 0; i < width; ++i)
	{
		for (cl_uint j = 0; j < height; ++j)
		{
			data[j * width + i].x = 255 - data[j * width + i].x;
			data[j * width + i].y = 255 - data[j * width + i].y;
			data[j * width + i].z = 255 - data[j * width + i].z;
		}
	}
}

void TestInverse()
{
	EasyCL ecl("TestInverse");
	ecl.Initialize();

	// ����λͼ��
	streamsdk::SDKBitMap bmp("Aurora.bmp");
	if (!bmp.isLoaded())
	{
		std::cout << "λͼ����ʧ�ܣ�" << std::endl;
		return;
	}
	std::cout << "λͼ��С��" << bmp.getWidth() << " x " << bmp.getHeight() << std::endl;
	size_t total_size = bmp.getWidth() * bmp.getHeight();
	size_t total_byte_size = total_size * sizeof(streamsdk::uchar4);

	// ����block��С��
	cl_uint block_width = 16;
	cl_uint block_height = 16;
	while (bmp.getWidth() % block_width != 0)
		--block_width;
	while (bmp.getHeight() % block_height != 0)
		--block_height;
	std::cout << "WorkGroup��С���ֿ��С����" << block_width << " x " << block_height << std::endl;
	
	// д������������
	std::auto_ptr<cl::Buffer> buf = ecl.CreateBuffer<cl_uchar4>(total_size);
	std::auto_ptr<WriteBufferAction> write = ActionFactory::CreateWriteBufferAction<cl_uchar4>("Write to buffer", buf.get(), bmp.getPixels(), total_size);

	// �����ں˶���������GPU����Test())��
	std::auto_ptr<cl::Kernel> ker = ecl.Compile("Test", "TestInverse.cl");
	std::auto_ptr<cl::NDRange> global_grid = EasyCL::CreateNDRange(bmp.getWidth(), bmp.getHeight());
	std::auto_ptr<cl::NDRange> workgroup_grid = EasyCL::CreateNDRange(block_width, block_height);
	std::auto_ptr<KernelAction> kernel = ActionFactory::CreateKernelAction("Call Test()", ker.get(), global_grid.get(), workgroup_grid.get());
	kernel->Kernel->setArg(0, *buf);
	kernel->Kernel->setArg(1, bmp.getWidth());

	// ��������������
	cl_uchar4* la = new cl_uchar4[total_size];
	std::auto_ptr<ReadBufferAction> read = ActionFactory::CreateReadBufferAction<cl_uchar4>("Read from buffer", buf.get(), la, total_size);

	// �趨������������ϵ��
	read->BeforeActions.push_back(kernel.get());
	kernel->BeforeActions.push_back(write.get());

	// �ŵ�WorkFlow��ִ�С�
	std::auto_ptr<WorkFlow> wf = ecl.CreateWorkFlow();
	wf->AddAction(read.get());
	wf->AddAction(kernel.get());
	wf->AddAction(write.get());
	wf->CreateTopologicalSequence();
	wf->StartWorkFlow(10);

	memcpy(bmp.getPixels(), la, total_byte_size);
	bmp.write("Aurora Output.bmp");

	clock_t start = clock();
	Test(bmp.getPixels(), bmp.getWidth(), bmp.getHeight());
	clock_t end = clock();
	std::cout << "CPU��ʱ��" << end - start << "ms" << std::endl;
}
