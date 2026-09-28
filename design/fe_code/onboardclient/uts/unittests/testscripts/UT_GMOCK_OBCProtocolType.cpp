
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>
#include <tuple>

// Include Source File for testing!!
#include "OBCProtocolType.cpp"

#if 0
/*
 * Define Mock/Mock function
 */
class BaseOBCProtocolType {
public:
    virtual ~BaseOBCProtocolType(){  }
};

class MockOBCProtocolType : public BaseOBCProtocolType {
public:
};

MockOBCProtocolType *M_OBCProtocolType;
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


class OBCProtocolTypeTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        //M_OBCProtocolType = new MockOBCProtocolType();
    }
    virtual void TearDown() {
        //delete M_OBCProtocolType;
    }
};

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCProtocolType
*   @methodID OBCProtocolType_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCProtocolType_writeToParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCProtocolTypeTest, OBCProtocolType_writeToParcel_TC001)
{

    OBCProtocolType testObj;

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj.writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCProtocolType
*   @methodID OBCProtocolType_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCProtocolType_readFromParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCProtocolTypeTest, OBCProtocolType_readFromParcel_TC001)
{

    OBCProtocolType *testObj = new OBCProtocolType();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}


/**
*   @brief This is a test script for the getValue function
*   @classID OBCProtocolType
*   @methodID OBCProtocolType_getValue
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCProtocolType_getValue_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCProtocolTypeTest, OBCProtocolType_getValue_TC001)
{
    OBCProtocolType testObj;

    testObj = static_cast< uint8_t >(1);

    EXPECT_EQ(testObj.getValue(), static_cast< uint8_t >(1) );
}

class OBCProtocolType_param : public ::testing::TestWithParam<std::tuple<uint8_t,std::string>> {
protected:
    virtual void SetUp() {
        //M_OBCProtocolType = new MockOBCProtocolType();
    }
    virtual void TearDown() {
        //delete M_OBCProtocolType;
    }
};

INSTANTIATE_TEST_CASE_P(enum, OBCProtocolType_param,
    Values(

        std::make_tuple(OBCProtocolType::UNKNOWN , "OBCProtocolType::UNKNOWN"),
        std::make_tuple(OBCProtocolType::DOCAN , "OBCProtocolType::DOCAN"),
        std::make_tuple(OBCProtocolType::DOCAN29BIT , "OBCProtocolType::DOCAN29BIT"),
        std::make_tuple(OBCProtocolType::DOCAN11BITEX , "OBCProtocolType::DOCAN11BITEX"),
        std::make_tuple(OBCProtocolType::DOCAN29BITCANFD , "OBCProtocolType::DOCAN29BITCANFD"),
        std::make_tuple(-1 , "NULL")
));


/**
*   @brief This is a test script for the getValueAsInt32 function
*   @classID OBCProtocolType
*   @methodID OBCProtocolType_getValueAsInt32
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCProtocolType_getValueAsInt32_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OBCProtocolType_param, OBCProtocolType_getValueAsInt32_TC001)
{
    OBCProtocolType testObj;

    testObj = static_cast< uint8_t >(get<0>(GetParam()));

    EXPECT_EQ(testObj.getValueAsInt32(), get<0>(GetParam()) );
}

/**
*   @brief This is a test script for the getStringEnum function
*   @classID OBCProtocolType
*   @methodID OBCProtocolType_getStringEnum
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCProtocolType_getStringEnum_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OBCProtocolType_param, OBCProtocolType_getStringEnum_TC001)
{

    OBCProtocolType testObj;
    testObj = get<0>(GetParam());

    EXPECT_EQ(testObj.getStringEnum(), get<1>(GetParam()) );

}
