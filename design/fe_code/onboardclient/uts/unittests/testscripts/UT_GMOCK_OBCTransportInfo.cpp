

/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>

#include "binder/IBinder.h"

// Include Source File for testing!!
#include "OBCTransportInfo.cpp"

#include "mock/binder/Parcel_mock.h"
//#include "mock/binder/IBinder_mock.h"


#include "mock/onboardclient-service/include/OBCErrCode_mock.h"
// MockOBCErrCode *M_OBCErrCode; // redefinition
#include "mock/onboardclient-service/include/OBCPriorityType_mock.h"
// MockOBCPriorityType *M_OBCPriorityType; // redefinition
#include "mock/onboardclient-service/include/OBCProtocolType_mock.h"
// MockOBCProtocolType *M_OBCProtocolType; // redefinition
#include "mock/onboardclient-service/include/OBCUdsResponseType_mock.h"
// MockOBCUdsResponseType *M_OBCUdsResponseType; // redefinition

#if 0
/*
 * Define Mock/Mock function
 */
class BaseOBCTransportInfo {
public:
    virtual ~BaseOBCTransportInfo(){  }
};

class MockOBCTransportInfo : public BaseOBCTransportInfo {
public:
};

MockOBCTransportInfo *M_OBCTransportInfo;
#endif

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
class OBCTransportInfoTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        // M_OBCTransportInfo = new MockOBCTransportInfo();
        M_Parcel = new android::MockParcel();
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
        // delete M_OBCTransportInfo;
        delete M_Parcel;
        delete M_OBCErrCode;
        delete M_OBCPriorityType;
        delete M_OBCProtocolType;
        delete M_OBCUdsResponseType;
    }
};

/**
*   @brief This is a test script for the setDataFormat function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_setData
*   @paramList uint16_t
*   @paramList uint8_t
*   @paramList size_t
*   @paramList muint8_t*
*   @paramList uint64_t
*   @paramList double
*   @paramList muint8_t*
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_setData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_setDataFormat_TC001)
{

    OBCTransportInfoDataFormat mDf;
    //OBCTransportInfo * other0 = new OBCTransportInfo();
    //other0.did = 1;

    OBCTransportInfo* testObj = new OBCTransportInfo();
    //testObj = *other0;
    
        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mUdsResTimeout = static_cast< uint16_t >();

    //uint16_t mdid = 0;
    //uint8_t matt = 0;
    //uint8_t* mstrAutoLenUse = "initstring";
    //size_t mstrAutoLenUseLen = strlen(mstrAutoLenUse);
    //uint64_t mlen = 0;
    //double md5 = 0.71;
    //uint8_t* mstrNoLenUse = NULL;

    testObj->setDataFormat(
        mDf
        , mProtocolType
        , mCanInfo
        , mUdsResTimeout
            //df
            //, mdid
            //, matt
            //, mstrAutoLenUseLen
            //, mstrAutoLenUse
            //, mlen
            //, md5
            //, mstrNoLenUse
    );
    //testObj.setDataFormat(mdid32527, matt32528, mloc32529, mdataLen32530, mdata32531);

    EXPECT_EQ(mDf.protocolType, );

    delete testObj;
}

/**
*   @brief This is a test script for the setData function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_setData
*   @paramList uint16_t
*   @paramList uint8_t
*   @paramList size_t
*   @paramList muint8_t*
*   @paramList uint64_t
*   @paramList double
*   @paramList muint8_t*
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_setData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_setData_TC001)
{
    OBCTransportInfo* testObj = new OBCTransportInfo();
    OBCTransportInfo* testCp = new OBCTransportInfo();
    OBCTransportInfo* testCon = new OBCTransportInfo(*testObj);

        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mUdsResTimeout = static_cast< uint16_t >();



    testObj->setData(
         mProtocolType
        , mCanInfo
        , mUdsResTimeout
    );

    *testCp = *testObj;
    testCp->setData(
         mProtocolType
        , mCanInfo
        , mUdsResTimeout
    );
    //testObj.setData(mdid32527, matt32528, mloc32529, mdataLen32530, mdata32531);

    EXPECT_EQ(testObj->protocolType, );

    delete testObj;
    delete testCp;
}

/**
*   @brief This is a test script for the setTo function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_setTo
*   @paramList const OBCTransportInfo &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_setTo_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_setTo_TC001)
{
    OBCTransportInfo * other0 = new OBCTransportInfo();

    // @CGA_VARIANT_START{"OBCTransportInfo_setTo_TC001_Set_First_Argument"}
    other0->protocolType = 0;
    // @CGA_VARIANT___END{"OBCTransportInfo_setTo_TC001_Set_First_Argument"}

    OBCTransportInfo *testObj = new OBCTransportInfo();

    testObj->setTo(*other0);

    // @CGA_VARIANT_START{"OBCTransportInfo_setTo_TC001_Compare"}
    EXPECT_EQ(testObj->protocolType , 0);
    // @CGA_VARIANT___END{"OBCTransportInfo_setTo_TC001_Compare"}
}

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_writeToParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_writeToParcel_TC001)
{

    OBCTransportInfo *testObj = new OBCTransportInfo();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_writeToParcel_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_writeToParcel_TC002)
{

    OBCTransportInfo *testObj = new OBCTransportInfo();

        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mUdsResTimeout = static_cast< uint16_t >();



    testObj->setData(
         mProtocolType
        , mCanInfo
        , mUdsResTimeout
    );

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_readFromParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_readFromParcel_TC001)
{

    OBCTransportInfo *testObj = new OBCTransportInfo();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_readFromParcel_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_readFromParcel_TC002)
{


// 0
// 0

    //EXPECT_CALL(*M_Parcel, read(_,_)).WillRepeatedly(Return(0));

    OBCTransportInfo *testObj = new OBCTransportInfo();

        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mUdsResTimeout = static_cast< uint16_t >();



    testObj->setData(
         mProtocolType
        , mCanInfo
        , mUdsResTimeout
    );

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}


/**
*   @brief This is a test script for the getProtocolType function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_getProtocolType
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_getProtocolType_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_getProtocolType_TC001)
{
    OBCTransportInfo *testObj = new OBCTransportInfo();

    testObj->protocolType = ;

    EXPECT_EQ(testObj->getProtocolType() ,  );

    delete testObj;

}


/**
*   @brief This is a test script for the getCanInfo function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_getCanInfo
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_getCanInfo_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_getCanInfo_TC001)
{
    OBCTransportInfo *testObj = new OBCTransportInfo();

    testObj->canInfo = ;

    EXPECT_EQ(testObj->getCanInfo() ,  );

    delete testObj;

}


/**
*   @brief This is a test script for the getUdsResTimeout function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_getUdsResTimeout
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_getUdsResTimeout_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_getUdsResTimeout_TC001)
{
    OBCTransportInfo *testObj = new OBCTransportInfo();

    testObj->udsResTimeout = ;

    EXPECT_EQ(testObj->getUdsResTimeout() ,  );

    delete testObj;

}




/**
*   @brief This is a test script for the toString function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_toString
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_toString_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_toString_TC001)
{
    OBCTransportInfo *testObj = new OBCTransportInfo();
    testObj->toString();
}

/**
*   @brief This is a test script for the toString function
*   @classID OBCTransportInfo
*   @methodID OBCTransportInfo_toString
*   @paramList const const char *
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCTransportInfo_toString_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCTransportInfoTest, OBCTransportInfo_toString_TC002)
{

    OBCTransportInfo *testObj = new OBCTransportInfo();

        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mUdsResTimeout = static_cast< uint16_t >();



    testObj->setData(
         mProtocolType
        , mCanInfo
        , mUdsResTimeout

    );
#ifdef uts_remove
    uint8_t data[5] = {0,};

    testObj->data = data;
    testObj->dataLen = 5;
#endif

    testObj->toString("test:");

}
