
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>
#include <tuple>

// Include Source File for testing!!
#include "OBCErrCode.cpp"

#if 0
/*
 * Define Mock/Mock function
 */
class BaseOBCErrCode {
public:
    virtual ~BaseOBCErrCode(){  }
};

class MockOBCErrCode : public BaseOBCErrCode {
public:
};

MockOBCErrCode *M_OBCErrCode;
#endif
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


class OBCErrCodeTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        //M_OBCErrCode = new MockOBCErrCode();
    }
    virtual void TearDown() {
        //delete M_OBCErrCode;
    }
};

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCErrCode
*   @methodID OBCErrCode_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCErrCode_writeToParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCErrCodeTest, OBCErrCode_writeToParcel_TC001)
{

    OBCErrCode testObj;

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj.writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCErrCode
*   @methodID OBCErrCode_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCErrCode_readFromParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCErrCodeTest, OBCErrCode_readFromParcel_TC001)
{

    OBCErrCode *testObj = new OBCErrCode();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}


/**
*   @brief This is a test script for the getValue function
*   @classID OBCErrCode
*   @methodID OBCErrCode_getValue
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCErrCode_getValue_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCErrCodeTest, OBCErrCode_getValue_TC001)
{
    OBCErrCode testObj;

    testObj = static_cast< uint8_t >(1);

    EXPECT_EQ(testObj.getValue(), static_cast< uint8_t >(1) );
}

class OBCErrCode_param : public ::testing::TestWithParam<std::tuple<uint8_t,std::string>> {
protected:
    virtual void SetUp() {
        //M_OBCErrCode = new MockOBCErrCode();
    }
    virtual void TearDown() {
        //delete M_OBCErrCode;
    }
};

INSTANTIATE_TEST_CASE_P(enum, OBCErrCode_param,
    Values(

        std::make_tuple(OBCErrCode::OBC_OK , "OBCErrCode::OBC_OK"),
        std::make_tuple(OBCErrCode::OBC_ERR_BUSY , "OBCErrCode::OBC_ERR_BUSY"),
        std::make_tuple(OBCErrCode::OBC_ERR_CONNECTED_MAX , "OBCErrCode::OBC_ERR_CONNECTED_MAX"),
        std::make_tuple(OBCErrCode::OBC_ERR_USED , "OBCErrCode::OBC_ERR_USED"),
        std::make_tuple(OBCErrCode::OBC_ERR_UDS_CANCEL_PRIORITY , "OBCErrCode::OBC_ERR_UDS_CANCEL_PRIORITY"),
        std::make_tuple(OBCErrCode::OBC_ERR_DISCONNECTED_PRIORITY , "OBCErrCode::OBC_ERR_DISCONNECTED_PRIORITY"),
        std::make_tuple(OBCErrCode::OBC_NEGATIVE , "OBCErrCode::OBC_NEGATIVE"),
        std::make_tuple(OBCErrCode::OBC_TIMEOUT , "OBCErrCode::OBC_TIMEOUT"),
        std::make_tuple(OBCErrCode::OBC_ERR_FAILED , "OBCErrCode::OBC_ERR_FAILED"),
        std::make_tuple(OBCErrCode::OBC_ERR_INVALID_PARAMETERS , "OBCErrCode::OBC_ERR_INVALID_PARAMETERS"),
        std::make_tuple(OBCErrCode::OBC_ERR_NOT_CONNECTED , "OBCErrCode::OBC_ERR_NOT_CONNECTED"),
        std::make_tuple(OBCErrCode::OBC_ERR_SEND_UDS_DATA , "OBCErrCode::OBC_ERR_SEND_UDS_DATA"),
        std::make_tuple(OBCErrCode::OBC_ERR_NOT_DISCONNECTED , "OBCErrCode::OBC_ERR_NOT_DISCONNECTED"),
        std::make_tuple(OBCErrCode::OBC_ERR_NOT_SUPPORTED , "OBCErrCode::OBC_ERR_NOT_SUPPORTED"),
        std::make_tuple(-1 , "NULL")
));


/**
*   @brief This is a test script for the getValueAsInt32 function
*   @classID OBCErrCode
*   @methodID OBCErrCode_getValueAsInt32
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCErrCode_getValueAsInt32_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OBCErrCode_param, OBCErrCode_getValueAsInt32_TC001)
{
    OBCErrCode testObj;

    testObj = static_cast< uint8_t >(get<0>(GetParam()));

    EXPECT_EQ(testObj.getValueAsInt32(), get<0>(GetParam()) );
}

/**
*   @brief This is a test script for the getStringEnum function
*   @classID OBCErrCode
*   @methodID OBCErrCode_getStringEnum
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCErrCode_getStringEnum_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OBCErrCode_param, OBCErrCode_getStringEnum_TC001)
{

    OBCErrCode testObj;
    testObj = get<0>(GetParam());

    EXPECT_EQ(testObj.getStringEnum(), get<1>(GetParam()) );

}
