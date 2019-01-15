/*
 *  DA probe
 *
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Contributors:
 * - S-Core Co., Ltd
 * - Samsung RnD Institute Russia
 *
 */
#ifndef __ALIASES_H__
#define __ALIASES_H__


#define weak_alias(f1, f2) void f1 () __attribute__ ((weak, alias (#f2)))
#define weak_alias_2(f1, pref, f2) weak_alias(f1, pref ## f2)
#define weak_alias_1(f1, pref, f2) weak_alias_2(f1, pref, f2)
#define weak_alias_pref(f1, f2) weak_alias_1(f1, PROBE_NAME_PREFIX, f2)

#endif /* __ALIASES_H__ */
