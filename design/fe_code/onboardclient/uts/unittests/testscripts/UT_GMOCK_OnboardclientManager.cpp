
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>


// Include Source File for testing!!
#include "OnboardclientManager.cpp"

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

#include "mock/onboardclient-service/include/IOnboardclientManagerService_mock.h"

/*
 * Define Mock/Mock function
 */
class BaseOnboardclientManager {
public:
    virtual ~BaseOnboardclientManager(){  }
    virtual android::sp<IOnboardclientManagerService> getService_mock(){}
};

class MockOnboardclientManager : public BaseOnboardclientManager {
public:

    MOCK_METHOD0(getService_mock, android::sp<IOnboardclientManagerService>());
};



using namespace android;
using namespace sl;
using namespace std;


MockOnboardclientManager *M_OnboardclientManager;

android::sp<IOnboardclientManagerService> OnboardclientManager::getService_mock()
{
    return M_OnboardclientManager->getService_mock();
}
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
class OnboardclientManagerTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        M_OnboardclientManager = new MockOnboardclientManager();
        M_IServiceManager = new MockIServiceManager();
        M_BBinder = new MockBBinder();
        M_IInterface = new MockIInterface();
        M_IOnboardclientManagerService = new MockIOnboardclientManagerService();
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
        delete M_OnboardclientManager;
        delete M_IServiceManager;
        delete M_BBinder;
        delete M_IInterface;
        delete M_IOnboardclientManagerService;
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

class OnboardclientManagerTest__param__nullnErr : public ::testing::TestWithParam< tuple<int, error_t> > {
protected:
    virtual void SetUp() {
    M_OnboardclientManager = new MockOnboardclientManager();
        M_IServiceManager = new MockIServiceManager();
        M_BBinder = new MockBBinder();
        M_IInterface = new MockIInterface();
        M_IOnboardclientManagerService = new MockIOnboardclientManagerService();
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
        delete M_OnboardclientManager;
        delete M_IServiceManager;
        delete M_BBinder;
        delete M_IInterface;
        delete M_IOnboardclientManagerService;
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

INSTANTIATE_TEST_CASE_P(nullerr, OnboardclientManagerTest__param__nullnErr,
    Combine(Values(1,0), Values(E_OK, E_ERROR)));


class OnboardclientManagerTest__param__bool : public ::testing::TestWithParam<bool> {
protected:
    virtual void SetUp() {
    M_OnboardclientManager = new MockOnboardclientManager();
        M_IServiceManager = new MockIServiceManager();
        M_BBinder = new MockBBinder();
        M_IInterface = new MockIInterface();
        M_IOnboardclientManagerService = new MockIOnboardclientManagerService();
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
        delete M_OnboardclientManager;
        delete M_IServiceManager;
        delete M_BBinder;
        delete M_IInterface;
        delete M_IOnboardclientManagerService;
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

INSTANTIATE_TEST_CASE_P(BOOL, OnboardclientManagerTest__param__bool,
    Values(1, 0));


/**
*   @brief This is a test script for the instance function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_instance
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_instance_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_instance_TC001)
{
    IBinder* pBinder = new BpBinder(0);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_003(_)).WillRepeatedly(Return(pInterface));
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));

    OnboardclientManager* ret = OnboardclientManager::instance();

    EXPECT_TRUE(ret);
    if(ret)
        delete ret;

}

/**
*   @brief This is a test script for the instance function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_instance
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_instance_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_instance_TC002)
{
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(nullptr));
    EXPECT_TRUE(OnboardclientManager::instance());
}

/**
*   @brief This is a test script for the getService function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_getService
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_getService_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_getService_TC001)
{

#ifdef uts_remove
    OnboardclientManager::DEBUG = false;
#endif

    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    testObj->getService();

    delete testObj;

}

/**
*   @brief This is a test script for the getService function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_getService
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_getService_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_getService_TC002)
{

#ifdef uts_remove
    OnboardclientManager::DEBUG = true;
#endif

    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    testObj->getService();

    delete testObj;

}


/**
*   @brief This is a test script for the onBinderDied function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_onBinderDied
*   @paramList const android::wp<android::IBinder> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_onBinderDied_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_onBinderDied_TC001)
{

    IBinder* pBinder = new BpBinder(0);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_003(_)).WillRepeatedly(Return(pInterface));
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));

    wp<IBinder> who = wp<IBinder>(pBBinder);


//EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

OnboardclientManager *testObj = new OnboardclientManager();

    //BBinder * pBBinder = new BBinder();
    //sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_001()).WillRepeatedly(Return(pInterface));

    testObj->onBinderDied(who);

    delete testObj;
}

/**
*   @brief This is a test script for the onBinderDied function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_onBinderDied
*   @paramList const android::wp<android::IBinder> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_onBinderDied_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_onBinderDied_TC002)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    EXPECT_CALL(*M_IInterface, asBinder_001()).WillRepeatedly(Return(nullptr));

    testObj->onBinderDied(nullptr);
    delete testObj;

}

/**
*   @brief This is a test script for the onBinderDied function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_onBinderDied
*   @paramList const android::wp<android::IBinder> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_onBinderDied_TC004
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_onBinderDied_TC003)
{
    android::sp<IOnboardclientManagerService> pmOnboardclientService = new ITestIOnboardclientManagerService();

    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(pmOnboardclientService));

    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_1));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_001()).WillRepeatedly(Return(pInterface));

    OnboardclientManager *testObj = new OnboardclientManager();

    testObj->onBinderDied(pBBinder);
    delete testObj;

}

#if 0
/**
*   @brief This is a test script for the onBinderDied function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_onBinderDied
*   @paramList const android::wp<android::IBinder> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_onBinderDied_TC004
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_onBinderDied_TC004)
{
    android::sp<IOnboardclientManagerService> pmOnboardclientService = new ITestIOnboardclientManagerService();

    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(pmOnboardclientService));

    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_1));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_001()).WillRepeatedly(Return(pInterface));

    OnboardclientManager *testObj = new OnboardclientManager();

    testObj->mStateReceiver = new ITestOnboardclientManagerReceiver();


    testObj->signal_list.push_back(0);

    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiver(_,_)).WillRepeatedly(Return(E_OK));

    testObj->onBinderDied(pBBinder);
    delete testObj;

}
#endif




TEST_F(OnboardclientManagerTest, OnboardclientManager_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC01)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    error_t result = E_OK;

    testObj->mOnboardclientService = new ITestIOnboardclientManagerService();

    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnNotifyOBD2Event(_
        )).WillRepeatedly(Return(result));


    //local variables


    testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        );



    delete testObj;
}


/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_registerReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC0001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerTest__param__nullnErr, OnboardclientManager_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC0001)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    serviceid_t id0 = 0;
    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    int isNull;
    error_t result;
    error_t expVal;

    tie(isNull, result) = GetParam();

    if(isNull)
        testObj->mOnboardclientService = NULL;
    else
    {
        testObj->mOnboardclientService = new ITestIOnboardclientManagerService();
    }

    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnNotifyOBD2Event(_
        )).WillRepeatedly(Return(result));

    expVal = isNull? E_ERROR:result;


    EXPECT_EQ(testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        ), expVal);

    delete testObj;
}

/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList const android::sp<IOnboardclientManagerReceiver>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC01
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC01)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    testObj->mOnboardclientService = new ITestIOnboardclientManagerService();


    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnNotifyOBD2Event(_
        )).WillRepeatedly(Return(E_OK));


    //local variables    N:0


    testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        );


    delete testObj;

}


/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList const android::sp<IOnboardclientManagerReceiver>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerTest__param__bool, OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    bool isNull = GetParam();

    if (isNull) {
        testObj->mOnboardclientService = NULL;
    }
    else {
        testObj->mOnboardclientService = new ITestIOnboardclientManagerService();
    }

    EXPECT_CALL(*M_IOnboardclientManagerService, unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(_
        )).WillRepeatedly(Return(E_OK));

    error_t  expVal = isNull? E_ERROR:E_OK;

    //variables

    testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        );

    EXPECT_EQ(testObj->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        ), expVal);

    delete testObj;

}



/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList const android::sp<IOnboardclientManagerReceiver>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC0001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerTest__param__nullnErr, OnboardclientManager_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC0001)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    int isNull;
    error_t result;
    error_t expVal;

    tie(isNull, result) = GetParam();
    expVal = isNull? E_ERROR:result;


    if (isNull) {
        testObj->mOnboardclientService = NULL;
    }
    else {
        testObj->mOnboardclientService = new ITestIOnboardclientManagerService();
    }

    EXPECT_CALL(*M_IOnboardclientManagerService, unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(_
        )).WillRepeatedly(Return(result));

    //variables

    testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        );

    EXPECT_EQ(testObj->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver1
        ), expVal);

    delete testObj;

}




TEST_F(OnboardclientManagerTest, OnboardclientManager_registerReceiverOnboardClientReceiverOnResponseEvent_TC01)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    error_t result = E_OK;

    testObj->mOnboardclientService = new ITestIOnboardclientManagerService();

    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnResponseEvent(_
        )).WillRepeatedly(Return(result));


    //local variables
    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;


    testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
        );



    delete testObj;
}


/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_registerReceiverOnboardClientReceiverOnResponseEvent
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_registerReceiverOnboardClientReceiverOnResponseEvent_TC0001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerTest__param__nullnErr, OnboardclientManager_registerReceiverOnboardClientReceiverOnResponseEvent_TC0001)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    serviceid_t id0 = 0;
    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    int isNull;
    error_t result;
    error_t expVal;

    tie(isNull, result) = GetParam();

    if(isNull)
        testObj->mOnboardclientService = NULL;
    else
    {
        testObj->mOnboardclientService = new ITestIOnboardclientManagerService();
    }

    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnResponseEvent(_
        )).WillRepeatedly(Return(result));

    expVal = isNull? E_ERROR:result;

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    EXPECT_EQ(testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
        ), expVal);

    delete testObj;
}

/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent
*   @paramList const android::sp<IOnboardclientManagerReceiver>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC01
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC01)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    testObj->mOnboardclientService = new ITestIOnboardclientManagerService();


    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnResponseEvent(_
        )).WillRepeatedly(Return(E_OK));


    //local variables    N:0
    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;


    testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
        );


    delete testObj;

}


/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent
*   @paramList const android::sp<IOnboardclientManagerReceiver>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerTest__param__bool, OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC001)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    bool isNull = GetParam();

    if (isNull) {
        testObj->mOnboardclientService = NULL;
    }
    else {
        testObj->mOnboardclientService = new ITestIOnboardclientManagerService();
    }

    EXPECT_CALL(*M_IOnboardclientManagerService, unregisterReceiverOnboardClientReceiverOnResponseEvent(_
        )).WillRepeatedly(Return(E_OK));

    error_t  expVal = isNull? E_ERROR:E_OK;

    //variables
    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
        );

    EXPECT_EQ(testObj->unregisterReceiverOnboardClientReceiverOnResponseEvent( receiver1
        ), expVal);

    delete testObj;

}



/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnResponseEvent function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent
*   @paramList const android::sp<IOnboardclientManagerReceiver>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC0001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerTest__param__nullnErr, OnboardclientManager_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC0001)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    sp<IOnboardClientReceiver> receiver1 = new ITestOnboardClientReceiver();

    int isNull;
    error_t result;
    error_t expVal;

    tie(isNull, result) = GetParam();
    expVal = isNull? E_ERROR:result;


    if (isNull) {
        testObj->mOnboardclientService = NULL;
    }
    else {
        testObj->mOnboardclientService = new ITestIOnboardclientManagerService();
    }

    EXPECT_CALL(*M_IOnboardclientManagerService, unregisterReceiverOnboardClientReceiverOnResponseEvent(_
        )).WillRepeatedly(Return(result));

    //variables
    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver1
        );

    EXPECT_EQ(testObj->unregisterReceiverOnboardClientReceiverOnResponseEvent( receiver1
        ), expVal);

    delete testObj;

}



/**
*   @brief This is a test script for the reregisterReceiver function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_reregisterReceiver
*   @paramList const uint32_t
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_reregisterReceiver_TC01
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_reregisterReceiver_TC01)
{

    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    testObj->mOnboardclientService = new ITestIOnboardclientManagerService();


    // Init
    sp<IOnboardClientReceiver> receiver_OnboardClientReceiver = new ITestOnboardClientReceiver();





    {
    //variables     N:0

    testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event( receiver_OnboardClientReceiver
        );

    }




    {
    //variables     N:0
    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;

    testObj->registerReceiverOnboardClientReceiverOnResponseEvent( receiver_OnboardClientReceiver
        );

    }






    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnNotifyOBD2Event(_
    )).WillRepeatedly(Return(E_ERROR));


    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnResponseEvent(_
    )).WillRepeatedly(Return(E_OK));

    testObj->reregisterReceiver();


    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnNotifyOBD2Event(_
    )).WillRepeatedly(Return(E_OK));


    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnResponseEvent(_
    )).WillRepeatedly(Return(E_ERROR));

    testObj->reregisterReceiver();

    delete testObj;
}

/**
*   @brief This is a test script for the reregisterReceiver function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_reregisterReceiver
*   @paramList const uint32_t
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_reregisterReceiver_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerTest__param__nullnErr, OnboardclientManager_reregisterReceiver_TC001)
{

    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    int isNull;
    error_t result;
    error_t expVal;

    tie(isNull, result) = GetParam();
    expVal = isNull? E_ERROR:E_OK;

    if(isNull) {
        testObj->mOnboardclientService = NULL;
    }
    else {
        testObj->mOnboardclientService = new ITestIOnboardclientManagerService();
    }


    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnNotifyOBD2Event(_
    )).WillRepeatedly(Return(result));


    EXPECT_CALL(*M_IOnboardclientManagerService, registerReceiverOnboardClientReceiverOnResponseEvent(_
    )).WillRepeatedly(Return(result));

     EXPECT_EQ(testObj->reregisterReceiver(), expVal);

    delete testObj;

}


// auto CGA start : wishtoUseAPI
/**
*   @brief This is a test script for the sendUdsData function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_sendUdsData
*   @paramList uint16_t
*   @paramList android::sp<Buffer>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_sendUdsData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientManagerTest__param__bool, OnboardclientManager_sendUdsData_TC001)
{
    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *testObj = new OnboardclientManager();

    bool isNull = GetParam();

    if(isNull)
        testObj->mOnboardclientService = NULL;
    else
    {
        testObj->mOnboardclientService = new ITestIOnboardclientManagerService();
    }
// @CGA_VARIANT_START{"OnboardclientManager_sendUdsData_TC001"}
    // write your own code here
// @CGA_VARIANT___END{"OnboardclientManager_sendUdsData_TC001"}
    delete testObj;

}



/**
*   @brief This is a test script for the binderDied function
*   @classID OnboardclientManager
*   @methodID OnboardclientManager_binderDied
*   @paramList const android::wp<android::IBinder>&
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientManager_binderDied_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientManagerTest, OnboardclientManager_binderDied_TC001)
{

    EXPECT_CALL(*M_OnboardclientManager, getService_mock()).WillRepeatedly(Return(nullptr));

    OnboardclientManager *pParent = new OnboardclientManager();

    OnboardclientManager::ManagerDeathRecipient *testObj = new OnboardclientManager::ManagerDeathRecipient(* pParent);

    testObj->binderDied(nullptr);

    delete testObj;

}
