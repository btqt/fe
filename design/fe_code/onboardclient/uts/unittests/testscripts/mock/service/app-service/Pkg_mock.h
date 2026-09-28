class Mockservice_t {
};

class MockPkg {
 public:
  MOCK_CONST_METHOD0(toString, void());
};



MockPkg * M_Pkg;


Pkg::Pkg()
{

}

void Pkg::toString() const
{
  M_Pkg->toString();
}
