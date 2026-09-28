
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/

// ppp

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>

// Include Source File for testing!!
#include "OnboardclientManagerService.cpp"
#include "OnboardclientYoursManagerService.cpp"

#include "mock/onboardclient-service/service/OnboardclientInputManager_mock.h"
#include "mock/servicelayer/corebase/SystemService_mock.h"
#include "mock/servicelayer/utils/Handler_mock.h"
#include "mock/servicelayer/utils/Timer_mock.h"
#include "mock/servicelayer/utils/Buffer_mock.h"
#include "mock/servicelayer/utils/SLLooper_mock.h"
#include "mock/servicelayer/utils/THandler_mock.h"
#include "mock/servicelayer/utils/Message_mock.h"
#include "mock/servicelayer/utils/LogOutput_mock.h"
#include "mock/binder/Binder_mock.h"
#include "mock/binder/BpBinder_mock.h"
#include "mock/binder/IInterface_mock.h"
#include "mock/binder/IServiceManager_mock.h"

#include "mock/onboardclient-service/include/OBCCanInfo_mock.h"
// MockOBCCanInfo *M_OBCCanInfo;  // redefinition
#include "mock/onboardclient-service/include/OBCResponseEventInfo_mock.h"
// MockOBCResponseEventInfo *M_OBCResponseEventInfo;  // redefinition
#include "mock/onboardclient-service/include/OBCTransportInfo_mock.h"
// MockOBCTransportInfo *M_OBCTransportInfo;  // redefinition
#include "mock/onboardclient-service/include/OBCUDSResInfo_mock.h"
// MockOBCUDSResInfo *M_OBCUDSResInfo;  // redefinition

#include "mock/onboardclient-service/include/OBCErrCode_mock.h"
// MockOBCErrCode *M_OBCErrCode; // redefinition
#include "mock/onboardclient-service/include/OBCPriorityType_mock.h"
// MockOBCPriorityType *M_OBCPriorityType; // redefinition
#include "mock/onboardclient-service/include/OBCProtocolType_mock.h"
// MockOBCProtocolType *M_OBCProtocolType; // redefinition
#include "mock/onboardclient-service/include/OBCUdsResponseType_mock.h"
// MockOBCUdsResponseType *M_OBCUdsResponseType; // redefinition

#include "mock/onboardclient-service/include/IOnboardClientReceiver_mock.h"
//#include "mock/onboardclient-service/include/IOnboardclientManagerReceiver_mock.h"

/*
 * Define Mock/Mock function
 */
class BaseOnboardclientManagerService {
public:
    virtual ~BaseOnboardclientManagerService(){  }
};

class MockOnboardclientManagerService : public BaseOnboardclientManagerService {
public:
};

MockOnboardclientManagerService *M_OnboardclientManagerService;

using namespace android;
using namespace sl;
using namespace std;

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

class OnboardclientManagerServiceTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        M_OnboardclientManagerService = new MockOnboardclientManagerService();
        M_IServiceManager = new MockIServiceManager();
        M_OnboardclientInputManager = new MockOnboardclientInputManager();
        M_SystemService = new MockSystemService();
        M_IInterface = new MockIInterface();
        M_BBinder = new MockBBinder();
        M_OBCCanInfo = new MockOBCCanInfo;
        EXPECT_CALL(*M_OBCCanInfo, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCResponseEventInfo = new MockOBCResponseEventInfo;
        EXPECT_CALL(*M_OBCResponseEventInfo, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCTransportInfo = new MockOBCTransportInfo;
        EXPECT_CALL(*M_OBCTransportInfo, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCUDSResInfo = new MockOBCUDSResInfo;
        EXPECT_CALL(*M_OBCUDSResInfo, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCErrCode = new MockOBCErrCode;
        EXPECT_CALL(*M_OBCErrCode, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCPriorityType = new MockOBCPriorityType;
        EXPECT_CALL(*M_OBCPriorityType, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCProtocolType = new MockOBCProtocolType;
        EXPECT_CALL(*M_OBCProtocolType, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCUdsResponseType = new MockOBCUdsResponseType;
        EXPECT_CALL(*M_OBCUdsResponseType, writeToParcel(_)).WillRepeatedly(Return(1));
    }
    virtual void TearDown() {
        delete M_OnboardclientManagerService;
        delete M_IServiceManager;
        delete M_OnboardclientInputManager;
        delete M_SystemService;
        delete M_IInterface;
        delete M_BBinder;
        delete M_OBCCanInfo;
        delete M_OBCResponseEventInfo;
        delete M_OBCTransportInfo;
        delete M_OBCUDSResInfo;
        delete M_OBCErrCode;
        delete M_OBCPriorityType;
        delete M_OBCProtocolType;
        delete M_OBCUdsResponseType;
    }
};


class OnboardclientManagerServiceTest__param__bool : public ::testing::TestWithParam<bool> {
protected:
    virtual void SetUp() {
        M_OnboardclientManagerService = new MockOnboardclientManagerService();
        M_IServiceManager = new MockIServiceManager();
        M_OnboardclientInputManager = new MockOnboardclientInputManager();
        M_SystemService = new MockSystemService();
        M_IInterface = new MockIInterface();
        M_BBinder = new MockBBinder();
        M_OBCCanInfo = new MockOBCCanInfo;
        EXPECT_CALL(*M_OBCCanInfo, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCResponseEventInfo = new MockOBCResponseEventInfo;
        EXPECT_CALL(*M_OBCResponseEventInfo, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCTransportInfo = new MockOBCTransportInfo;
        EXPECT_CALL(*M_OBCTransportInfo, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCUDSResInfo = new MockOBCUDSResInfo;
        EXPECT_CALL(*M_OBCUDSResInfo, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCErrCode = new MockOBCErrCode;
        EXPECT_CALL(*M_OBCErrCode, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCPriorityType = new MockOBCPriorityType;
        EXPECT_CALL(*M_OBCPriorityType, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCProtocolType = new MockOBCProtocolType;
        EXPECT_CALL(*M_OBCProtocolType, writeToParcel(_)).WillRepeatedly(Return(1));
        M_OBCUdsResponseType = new MockOBCUdsResponseType;
        EXPECT_CALL(*M_OBCUdsResponseType, writeToParcel(_)).WillRepeatedly(Return(1));
    }
    virtual void TearDown() {
        delete M_OnboardclientManagerService;
        delete M_IServiceManager;
        delete M_OnboardclientInputManager;
        delete M_SystemService;
        delete M_IInterface;
        delete M_BBinder;
        delete M_OBCCanInfo;
        delete M_OBCResponseEventInfo;
        delete M_OBCTransportInfo;
        delete M_OBCUDSResInfo;
        delete M_OBCErrCode;
        delete M_OBCPriorityType;
        delete M_OBCProtocolType;
        delete M_OBCUdsResponseType;
    }
};

INSTANTIATE_TEST_CASE_P(BOOL, OnboardclientManagerServiceTest__param__bool, Values(1, 0));


/**
*   @brief This is a test script for the dump function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_dump
*   @paramList LogOutput &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_dump_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_dump_TC001)
{

    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_ERROR;

    TestLogOutput args = TestLogOutput();

    retVal = testObj->dump(args);
    EXPECT_EQ(retVal, E_OK);

    delete testObj;
}

/**
*   @brief This is a test script for the onInit function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_onInit
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_onInit_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_onInit_TC001)
{
    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    bool retVal = true;

    retVal = testObj->onInit();
    EXPECT_EQ(retVal, false);
    delete testObj;
}

/**
*   @brief This is a test script for the instantiate function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_instantiate
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_instantiate_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_instantiate_TC001)
{
    status_t ret_param_0 = NO_ERROR;
    String16 get_param_0;
    String16 expect_param_0 = String16("service_layer.OnboardclientManagerService");
    EXPECT_CALL(*M_IServiceManager, addService(_,_,_)).WillRepeatedly(DoAll(SaveArg<0>(&get_param_0),
                Return(ret_param_0)));

    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    testObj->instantiate();
    for (int i = 0; i < (int)get_param_0.size(); ++i) {
          EXPECT_EQ(get_param_0.string()[i], expect_param_0.string()[i]) << "Service Name x and y differ at index " << i;

    }

    delete testObj;
}

/**
*   @brief This is a test script for the onStart function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_onStart
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_onStart_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_onStart_TC001)
{
    error_t ret_param_0 = E_OK;
    EXPECT_CALL(*M_OnboardclientInputManager, init()).WillRepeatedly(Return(ret_param_0));
    error_t ret_param_1 = E_OK;
    EXPECT_CALL(*M_SystemService, onStart()).WillRepeatedly(Return(ret_param_1));

    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_ERROR;
    retVal = testObj->onStart();
    EXPECT_EQ(retVal, E_OK);
}

/**
*   @brief This is a test script for the onStop function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_onStop
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_onStop_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_onStop_TC001)
{
    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_ERROR;
    retVal = testObj->onStop();
    EXPECT_EQ(retVal, E_OK);
}

/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001)
{

    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_OK;
    serviceid_t id0 = 0;
    sp<IOnboardClientReceiver> receiver1;



    retVal = testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
    );
    EXPECT_EQ(retVal, E_ERROR);

    delete testObj;
}


/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerServiceTest__param__bool, OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC002)
{
    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_FRAME_NOTREADY;
    error_t expret = E_FRAME_NOTREADY;
    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();


    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_1));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    if(GetParam())
    {
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));
        expret = E_ERROR;
    }
    else
    {
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillOnce(Return(nullptr)).WillRepeatedly(Return(pInterface));
        expret = E_OK;
    }

    list< android::sp<IOnboardClientReceiver>> receivers;
    receivers.push_back(receiver1);

    testObj-> mlReceiversOnboardClientReceiverOnNotifyOBD2Event = receivers;


    retVal = testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
    );
    EXPECT_EQ(expret, retVal);

    delete testObj;
}

/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList const android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerServiceTest__param__bool, OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001)
{

    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_FRAME_NOTREADY;
    error_t expret = E_FRAME_NOTREADY;
    //android::sp<IOnboardClientReceiver> receiver0 = sp<IOnboardClientReceiver>();

    //list< android::sp<IOnboardClientReceiver>> receivers;
    //receivers.push_back(receiver0);

    //testObj->mReceivers[0] = receivers;

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    if(GetParam())
    {
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));
        expret = E_OK;
    }
    else
    {
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillOnce(Return(nullptr)).WillRepeatedly(Return(pInterface));
        expret = E_ERROR;
    }
    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, unlinkToDeath(_,_,_,_)).WillRepeatedly(Return(ret_param_1));

    sp<IOnboardClientReceiver> receiver1;


    list< android::sp<IOnboardClientReceiver>> receivers;
    receivers.push_back(receiver1);
    testObj-> mlReceiversOnboardClientReceiverOnNotifyOBD2Event = receivers;


    testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        );

    retVal = testObj->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
    );
    //retVal = testObj->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(receiver0);
    EXPECT_EQ(expret, retVal);
}


/**
*   @brief This is a test script for the queryReceiverByOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnNotifyOBD2Event
*   @paramList appid_t, android::sp<OnboardclientData> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnNotifyOBD2Event_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerServiceTest__param__bool, OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnNotifyOBD2Event_TC001)
{
    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_FRAME_NOTREADY;
    error_t expret = E_OK;

    //sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();
    //list< android::sp<IOnboardclientManagerReceiver>> receivers;
    //receivers.push_back(receiver1);

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();
    list< android::sp<IOnboardClientReceiver>> receivers;
    receivers.push_back(receiver1);
    testObj-> mlReceiversOnboardClientReceiverOnNotifyOBD2Event = receivers;


    retVal = testObj->queryReceiverByOnboardClientReceiverOnNotifyOBD2Event(         //  Parm:0
        );
    EXPECT_EQ(expret, retVal);
}


/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC101
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC101)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
    error_t retVal = E_ERROR;

    sp<IOnboardClientReceiver> receiver1;


    retVal = testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        );
    EXPECT_EQ(E_ERROR, retVal);
}

/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList const android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC101
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC101)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
    error_t retVal = E_ERROR;

    sp<IOnboardClientReceiver> receiver1;


    retVal = testObj->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        );
    EXPECT_EQ(E_ERROR, retVal);
}

#if 0
/**
*   @brief This is a test script for the queryReceiverByOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnNotifyOBD2Event
*   @paramList appid_t, android::sp<OnboardclientData> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnNotifyOBD2Event_TC101
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnNotifyOBD2Event_TC101)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
    error_t retVal = E_ERROR;

    sp<IOnboardClientReceiver> receiver1;


    retVal = testObj->queryReceiverByOnboardClientReceiverOnNotifyOBD2Event(         //  Parm:0
        );
    EXPECT_EQ(E_ERROR, retVal);
}
#endif
/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent_TC001)
{

    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_OK;
    serviceid_t id0 = 0;
    sp<IOnboardClientReceiver> receiver1;

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;


    retVal = testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
    );
    EXPECT_EQ(retVal, E_ERROR);

    delete testObj;
}


/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerServiceTest__param__bool, OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent_TC002)
{
    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_FRAME_NOTREADY;
    error_t expret = E_FRAME_NOTREADY;
    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_1));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    if(GetParam())
    {
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));
        expret = E_ERROR;
    }
    else
    {
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillOnce(Return(nullptr)).WillRepeatedly(Return(pInterface));
        expret = E_OK;
    }

    list< android::sp<IOnboardClientReceiver>> receivers;
    receivers.push_back(receiver1);

    testObj-> mlReceiversOnboardClientReceiverOnResponseEvent = receivers;


    retVal = testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
    );
    EXPECT_EQ(expret, retVal);

    delete testObj;
}

/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent
*   @paramList const android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerServiceTest__param__bool, OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC001)
{

    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_FRAME_NOTREADY;
    error_t expret = E_FRAME_NOTREADY;
    //android::sp<IOnboardClientReceiver> receiver0 = sp<IOnboardClientReceiver>();

    //list< android::sp<IOnboardClientReceiver>> receivers;
    //receivers.push_back(receiver0);

    //testObj->mReceivers[0] = receivers;

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    if(GetParam())
    {
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));
        expret = E_OK;
    }
    else
    {
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillOnce(Return(nullptr)).WillRepeatedly(Return(pInterface));
        expret = E_ERROR;
    }
    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, unlinkToDeath(_,_,_,_)).WillRepeatedly(Return(ret_param_1));

    sp<IOnboardClientReceiver> receiver1;

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    list< android::sp<IOnboardClientReceiver>> receivers;
    receivers.push_back(receiver1);
    testObj-> mlReceiversOnboardClientReceiverOnResponseEvent = receivers;


    testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
        );

    retVal = testObj->unregisterReceiverOnboardClientReceiverOnResponseEvent( receiver1
    );
    //retVal = testObj->unregisterReceiverOnboardClientReceiverOnResponseEvent(receiver0);
    EXPECT_EQ(expret, retVal);
}


/**
*   @brief This is a test script for the queryReceiverByOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnResponseEvent
*   @paramList appid_t, android::sp<OnboardclientData> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnResponseEvent_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerServiceTest__param__bool, OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnResponseEvent_TC001)
{
    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    error_t retVal = E_FRAME_NOTREADY;
    error_t expret = E_OK;
    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    //sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();
    //list< android::sp<IOnboardclientManagerReceiver>> receivers;
    //receivers.push_back(receiver1);

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();
    list< android::sp<IOnboardClientReceiver>> receivers;
    receivers.push_back(receiver1);
    testObj-> mlReceiversOnboardClientReceiverOnResponseEvent = receivers;


    retVal = testObj->queryReceiverByOnboardClientReceiverOnResponseEvent(         //  Parm:0
     mMessage      //  Parm:1
        );
    EXPECT_EQ(expret, retVal);
}


/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent_TC101
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent_TC101)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
    error_t retVal = E_ERROR;

    sp<IOnboardClientReceiver> receiver1;

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    retVal = testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
        );
    EXPECT_EQ(E_ERROR, retVal);
}

/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent
*   @paramList const android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC101
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC101)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
    error_t retVal = E_ERROR;

    sp<IOnboardClientReceiver> receiver1;

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    retVal = testObj->unregisterReceiverOnboardClientReceiverOnResponseEvent( receiver1
        );
    EXPECT_EQ(E_ERROR, retVal);
}

#if 0
/**
*   @brief This is a test script for the queryReceiverByOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnResponseEvent
*   @paramList appid_t, android::sp<OnboardclientData> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnResponseEvent_TC101
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_queryReceiverByOnboardClientReceiverOnResponseEvent_TC101)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
    error_t retVal = E_ERROR;

    sp<IOnboardClientReceiver> receiver1;

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    retVal = testObj->queryReceiverByOnboardClientReceiverOnResponseEvent(         //  Parm:0
     mMessage      //  Parm:1
        );
    EXPECT_EQ(E_ERROR, retVal);
}
#endif

/**
*   @brief This is a test script for the onReceiverBinderDied function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_onReceiverBinderDied
*   @paramList const android::wp<android::IBinder> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_onReceiverBinderDied_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerServiceTest__param__bool, OnboardclientManagerService_onReceiverBinderDied_TC001)
{
    OnboardclientManagerService * testObj = new OnboardclientManagerService();

    sp<IOnboardClientReceiver> receiver1OnboardClientReceiver = new ITestOnboardClientReceiver();
    list< android::sp<IOnboardClientReceiver>> receiversOnboardClientReceiver;
    receiversOnboardClientReceiver.push_back(receiver1OnboardClientReceiver);
    {

        testObj-> mlReceiversOnboardClientReceiverOnNotifyOBD2Event = receiversOnboardClientReceiver;
    }
    {
        android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

        testObj-> mlReceiversOnboardClientReceiverOnResponseEvent = receiversOnboardClientReceiver;
    }

    BBinder * pBBinder = new BBinder();
    wp<IBinder> who0 = wp<IBinder>(pBBinder);

    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    if(GetParam())
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));
    else
        EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(nullptr));

    testObj->onReceiverBinderDied(who0);

    EXPECT_EQ(E_ERROR, E_OK);
}






// auto CGA start : wishtoUseAPI
/**
*   @brief This is a test script for the sendUdsData function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_sendUdsData
*   @paramList uint16_t
*   @paramList android::sp<Buffer>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_sendUdsData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_sendUdsData_TC001)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
// @CGA_VARIANT_START{"OnboardclientManagerService_sendUdsData_TC001"}
    // write your own code here
// @CGA_VARIANT___END{"OnboardclientManagerService_sendUdsData_TC001"}
}
// auto CGA end : wishtoUseAPI

// auto CGA start : testAPI
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_testOnNotifyOBD2Event_TC001)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
    error_t retVal = E_ERROR;

    /*
    retVal = testObj->testOnNotifyOBD2Event(
        );
        */
    retVal = testObj-> testOnNotifyOBD2Event(             //  Parm:0
        );
    EXPECT_EQ(E_OK, retVal);
}
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_testOnResponseEvent_TC001)
{
    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ServiceStub * testObj = new OnboardclientManagerService::ServiceStub(*mParent);
    error_t retVal = E_ERROR;
    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    /*
    retVal = testObj->testOnResponseEvent(
     mMessage
        );
        */
    retVal = testObj-> testOnResponseEvent(             //  Parm:0
         mMessage          //  Parm:1
        );
    EXPECT_EQ(E_OK, retVal);
}
// auto CGA end : testAPI



#if 0
/**
*   @brief This is a test script for the isApplicationExecuted function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_isApplicationExecuted
*   @paramList appid_t
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_isApplicationExecuted_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerServiceTest__param__bool, OnboardclientManagerService_isApplicationExecuted_TC001)
{
    OnboardclientManagerService * testObj = new OnboardclientManagerService();
    bool retVal = false;
    bool expret = true;
    appid_t id0 = 0;
    appid_t id1 = 0;

    android::sp<OnboardclientData> xxData0 = android::sp<OnboardclientData>(new OnboardclientData);

    sp<IOnboardclientManagerReceiver> receiver1 = new ITestOnboardclientManagerReceiver();
    list< android::sp<IOnboardclientManagerReceiver>> receivers;
    receivers.push_back(receiver1);

    if(GetParam())
    {
        id1 = id0;
        expret = true;
    }
    else
    {
        id1 = id0 + 1;
        expret = false;
    }
    testObj->mReceivers[id1] = receivers;

    retVal = testObj->isApplicationExecuted(id0);
    EXPECT_EQ(expret, retVal);
}
#endif


/* ============================= for *.h =========================*/
/**
*   @brief This is a test script for the getModuleID function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_getModuleID
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_getModuleID_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_getModuleID_TC001)
{

    OnboardclientManagerService * testObj = new OnboardclientManagerService();

    EXPECT_EQ(MODULE_ONBOARDCLIENT_MGR, testObj->getModuleID());
}

/**
*   @brief This is a test script for the binderDied function
*   @classID OnboardclientManagerService
*   @methodID OnboardclientManagerService_binderDied
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManagerService_binderDied_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*/
TEST_F(OnboardclientManagerServiceTest, OnboardclientManagerService_binderDied_TC001)
{

    OnboardclientManagerService * mParent = new OnboardclientManagerService();
    OnboardclientManagerService::ReceiverDeathRecipient  * testObj = new OnboardclientManagerService::ReceiverDeathRecipient (*mParent);

    testObj->binderDied(nullptr);
}
