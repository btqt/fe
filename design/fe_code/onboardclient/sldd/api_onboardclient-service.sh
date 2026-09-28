

# tput value
## 0    black     COLOR_BLACK     0,0,0
## 1    red       COLOR_RED       1,0,0
## 2    green     COLOR_GREEN     0,1,0
## 3    yellow    COLOR_YELLOW    1,1,0
## 4    blue      COLOR_BLUE      0,0,1
## 5    magenta   COLOR_MAGENTA   1,0,1
## 6    cyan      COLOR_CYAN      0,1,1
## 7    white     COLOR_WHITE     1,1,1
## sgr0 Reset text format to the terminal's default

# tput setaf 3
# echo "----------------- Manual Test ----------------"
# tput sgr0


# tput setaf 3
# echo "sldd-tcli onboardclient getLogLevel"
# tput sgr0
# sldd-tcli onboardclient  getLogLevel

# tput setaf 3
# echo "sldd-tcli onboardclient setLogLevel 1"
# tput sgr0
# sldd-tcli onboardclient  setLogLevel 1

# tput setaf 3
# echo "sldd-tcli onboardclient getLogLevel"
# tput sgr0
# sldd-tcli onboardclient  getLogLevel

# tput setaf 3
# echo "sldd-tcli onboardclient setLogLevelAsDefault"
# tput sgr0
# sldd-tcli onboardclient  setLogLevelAsDefault

# tput setaf 3
# echo "sldd-tcli onboardclient sendUdsData "" "" "
# tput sgr0
# sldd-tcli onboardclient sendUdsData   

# tput setaf 3
# echo "sldd-tcli onboardclient testOnNotifyOBD2Event"
# tput sgr0
# sldd-tcli onboardclient testOnNotifyOBD2Event
# tput setaf 3
# echo "sldd-tcli onboardclient testOnResponseEvent" "" " " " "
# tput sgr0
# sldd-tcli onboardclient testOnResponseEvent   

# @CGA_VARIANT_START{"onboardclient_api_test_shell"}
# @CGA_VARIANT___END{"onboardclient_api_test_shell"}
