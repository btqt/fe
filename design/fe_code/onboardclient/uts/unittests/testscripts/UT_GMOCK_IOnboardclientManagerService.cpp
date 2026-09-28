
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>

// Include Source File for testing!!
#include "IOnboardclientManagerService.cpp"


#include "mock/binder/Parcel_mock.h"
//#include "mock/onboardclient-service/include/OnboardclientCommand_mock.h"

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

#include "mock/onboardclient-service/include/IOnboardclientManagerService_mock2.h"

/*
 * Define Mock/Mock function
 */
class BaseIOnboardclientManagerService {
public:
    virtual ~BaseIOnboardclientManagerService(){  }
};

class MockIOnboardclientManagerService : public BaseIOnboardclientManagerService {
public:
};

MockIOnboardclientManagerService *M_IOnboardclientManagerService;

using namespace android;
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
class IOnboardclientManagerServiceTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        M_IOnboardclientManagerService = new MockIOnboardclientManagerService();
        M_Parcel = new MockParcel;
        //M_OnboardclientData = new MockOnboardclientData;
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
         delete M_IOnboardclientManagerService;
         delete M_Parcel;
         //delete M_OnboardclientData;
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

/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID IOnboardclientManagerService
*   @methodID IOnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_registerReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001)
{
    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);

    android::sp<IOnboardClientReceiver> receiver32707 = android::sp<IOnboardClientReceiver>();

    testObj->registerReceiverOnboardClientReceiverOnNotifyOBD2Event(
            receiver32707
        );

    delete testObj;
}

/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event function
*   @classID IOnboardclientManagerService
*   @methodID IOnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event
*   @paramList const android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event_TC001)
{

    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);

    android::sp<IOnboardClientReceiver> receiver0 = android::sp<IOnboardClientReceiver>();

    testObj->unregisterReceiverOnboardClientReceiverOnNotifyOBD2Event(
            receiver0
        );

    delete testObj;
}

/**
*   @brief This is a test script for the registerReceiverOnboardClientReceiverOnResponseEvent function
*   @classID IOnboardclientManagerService
*   @methodID IOnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent
*   @paramList serviceid_t, android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_registerReceiverOnboardClientReceiverOnResponseEvent_TC001)
{
    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;
    android::sp<IOnboardClientReceiver> receiver32707 = android::sp<IOnboardClientReceiver>();

    testObj->registerReceiverOnboardClientReceiverOnResponseEvent(
            receiver32707
        );

    delete testObj;
}

/**
*   @brief This is a test script for the unregisterReceiverOnboardClientReceiverOnResponseEvent function
*   @classID IOnboardclientManagerService
*   @methodID IOnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent
*   @paramList const android::sp<IOnboardclientManagerReceiver> &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_unregisterReceiverOnboardClientReceiverOnResponseEvent_TC001)
{

    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;
    android::sp<IOnboardClientReceiver> receiver0 = android::sp<IOnboardClientReceiver>();

    testObj->unregisterReceiverOnboardClientReceiverOnResponseEvent(
            receiver0
        );

    delete testObj;
}


// auto CGA start : wishtoUseAPI
/**
*   @brief This is a test script for the sendUdsData function
*   @classID IOnboardclientManagerService
*   @methodID IOnboardclientManagerService_sendUdsData
*   @paramList uint16_t
*   @paramList android::sp<Buffer>
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardclientManagerService_sendUdsData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_sendUdsData_TC001)
{
    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);
// @CGA_VARIANT_START{"IOnboardclientManagerService_sendUdsData_TC001"}
    // write your own code here
// @CGA_VARIANT___END{"IOnboardclientManagerService_sendUdsData_TC001"}
    delete testObj;
}

// auto CGA end : wishtoUseAPI


TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_testOnNotifyOBD2Event_TC001)
{
    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);

// Initialize: declare variables


// Set xxData class  if has xxData : each API has only one xxData.


// Call API
    testObj->testOnNotifyOBD2Event(   //  Parm:0
    );


            //  Parm:0



    delete testObj;

}
TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_testOnResponseEvent_TC001)
{
    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);

// Initialize: declare variables
                
    uint8_t merrCode;
    OBCUDSResInfo mresInfo;


// Set xxData class  if has xxData : each API has only one xxData.
    android::sp<OBCResponseEventInfo> _new_OBCResponseEventInfo = new OBCResponseEventInfo();
    //memcpy(. . .);
    _new_OBCResponseEventInfo->setData(
        
         merrCode
        ,  mresInfo
    );


// Call API
    testObj->testOnResponseEvent(   //  Parm:0
                 _new_OBCResponseEventInfo         //  Parm:1
    );


            //  Parm:0



    delete testObj;

}


TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_getInterfaceDescriptor_TC001)
{
    //test for IMPLEMENT_META_INTERFACE(TidlManagerService, "service_layer.IOnboardclientManagerService");
    //--> IOnboardclientManagerService::getInterfaceDescriptor();

    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);

    testObj->getInterfaceDescriptor();

    delete testObj;
}


TEST_F(IOnboardclientManagerServiceTest, IOnboardclientManagerService_asInterface_TC001)
{
    //test for IMPLEMENT_META_INTERFACE(OnboardclientManagerService, "service_layer.IOnboardclientManagerService");
    //android::sp<I##INTERFACE> I##INTERFACE::asInterface(const android::sp<android::IBinder>& obj)
    //--> IOnboardclientManagerService::asInterface();

    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardclientManagerService *testObj = new BpOnboardclientManagerService(impl);

    testObj->asInterface(impl);

    testObj->asInterface(nullptr);

    delete testObj;
}

class IOnboardclientManagerServiceTest__param__01 : public ::testing::TestWithParam<tuple<bool, int32_t, int32_t>> {
protected:
    virtual void SetUp() {
        M_IOnboardclientManagerService = new MockIOnboardclientManagerService();
        M_Parcel = new MockParcel;
        //M_OnboardclientData = new MockOnboardclientData;
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
         delete M_IOnboardclientManagerService;
         delete M_Parcel;
         //delete M_OnboardclientData;
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

INSTANTIATE_TEST_CASE_P(code, IOnboardclientManagerServiceTest__param__01,
    Combine(Values(0,1),
            Values(0,2),
            Values(
                OP_REGISTER_RECEIVER,
                OP_REGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT,
                OP_REGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT,

                OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT,
                OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT,

                OP_SENDUDSDATA,

                OP_REGISTER_TEST_ONNOTIFYOBD2EVENT,
                OP_REGISTER_TEST_ONRESPONSEEVENT,

           -1))
    );

/**
*   @brief This is a test script for the registerReceiver function
*   @classID IOnboardclientManagerService
*   @methodID IOnboardclientManagerService_onTransact
*   @paramList uint32_t , const android::Parcel& , android::Parcel* , uint32_t
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardclientManagerService_onTransact_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(IOnboardclientManagerServiceTest__param__01, IOnboardclientManagerService_onTransact_TC001)
{

    uint32_t code;
    bool ckInterface;
    Parcel * data  = new Parcel;
    Parcel* reply  = new Parcel;
    uint32_t flags = 0;
    int32_t change_value;

    tie(ckInterface, change_value, code) = GetParam();

    ITestIOnboardclientManagerService *testObj = new ITestIOnboardclientManagerService();

    EXPECT_CALL(*M_Parcel, checkInterface(_)).WillRepeatedly(Return(ckInterface));
    //EXPECT_CALL(*M_OnboardclientData, writeToParcel(_)).WillRepeatedly(Return(1));

    testObj->change_value = change_value;
    testObj->onTransact(code, *data, reply, flags);

    delete testObj;
}




class IOnboardclientManagerServiceTest__param__02 : public ::testing::TestWithParam<tuple<int32_t, int32_t>> {
protected:
    virtual void SetUp() {
        M_IOnboardclientManagerService = new MockIOnboardclientManagerService();
        M_Parcel = new MockParcel;
        //M_OnboardclientData = new MockOnboardclientData;
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
         delete M_IOnboardclientManagerService;
         delete M_Parcel;
         //delete M_OnboardclientData;
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


INSTANTIATE_TEST_CASE_P(code, IOnboardclientManagerServiceTest__param__02,
    Combine(Values(0,2),
            Values(
                OP_REGISTER_RECEIVER,
                OP_REGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT,
                OP_REGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT,

                OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT,
                OP_UNREGISTER_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT,

                OP_SENDUDSDATA,

                OP_REGISTER_TEST_ONNOTIFYOBD2EVENT,
                OP_REGISTER_TEST_ONRESPONSEEVENT,
           -1))
    );


TEST_P(IOnboardclientManagerServiceTest__param__02, IOnboardclientManagerService_onTransact_TC001)
{

    int32_t readInt_v = 1;
    uint32_t code;
    bool ckInterface = true;
    Parcel * data  = new Parcel;
    Parcel* reply  = new Parcel;
    uint32_t flags = 0;
    int32_t change_value;

    tie(change_value, code) = GetParam();

    printf("$$$_001   Code = %d \n", code);

    ITestIOnboardclientManagerService *testObj = new ITestIOnboardclientManagerService();

    EXPECT_CALL(*M_Parcel, readFloat()).WillRepeatedly(Return(0.0));
    EXPECT_CALL(*M_Parcel, readDouble()).WillRepeatedly(Return(0.0));
    EXPECT_CALL(*M_Parcel, readUint32()).WillRepeatedly(Return(0));

    EXPECT_CALL(*M_Parcel, readInt32()).WillRepeatedly(Return(readInt_v));
    EXPECT_CALL(*M_Parcel, checkInterface(_)).WillRepeatedly(Return(ckInterface));

    testObj->change_value = change_value;
    testObj->onTransact(code, *data, reply, flags);

}


TEST_P(IOnboardclientManagerServiceTest__param__02, IOnboardclientManagerService_onTransact_TC002)
{

    int32_t readInt_v = 1024000 + 1;
    uint32_t code;
    bool ckInterface = true;
    Parcel * data  = new Parcel;
    Parcel* reply  = new Parcel;
    uint32_t flags = 0;
    int32_t change_value;

    tie(change_value, code) = GetParam();

    printf("$$$_002   Code = %d \n", code);

    ITestIOnboardclientManagerService *testObj = new ITestIOnboardclientManagerService();

    EXPECT_CALL(*M_Parcel, readFloat()).WillRepeatedly(Return(0.0));
    EXPECT_CALL(*M_Parcel, readDouble()).WillRepeatedly(Return(0.0));
    EXPECT_CALL(*M_Parcel, readUint32()).WillRepeatedly(Return(0));

    EXPECT_CALL(*M_Parcel, readInt32()).WillRepeatedly(Return(readInt_v));
    EXPECT_CALL(*M_Parcel, checkInterface(_)).WillRepeatedly(Return(ckInterface));

    testObj->change_value = change_value;
    testObj->onTransact(code, *data, reply, flags);

}
