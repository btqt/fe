
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>
#include <tuple>

// Include Source File for testing!!
#include "OBCPriorityType.cpp"

#if 0
/*
 * Define Mock/Mock function
 */
class BaseOBCPriorityType {
public:
    virtual ~BaseOBCPriorityType(){  }
};

class MockOBCPriorityType : public BaseOBCPriorityType {
public:
};

MockOBCPriorityType *M_OBCPriorityType;
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


class OBCPriorityTypeTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        //M_OBCPriorityType = new MockOBCPriorityType();
    }
    virtual void TearDown() {
        //delete M_OBCPriorityType;
    }
};

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCPriorityType
*   @methodID OBCPriorityType_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCPriorityType_writeToParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCPriorityTypeTest, OBCPriorityType_writeToParcel_TC001)
{

    OBCPriorityType testObj;

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj.writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCPriorityType
*   @methodID OBCPriorityType_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCPriorityType_readFromParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCPriorityTypeTest, OBCPriorityType_readFromParcel_TC001)
{

    OBCPriorityType *testObj = new OBCPriorityType();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}


/**
*   @brief This is a test script for the getValue function
*   @classID OBCPriorityType
*   @methodID OBCPriorityType_getValue
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCPriorityType_getValue_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCPriorityTypeTest, OBCPriorityType_getValue_TC001)
{
    OBCPriorityType testObj;

    testObj = static_cast< uint8_t >(1);

    EXPECT_EQ(testObj.getValue(), static_cast< uint8_t >(1) );
}

class OBCPriorityType_param : public ::testing::TestWithParam<std::tuple<uint8_t,std::string>> {
protected:
    virtual void SetUp() {
        //M_OBCPriorityType = new MockOBCPriorityType();
    }
    virtual void TearDown() {
        //delete M_OBCPriorityType;
    }
};

INSTANTIATE_TEST_CASE_P(enum, OBCPriorityType_param,
    Values(

        std::make_tuple(OBCPriorityType::NONE , "OBCPriorityType::NONE"),
        std::make_tuple(OBCPriorityType::LOW , "OBCPriorityType::LOW"),
        std::make_tuple(OBCPriorityType::MIDDLE , "OBCPriorityType::MIDDLE"),
        std::make_tuple(OBCPriorityType::HIGH , "OBCPriorityType::HIGH"),
        std::make_tuple(-1 , "NULL")
));


/**
*   @brief This is a test script for the getValueAsInt32 function
*   @classID OBCPriorityType
*   @methodID OBCPriorityType_getValueAsInt32
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCPriorityType_getValueAsInt32_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OBCPriorityType_param, OBCPriorityType_getValueAsInt32_TC001)
{
    OBCPriorityType testObj;

    testObj = static_cast< uint8_t >(get<0>(GetParam()));

    EXPECT_EQ(testObj.getValueAsInt32(), get<0>(GetParam()) );
}

/**
*   @brief This is a test script for the getStringEnum function
*   @classID OBCPriorityType
*   @methodID OBCPriorityType_getStringEnum
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCPriorityType_getStringEnum_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OBCPriorityType_param, OBCPriorityType_getStringEnum_TC001)
{

    OBCPriorityType testObj;
    testObj = get<0>(GetParam());

    EXPECT_EQ(testObj.getStringEnum(), get<1>(GetParam()) );

}
