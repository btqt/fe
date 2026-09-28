
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>

// Include Source File for testing!!
#include "IOnboardclientManagerReceiver.cpp"

#include "mock/binder/Parcel_mock.h"
//#include "mock/onboardclient-service/include/onboardclientCommand_mock.h"

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
class BaseIOnboardClientReceiver {
public:
    virtual ~BaseIOnboardClientReceiver(){  }
};

class MockIOnboardClientReceiver : public BaseIOnboardClientReceiver {
public:
};

MockIOnboardClientReceiver *M_IOnboardClientReceiver;




using namespace android;


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

class IOnboardClientReceiverTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        M_IOnboardClientReceiver = new MockIOnboardClientReceiver();
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
        delete M_IOnboardClientReceiver;
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
*   @brief This is a test script for the onNotifyOBD2Event function
*   @classID IOnboardClientReceiver
*   @methodID IOnboardClientReceiver_onNotifyOBD2Event
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardClientReceiver_onNotifyOBD2Event_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardClientReceiverTest, IOnboardClientReceiver_onNotifyOBD2Event_TC001)
{
    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardClientReceiver *testObj = new BpOnboardClientReceiver(impl);

    //EXPECT_CALL(*M_, writeToParcel(_)).WillRepeatedly(Return(1));

    // sp<OnboardclientData> mOnboardclientData32682 = sp<OnboardclientData>();

    testObj->onNotifyOBD2Event(        //  P0 : 0
    );


    delete testObj;
}

/**
*   @brief This is a test script for the onResponseEvent function
*   @classID IOnboardClientReceiver
*   @methodID IOnboardClientReceiver_onResponseEvent
*   @paramList android::sp< OBCResponseEventInfo >&
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardClientReceiver_onResponseEvent_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardClientReceiverTest, IOnboardClientReceiver_onResponseEvent_TC001)
{
    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardClientReceiver *testObj = new BpOnboardClientReceiver(impl);

    //EXPECT_CALL(*M_, writeToParcel(_)).WillRepeatedly(Return(1));

    android::sp< OBCResponseEventInfo >  mMessage = new OBCResponseEventInfo;
    // sp<OnboardclientData> mOnboardclientData32682 = sp<OnboardclientData>();

    testObj->onResponseEvent(        //  P0 : 0
     mMessage      // 0 argument  
    );


    delete testObj;
}

/**
*   @brief This is a test script for the onTransact function
*   @classID IOnboardClientReceiver
*   @methodID IOnboardClientReceiver_onTransact
*   @paramList uint32_t , const android::Parcel& , android::Parcel* , uint32_t
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardClientReceiver_onTransact_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardClientReceiverTest, IOnboardClientReceiver_onTransact_TC001)
{
    uint32_t code = TRANSACT_ONRECEIVE;
    Parcel * data  = new Parcel;
    Parcel* reply  = new Parcel;
    uint32_t flags = 0;

    ITestOnboardClientReceiver *testObj = new ITestOnboardClientReceiver();

    EXPECT_CALL(*M_Parcel, checkInterface(_)).WillRepeatedly(Return(true));
    //EXPECT_CALL(*M_OnboardclientData, writeToParcel(_)).WillRepeatedly(Return(1));


    testObj->onTransact(code, *data, reply, flags);

}

TEST_F(IOnboardClientReceiverTest,  IOnboardClientReceiver_IMPL_TC001)
{
    BBinder * pBBinder = new BBinder();
    sp<IBinder> impl = sp<IBinder>(pBBinder);
    BpOnboardClientReceiver *testObj = new BpOnboardClientReceiver(impl);


    testObj->getInterfaceDescriptor();
    testObj->asInterface(nullptr);
    sp<IOnboardClientReceiver> ircv = testObj->asInterface(impl);

    ircv = nullptr;
}


class ITestOnboardClientReceiver__param__01 : public ::testing::TestWithParam<tuple<bool,uint32_t>> {
protected:
    virtual void SetUp() {
        //M_IOnboardClientReceiver = new MockIOnboardClientReceiver();
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
        //delete M_IOnboardClientReceiver;
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

INSTANTIATE_TEST_CASE_P(code, ITestOnboardClientReceiver__param__01,
    Combine(Values(0,1),
            Values(
                    TRANSACT_ONRECEIVE,
                    TRANSACT_ONBOARDCLIENTRECEIVER_ONNOTIFYOBD2EVENT ,
                    TRANSACT_ONBOARDCLIENTRECEIVER_ONRESPONSEEVENT ,
                    TRANSACT_ONBOARDCLIENT_COMMAND
                ))
    );


/**
*   @brief This is a test script for the onTransact function
*   @methodID ITestOnboardClientReceiver__param__01
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardClientReceiver_onTransact_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(ITestOnboardClientReceiver__param__01, IOnboardClientReceiver_onTransact_TC001)
{
    uint32_t code;
    bool ckInterface;
    Parcel * data  = new Parcel;
    Parcel* reply  = new Parcel;
    uint32_t flags = 0;

    tie(ckInterface, code) = GetParam();
    ITestOnboardClientReceiver *testObj = new ITestOnboardClientReceiver();
    EXPECT_CALL(*M_Parcel, checkInterface(_)).WillRepeatedly(Return(ckInterface));
    //EXPECT_CALL(*M_TidlData, writeToParcel(_)).WillRepeatedly(Return(1));

    testObj->onTransact(code, *data, reply, flags);
}

#if 0
/**
*   @brief This is a test script for the onTransact function
*   @classID IOnboardclientManagerReceiver
*   @methodID IOnboardclientManagerReceiver_onTransact
*   @paramList uint32_t , const android::Parcel& , android::Parcel* , uint32_t
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardclientManagerReceiver_onTransact_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardclientManagerReceiverTest, IOnboardclientManagerReceiver_onTransact_TC002)
{
    uint32_t code = TRANSACT_ONRECEIVE;
    Parcel * data  = new Parcel;
    Parcel* reply  = new Parcel;
    uint32_t flags = 0;

    ITestOnboardclientManagerReceiver *testObj = new ITestOnboardclientManagerReceiver();

    EXPECT_CALL(*M_Parcel, checkInterface(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*M_OnboardclientData, writeToParcel(_)).WillRepeatedly(Return(1));


    testObj->onTransact(code, *data, reply, flags);

}

/**
*   @brief This is a test script for the onTransact function
*   @classID IOnboardclientManagerReceiver
*   @methodID IOnboardclientManagerReceiver_onTransact
*   @paramList uint32_t , const android::Parcel& , android::Parcel* , uint32_t
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID IOnboardclientManagerReceiver_onTransact_TC003
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(IOnboardclientManagerReceiverTest, IOnboardclientManagerReceiver_onTransact_TC003)
{
    uint32_t code = TRANSACT_ONRECEIVE+1;
    Parcel * data  = new Parcel;
    Parcel* reply  = new Parcel;
    uint32_t flags = 0;

    ITestOnboardclientManagerReceiver *testObj = new ITestOnboardclientManagerReceiver();

    testObj->onTransact(code, *data, reply, flags);

}
#endif
