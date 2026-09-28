
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>

// Include Source File for testing!!
#include "OnboardclientManagerService_main.cpp"

#include "mock/servicelayer/corebase/SystemService_mock.h"
#include "mock/servicelayer/utils/Handler_mock.h"
#include "mock/servicelayer/utils/Timer_mock.h"
#include "mock/servicelayer/utils/Buffer_mock.h"
#include "mock/servicelayer/utils/SLLooper_mock.h"
#include "mock/servicelayer/utils/THandler_mock.h"
#include "mock/servicelayer/utils/Message_mock.h"

#include "mock/binder/Binder_mock.h"
#include "mock/binder/BpBinder_mock.h"
#include "mock/binder/IInterface_mock.h"
#include "mock/binder/IServiceManager_mock.h"

#include "mock/onboardclient-service/service/OnboardclientManagerService_mock.h"

/*
 * Define Mock/Mock function
 */
class BaseOnboardclientManagerService_main {
public:
    virtual ~BaseOnboardclientManagerService_main(){  }
};

class MockOnboardclientManagerService_main : public BaseOnboardclientManagerService_main {
public:
};

MockOnboardclientManagerService_main *M_OnboardclientManagerService_main;

using namespace android;
using namespace sl;

using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;

class OnboardclientManagerService_mainTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        M_OnboardclientManagerService_main = new MockOnboardclientManagerService_main();
        M_OnboardclientManagerService = new MockOnboardclientManagerService();
    }
    virtual void TearDown() {
        delete M_OnboardclientManagerService_main;
        delete M_OnboardclientManagerService;
    }
};

/**
*   @brief This is a test script for the main function
*   @classID OnboardclientManagerService_main
*   @methodID OnboardclientManagerService_main_UT_main
*   @paramList int, char **
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_main_UT_main_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerService_mainTest, OnboardclientManagerService_main_UT_main_TC001)
{
    bool ret_param_0 = true;
    EXPECT_CALL(*M_OnboardclientManagerService, onInit()).WillRepeatedly(Return(ret_param_0));
    error_t ret_param_1 = E_OK;
    EXPECT_CALL(*M_OnboardclientManagerService, onStart()).WillRepeatedly(Return(ret_param_1));

    int argc32611 = 0;
    char* argv32612;

    UT_main(argc32611, &argv32612);
}
