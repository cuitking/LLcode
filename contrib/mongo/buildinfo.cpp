#include "pch.h"
#include <iostream>
#include <boost/version.hpp>
namespace mongo
{
	const char* gitVersion()
	{
		return "4bb6e76b20a3f8fd49fb4677b189b41088f3a962";
	}
}
namespace mongo
{
	string sysInfo()
	{
		return "Linux ip-10-2-29-40 2.6.21.7-2.ec2.v1.2.fc8xen #1 SMP Fri Nov 20 17:48:28 EST 2009 x86_64 BOOST_LIB_VERSION=" BOOST_LIB_VERSION ;
	}
}

