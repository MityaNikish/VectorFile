#pragma once
#include <exception>

class goind_out_of_file : std::exception
{
	char const* what() const override
	{
		return "Error";
	}
};


class write_error : std::exception
{
	char const* what() const override
	{
		return "No write access to file";
	}
};