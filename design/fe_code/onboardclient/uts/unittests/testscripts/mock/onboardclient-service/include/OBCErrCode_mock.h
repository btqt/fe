

class MockOBCErrCode {
  public:
    MOCK_METHOD(string , getStringEnum, ());
    MOCK_METHOD(error_t , writeToParcel, (android::Parcel* parcel));
    MOCK_METHOD(error_t , readFromParcel, (const android::Parcel& parcel));
    MOCK_METHOD(uint8_t , getValue, () );
    MOCK_METHOD(int32_t , getValueAsInt32, () );
};

MockOBCErrCode* M_OBCErrCode;


string  OBCErrCode::getStringEnum()
{
  return M_OBCErrCode->getStringEnum();
}

error_t OBCErrCode::writeToParcel(android::Parcel* parcel)
{
  return M_OBCErrCode->writeToParcel(parcel);
}

error_t OBCErrCode::readFromParcel(const android::Parcel& parcel)
{
  return M_OBCErrCode->readFromParcel(parcel);
}


uint8_t OBCErrCode::getValue()
{
  return M_OBCErrCode->getValue();
}

int32_t OBCErrCode::getValueAsInt32()
{
  return M_OBCErrCode->getValueAsInt32();
}
