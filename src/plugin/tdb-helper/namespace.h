/* $Id: namespace.h 602 2009-01-08 02:27:44Z jackda $ */
#ifndef __TTC_VERSIONED_NAMESPACE_H
#define __TTC_VERSIONED_NAMESPACE_H

#include "version.h"

#if TTC_HAS_VERSIONED_NAMESPACE && TTC_HAS_VERSIONED_NAMESPACE == 1

# ifndef TTC_VERSIONED_NAMESPACE

#  define MAKE_TTC_VERSIONED_NAMESPACE_IMPL(MAJOR, MINOR, BETA) TTC_##MAJOR##_##MINOR##_##BETA
#  define MAKE_TTC_VERSIONED_NAMESPACE(MAJOR, MINOR, BETA) MAKE_TTC_VERSIONED_NAMESPACE_IMPL(MAJOR, MINOR, BETA)
#  define TTC_VERSIONED_NAMESPACE MAKE_TTC_VERSIONED_NAMESPACE(TTC_MAJOR_VERSION, TTC_MINOR_VERSION, TTC_BETA_VERSION)

# endif //end TTC_VERSIONED_NAMESPACE

# define TTC_BEGIN_NAMESPACE namespace TTC_VERSIONED_NAMESPACE {
# define TTC_END_NAMESPACE }
# define TTC_USING_NAMESPACE using namespace TTC_VERSIONED_NAMESPACE;

#else

# define TTC_BEGIN_NAMESPACE
# define TTC_END_NAMESPACE
# define TTC_USING_NAMESPACE

#endif //end TTC_HAS_VERSIONED_NAMESPACE

#endif 