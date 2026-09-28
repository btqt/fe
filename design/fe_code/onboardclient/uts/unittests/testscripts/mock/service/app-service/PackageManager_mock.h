class MockPackageManager {
 public:
  // The following line won't really compile, as the return
  // type has multiple template arguments.  To fix it, use a
  // typedef for the return type.
  MOCK_METHOD0(getPackages, std::map<std::string, Pkg>());
  MOCK_METHOD1(getPackage, const Pkg&(const std::string name));
  MOCK_METHOD0(init, error_t());
  MOCK_METHOD0(printPackages, void());
  MOCK_METHOD1(exists, bool(const std::string name));
  MOCK_METHOD1(parse, error_t(const char* file));
  MOCK_METHOD1(parseByModernCpp, error_t(const char* file));
  MOCK_METHOD2(setAppModes, void(Pkg &p, const std::string app));
  MOCK_METHOD1(parseByJsonC, error_t(const char* file));
  MOCK_METHOD2(parseHmis, error_t(struct json_object* services, Pkg& p));
  MOCK_METHOD2(parsePosts, error_t(struct json_object* services, Pkg& p));
  MOCK_METHOD2(loadFromManifestFile, error_t(const char* file, char* outBuf));
};



MockPackageManager * M_PackageManager;


  // The following line won't really compile, as the return
  // type has multiple template arguments.  To fix it, use a
  // typedef for the return type.
std::map<std::string, Pkg> PackageManager::getPackages()
{
  return M_PackageManager->getPackages();
}

const Pkg& PackageManager::getPackage(const std::string name)
{
  return M_PackageManager->getPackage(name);
}

error_t PackageManager::init()
{
  return M_PackageManager->init();
}

void PackageManager::printPackages()
{
  M_PackageManager->printPackages();
}

bool PackageManager::exists(const std::string name)
{
  return M_PackageManager->exists(name);
}

error_t PackageManager::parse(const char* file)
{
  return M_PackageManager->parse(file);
}

error_t PackageManager::parseByModernCpp(const char* file)
{
  return M_PackageManager->parseByModernCpp(file);
}

void PackageManager::setAppModes(Pkg &p, const std::string app)
{
  M_PackageManager->setAppModes(p, app);
}

error_t PackageManager::parseByJsonC(const char* file)
{
  return M_PackageManager->parseByJsonC(file);
}

error_t PackageManager::parseHmis(struct json_object* services, Pkg& p)
{
  return M_PackageManager->parseHmis(services, p);
}

error_t PackageManager::parsePosts(struct json_object* services, Pkg& p)
{
  return M_PackageManager->parsePosts(services, p);
}

error_t PackageManager::loadFromManifestFile(const char* file, char* outBuf)
{
  return M_PackageManager->loadFromManifestFile(file, outBuf);
}
