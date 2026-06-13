/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __MUIC_KUNIT_H__
#define __MUIC_KUNIT_H__

#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
#endif

#ifndef __mockable
#define __mockable
#endif

#ifndef __visible_for_testing
#define __visible_for_testing static
#endif

#ifndef EXPORT_SYMBOL_KUNIT
#define EXPORT_SYMBOL_KUNIT(sym)        /* nothing */
#endif

#endif /* __MUIC_KUNIT_H__ */
