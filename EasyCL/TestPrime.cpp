#include "stdafx.h"
#include "Tests.hpp"

// Local array.
static const unsigned N = 20000;
static cl_uint la[N];

static unsigned Square(unsigned x)
{
	return x * x;
}

static void Test(unsigned* output, unsigned n)
{
	unsigned i = 1;
	unsigned x = 3;
	output[0] = 2;
	
	while (i < n)
	{
		bool prime = true;
		unsigned j;
		for (j = 0; j < i && Square(output[j]) < x; ++j)
		{
			if (x % output[j] == 0)
			{
				prime = false;
				break;
			}
		}
		if (prime)
			output[i++] = x;
		x++;
	}
}

void TestPrime()
{
	EasyCL ecl("TestPrime");
	ecl.Initialize();

	// д������������
	std::auto_ptr<cl::Buffer> buf = ecl.CreateBuffer<cl_uint>(N);
	std::auto_ptr<WriteBufferAction> write = ActionFactory::CreateWriteBufferAction<cl_uint>("Write to buffer", buf.get(), la, N);

	// �����ں˶���������GPU����Test())��
	std::auto_ptr<cl::Kernel> ker = ecl.Compile("Test", "TestPrime.cl");
	std::auto_ptr<cl::NDRange> global_grid = EasyCL::CreateNDRange(1);
	std::auto_ptr<cl::NDRange> workgroup_grid = EasyCL::CreateNDRange(1);
	std::auto_ptr<KernelAction> kernel = ActionFactory::CreateKernelAction("Call Test()", ker.get(), global_grid.get(), workgroup_grid.get());
	kernel->Kernel->setArg(0, *buf);
	kernel->Kernel->setArg(1, (cl_uint)N);

	// ��������������
	std::auto_ptr<ReadBufferAction> read = ActionFactory::CreateReadBufferAction<cl_uint>("Read from buffer", buf.get(), la, N);

	// �趨������������ϵ��
	read->BeforeActions.push_back(kernel.get());
	kernel->BeforeActions.push_back(write.get());

	// �ŵ�WorkFlow��ִ�С�
	std::auto_ptr<WorkFlow> wf = ecl.CreateWorkFlow();
	wf->AddAction(read.get());
	wf->AddAction(kernel.get());
	wf->AddAction(write.get());
	wf->CreateTopologicalSequence();
	wf->StartWorkFlow(1);

	clock_t start = clock();
	Test(la, N);
	clock_t end = clock();
	std::cout << "CPU��ʱ��" << end - start << "ms" << std::endl;

	return;

	// �������
	for (int i = 0; i < N; ++i)
	{
		if ((i + 1) % 8 == 0)
			std::cout << std::setw(5) << la[i] << std::endl;
		else
			std::cout << std::setw(5) << la[i] << " ";
	}
}
