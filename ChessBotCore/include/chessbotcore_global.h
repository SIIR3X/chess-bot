#pragma once

#ifdef _WIN32
# ifdef CHESSBOTCORE_LIB
#  define CHESSBOTCORE_EXPORT __declspec(dllexport)
# else
#  define CHESSBOTCORE_EXPORT __declspec(dllimport)
# endif
#else
# define CHESSBOTCORE_EXPORT
#endif
