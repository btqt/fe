

/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>

#include "binder/IBinder.h"

// Include Source File for testing!!
#include "OBCCanInfo.cpp"

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
class BaseOBCCanInfo {
public:
    virtual ~BaseOBCCanInfo(){  }
};

class MockOBCCanInfo : public BaseOBCCanInfo {
public:
};

MockOBCCanInfo *M_OBCCanInfo;
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
class OBCCanInfoTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        // M_OBCCanInfo = new MockOBCCanInfo();
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
        // delete M_OBCCanInfo;
        delete M_Parcel;
        delete M_OBCErrCode;
        delete M_OBCPriorityType;
        delete M_OBCProtocolType;
        delete M_OBCUdsResponseType;
    }
};

/**
*   @brief This is a test script for the setDataFormat function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_setData
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
*   @test_case_ID OBCCanInfo_setData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_setDataFormat_TC001)
{

    OBCCanInfoDataFormat mDf;
    //OBCCanInfo * other0 = new OBCCanInfo();
    //other0.did = 1;

    OBCCanInfo* testObj = new OBCCanInfo();
    //testObj = *other0;
    
        uint32_t mCanId = static_cast< uint32_t >();

        android::sp<Buffer> mNTa = static_cast< android::sp<Buffer> >();

    //uint16_t mdid = 0;
    //uint8_t matt = 0;
    //uint8_t* mstrAutoLenUse = "initstring";
    //size_t mstrAutoLenUseLen = strlen(mstrAutoLenUse);
    //uint64_t mlen = 0;
    //double md5 = 0.71;
    //uint8_t* mstrNoLenUse = NULL;

    testObj->setDataFormat(
        mDf
        , mCanId
        , mNTa
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

    EXPECT_EQ(mDf.canId, );

    delete testObj;
}

/**
*   @brief This is a test script for the setData function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_setData
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
*   @test_case_ID OBCCanInfo_setData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_setData_TC001)
{
    OBCCanInfo* testObj = new OBCCanInfo();
    OBCCanInfo* testCp = new OBCCanInfo();
    OBCCanInfo* testCon = new OBCCanInfo(*testObj);

        uint32_t mCanId = static_cast< uint32_t >();

        android::sp<Buffer> mNTa = static_cast< android::sp<Buffer> >();



    testObj->setData(
         mCanId
        , mNTa
    );

    *testCp = *testObj;
    testCp->setData(
         mCanId
        , mNTa
    );
    //testObj.setData(mdid32527, matt32528, mloc32529, mdataLen32530, mdata32531);

    EXPECT_EQ(testObj->canId, );

    delete testObj;
    delete testCp;
}

/**
*   @brief This is a test script for the setTo function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_setTo
*   @paramList const OBCCanInfo &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_setTo_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_setTo_TC001)
{
    OBCCanInfo * other0 = new OBCCanInfo();

    // @CGA_VARIANT_START{"OBCCanInfo_setTo_TC001_Set_First_Argument"}
    other0->canId = 0;
    // @CGA_VARIANT___END{"OBCCanInfo_setTo_TC001_Set_First_Argument"}

    OBCCanInfo *testObj = new OBCCanInfo();

    testObj->setTo(*other0);

    // @CGA_VARIANT_START{"OBCCanInfo_setTo_TC001_Compare"}
    EXPECT_EQ(testObj->canId , 0);
    // @CGA_VARIANT___END{"OBCCanInfo_setTo_TC001_Compare"}
}

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_writeToParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_writeToParcel_TC001)
{

    OBCCanInfo *testObj = new OBCCanInfo();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_writeToParcel_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_writeToParcel_TC002)
{

    OBCCanInfo *testObj = new OBCCanInfo();

        uint32_t mCanId = static_cast< uint32_t >();

        android::sp<Buffer> mNTa = static_cast< android::sp<Buffer> >();



    testObj->setData(
         mCanId
        , mNTa
    );

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_readFromParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_readFromParcel_TC001)
{

    OBCCanInfo *testObj = new OBCCanInfo();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_readFromParcel_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_readFromParcel_TC002)
{


// 0
// 0

    //EXPECT_CALL(*M_Parcel, read(_,_)).WillRepeatedly(Return(0));

    OBCCanInfo *testObj = new OBCCanInfo();

        uint32_t mCanId = static_cast< uint32_t >();

        android::sp<Buffer> mNTa = static_cast< android::sp<Buffer> >();



    testObj->setData(
         mCanId
        , mNTa
    );

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}


/**
*   @brief This is a test script for the getCanId function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_getCanId
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_getCanId_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_getCanId_TC001)
{
    OBCCanInfo *testObj = new OBCCanInfo();

    testObj->canId = ;

    EXPECT_EQ(testObj->getCanId() ,  );

    delete testObj;

}


/**
*   @brief This is a test script for the getNTa function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_getNTa
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_getNTa_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_getNTa_TC001)
{
    OBCCanInfo *testObj = new OBCCanInfo();

    testObj->nTa = ;

    EXPECT_EQ(testObj->getNTa() ,  );

    delete testObj;

}




/**
*   @brief This is a test script for the toString function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_toString
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_toString_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_toString_TC001)
{
    OBCCanInfo *testObj = new OBCCanInfo();
    testObj->toString();
}

/**
*   @brief This is a test script for the toString function
*   @classID OBCCanInfo
*   @methodID OBCCanInfo_toString
*   @paramList const const char *
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCCanInfo_toString_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCCanInfoTest, OBCCanInfo_toString_TC002)
{

    OBCCanInfo *testObj = new OBCCanInfo();

        uint32_t mCanId = static_cast< uint32_t >();

        android::sp<Buffer> mNTa = static_cast< android::sp<Buffer> >();



    testObj->setData(
         mCanId
        , mNTa

    );
#ifdef uts_remove
    uint8_t data[5] = {0,};

    testObj->data = data;
    testObj->dataLen = 5;
#endif

    testObj->toString("test:");

}
