/* fff_config.h - FFF configurations
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef FFF_CONFIG_H
#define FFF_CONFIG_H

#ifdef FFF_GCC_FUNCTION_ATTRIBUTES
#undef FFF_GCC_FUNCTION_ATTRIBUTES
#endif
#define FFF_GCC_FUNCTION_ATTRIBUTES __attribute__((weak))

#include "fff/fff.h"

#endif /* FFF_CONFIG_H*/
