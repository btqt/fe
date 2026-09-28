#include "Logger.h"

namespace rdgapp {


DLT_DECLARE_CONTEXT(ctxRemoteDiagReg);

extern android::sp<Application> gApp;

std::string composeMessage(const std::string& input, const va_list ap)
{
    uint32_t n {input.size() * 2U};
    std::unique_ptr<char_t[]> formatted {};
    while(true) {
        formatted.reset(new char_t[n]);
        (void)strncpy(&formatted[0], input.c_str(), input.size());
        const int32_t final_n {vsnprintf(&formatted[0], n, input.c_str(), ap)};
        if ((final_n < 0) || (static_cast<uint32_t>(final_n) >= n)) {
            const int32_t abs_sum {abs(final_n - static_cast<int32_t>(n) + 1)};
            if (abs_sum >= 0)
            {
                n += static_cast<uint32_t>(abs_sum);
            }
        } else{
            break;
        }
    }
    return std::string(formatted.get());
}

void initDLTLog()
{
    /* Init DLT log for REMOTEDIAG */
    DLT_REGISTER_APP("RDG" ,"DLT log for RemoteDiag application");
    DLT_REGISTER_CONTEXT(ctxRemoteDiagReg, "RDG","DLT LOG context for Remotediag");
}

std::string generateJson(std::string jsonString) 
{
    std::string builder{""};
    std::string spaces{""};
    for (uint32_t i{0U}; i < jsonString.length(); i ++) {
        const char_t c {jsonString[i]};
        switch (static_cast<uint8_t>(c)) {
            case '{' :
                (void)spaces.append(" ");
                (void)builder.append(1U, c).append("\n").append(spaces);
                break;
            case '[' :
                (void)spaces.append("\t");
                (void)builder.append(1U, c).append("\n").append(spaces);
                break;
            case ']' :
                (void)builder.append("\n").append(spaces).append(1U, c).append("\n");
                if (spaces.length() != 0U) {
                    (void)spaces.erase(spaces.length()-1U, 1U);
                }
                break;
            case '}' : 
                if (spaces.length() != 0U) {
                    (void)spaces.erase(spaces.length()-1U, 1U);
                }
                (void)builder.append("\n").append(spaces).append(1U, c).append("\n");
                break;
            case ',':
                (void)builder.append(1U, c).append("\n").append(spaces);
                break;
            default :
                (void)builder.append(1U, c);
                break;
        }
    }
    return builder;
}

// void printData(const std::string data) 
// {
//     std::string data_tmp{generateJson(data)};
//     std::string::iterator ptr {data_tmp.begin()};
//     std::string a{""};
//     while(ptr != data_tmp.end() ) {
//         if(*ptr != '\n' ) {
//             a += *ptr;
//         }
//         else {
//             LOG_V(a.c_str());
//             a.clear();
//         }
//         ptr++;
//     }
// }

void printDataDebug(const std::string data) 
{
    std::string data_tmp{generateJson(data)};
    std::string::iterator ptr {data_tmp.begin()};
    std::string a{""};
    while(ptr != data_tmp.end() ) {
        if(*ptr != '\n' ) {
            a += *ptr;
        }
        else {
            LOG_V(a.c_str());
            a.clear();
        }
        ptr++;
    }
}
}
