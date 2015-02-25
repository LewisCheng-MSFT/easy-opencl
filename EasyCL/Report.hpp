#pragma once

#include "Global.hpp"

class Report
{
public:
	Report(std::ostream& out = std::cout, std::ostream& err = std::cerr)
		: out_(out),
		  err_(err),
		  null_("nul", std::ofstream::out)
	{
	}

	~Report()
	{
		null_.close();
	}

	std::ostream& Error(int code, const std::string& place)
	{
		err_ << "����" << code << "��������\"" << place << "\"����" << std::endl;
		return err_;
	}

	std::ostream& Trace(const std::string& place)
	{
#ifdef TRACE
#ifdef DISP_TRACE_HEADER
		out_ << "���٣�������\"" << place << "\"����" << std::endl;
#endif
		return out_;
#else
		return null_;
#endif
	}

private:
	std::ofstream null_;
	std::ostream& out_;
	std::ostream& err_;
};

extern Report report;