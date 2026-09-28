

/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>

#include "binder/IBinder.h"

// Include Source File for testing!!
#include "OBCUDSResInfo.cpp"

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
class BaseOBCUDSResInfo {
public:
    virtual ~BaseOBCUDSResInfo(){  }
};

class MockOBCUDSResInfo : public BaseOBCUDSResInfo {
public:
};

MockOBCUDSResInfo *M_OBCUDSResInfo;
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
class OBCUDSResInfoTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        // M_OBCUDSResInfo = new MockOBCUDSResInfo();
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
        // delete M_OBCUDSResInfo;
        delete M_Parcel;
        delete M_OBCErrCode;
        delete M_OBCPriorityType;
        delete M_OBCProtocolType;
        delete M_OBCUdsResponseType;
    }
};

/**
*   @brief This is a test script for the setDataFormat function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_setData
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
*   @test_case_ID OBCUDSResInfo_setData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_setDataFormat_TC001)
{

    OBCUDSResInfoDataFormat mDf;
    //OBCUDSResInfo * other0 = new OBCUDSResInfo();
    //other0.did = 1;

    OBCUDSResInfo* testObj = new OBCUDSResInfo();
    //testObj = *other0;
    
        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mConnectId = static_cast< uint16_t >();

        uint8_t mResponseType = static_cast< uint8_t >();

        android::sp<Buffer> mUdsData = static_cast< android::sp<Buffer> >();

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
        , mConnectId
        , mResponseType
        , mUdsData
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
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_setData
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
*   @test_case_ID OBCUDSResInfo_setData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_setData_TC001)
{
    OBCUDSResInfo* testObj = new OBCUDSResInfo();
    OBCUDSResInfo* testCp = new OBCUDSResInfo();
    OBCUDSResInfo* testCon = new OBCUDSResInfo(*testObj);

        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mConnectId = static_cast< uint16_t >();

        uint8_t mResponseType = static_cast< uint8_t >();

        android::sp<Buffer> mUdsData = static_cast< android::sp<Buffer> >();



    testObj->setData(
         mProtocolType
        , mCanInfo
        , mConnectId
        , mResponseType
        , mUdsData
    );

    *testCp = *testObj;
    testCp->setData(
         mProtocolType
        , mCanInfo
        , mConnectId
        , mResponseType
        , mUdsData
    );
    //testObj.setData(mdid32527, matt32528, mloc32529, mdataLen32530, mdata32531);

    EXPECT_EQ(testObj->protocolType, );

    delete testObj;
    delete testCp;
}

/**
*   @brief This is a test script for the setTo function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_setTo
*   @paramList const OBCUDSResInfo &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_setTo_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_setTo_TC001)
{
    OBCUDSResInfo * other0 = new OBCUDSResInfo();

    // @CGA_VARIANT_START{"OBCUDSResInfo_setTo_TC001_Set_First_Argument"}
    other0->protocolType = 0;
    // @CGA_VARIANT___END{"OBCUDSResInfo_setTo_TC001_Set_First_Argument"}

    OBCUDSResInfo *testObj = new OBCUDSResInfo();

    testObj->setTo(*other0);

    // @CGA_VARIANT_START{"OBCUDSResInfo_setTo_TC001_Compare"}
    EXPECT_EQ(testObj->protocolType , 0);
    // @CGA_VARIANT___END{"OBCUDSResInfo_setTo_TC001_Compare"}
}

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_writeToParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_writeToParcel_TC001)
{

    OBCUDSResInfo *testObj = new OBCUDSResInfo();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_writeToParcel_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_writeToParcel_TC002)
{

    OBCUDSResInfo *testObj = new OBCUDSResInfo();

        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mConnectId = static_cast< uint16_t >();

        uint8_t mResponseType = static_cast< uint8_t >();

        android::sp<Buffer> mUdsData = static_cast< android::sp<Buffer> >();



    testObj->setData(
         mProtocolType
        , mCanInfo
        , mConnectId
        , mResponseType
        , mUdsData
    );

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_readFromParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_readFromParcel_TC001)
{

    OBCUDSResInfo *testObj = new OBCUDSResInfo();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_readFromParcel_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_readFromParcel_TC002)
{


// 0
// 0

    //EXPECT_CALL(*M_Parcel, read(_,_)).WillRepeatedly(Return(0));

    OBCUDSResInfo *testObj = new OBCUDSResInfo();

        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mConnectId = static_cast< uint16_t >();

        uint8_t mResponseType = static_cast< uint8_t >();

        android::sp<Buffer> mUdsData = static_cast< android::sp<Buffer> >();



    testObj->setData(
         mProtocolType
        , mCanInfo
        , mConnectId
        , mResponseType
        , mUdsData
    );

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}


/**
*   @brief This is a test script for the getProtocolType function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_getProtocolType
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_getProtocolType_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_getProtocolType_TC001)
{
    OBCUDSResInfo *testObj = new OBCUDSResInfo();

    testObj->protocolType = ;

    EXPECT_EQ(testObj->getProtocolType() ,  );

    delete testObj;

}


/**
*   @brief This is a test script for the getCanInfo function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_getCanInfo
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_getCanInfo_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_getCanInfo_TC001)
{
    OBCUDSResInfo *testObj = new OBCUDSResInfo();

    testObj->canInfo = ;

    EXPECT_EQ(testObj->getCanInfo() ,  );

    delete testObj;

}


/**
*   @brief This is a test script for the getConnectId function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_getConnectId
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_getConnectId_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_getConnectId_TC001)
{
    OBCUDSResInfo *testObj = new OBCUDSResInfo();

    testObj->connectId = ;

    EXPECT_EQ(testObj->getConnectId() ,  );

    delete testObj;

}


/**
*   @brief This is a test script for the getResponseType function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_getResponseType
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_getResponseType_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_getResponseType_TC001)
{
    OBCUDSResInfo *testObj = new OBCUDSResInfo();

    testObj->responseType = ;

    EXPECT_EQ(testObj->getResponseType() ,  );

    delete testObj;

}


/**
*   @brief This is a test script for the getUdsData function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_getUdsData
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_getUdsData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_getUdsData_TC001)
{
    OBCUDSResInfo *testObj = new OBCUDSResInfo();

    testObj->udsData = ;

    EXPECT_EQ(testObj->getUdsData() ,  );

    delete testObj;

}




/**
*   @brief This is a test script for the toString function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_toString
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_toString_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_toString_TC001)
{
    OBCUDSResInfo *testObj = new OBCUDSResInfo();
    testObj->toString();
}

/**
*   @brief This is a test script for the toString function
*   @classID OBCUDSResInfo
*   @methodID OBCUDSResInfo_toString
*   @paramList const const char *
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUDSResInfo_toString_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUDSResInfoTest, OBCUDSResInfo_toString_TC002)
{

    OBCUDSResInfo *testObj = new OBCUDSResInfo();

        uint8_t mProtocolType = static_cast< uint8_t >();

        OBCCanInfo mCanInfo = static_cast< OBCCanInfo >();

        uint16_t mConnectId = static_cast< uint16_t >();

        uint8_t mResponseType = static_cast< uint8_t >();

        android::sp<Buffer> mUdsData = static_cast< android::sp<Buffer> >();



    testObj->setData(
         mProtocolType
        , mCanInfo
        , mConnectId
        , mResponseType
        , mUdsData

    );
#ifdef uts_remove
    uint8_t data[5] = {0,};

    testObj->data = data;
    testObj->dataLen = 5;
#endif

    testObj->toString("test:");

}
