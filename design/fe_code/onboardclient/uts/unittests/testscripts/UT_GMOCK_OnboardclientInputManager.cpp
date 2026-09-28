
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/

// not TIDL

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>
#include <memory>

// Include Source File for testing!!
#include "OnboardclientInputManager.cpp"
#include "OnboardclientYoursInputManager.cpp"

#include "mock/onboardclient-service/service/OnboardclientManagerService_mock.h"
//#include "mock/onboardclient-service/include/OnboardclientCommand_mock.h"
#include "mock/servicelayer/corebase/SystemService_mock.h"
#include "mock/servicelayer/utils/Handler_mock.h"
#include "mock/servicelayer/utils/Timer_mock.h"
#include "mock/servicelayer/utils/Buffer_mock.h"
#include "mock/servicelayer/utils/SLLooper_mock.h"
#include "mock/servicelayer/utils/THandler_mock.h"
#include "mock/servicelayer/utils/Message_mock.h"
#include "mock/servicelayer/utils/watchdog/watchdog_client_mock.h"

#include "mock/binder/Binder_mock.h"
#include "mock/binder/BpBinder_mock.h"
#include "mock/binder/IInterface_mock.h"
#include "mock/binder/IServiceManager_mock.h"
#include "mock/service/app-service/AppManager_mock.h"

/*
//  vvv
#include "mock/service/vvv-service/include/CDid_mock.h"
#include "mock/service/vvv-service/include/CVvvData_mock.h"
#include "mock/service/vvv-service/include/CVvvItaly_mock.h"
#include "mock/service/vvv-service/include/EVvvSignal_mock.h"
#include "mock/service/vvv-service/include/EVvvDid_mock.h"
#include "mock/service/vvv-service/include/IVvvBetterReceiver_mock.h"
#include "mock/service/vvv-service/include/IVvvMannerReceiver_mock.h"
*/


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

/*
 * Define Mock/Mock function
 */
class BaseOnboardclientInputManager {
public:
    virtual ~BaseOnboardclientInputManager(){  }
};

class MockOnboardclientInputManager : public BaseOnboardclientInputManager {
public:
};

MockOnboardclientInputManager *M_OnboardclientInputManager;

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

class OnboardclientInputManagerTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        M_OnboardclientInputManager = new MockOnboardclientInputManager();
        M_OnboardclientManagerService = new MockOnboardclientManagerService();
        M_IServiceManager = new MockIServiceManager();
        M_AppManager = new MockAppManager();
        M_IInterface = new MockIInterface();
        M_BBinder = new MockBBinder();
        M_Buffer = new MockBuffer();
        //M_OnboardclientData = new MockOnboardclientData();
        M_Handler = new MockHandler();
        M_Message = new MockMessage();
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
        delete M_OnboardclientInputManager;
        delete M_OnboardclientManagerService;
        delete M_IServiceManager;
        delete M_AppManager;
        delete M_IInterface;
        delete M_BBinder;
        delete M_Buffer;
        //delete M_OnboardclientData;
        delete M_Handler;
        delete M_Message;
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


#if 0  // todo
/**
*   @brief This is a test script for the init function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_init
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_init_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_init_TC001)
{
    bool ret_param_0 = false;
    EXPECT_CALL(*M_AppManager, getBootCompleted(_)).WillRepeatedly(Return(ret_param_0));
    error_t ret_param_2 = E_OK;
    EXPECT_CALL(*M_AppManager, registerSystemPostReceiver(_,_)).WillRepeatedly(Return(ret_param_2));

    AppManager * pAppManager = new AppManager();
    sp<IBinder> pBinder = sp<IBinder>(pAppManager);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));

    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_1));

    OnboardclientManagerService * pInstManager = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pInstManager);
    error_t retVal = E_OK;

    retVal = testObj->init();
    EXPECT_EQ(retVal, E_OK);

    delete testObj;
#ifdef uts_todo
    delete pInstManager;
    delete pAppManager;
    delete pBBinder;
#endif
}

/**
*   @brief This is a test script for the init function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_init
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_init_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_init_TC002)
{
    bool ret_param_0 = false;
    EXPECT_CALL(*M_AppManager, getBootCompleted(_)).WillRepeatedly(Return(ret_param_0));

    error_t ret_param_1 = E_OK;
    EXPECT_CALL(*M_AppManager, registerSystemPostReceiver(_,_)).WillRepeatedly(Return(ret_param_1));

    AppManager * pAppManager = NULL;
    sp<IBinder> pBinder = sp<IBinder>(pAppManager);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));

    status_t ret_param_2 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_2));

    bool ret_param_3 = true;
    EXPECT_CALL(*M_Handler, sendMessageDelayed(_,_)).WillRepeatedly(Return(ret_param_3));
    sp<sl::Message> ret_param_4 = sp<sl::Message>(new sl::Message);
    EXPECT_CALL(*M_Handler, obtainMessage_002(_)).WillRepeatedly(Return(ret_param_4));

    OnboardclientManagerService * pInstManager = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pInstManager);
    error_t retVal = E_OK;

    retVal = testObj->init();
    EXPECT_EQ(retVal, E_OK);

    delete testObj;
#ifdef uts_todo
    delete pInstManager;
    delete pAppManager;
    delete pBBinder;
#endif
}
#endif

class OnboardclientInputManagerTest__param__01 : public ::testing::TestWithParam<int32_t> {
protected:
    virtual void SetUp() {
        M_OnboardclientInputManager = new MockOnboardclientInputManager();
        M_IServiceManager = new MockIServiceManager();
        M_AppManager = new MockAppManager();
        M_IInterface = new MockIInterface();
        M_BBinder = new MockBBinder();
        M_Buffer = new MockBuffer();
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
        delete M_OnboardclientInputManager;
        delete M_IServiceManager;
        delete M_AppManager;
        delete M_IInterface;
        delete M_BBinder;
        delete M_Buffer;
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

INSTANTIATE_TEST_CASE_P(what, OnboardclientInputManagerTest__param__01,
    Values(
    MSG_RECEIVE_BOOT_COMPLETE,
    MSG_RECEIVE_WATCH_DOG,
    MSG_BOOT_COMPLETE_DID_START,
    MSG_SIGNAL_INTERNAL_IGNITION2_STATUS,
    MSG_CONNECT_TO_APPMGR,

    /* auto Related_Manager CGA start-------------------------------------------------*/
    /* auto Related_Manager Inheritance CGA end-------------------------------------------------*/

    // recivers of related managers (ex. v2x , wifi)

    -1
    ));

/**
*   @brief This is a test script for the handleMessage function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_handleMessage
*   @paramList const android::sp<sl::Message> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_handleMessage_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OnboardclientInputManagerTest__param__01, OnboardclientInputManager_handleMessage_TC001)
{
    bool ret_param_0 = false;
    EXPECT_CALL(*M_AppManager, getBootCompleted(_)).WillRepeatedly(Return(ret_param_0));
    error_t ret_param_1 = E_OK;
    EXPECT_CALL(*M_AppManager, registerSystemPostReceiver(_,_)).WillRepeatedly(Return(ret_param_1));

    AppManager * pAppManager = new AppManager();
    sp<IBinder> pBinder = sp<IBinder>(pAppManager);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));

    status_t ret_param_2 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_2));

    uint32_t ret_param_3 = 1;
    uint8_t param_data[10] = {0, };
    EXPECT_CALL(*M_Buffer, size()).WillRepeatedly(Return(ret_param_3));
    EXPECT_CALL(*M_Buffer, data()).WillRepeatedly(Return(param_data));

    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * pInputMgr = new OnboardclientInputManager(pMgrSvc);
    OnboardclientInputManager::OnboardclientHandler * testObj = new OnboardclientInputManager::OnboardclientHandler(pMgrSvc->looper(), *pInputMgr);
    sp<sl::Message> msg0 = sp<sl::Message>(new sl::Message);
    msg0->what = GetParam();

    testObj->handleMessage(msg0);
}

/**
*   @brief This is a test script for the handleMessage function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_handleMessage
*   @paramList const android::sp<sl::Message> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_handleMessage_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_handleMessage_TC002)
{
    bool ret_param_0 = false;
    EXPECT_CALL(*M_AppManager, getBootCompleted(_)).WillRepeatedly(Return(ret_param_0));
    error_t ret_param_1 = E_OK;
    EXPECT_CALL(*M_AppManager, registerSystemPostReceiver(_,_)).WillRepeatedly(Return(ret_param_1));

    AppManager * pAppManager = new AppManager();
    sp<IBinder> pBinder = sp<IBinder>(pAppManager);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));

    status_t ret_param_2 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_2));

    uint32_t ret_param_3 = 0;
    uint8_t param_data[10] = {0, };
    EXPECT_CALL(*M_Buffer, size()).WillRepeatedly(Return(ret_param_3));
    EXPECT_CALL(*M_Buffer, data()).WillRepeatedly(Return(param_data));

    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * pInputMgr = new OnboardclientInputManager(pMgrSvc);
    OnboardclientInputManager::OnboardclientHandler * testObj = new OnboardclientInputManager::OnboardclientHandler(pMgrSvc->looper(), *pInputMgr);
    sp<sl::Message> msg0 = sp<sl::Message>(new sl::Message);
    msg0->what = DID_WORK_FOR_DEFINE_8;

    testObj->handleMessage(msg0);

    delete pMgrSvc;
}

#if 0
/**
*   @brief This is a test script for the transferDatabyVIF function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_transferDatabyVIF
*   @paramList android::sp<OnboardclientData> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_transferDatabyVIF_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_transferDatabyVIF_TC001)
{
    error_t ret_param_0 = E_OK;
    //EXPECT_CALL(*M_OnboardclientManagerService, queryReceiverByID(_,_)).WillRepeatedly(Return(ret_param_0));
    uint16_t ret_param_1 = 0;
    //EXPECT_CALL(*M_OnboardclientData, getDid()).WillRepeatedly(Return(ret_param_1));

    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pMgrSvc);

    android::sp<OnboardclientData> mOnboardclientData0 = android::sp<OnboardclientData>();

    testObj->transferDatabyVIF(mOnboardclientData0);
}

/**
*   @brief This is a test script for the messagefromVIF function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_messagefromVIF
*   @paramList uint16_t, android::sp<Buffer> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_messagefromVIF_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_messagefromVIF_TC001)
{
    error_t ret_param_0 = E_OK;
    EXPECT_CALL(*M_OnboardclientManagerService, queryReceiverByID(_,_)).WillRepeatedly(Return(ret_param_0));
    uint16_t ret_param_1 = 0;
    EXPECT_CALL(*M_OnboardclientData, getDid()).WillRepeatedly(Return(ret_param_1));

    uint8_t param_data[10] = {0, };
    EXPECT_CALL(*M_Buffer, data()).WillRepeatedly(Return(param_data));

    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pMgrSvc);

    uint16_t sigId0 = 0;
    android::sp<Buffer> buf1 = android::sp<Buffer>(new Buffer);

    testObj->messagefromVIF(sigId0, buf1);
}

/**
*   @brief This is a test script for the messagefromVIF function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_messagefromVIF
*   @paramList uint16_t, android::sp<Buffer> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_messagefromVIF_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_messagefromVIF_TC002)
{
    error_t ret_param_0 = E_OK;
    EXPECT_CALL(*M_OnboardclientManagerService, queryReceiverByID(_,_)).WillRepeatedly(Return(ret_param_0));
    uint16_t ret_param_1 = 0;
    EXPECT_CALL(*M_OnboardclientData, getDid()).WillRepeatedly(Return(ret_param_1));

    uint8_t param_data[10] = {0, };
    EXPECT_CALL(*M_Buffer, data()).WillRepeatedly(Return(param_data));

    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pMgrSvc);

    uint16_t sigId0 = 0;
    android::sp<Buffer> buf1 = NULL;//android::sp<Buffer>(new Buffer);

    EXPECT_EQ(E_OK, testObj->messagefromVIF(sigId0, buf1));

//	delete testObj;
    delete pMgrSvc;
}

/**
*   @brief This is a test script for the messagefromVIF function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_messagefromVIF
*   @paramList uint16_t, android::sp<Buffer> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_messagefromVIF_TC003
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_messagefromVIF_TC003)
{
    error_t ret_param_0 = E_OK;
    EXPECT_CALL(*M_OnboardclientManagerService, queryReceiverByID(_,_)).WillRepeatedly(Return(ret_param_0));
    uint16_t ret_param_1 = 0;
    //EXPECT_CALL(*M_OnboardclientData, getDid()).WillRepeatedly(Return(ret_param_1));

    //uint8_t param_data[10] = {0, };
    EXPECT_CALL(*M_Buffer, data()).WillRepeatedly(Return(nullptr));

    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pMgrSvc);

    uint16_t sigId0 = 0;
    android::sp<Buffer> buf1 = android::sp<Buffer>(new Buffer);

    EXPECT_EQ(E_OK, testObj->messagefromVIF(sigId0, buf1));

    delete pMgrSvc;
}
#endif


/**
*   @brief This is a test script for the connectToAppMgr function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_connectToAppMgr
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_connectToAppMgr_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_connectToAppMgr_TC001)
{

    error_t ret_param_0 = E_OK;
    //EXPECT_CALL(*M_OnboardclientManagerService, queryReceiverByID(_,_)).WillRepeatedly(Return(ret_param_0));
    uint16_t ret_param_1 = 0;
    //EXPECT_CALL(*M_OnboardclientData, getDid()).WillRepeatedly(Return(ret_param_1));

    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pMgrSvc);

    testObj->connectToAppMgr();

    delete pMgrSvc;

}

/**
*   @brief This is a test script for the connectToAppMgr function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_connectToAppMgr
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_connectToAppMgr_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_connectToAppMgr_TC002)
{

    bool ret_param_0 = true;
    EXPECT_CALL(*M_AppManager, getBootCompleted(_)).WillRepeatedly(Return(ret_param_0));
    error_t ret_param_1 = E_OK;
    EXPECT_CALL(*M_AppManager, registerSystemPostReceiver(_,_)).WillRepeatedly(Return(ret_param_1));

    AppManager * pAppManager = new AppManager();
    sp<IBinder> pBinder = sp<IBinder>(pAppManager);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pInterface));

    status_t ret_param_2 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_2));

    uint32_t ret_param_3 = 1;
    uint8_t param_data[10] = {0, };
    EXPECT_CALL(*M_Buffer, size()).WillRepeatedly(Return(ret_param_3));
    EXPECT_CALL(*M_Buffer, data()).WillRepeatedly(Return(param_data));

    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pMgrSvc);


    testObj->connectToAppMgr();

    delete pMgrSvc;

}

/* auto Related_Manager Inheritance CGA start-------------------------------------------------*/
// current on/off status for related manager (Onboardclient register multiple callbacks on the following managers)
    // Related Manager - alarm : X : alarm
    // Related Manager - antenna : X : Antenna
    // Related Manager - audio : X : Audio
    // Related Manager - audioio : X : audioio
    // Related Manager - audiorouter : X : audiorouter
    // Related Manager - config : X : Config
    // Related Manager - diag : X : Diag
    // Related Manager - ecall : X : Ecall
    // Related Manager - ethmgr : X : 
    // Related Manager - filesys : X : Filesys
    // Related Manager - health : X : Health
    // Related Manager - hmi : X : HMI
    // Related Manager - locmgr : X : Location
    // Related Manager - packetaudio : X : PacketAudio
    // Related Manager - power : X : Power
    // Related Manager - progmgr : X : Prog
    // Related Manager - route : X : Route
    // Related Manager - telephony : X : Telephony
    // Related Manager - time : X : Time
    // Related Manager - updateagent : X : UpdateAgent
    // Related Manager - vif : X : vif



/**
*   @brief This is a test script for the onServiceBinderDied function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_onServiceBinderDied
*   @paramList const android::wp<android::IBinder> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_onServiceBinderDied_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_onServiceBinderDied_TC001)
{
    bool ret_param_0 = false;
    EXPECT_CALL(*M_AppManager, getBootCompleted(_)).WillRepeatedly(Return(ret_param_0));
    error_t ret_param_2 = E_OK;
    EXPECT_CALL(*M_AppManager, registerSystemPostReceiver(_,_)).WillRepeatedly(Return(ret_param_2));

    AppManager * pAppManager = new AppManager();
    sp<IBinder> pBinder = sp<IBinder>(pAppManager);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pAppManager));

    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_1));

    OnboardclientManagerService * pInstManager = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pInstManager);
    error_t retVal = E_OK;

    retVal = testObj->init();
    EXPECT_EQ(retVal, E_OK);

    wp<IBinder> who0 = wp<IBinder>(pBBinder);

    testObj->onServiceBinderDied(who0);

    if(testObj)
        delete testObj;
}

class OnboardclientInputManagerTest__param__02 : public ::testing::TestWithParam<int>{
protected:
    virtual void SetUp() {
        M_OnboardclientInputManager = new MockOnboardclientInputManager();
        M_IServiceManager = new MockIServiceManager();
        M_AppManager = new MockAppManager();
        M_IInterface = new MockIInterface();
        M_BBinder = new MockBBinder();
        M_Buffer = new MockBuffer();
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
        delete M_OnboardclientInputManager;
        delete M_IServiceManager;
        delete M_AppManager;
        delete M_IInterface;
        delete M_BBinder;
        delete M_Buffer;
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

INSTANTIATE_TEST_CASE_P(what, OnboardclientInputManagerTest__param__02,
    Values(0,1,2,3,4,5,6,7));

/**
*   @brief This is a test script for the onServiceBinderDied function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_onServiceBinderDied
*   @paramList const android::wp<android::IBinder> &
*   @priority
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_onServiceBinderDied_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/

TEST_P(OnboardclientInputManagerTest__param__02, OnboardclientInputManager_onServiceBinderDied_TC002)
{
    bool ret_param_0 = false;
    EXPECT_CALL(*M_AppManager, getBootCompleted(_)).WillRepeatedly(Return(ret_param_0));
    error_t ret_param_2 = E_OK;
    EXPECT_CALL(*M_AppManager, registerSystemPostReceiver(_,_)).WillRepeatedly(Return(ret_param_2));

    AppManager * pAppManager = new AppManager();
    sp<IBinder> pBinder = sp<IBinder>(pAppManager);
    EXPECT_CALL(*M_IServiceManager, getService(_)).WillRepeatedly(Return(pBinder));

    BBinder * pBBinder = new BBinder();
    sp<IBinder> pInterface = sp<IBinder>(pBBinder);
    EXPECT_CALL(*M_IInterface, asBinder_004(_)).WillRepeatedly(Return(pBBinder));

    status_t ret_param_1 = NO_ERROR;
    EXPECT_CALL(*M_BBinder, linkToDeath(_,_,_)).WillRepeatedly(Return(ret_param_1));

    OnboardclientManagerService * pInstManager = new OnboardclientManagerService();
    OnboardclientInputManager * testObj = new OnboardclientInputManager(pInstManager);
    error_t retVal = E_OK;

    retVal = testObj->init();
    EXPECT_EQ(retVal, E_OK);

    wp<IBinder> who0 = wp<IBinder>(pBBinder);

    //auto caseCondition = {testObj->mAppManager, testObj->mAudioManager, testObj->mDiagManager,
    //			testObj->mHmiManager, testObj->mLocManager, testObj->mSystemManager, testObj->mVifManager};

    //int i = 0;
    // 0
    int caseCondition = GetParam();
    testObj->mAppManager = caseCondition>0? NULL:testObj->mAppManager;     // 1
    //testObj->mAudioManager = caseCondition>1? NULL:testObj->mAudioManager;
    //testObj->mDiagManager = caseCondition>2? NULL:testObj->mDiagManager;
    //testObj->mHmiManager = caseCondition>3? NULL:testObj->mHmiManager;
    //testObj->mLocManager = caseCondition>4? NULL:testObj->mLocManager;
    //testObj->mSystemManager = caseCondition>5? NULL:testObj->mSystemManager;
    //testObj->mVifManager = caseCondition>6? NULL:testObj->mVifManager;

    testObj->onServiceBinderDied(who0);


    if(testObj)
        delete testObj;
}




/* ============================= for *.h =========================*/
/**
*   @brief This is a test script for the handlerFunction function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_handlerFunction
*   @paramList int
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_handlerFunction_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_handlerFunction_TC001)
{
    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * pInputMgr = new OnboardclientInputManager(pMgrSvc);
    OnboardclientInputManager::OnboardclientHandler * pHandler = new OnboardclientInputManager::OnboardclientHandler(pMgrSvc->looper(), *pInputMgr);
    OnboardclientInputManager::OnboardclientInputMgrTimer * testObj = new OnboardclientInputManager::OnboardclientInputMgrTimer(pHandler);

    sp<sl::Message> ret_param_0 = sp<sl::Message>(new sl::Message);
    EXPECT_CALL(*M_Handler, obtainMessage_002(_)).WillRepeatedly(Return(ret_param_0));
    EXPECT_CALL(*M_Message, sendToTarget()).WillRepeatedly(Return(0));


    int timer_id = testObj->ONBOARDCLIENT_WATCHDOG_TIMER;

    testObj->handlerFunction(timer_id);

    delete pMgrSvc;
}

/**
*   @brief This is a test script for the handlerFunction function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_handlerFunction
*   @paramList int
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_handlerFunction_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_handlerFunction_TC002)
{
    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * pInputMgr = new OnboardclientInputManager(pMgrSvc);
    OnboardclientInputManager::OnboardclientHandler * pHandler = new OnboardclientInputManager::OnboardclientHandler(pMgrSvc->looper(), *pInputMgr);
    OnboardclientInputManager::OnboardclientInputMgrTimer * testObj = new OnboardclientInputManager::OnboardclientInputMgrTimer(pHandler);


    int timer_id = 0;

    testObj->handlerFunction(timer_id);

    delete pMgrSvc;
}

/**
*   @brief This is a test script for the binderDied function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_binderDied
*   @paramList const android::wp<android::IBinder>&
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_binderDied_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_binderDied_TC001)
{
    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * pInputMgr = new OnboardclientInputManager(pMgrSvc);
    OnboardclientInputManager::ServiceDeathRecipient  * testObj = new OnboardclientInputManager::ServiceDeathRecipient (*pInputMgr);

    BBinder * pBBinder = new BBinder();
    wp<IBinder> who0 = wp<IBinder>(pBBinder);

    testObj->binderDied(who0);

    delete pMgrSvc;
}

/**
*   @brief This is a test script for the onSystemPostReceived function
*   @classID OnboardclientInputManager
*   @methodID OnboardclientInputManager_onSystemPostReceived
*   @paramList const android::sp<Post>&
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OnboardclientInputManager_onSystemPostReceived_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OnboardclientInputManagerTest, OnboardclientInputManager_onSystemPostReceived_TC001)
{
    OnboardclientManagerService * pMgrSvc = new OnboardclientManagerService();
    OnboardclientInputManager * pInputMgr = new OnboardclientInputManager(pMgrSvc);
    OnboardclientInputManager::OnboardclientHandler * pHandler = new OnboardclientInputManager::OnboardclientHandler(pMgrSvc->looper(), *pInputMgr);
    OnboardclientInputManager::systemPostReceiver  * testObj = new OnboardclientInputManager::systemPostReceiver(pHandler);

    sp<sl::Message> ret_param_0 = sp<sl::Message>(new sl::Message);
    EXPECT_CALL(*M_Handler, obtainMessage_002(_)).WillRepeatedly(Return(ret_param_0));
    EXPECT_CALL(*M_Message, sendToTarget()).WillRepeatedly(Return(0));


    testObj->onSystemPostReceived(nullptr);

    delete pMgrSvc;
}


/*=============  OnboardclientYoursInputManager.cpp 's Unit Test ================*/

/* auto Related_Manager Inheritance CGA end-------------------------------------------------*/
