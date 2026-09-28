#include "RemoteDiagDatastore.h"
#include "services/DiagManagerAdapter.h"

namespace rdgapp {
std::shared_ptr<RemoteDiagDatastore> RemoteDiagDatastore::instance {nullptr};
std::shared_ptr<RemoteDiagDatastore> RemoteDiagDatastore::getInstance()
{
    if (instance == nullptr) {
        instance = std::make_shared<RemoteDiagDatastore>();
    }
    return instance;
}

// void RemoteDiagDatastore::onCollectionConditionReceive(const android::sp<Buffer> buf)
// {
//     // parseCollectionCondition(buf);
//     //save to DID
//     constexpr uint16_t did {3501U};
//     DiagManagerAdapter::getInstance()->writeDidData(did, buf);
//     //call diag processes

// }

// void RemoteDiagDatastore::getCollectionCondition(const android::sp<Buffer> buf)
// {

// }

// void RemoteDiagDatastore::saveDataToFile(const android::sp<Buffer> buf)
// {

// }

// void RemoteDiagDatastore::parseCollectionCondition(const android::sp<::Buffer> buf)
// {

// }
}
