

class MockOBCUdsResponseType {
  public:
    MOCK_METHOD(string , getStringEnum, ());
    MOCK_METHOD(error_t , writeToParcel, (android::Parcel* parcel));
    MOCK_METHOD(error_t , readFromParcel, (const android::Parcel& parcel));
    MOCK_METHOD(uint8_t , getValue, () );
    MOCK_METHOD(int32_t , getValueAsInt32, () );
};

MockOBCUdsResponseType* M_OBCUdsResponseType;


string  OBCUdsResponseType::getStringEnum()
{
  return M_OBCUdsResponseType->getStringEnum();
}

error_t OBCUdsResponseType::writeToParcel(android::Parcel* parcel)
{
  return M_OBCUdsResponseType->writeToParcel(parcel);
}

error_t OBCUdsResponseType::readFromParcel(const android::Parcel& parcel)
{
  return M_OBCUdsResponseType->readFromParcel(parcel);
}


uint8_t OBCUdsResponseType::getValue()
{
  return M_OBCUdsResponseType->getValue();
}

int32_t OBCUdsResponseType::getValueAsInt32()
{
  return M_OBCUdsResponseType->getValueAsInt32();
}
