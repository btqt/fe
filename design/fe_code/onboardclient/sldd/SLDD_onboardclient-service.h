
/**
 * \brief     Declare of onboardclientManager
 *
 * \details
 *    This software is copyright protected and proprietary to
 *    LG electronics. LGE grants to you only those rights as
 *    set out in the license conditions. All other rights remain
 *    with LG electronics.
 * \author       sungwoo.oh
 * \date       2015.10.23 UK time
 * \attention Copyright (c) 2015 by LG electronics co, Ltd. All rights reserved.
 */

#ifndef SLDD_ONBOARDCLIENT_H
#define SLDD_ONBOARDCLIENT_H

#include <sys/types.h>

#define MODULE_onboardclient_SLDD "onboardclient" /* MODULE_NAME */

bool commandActiononboardclient(int32_t argc, char **argv); /* ACTION_FUNC */
char *usage_onboardclient(char *cmd);

void register_onboardclient(); /* REGISTER_FUNC */

#endif /// SLDD_ONBOARDCLIENT_H
