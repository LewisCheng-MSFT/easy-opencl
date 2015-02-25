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
		// Ϊ���Ҳ�����ʼ��������ӡ�
		srand(time(0));
	}

	void AddAction(AbstractAction* action)
	{
		action->Queue = queue_;
		actions_.push_back(action);
	}

	// Ϊ��������ͼ�����������С�
	bool CreateTopologicalSequence()
	{
		seq_.clear();
		while (!actions_.empty())
		{
			// �������Ϊ0�Ķ�����
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
				// ɾ�����жԸö���������
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
			{ // ʣ��ȫ����Ȳ�Ϊ0�Ķ�������ʾ��������
				report.Error(DEADLOCK_FOUND_IN_ACTION_GRAPH, "WorkFlow::CreateTopologicalSequence()") << "��⵽��������ͼ��������" << std::endl;
				return false;
			}
		}

		return true;
	}

	// ����������ִ�й�������
	bool StartWorkFlow(size_t repeat_count = 1, bool blocking = true)
	{
#ifdef SET_TIMER
		clock_t total = 0;
#endif
		int n = 0;
		while (repeat_count > n)
		{
			report.Trace("WorkFlow::StartWorkFlow()") << "---------- ��" << n + 1 << "�ֿ�ʼ ----------" << std::endl;
#ifdef SET_TIMER
			// ���ּ�ʱ��ʼ��
			clock_t start = clock();
#endif
			for (std::vector<AbstractAction*>::iterator i = seq_.begin(); i != seq_.end(); ++i)
			{
				// �ռ��Ⱦ�����������¼����Ա�ȴ�������ɡ�
				std::vector<AbstractAction*>& before = (*i)->BeforeActions;
				std::vector<cl::Event>& events = (*i)->BeforeEvents;
				events.clear();
				for (std::vector<AbstractAction*>::iterator j = before.begin(); j != before.end(); ++j)
					events.push_back((*j)->Completion);

				// ������������¼���
				(*i)->Completion = cl::Event();

				cl_int ErrorCode = (*i)->Execute();
				if (ErrorCode != CL_SUCCESS)
				{
					report.Error(ErrorCode, "WorkFlow::StartWorkFlow()") << "����\"" << (*i)->Name << "\"ִ��ʧ��" << std::endl;
					return false;
				}
			}
		
			if (blocking) queue_->finish();
#ifdef SET_TIMER
			// ���ּ�ʱ������
			clock_t end = clock();
			clock_t this_turn = end - start;
			total += this_turn;
			report.Trace("WorkFlow::StartWorkFlow()") << "���ֺ�ʱ��" << this_turn << "ms" << std::endl;
#endif
			report.Trace("WorkFlow::StartWorkFlow()") << "---------- ��" << n + 1 << "�ֽ��� ----------" << std::endl << std::endl;
			++n;
		}
#ifdef SET_TIMER
		report.Trace("WorkFlow::StartWorkFlow()") << "�ܺ�ʱ��" << total << "ms��ƽ����ʱ��" << (double)total / n << "ms" << std::endl;
#endif
		return true;
	}

	// ���Ҷ����б������ڲ�������������ȷ�ԣ�
	// û�²�Ҫ���á�
	void ShuffleActions()
	{
		std::random_shuffle(actions_.begin(), actions_.end());
	}

private:
	cl::CommandQueue* queue_;
	std::vector<AbstractAction*> actions_;
	std::vector<AbstractAction*> seq_;
};
