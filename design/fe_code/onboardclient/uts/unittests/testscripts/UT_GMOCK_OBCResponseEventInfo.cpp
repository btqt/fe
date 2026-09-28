

/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>

#include "binder/IBinder.h"

// Include Source File for testing!!
#include "OBCResponseEventInfo.cpp"

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
class BaseOBCResponseEventInfo {
public:
    virtual ~BaseOBCResponseEventInfo(){  }
};

class MockOBCResponseEventInfo : public BaseOBCResponseEventInfo {
public:
};

MockOBCResponseEventInfo *M_OBCResponseEventInfo;
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
class OBCResponseEventInfoTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        // M_OBCResponseEventInfo = new MockOBCResponseEventInfo();
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
        // delete M_OBCResponseEventInfo;
        delete M_Parcel;
        delete M_OBCErrCode;
        delete M_OBCPriorityType;
        delete M_OBCProtocolType;
        delete M_OBCUdsResponseType;
    }
};

/**
*   @brief This is a test script for the setDataFormat function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_setData
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
*   @test_case_ID OBCResponseEventInfo_setData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_setDataFormat_TC001)
{

    OBCResponseEventInfoDataFormat mDf;
    //OBCResponseEventInfo * other0 = new OBCResponseEventInfo();
    //other0.did = 1;

    OBCResponseEventInfo* testObj = new OBCResponseEventInfo();
    //testObj = *other0;
    
        uint8_t mErrCode = static_cast< uint8_t >();

        OBCUDSResInfo mResInfo = static_cast< OBCUDSResInfo >();

    //uint16_t mdid = 0;
    //uint8_t matt = 0;
    //uint8_t* mstrAutoLenUse = "initstring";
    //size_t mstrAutoLenUseLen = strlen(mstrAutoLenUse);
    //uint64_t mlen = 0;
    //double md5 = 0.71;
    //uint8_t* mstrNoLenUse = NULL;

    testObj->setDataFormat(
        mDf
        , mErrCode
        , mResInfo
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

    EXPECT_EQ(mDf.errCode, );

    delete testObj;
}

/**
*   @brief This is a test script for the setData function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_setData
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
*   @test_case_ID OBCResponseEventInfo_setData_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_setData_TC001)
{
    OBCResponseEventInfo* testObj = new OBCResponseEventInfo();
    OBCResponseEventInfo* testCp = new OBCResponseEventInfo();
    OBCResponseEventInfo* testCon = new OBCResponseEventInfo(*testObj);

        uint8_t mErrCode = static_cast< uint8_t >();

        OBCUDSResInfo mResInfo = static_cast< OBCUDSResInfo >();



    testObj->setData(
         mErrCode
        , mResInfo
    );

    *testCp = *testObj;
    testCp->setData(
         mErrCode
        , mResInfo
    );
    //testObj.setData(mdid32527, matt32528, mloc32529, mdataLen32530, mdata32531);

    EXPECT_EQ(testObj->errCode, );

    delete testObj;
    delete testCp;
}

/**
*   @brief This is a test script for the setTo function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_setTo
*   @paramList const OBCResponseEventInfo &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_setTo_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_setTo_TC001)
{
    OBCResponseEventInfo * other0 = new OBCResponseEventInfo();

    // @CGA_VARIANT_START{"OBCResponseEventInfo_setTo_TC001_Set_First_Argument"}
    other0->errCode = 0;
    // @CGA_VARIANT___END{"OBCResponseEventInfo_setTo_TC001_Set_First_Argument"}

    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();

    testObj->setTo(*other0);

    // @CGA_VARIANT_START{"OBCResponseEventInfo_setTo_TC001_Compare"}
    EXPECT_EQ(testObj->errCode , 0);
    // @CGA_VARIANT___END{"OBCResponseEventInfo_setTo_TC001_Compare"}
}

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_writeToParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_writeToParcel_TC001)
{

    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_writeToParcel_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_writeToParcel_TC002)
{

    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();

        uint8_t mErrCode = static_cast< uint8_t >();

        OBCUDSResInfo mResInfo = static_cast< OBCUDSResInfo >();



    testObj->setData(
         mErrCode
        , mResInfo
    );

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_readFromParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_readFromParcel_TC001)
{

    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_readFromParcel_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_readFromParcel_TC002)
{


// 0
// 0

    //EXPECT_CALL(*M_Parcel, read(_,_)).WillRepeatedly(Return(0));

    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();

        uint8_t mErrCode = static_cast< uint8_t >();

        OBCUDSResInfo mResInfo = static_cast< OBCUDSResInfo >();



    testObj->setData(
         mErrCode
        , mResInfo
    );

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}


/**
*   @brief This is a test script for the getErrCode function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_getErrCode
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_getErrCode_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_getErrCode_TC001)
{
    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();

    testObj->errCode = ;

    EXPECT_EQ(testObj->getErrCode() ,  );

    delete testObj;

}


/**
*   @brief This is a test script for the getResInfo function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_getResInfo
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_getResInfo_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_getResInfo_TC001)
{
    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();

    testObj->resInfo = ;

    EXPECT_EQ(testObj->getResInfo() ,  );

    delete testObj;

}




/**
*   @brief This is a test script for the toString function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_toString
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_toString_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_toString_TC001)
{
    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();
    testObj->toString();
}

/**
*   @brief This is a test script for the toString function
*   @classID OBCResponseEventInfo
*   @methodID OBCResponseEventInfo_toString
*   @paramList const const char *
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCResponseEventInfo_toString_TC002
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCResponseEventInfoTest, OBCResponseEventInfo_toString_TC002)
{

    OBCResponseEventInfo *testObj = new OBCResponseEventInfo();

        uint8_t mErrCode = static_cast< uint8_t >();

        OBCUDSResInfo mResInfo = static_cast< OBCUDSResInfo >();



    testObj->setData(
         mErrCode
        , mResInfo

    );
#ifdef uts_remove
    uint8_t data[5] = {0,};

    testObj->data = data;
    testObj->dataLen = 5;
#endif

    testObj->toString("test:");

}
