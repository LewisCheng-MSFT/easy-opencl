#pragma once

#include "WorkFlow.hpp"

static enum ERROR_CODES_EASYCL {
	NO_AVAILABLE_DEVICE = 2001,
	FAILED_TO_READ_KERNEL_FILE
};

class EasyCL
{
public:
	EasyCL(const std::string& name)
		: name_(name)
	{
		UseGPU();
	}

	~EasyCL(void)
	{
		delete context_;
		delete queue_;
	}

	bool Initialize()
	{
		report.Trace("EasyCL::Initialize()") << "EasyCLӦ�ó���" << name_ << std::endl;

		// 1. ��ȡƽ̨��
		ErrorCode = cl::Platform::get(&platforms_);
		if (ErrorCode != CL_SUCCESS)
		{
			report.Error(ErrorCode, "EasyCL::Initialize()") << "��ȡƽ̨ʧ��" << std::endl;
			return false;
		}
		std::vector<cl::Platform>::const_iterator i;
		if (platforms_.size() > 0)
		{
			for (i = platforms_.cbegin(); i != platforms_.cend(); ++i)
			{ // ѡ���һ��AMDƽ̨��
				if (i->getInfo<CL_PLATFORM_VENDOR>() == "Advanced Micro Devices, Inc.")
					break;
			}
		}
		report.Trace("EasyCL::Initialize()") << "ƽ̨��" << i->getInfo<CL_PLATFORM_VENDOR>() << std::endl;

		// 2. ��ȡ������
		cl_context_properties cps[3] = 
		{ 
			CL_CONTEXT_PLATFORM,			// ��
			(cl_context_properties)(*i)(),	// ֵ
			0								// �����б��ս�
		};
		context_ = new cl::Context(DeviceType, cps, NULL, NULL, &ErrorCode);
		if (ErrorCode != CL_SUCCESS)
		{
			report.Error(ErrorCode, "EasyCL::Initialize()") << "��ȡ����ʧ��" << std::endl;
			return false;
		}

		// 3. ��ȡ�豸��
		devices_ = context_->getInfo<CL_CONTEXT_DEVICES>(&ErrorCode);
		if (ErrorCode != CL_SUCCESS)
		{
			report.Error(ErrorCode, "EasyCL::Initialize()") << "��ȡ�豸ʧ��" << std::endl;
			return false;
		}
		std::vector<cl::Device>::iterator j;
		int n;
		for (n = 0, j = devices_.begin(); j != devices_.end(); ++n, ++j)
		{
			report.Trace("EasyCL::Initialize()") << "�豸[" << n << "]��" << j->getInfo<CL_DEVICE_NAME>() << std::endl;
		}
		if (n == 0)
		{
			report.Error(NO_AVAILABLE_DEVICE, "EasyCL::Initialize()") << "�޿����豸" << std::endl;
			return false;
		}
		// Ĭ��ѡ���0���豸��
		device_ = &devices_[0];
		report.Trace("EasyCL::Initialize()") << "ѡ����豸��" << device_->getInfo<CL_DEVICE_NAME>() << std::endl;

		// 4. �������豸�󶨵�������С�
		queue_ = new cl::CommandQueue(*context_, *device_, 0/*�������б�*/, &ErrorCode);
		if (ErrorCode != CL_SUCCESS)
		{
			report.Error(ErrorCode, "EasyCL::Initialize()") << "�����������ʧ��" << std::endl;
			return false;
		}
		return true;
	}

	std::auto_ptr<cl::Kernel> Compile(const std::string& kernel_name, const std::string& kernel_file, const std::string& options = "")
	{
		// �����ں��ļ���
		report.Trace("EasyCL::Compile()") << "�ں�[" << kernel_name << "]��" << kernel_file << std::endl;
		std::ifstream in(kernel_file, std::ifstream::binary);
		if (!in)
		{
			report.Error(FAILED_TO_READ_KERNEL_FILE, "EasyCL::Compile()") << "��ȡ�ں��ļ�" << kernel_file << "ʧ��" << std::endl;
			return static_cast<std::auto_ptr<cl::Kernel> >(0);
		}
		
		// ����ļ����ȡ�
		in.seekg(0, std::ifstream::end);
		std::streamsize length = in.tellg();
		in.seekg(0, std::ifstream::beg);
		
		// �����ļ����ݡ�
		std::vector<char> text(length + 1);
		in.read(text.data(), length);
		text[length] = '\0';
		in.close();

		// ����Դ����
		cl::Program::Sources sources;
		sources.push_back(std::make_pair(text.data(), length));

		// ��������
		cl::Program program(*context_, sources, &ErrorCode);
		if (ErrorCode != CL_SUCCESS)
		{
			report.Error(ErrorCode, "EasyCL::Compile()") << "��������ʧ��" << std::endl;
			return static_cast<std::auto_ptr<cl::Kernel> >(0);
		}

		// Ϊ�����豸�������
		ErrorCode = program.build(devices_, options.c_str());
		if (ErrorCode != CL_SUCCESS)
		{
			std::ostream& os = report.Error(ErrorCode, "EasyCL::Compile()") << "�������ʧ��";
			if (ErrorCode == CL_BUILD_PROGRAM_FAILURE)
			{ // ֻ�����﷨���󣬶������豸��һ����
				os << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device_) << std::endl;
			}
			else
			{
				os << std::endl;
			}
			return static_cast<std::auto_ptr<cl::Kernel> >(0);;
		}

		// �����ںˡ�
		std::auto_ptr<cl::Kernel> kernel(new cl::Kernel(program, kernel_name.c_str(), &ErrorCode));
		if (ErrorCode != CL_SUCCESS)
		{
			report.Error(ErrorCode, "EasyCL::Compile()") << "�����ں�ʧ��" << std::endl;
			return static_cast<std::auto_ptr<cl::Kernel> >(0);
		}

		return kernel;
	}

	void UseCPU()
	{
		DeviceType = CL_DEVICE_TYPE_CPU;
	}

	void UseGPU()
	{
		DeviceType = CL_DEVICE_TYPE_GPU;
	}

	// Global memory
	template <typename T>
	std::auto_ptr<cl::Buffer> CreateBuffer(size_t count, int read_write_flag = CL_MEM_READ_WRITE)
	{
		std::auto_ptr<cl::Buffer> buffer(
			new cl::Buffer(
				*context_,
				read_write_flag | CL_MEM_ALLOC_HOST_PTR,
				count * sizeof(T),
				0,
				&ErrorCode
			)
		);
		if (ErrorCode != CL_SUCCESS)
		{
			report.Error(ErrorCode, "EasyCL::CreateBuffer()") << "����������ʧ��" << std::endl;
			return static_cast<std::auto_ptr<cl::Buffer> >(0);
		}
		return buffer;
	}

	// Local memory
	template <typename T>
	static cl::LocalSpaceArg CreateLocalSpace(size_t count)
	{
		cl::LocalSpaceArg arg = { count * sizeof(T) };
		return arg;
	}

	// һά����
	static std::auto_ptr<cl::NDRange> CreateNDRange(size_t d0)
	{
		return static_cast<std::auto_ptr<cl::NDRange> >(new cl::NDRange(d0));
	}

	// ��ά����
	static std::auto_ptr<cl::NDRange> CreateNDRange(size_t d0, size_t d1)
	{
		return static_cast<std::auto_ptr<cl::NDRange> >(new cl::NDRange(d0, d1));
	}

	// ��ά����
	static std::auto_ptr<cl::NDRange> CreateNDRange(size_t d0, size_t d1, size_t d2)
	{
		return static_cast<std::auto_ptr<cl::NDRange> >(new cl::NDRange(d0, d1, d2));
	}

	std::auto_ptr<WorkFlow> CreateWorkFlow()
	{
		return static_cast<std::auto_ptr<WorkFlow> >(new WorkFlow(queue_));
	}

public:
	cl_int ErrorCode;
	cl_device_type DeviceType;

protected:
	std::string name_;
	std::vector<cl::Platform> platforms_;
	cl::Context* context_;
	std::vector<cl::Device> devices_;
	cl::Device* device_;
	cl::CommandQueue* queue_;
};
