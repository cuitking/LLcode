#ifndef SCGL_CATCH_H
#define SCGL_CATCH_H

#include "Assert.h"
#include <boost/property_tree/detail/xml_parser_error.hpp>

namespace scgl
{
	void ShowException(const boost::property_tree::xml_parser::xml_parser_error& e);
	void ShowException(const scgl::exception& e);
	void ShowException(const std::exception& e);
}

#define SCGL_CATCH(expression)						\
	try												\
	{												\
		{											\
			expression;								\
		}											\
	}												\
	catch(const boost::property_tree::xml_parser::xml_parser_error& e)		\
	{												\
		scgl::ShowException(e);						\
		throw;										\
	}												\
	catch(const scgl::exception & e)				\
	{												\
		scgl::ShowException(e);						\
		throw;										\
	}												\
	catch(const std::exception & e)					\
	{												\
		scgl::ShowException(e);						\
		throw;										\
	}												\

#define SCGL_CATCH_M(expression, message)						\
	try												\
	{												\
		{											\
			expression;								\
		}											\
	}												\
	catch(...)			\
	{												\
		std::ostringstream m;								\
		m << message;										\
		::MessageBox(GetConsoleWindow(), m.str().c_str(), "Exception", MB_OK | MB_SYSTEMMODAL);	\
	}\
	 
#endif