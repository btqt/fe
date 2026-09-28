

class MockOBCPriorityType {
  public:
    MOCK_METHOD(string , getStringEnum, ());
    MOCK_METHOD(error_t , writeToParcel, (android::Parcel* parcel));
    MOCK_METHOD(error_t , readFromParcel, (const android::Parcel& parcel));
    MOCK_METHOD(uint8_t , getValue, () );
    MOCK_METHOD(int32_t , getValueAsInt32, () );
};

MockOBCPriorityType* M_OBCPriorityType;


string  OBCPriorityType::getStringEnum()
{
  return M_OBCPriorityType->getStringEnum();
}

error_t OBCPriorityType::writeToParcel(android::Parcel* parcel)
{
  return M_OBCPriorityType->writeToParcel(parcel);
}

error_t OBCPriorityType::readFromParcel(const android::Parcel& parcel)
{
  return M_OBCPriorityType->readFromParcel(parcel);
}


uint8_t OBCPriorityType::getValue()
{
  return M_OBCPriorityType->getValue();
}

int32_t OBCPriorityType::getValueAsInt32()
{
  return M_OBCPriorityType->getValueAsInt32();
}
