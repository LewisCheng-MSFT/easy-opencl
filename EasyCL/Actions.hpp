#pragma once

#include "Report.hpp"

// 一切动作的抽象
class AbstractAction
{
public:
	AbstractAction(const std::string& name)
		: Name(name),
		  Queue(0)
	{
	}

	virtual ~AbstractAction()
	{
	}

	virtual cl_int Execute()
	{
		report.Trace("WorkFlow::Execute()") << "执行动作：" << Name << std::endl;
		return CL_SUCCESS;
	}

	void AddBeforeAction(AbstractAction* action)
	{
		BeforeActions.push_back(action);
	}

public:
	const cl::CommandQueue* Queue;
	std::string Name;
	std::vector<AbstractAction*> BeforeActions;
	std::vector<cl::Event> BeforeEvents;
	cl::Event Completion;
};

// 内核调用
class KernelAction : public AbstractAction
{
public:
	KernelAction(const std::string& name)
		: AbstractAction(name),
		  Kernel(0),
		  GlobalIdOffset(0),
		  GlobalGrid(0),
		  WorkGroupGrid(0)
	{
	}

	cl_int Execute()
	{
		AbstractAction::Execute();
		return Queue->enqueueNDRangeKernel(
			*Kernel,
			*GlobalIdOffset,
			*GlobalGrid,
			*WorkGroupGrid,
			&BeforeEvents,
			&Completion
		);
	}

public:
	cl::Kernel* Kernel;
	const cl::NDRange* GlobalIdOffset;
	const cl::NDRange* GlobalGrid;
	const cl::NDRange* WorkGroupGrid;
};

// 从缓冲区读
class ReadBufferAction : public AbstractAction
{
public:
	ReadBufferAction(const std::string& name)
		: AbstractAction(name),
		  Buffer(0),
		  Offset(0),
		  Size(0),
		  Ptr(0)
	{
	}

	cl_int Execute()
	{
		AbstractAction::Execute();
		return Queue->enqueueReadBuffer(
			*Buffer,
			CL_FALSE,
			Offset,
			Size,
			Ptr,
			&BeforeEvents,
			&Completion
		);
		 return 0;
	}

public:
	const cl::Buffer* Buffer;
	size_t Offset;
	size_t Size;
	void* Ptr;
};

// 向缓冲区写
class WriteBufferAction : public AbstractAction
{
public:
	WriteBufferAction(const std::string& name)
		: AbstractAction(name),
		  Buffer(0),
		  Offset(0),
		  Size(0),
		  Ptr(0)
	{
	}

	cl_int Execute()
	{
		AbstractAction::Execute();
		return Queue->enqueueWriteBuffer(
			*Buffer,
			CL_FALSE,
			Offset,
			Size,
			Ptr,
			&BeforeEvents,
			&Completion
		);
	}

public:
	const cl::Buffer* Buffer;
	size_t Offset;
	size_t Size;
	const void* Ptr;
};

// 动作工厂
class ActionFactory
{
public:
	template <typename T>
	static std::auto_ptr<WriteBufferAction> CreateWriteBufferAction(
		const std::string& name,
		const cl::Buffer* buffer,
		const void* ptr,
		size_t count,
		size_t offset = 0)
	{
		std::auto_ptr<WriteBufferAction> write(new WriteBufferAction(name));
		write->Buffer = buffer;
		write->Size = count * sizeof(T);
		write->Offset = offset;
		write->Ptr = ptr;
		return write;
	}

	template <typename T>
	static std::auto_ptr<ReadBufferAction> CreateReadBufferAction(
		const std::string& name,
		const cl::Buffer* buffer,
		void* ptr,
		size_t count,
		size_t offset = 0)
	{
		std::auto_ptr<ReadBufferAction> read(new ReadBufferAction(name));
		read->Buffer = buffer;
		read->Size = count * sizeof(T);
		read->Offset = offset;
		read->Ptr = ptr;
		return read;
	}

	static std::auto_ptr<KernelAction> CreateKernelAction(
		const std::string& name,
		cl::Kernel* kernel,
		const cl::NDRange* global_grid,
		const cl::NDRange* workgroup_grid,
		const cl::NDRange* global_id_offset = &cl::NullRange)
	{
		std::auto_ptr<KernelAction> ker(new KernelAction(name));
		ker->Kernel = kernel;
		ker->GlobalGrid = global_grid;
		ker->WorkGroupGrid = workgroup_grid;
		ker->GlobalIdOffset = global_id_offset;
		return ker;
	}

private:
	// 防止实例化。
	virtual void foo() = 0;
};
