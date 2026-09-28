/**
 * \brief Implementation of sldb
 *
 * \details
 *    This software is copyright protected and proprietary to
 *    LG electronics. LGE grants to you only those rights as
 *    set out in the license conditions. All other rights remain
 *    with LG electronics.
 * \author sungwoo.oh
 * \date   2015.09.01
 * \attention Copyright (c) 2015 by LG electronics co, Ltd. All rights reserved.
 */

#include <iostream>
#include <cstdio>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>

#include <algorithm>
#include <memory>
#include <utils/RefBase.h>
#include "Log.h"

#include "SLDD_onboardclient-service.h"
#include "sldd_common.h"

/**
 * @MISRA{MISRA C++-2008 Rule 16-2-1,"This is intended design"}
 */
#define LOG_TAG "sldd"
#define LOGH_TAG "sldd_cmd"

//#######################################################################################
//# Constant Define
//#######################################################################################

typedef enum
{
    TYPE_ACCELOMETER = 0,
} cmd_type_t;

typedef enum
{
    ACTION_SET_ENABLE = 0,
    ACTION_SET_DISABLE
} cmd_action_t;

typedef void (*commandAction)(int32_t type, int32_t action);
typedef bool (*commandAction2)(int32_t argc, char **argv);

typedef struct
{
    const char *module;
    commandAction2 action;
} action2_table_t;

action2_table_t action2_table[] = {
#include "SLDD_onboardclient-service.h"
    {NULL, NULL}};

void usage_man();

//#######################################################################################
//# Structure Define
//#######################################################################################

class CommanderClient // : public RefBase
{
public:
    CommanderClient()
    {
    }
    virtual ~CommanderClient() {}

    bool init()
    {
#include "SLDD_onboardclient-service.h"
        return true;
    }

    void act(commandAction fn, cmd_type_t type, cmd_action_t action)
    {
        fn(type, action);
    }

    void act2(commandAction2 fn, int32_t argc, char **argv)
    {
        if (fn(argc, argv) == false)
        {
            printf(
                "==========================================\n"
                "            FAILED                       \n"
                "==========================================\n");
        }
    }
};

CommanderClient *sClient;

void str_convert(int8_t &a, char *s)
{
    a = static_cast<int8_t>(atoi(s));
}
void str_convert(uint8_t &a, char *s)
{
    a = static_cast<uint8_t>(strtoul(s, NULL, 10));
}
void str_convert(int16_t &a, char *s)
{
    a = static_cast<int16_t>(atoi(s));
}
void str_convert(uint16_t &a, char *s)
{
    a = static_cast<uint16_t>(strtoul(s, NULL, 10));
}
void str_convert(int32_t &a, char *s)
{
    a = static_cast<int32_t>(atoi(s));
}
void str_convert(uint32_t &a, char *s)
{
    a = static_cast<uint32_t>(strtoul(s, NULL, 10));
}
void str_convert(int64_t &a, char *s)
{
    a = static_cast<int64_t>(strtol(s, NULL, 10));
}
void str_convert(uint64_t &a, char *s)
{
    a = static_cast<uint64_t>(strtoul(s, NULL, 10));
}
void str_convert(uint8_t *a, char *s)
{
    strncpy((char *)a, s, BUFSIZ);
}
void str_convert(float &a, char *s)
{
    a = static_cast<float>(strtof(s, NULL));
}
void str_convert(double &a, char *s)
{
    a = static_cast<double>(strtod(s, NULL));
}

void make_history(int32_t argc, char **argv)
{
    int32_t command_len = 0;
    for (int8_t i = 0; i < argc; i++)
    {
        command_len += strlen(argv[i]) + 1;
    }

    char command_string[256]; // = new char[command_len];
    char *buf = command_string;
    strcpy(buf, argv[0]);
    strcat(buf, " ");
    for (int8_t i = 1; i < argc; i++)
    {
        buf += strlen(argv[i - 1]) + 1;
        strcat(buf, argv[i]);
        strcat(buf, " ");
    }
}

void usage(char *module, char *cmd)
{
    const char *header = "Service Layer Debug Deamon V0.1 \n\n";

    if (module == NULL)
    {
        fprintf(stderr, "%s\n", header);
        fprintf(stderr, "start from below command \n  sldd man \n");
    }
}

void usage_man()
{
    printMessage(" Supported module \n");
    for (int i = 0; action2_table[i].module != NULL; i++)
    {
        uint8_t moduleInt = 0;
        str_convert(moduleInt, action2_table[i].module);
        printMessage(" sldd %d \n", moduleInt);
    }
    printMessage(" If you want to find command in module \n");
    printMessage(" You can do like belew \n");
    printMessage("   sldd man list <module> \n");
    printMessage("   sldd man find <module> <command> \n");
}

bool command(int argc, char **argv)
{
    bool enable = false;
    bool disable = false;

    while (argc > 0)
    {
        LOGI("command : argv:%s", argv[0]);
        if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--h"))
        {
            usage(argv[1], argv[2]);
            return true;
        }
        else if (!strcmp(argv[0], "-e"))
        {
            enable = true;
        }
        else if (!strcmp(argv[0], "-d"))
        {
            /* this is a special flag used only when the ADB client launches the ADB Server */
            disable = true;
        }
        else
        {
            for (int i = 0; action2_table[i].module != NULL; i++)
            {
                if (argv[0] != NULL && !strcmp(argv[0], action2_table[i].module))
                {
                    // make_history(argc, argv);
                    sClient->act2(action2_table[i].action, argc - 1, argv + 1);
                    make_history(argc, argv);
                    return true;
                }
            }

            usage(argv[1], argv[2]);
            return true;
        }

        argc--;
        argv++;
    }

    return false;
}

int main(int argc, char **argv)
{
    android::ProcessState::self()->startThreadPool();

    sClient = new CommanderClient();
    if (sClient->init() == false)
    {
        printMessage(" can not use sldd now because TCU3 is not ready \n");
        return 0;
    }

    if (command(argc - 1, argv + 1) == true)
    {
        ::usleep(100000); // 0.1s
        // sClient.clear();

        // exit
        return 0;
    }

    ::usleep(100000U); // 0.1s
    // sClient.clear();
    return 1;
}
