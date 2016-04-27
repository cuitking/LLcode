#include <log4cplus/win32fileappender.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <hpf/hpf.h>


#if defined(_WIN32)
namespace log4cplus
{

	Win32FileAppender::Win32FileAppender(const log4cplus::tstring& filename)
		: file(NULL)
	{
		Constructor(filename);
	}


	Win32FileAppender::Win32FileAppender(
		helpers::Properties const& properties)
		: Appender(properties)
		, file(NULL)
	{
	}


	void Win32FileAppender::Constructor(const log4cplus::tstring& filename)
	{
		file = hpf_create_file(filename.c_str(), HPF_DEFAULT_VIEW_SIZE, 0);
		if (file == NULL)
		{
			getLogLog().error(
				LOG4CPLUS_TEXT("Win32FileAppender::Constructor")
				LOG4CPLUS_TEXT("- Unable to create file."));
			getLogLog().error(filename);
			return;
		}
	}


	Win32FileAppender::~Win32FileAppender()
	{
		hpf_close_file(static_cast<hpf_file*>(file));
		destructorImpl();
	}


	void Win32FileAppender::close()
	{
		closed = true;
	}


	void Win32FileAppender::append(spi::InternalLoggingEvent const& event)
	{
		if (file == NULL)
		{
			return;
		}

		tostringstream oss;
		layout->formatAndAppend(oss, event);

		const std::string& s = oss.str();

		if (hpf_append(static_cast<hpf_file*>(file), s.c_str(), s.length()) != 0)
		{
			getLogLog().error(
				LOG4CPLUS_TEXT("Win32FileAppender::append")
				LOG4CPLUS_TEXT("- WriteFile has failed."));
			hpf_close_file(static_cast<hpf_file*>(file));
			file = NULL;
			return;
		}
	}

} // namespace log4cplus

#endif
