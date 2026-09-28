

class MockOBCProtocolType {
  public:
    MOCK_METHOD(string , getStringEnum, ());
    MOCK_METHOD(error_t , writeToParcel, (android::Parcel* parcel));
    MOCK_METHOD(error_t , readFromParcel, (const android::Parcel& parcel));
    MOCK_METHOD(uint8_t , getValue, () );
    MOCK_METHOD(int32_t , getValueAsInt32, () );
};

MockOBCProtocolType* M_OBCProtocolType;


string  OBCProtocolType::getStringEnum()
{
  return M_OBCProtocolType->getStringEnum();
}

error_t OBCProtocolType::writeToParcel(android::Parcel* parcel)
{
  return M_OBCProtocolType->writeToParcel(parcel);
}

error_t OBCProtocolType::readFromParcel(const android::Parcel& parcel)
{
  return M_OBCProtocolType->readFromParcel(parcel);
}


uint8_t OBCProtocolType::getValue()
{
  return M_OBCProtocolType->getValue();
}

int32_t OBCProtocolType::getValueAsInt32()
{
  return M_OBCProtocolType->getValueAsInt32();
}
