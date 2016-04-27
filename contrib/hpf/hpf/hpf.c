#include "../hpf.h"
#include <stdlib.h>
#include <scgl/Win32.h>

typedef union hpf_size
{
	uint64_t		v;
	struct 
	{
		uint32_t	low;
		uint32_t	high;
	} s;
} hpf_size;

#define _high_of(hpf_size)	(hpf_size).s.high
#define _low_of(hpf_size)	(hpf_size).s.low

static const char* const HPF_SIGN = "HPF_SIGN";
static const char* const HPF_SIGN_NOT_INIT = "HPF_NOT_INIT";
static const char* const HPF_SIGN_INVALID = "HPF_SIGN_INVALID";

typedef struct hpf_file
{
	const char* sign;
	HANDLE		handle;
	HANDLE		map;					
	hpf_size	size;					// 文件大小
	hpf_size	offset;					// 已经写完的文件大小
	PVOID		view;
	DWORD		view_size;
	DWORD		view_offset;
	char		initial_value;
	char		filename[MAX_PATH];
} hpf_file;

HPF_API int _hpf_append_stderr(const char* string)
{
	HANDLE errorHandle = GetStdHandle(STD_ERROR_HANDLE);
	if (errorHandle == INVALID_HANDLE_VALUE)
	{
		return GetLastError();
	}

	if (!WriteConsole(errorHandle, string, strlen(string), NULL, NULL))
	{
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

int _hpf_show_error(DWORD error)
{
	LPVOID lpMsgBuf =  NULL;

	if (FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				error,
				0,
				(LPTSTR) &lpMsgBuf,
				0, NULL) == 0)
	{
		return GetLastError();
	}

	_hpf_append_stderr(lpMsgBuf);
	LocalFree(lpMsgBuf);
	return error;
}

HPF_API hpf_file* hpf_create_file(const char* filename, size_t view_size, char initial_value)
{
	hpf_file* file = malloc(sizeof(hpf_file));
	if (file == NULL)
	{
		_hpf_show_error(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}

	memset(file, 0, sizeof(*file));
	file->sign = HPF_SIGN_NOT_INIT;
	file->view_size = view_size;
	file->initial_value = initial_value;
	strncpy(file->filename, filename, sizeof(file->filename));
	
	return file;
}

int _hpf_enlarge_map(hpf_file* file, uint64_t size)
{
	uint64_t least = file->offset.v + size;
	hpf_size target = { HPF_DEFAULT_VIEW_SIZE >= file->view_size ? HPF_DEFAULT_VIEW_SIZE : file->view_size };

	if (file == NULL || file->sign != HPF_SIGN)
	{
		return ERROR_INVALID_HANDLE;
	}

	if (file->size.v >= least)
	{
		return ERROR_SUCCESS;
	}

	if (file->map != NULL && !CloseHandle(file->map))
	{
		return GetLastError();
	}

	if (target.v < file->size.v)
	{
		target = file->size;
	}

	while (target.v < least)
	{
		// 已经超过4GB了每次增加4GB
		if (_high_of(target) > 0)
		{
			++(_high_of(target));
		}
		else
		{
			target.v <<= 1;
		}
	}

	file->map = CreateFileMapping(file->handle, NULL, PAGE_READWRITE, _high_of(target), _low_of(target), NULL);
	if (file->map == NULL)
	{
		return GetLastError();
	}

	file->size = target;

	return ERROR_SUCCESS;
}

int _hpf_remap_view(hpf_file* file)
{
	if (file == NULL || file->sign != HPF_SIGN || file->map == NULL)
	{
		return ERROR_INVALID_HANDLE;
	}

	if (file->view == NULL || file->view_size - file->view_offset <= 0)
	{
		if (file->view != NULL)
		{
			UnmapViewOfFile(file->view);
		}

		file->view = MapViewOfFile(file->map, FILE_MAP_WRITE, _high_of(file->offset), _low_of(file->offset), file->view_size);
		if (file->view == NULL)
		{
			return GetLastError();
		}

		if (file->initial_value != 0)
		{
			memset(file->view, file->initial_value, file->view_size);
		}

		file->view_offset = 0;
	}

	return ERROR_SUCCESS;
}

int _hpf_init_file(hpf_file* file)
{
	if (file == NULL || file->sign != HPF_SIGN_NOT_INIT)
	{
		return ERROR_INVALID_HANDLE;
	}

	file->handle = CreateFile(file->filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (file->handle == INVALID_HANDLE_VALUE)
	{
		return GetLastError();
	}

	file->sign = HPF_SIGN;

	return ERROR_SUCCESS;
}

HPF_API int hpf_close_file(hpf_file* file)
{
	LARGE_INTEGER fileLength = { 0 };

	if (file == NULL || (file->sign != HPF_SIGN && file->sign != HPF_SIGN_INVALID))
	{
		return ERROR_INVALID_HANDLE;
	}

	if (file->view != NULL && !UnmapViewOfFile(file->view))
	{
		_hpf_show_error(GetLastError());
	}

	if (file->map != NULL && !CloseHandle(file->map))
	{
		_hpf_show_error(GetLastError());
	}

	fileLength.QuadPart = file->offset.v;
	if (!SetFilePointerEx(file->handle, fileLength, NULL, FILE_BEGIN))
	{
		_hpf_show_error(GetLastError());
	}

	if (!SetEndOfFile(file->handle))
	{
		_hpf_show_error(GetLastError());
	}

	if (!CloseHandle(file->handle))
	{
		_hpf_show_error(GetLastError());
	}

	free(file);
	return ERROR_SUCCESS;
}

typedef void* (*_hpf_do_memory)(void* target, const void* data, size_t amount);

int _hpf_append_f( hpf_file* file, const void* data, size_t amount, _hpf_do_memory do_memory )
{
	int error_code = ERROR_SUCCESS;
	size_t left = amount;
	size_t wrote = 0;

	if (file == NULL || (file->sign != HPF_SIGN_NOT_INIT && file->sign != HPF_SIGN))
	{
		return ERROR_INVALID_HANDLE;
	}

	if (file->sign == HPF_SIGN_NOT_INIT)
	{
		error_code = _hpf_init_file(file);
		if (error_code != ERROR_SUCCESS)
		{
			file->sign = HPF_SIGN_INVALID;
			return error_code;
		}
	}

	error_code = _hpf_enlarge_map(file, amount);
	if (error_code != ERROR_SUCCESS)
	{
		file->sign = HPF_SIGN_INVALID;
		return error_code;
	}

	while (left > 0)
	{
		size_t space = 0;
		error_code = _hpf_remap_view(file);
		if (error_code != ERROR_SUCCESS)
		{
			file->sign = HPF_SIGN_INVALID;
			return error_code;
		}

		// 判断view剩余空间是否足够
		space = file->view_size - file->view_offset;
		space = space >= left ? left : space;

		do_memory((char*)(file->view) + file->view_offset, ((char*)data) + wrote, space);
		file->view_offset += space;
		file->offset.v += space;
		left -= space;
		wrote += space;
	}
	return ERROR_SUCCESS;

}

HPF_API int hpf_append( hpf_file* file, const void* data, size_t amount )
{
	return _hpf_append_f(file, data, amount, memcpy);
}

HPF_API int hpf_append_string(hpf_file* file, const char* string)
{
	return hpf_append(file, string, strlen(string));
}

#pragma warning( push )
#pragma warning( disable : 4305)

void* _hpf_memset(void* target, const void* data, size_t amount)
{
	return memset(target, (char)data, amount);
}

#pragma warning( disable : 4306)

HPF_API int hpf_append_char(hpf_file* file, char c, size_t amount)
{
	return _hpf_append_f(file, (void*)c, amount, _hpf_memset);
}

#pragma warning( pop )
