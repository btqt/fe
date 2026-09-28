
/**********************************************
 *********   UTS VERSION : 1.5.0-a1    *********
 **********************************************/


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <dlfcn.h>
#include <tuple>

// Include Source File for testing!!
#include "OBCUdsResponseType.cpp"

#if 0
/*
 * Define Mock/Mock function
 */
class BaseOBCUdsResponseType {
public:
    virtual ~BaseOBCUdsResponseType(){  }
};

class MockOBCUdsResponseType : public BaseOBCUdsResponseType {
public:
};

MockOBCUdsResponseType *M_OBCUdsResponseType;
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


class OBCUdsResponseTypeTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        //M_OBCUdsResponseType = new MockOBCUdsResponseType();
    }
    virtual void TearDown() {
        //delete M_OBCUdsResponseType;
    }
};

/**
*   @brief This is a test script for the writeToParcel function
*   @classID OBCUdsResponseType
*   @methodID OBCUdsResponseType_writeToParcel
*   @paramList android::Parcel
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUdsResponseType_writeToParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUdsResponseTypeTest, OBCUdsResponseType_writeToParcel_TC001)
{

    OBCUdsResponseType testObj;

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj.writeToParcel(parcel0),E_OK) ;

}

/**
*   @brief This is a test script for the readFromParcel function
*   @classID OBCUdsResponseType
*   @methodID OBCUdsResponseType_readFromParcel
*   @paramList const android::Parcel &
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUdsResponseType_readFromParcel_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUdsResponseTypeTest, OBCUdsResponseType_readFromParcel_TC001)
{

    OBCUdsResponseType *testObj = new OBCUdsResponseType();

    android::Parcel *parcel0 = new android::Parcel;

    EXPECT_EQ(testObj->readFromParcel(*parcel0),E_OK) ;

}


/**
*   @brief This is a test script for the getValue function
*   @classID OBCUdsResponseType
*   @methodID OBCUdsResponseType_getValue
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUdsResponseType_getValue_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_F(OBCUdsResponseTypeTest, OBCUdsResponseType_getValue_TC001)
{
    OBCUdsResponseType testObj;

    testObj = static_cast< uint8_t >(1);

    EXPECT_EQ(testObj.getValue(), static_cast< uint8_t >(1) );
}

class OBCUdsResponseType_param : public ::testing::TestWithParam<std::tuple<uint8_t,std::string>> {
protected:
    virtual void SetUp() {
        //M_OBCUdsResponseType = new MockOBCUdsResponseType();
    }
    virtual void TearDown() {
        //delete M_OBCUdsResponseType;
    }
};

INSTANTIATE_TEST_CASE_P(enum, OBCUdsResponseType_param,
    Values(

        std::make_tuple(OBCUdsResponseType::UNKOWN , "OBCUdsResponseType::UNKOWN"),
        std::make_tuple(OBCUdsResponseType::NORMAL , "OBCUdsResponseType::NORMAL"),
        std::make_tuple(OBCUdsResponseType::PERIODEC , "OBCUdsResponseType::PERIODEC"),
        std::make_tuple(OBCUdsResponseType::EVENT , "OBCUdsResponseType::EVENT"),
        std::make_tuple(-1 , "NULL")
));


/**
*   @brief This is a test script for the getValueAsInt32 function
*   @classID OBCUdsResponseType
*   @methodID OBCUdsResponseType_getValueAsInt32
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUdsResponseType_getValueAsInt32_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OBCUdsResponseType_param, OBCUdsResponseType_getValueAsInt32_TC001)
{
    OBCUdsResponseType testObj;

    testObj = static_cast< uint8_t >(get<0>(GetParam()));

    EXPECT_EQ(testObj.getValueAsInt32(), get<0>(GetParam()) );
}

/**
*   @brief This is a test script for the getStringEnum function
*   @classID OBCUdsResponseType
*   @methodID OBCUdsResponseType_getStringEnum
*   @paramList
*   @priority P2
*   @resolution_Method Equivalence Partitioning
*   @test_condition
*   @test_Coverage_Item
*   @test_case_ID OBCUdsResponseType_getStringEnum_TC001
*   @test_type functionality
*   @test_objective
*   @test_precon
*   @test_input
*   @test_expected_result
*   @test_module
*   @design_id
*/
TEST_P(OBCUdsResponseType_param, OBCUdsResponseType_getStringEnum_TC001)
{

    OBCUdsResponseType testObj;
    testObj = get<0>(GetParam());

    EXPECT_EQ(testObj.getStringEnum(), get<1>(GetParam()) );

}
