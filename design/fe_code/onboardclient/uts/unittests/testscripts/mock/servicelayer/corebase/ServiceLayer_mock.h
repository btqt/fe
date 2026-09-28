class MockServiceLayer {
  public:
    MOCK_METHOD0(createServiceLayerContext, Context*());
};

MockServiceLayer * M_ServiceLayer;

Context* ServiceLayer::createServiceLayerContext()
{
    return M_ServiceLayer->createServiceLayerContext();
}
