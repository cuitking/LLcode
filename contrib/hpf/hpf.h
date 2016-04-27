#ifndef HIGH__PERFORMANCE_FILE_H
#define HIGH__PERFORMANCE_FILE_H


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	/* Linkage of public API functions. */
#if defined(HPF_BUILD_AS_DLL)
#if defined(HPF_CORE) || defined(HPF_LIB)
#define HPF_API		__declspec(dllexport)
#else
#define HPF_API		__declspec(dllimport)
#endif
#else
#define HPF_API		extern
#endif

#define HPFLIB_API	HPF_API

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
	typedef unsigned __int64    size_t;
#else  /* _WIN64 */
	typedef unsigned int   size_t;
#endif  /* _WIN64 */
#define _SIZE_T_DEFINED
#endif  /* _SIZE_T_DEFINED */

	typedef struct hpf_file hpf_file;

#define HPF_DEFAULT_VIEW_SIZE	64 * 1024

	HPF_API hpf_file* (hpf_create_file)(const char* filename, size_t view_size, char initial_value);
	HPF_API int (hpf_close_file)(hpf_file* file);
	HPF_API int (hpf_append)(hpf_file* file, const void* data, size_t amount);
	HPF_API int (hpf_append_string)(hpf_file* file, const char* string);
	HPF_API int (hpf_append_char)(hpf_file* file, char c, size_t amount);

#ifdef __cplusplus
}
#endif

#endif
