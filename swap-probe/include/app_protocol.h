/*
 *  DA probe
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Vitaliy Cherepanov <v.cherepanov@samsung.com>
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
#ifndef __APP_PROTOCOL_H__
#define __APP_PROTOCOL_H__

enum AppMessageType
{
	APP_MSG_IMAGE = 6,
	APP_MSG_TERMINATE = 7,
	APP_MSG_PID = 8,
	APP_MSG_MSG = 9,
	APP_MSG_ALLOC = 10,
	APP_MSG_ERROR = 11,
	APP_MSG_WARNING = 12,

	APP_MSG_STOP = 101,
	APP_MSG_CONFIG = 103,
	APP_MSG_CAPTURE_SCREEN= 108,
	APP_MSG_MAPS_INST_LIST= 109
};

#endif /* __APP_PROTOCOL_H__  */
