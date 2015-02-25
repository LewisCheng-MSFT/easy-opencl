#pragma once

#include "Actions.hpp"

static enum ERROR_CODES_WORKFLOW {
	DEADLOCK_FOUND_IN_ACTION_GRAPH = 1001
};

class WorkFlow
{
public:
	WorkFlow(cl::CommandQueue* queue)
		: queue_(queue)
	{
		// 为打乱操作初始化随机种子。
		srand(time(0));
	}

	void AddAction(AbstractAction* action)
	{
		action->Queue = queue_;
		actions_.push_back(action);
	}

	// 为动作依赖图创建拓扑序列。
	bool CreateTopologicalSequence()
	{
		seq_.clear();
		while (!actions_.empty())
		{
			// 查找入度为0的动作。
			std::vector<AbstractAction*>::const_iterator i;
			for (i = actions_.cbegin(); i != actions_.cend(); ++i)
			{
				if ((*i)->BeforeActions.empty())
					break;
			}
			if (i != actions_.cend())
			{
				AbstractAction* rem = *i;
				actions_.erase(i);
				seq_.push_back(rem);
				// 删除所有对该动作的引用
				std::vector<AbstractAction*>::const_iterator j;
				for (j = actions_.cbegin(); j != actions_.cend(); ++j)
				{
					std::vector<AbstractAction*>& before = (*j)->BeforeActions;
					std::vector<AbstractAction*>::const_iterator new_end = 
						std::remove(before.begin(), before.end(), rem);
					before.erase(new_end, before.end());
				}
			}
			else
			{ // 剩下全是入度不为0的动作，表示有死锁。
				report.Error(DEADLOCK_FOUND_IN_ACTION_GRAPH, "WorkFlow::CreateTopologicalSequence()") << "检测到动作依赖图有死锁！" << std::endl;
				return false;
			}
		}

		return true;
	}

	// 按拓扑序列执行工作流。
	bool StartWorkFlow(size_t repeat_count = 1, bool blocking = true)
	{
#ifdef SET_TIMER
		clock_t total = 0;
#endif
		int n = 0;
		while (repeat_count > n)
		{
			report.Trace("WorkFlow::StartWorkFlow()") << "---------- 第" << n + 1 << "轮开始 ----------" << std::endl;
#ifdef SET_TIMER
			// 该轮计时开始。
			clock_t start = clock();
#endif
			for (std::vector<AbstractAction*>::iterator i = seq_.begin(); i != seq_.end(); ++i)
			{
				// 收集先决操作的完成事件，以便等待它们完成。
				std::vector<AbstractAction*>& before = (*i)->BeforeActions;
				std::vector<cl::Event>& events = (*i)->BeforeEvents;
				events.clear();
				for (std::vector<AbstractAction*>::iterator j = before.begin(); j != before.end(); ++j)
					events.push_back((*j)->Completion);

				// 清空自身的完成事件。
				(*i)->Completion = cl::Event();

				cl_int ErrorCode = (*i)->Execute();
				if (ErrorCode != CL_SUCCESS)
				{
					report.Error(ErrorCode, "WorkFlow::StartWorkFlow()") << "动作\"" << (*i)->Name << "\"执行失败" << std::endl;
					return false;
				}
			}
		
			if (blocking) queue_->finish();
#ifdef SET_TIMER
			// 该轮计时结束。
			clock_t end = clock();
			clock_t this_turn = end - start;
			total += this_turn;
			report.Trace("WorkFlow::StartWorkFlow()") << "该轮耗时：" << this_turn << "ms" << std::endl;
#endif
			report.Trace("WorkFlow::StartWorkFlow()") << "---------- 第" << n + 1 << "轮结束 ----------" << std::endl << std::endl;
			++n;
		}
#ifdef SET_TIMER
		report.Trace("WorkFlow::StartWorkFlow()") << "总耗时：" << total << "ms，平均耗时：" << (double)total / n << "ms" << std::endl;
#endif
		return true;
	}

	// 打乱动作列表，仅用于测试拓扑排序正确性，
	// 没事不要乱用。
	void ShuffleActions()
	{
		std::random_shuffle(actions_.begin(), actions_.end());
	}

private:
	cl::CommandQueue* queue_;
	std::vector<AbstractAction*> actions_;
	std::vector<AbstractAction*> seq_;
};
