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
		err_ << "错误" << code << "：发生在\"" << place << "\"处：" << std::endl;
		return err_;
	}

	std::ostream& Trace(const std::string& place)
	{
#ifdef TRACE
#ifdef DISP_TRACE_HEADER
		out_ << "跟踪：发生在\"" << place << "\"处：" << std::endl;
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