#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(CHESSBOTCORE_LIB)
#  define CHESSBOTCORE_EXPORT Q_DECL_EXPORT
# else
#  define CHESSBOTCORE_EXPORT Q_DECL_IMPORT
# endif
#else
# define CHESSBOTCORE_EXPORT
#endif
