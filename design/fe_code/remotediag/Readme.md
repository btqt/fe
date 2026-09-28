## Test command document is for RemoteDiag

## SLDD command structure
adb shell sldd am send_post remotediag <what> <arg1> <arg2>

## SLDDs are supported by RemoteDiag Application
namespace MsgTestSLDD
{
    constexpr static int32_t MSG_SLDD_TEST_INIT{10000};  // CMD TEST---BEGIN----

    constexpr static int32_t MSG_SLDD_TEST_END{10100};   // CMD TEST---END------
}
