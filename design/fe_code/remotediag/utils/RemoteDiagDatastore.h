#ifndef REMOTEDIAG_REMOTEDIAGDATASTORE_H
#define REMOTEDIAG_REMOTEDIAGDATASTORE_H

#include "utils/Logger.h"

namespace rdgapp {

class RemoteDiagDatastore
{
public:
    RemoteDiagDatastore() = default;
    ~RemoteDiagDatastore() = default;

    RemoteDiagDatastore(const RemoteDiagDatastore& ) = delete;
    RemoteDiagDatastore& operator=(const RemoteDiagDatastore& ) = delete;

    RemoteDiagDatastore(RemoteDiagDatastore&& ) = delete;
    RemoteDiagDatastore& operator=(RemoteDiagDatastore&& ) = delete;

    static std::shared_ptr<RemoteDiagDatastore> getInstance();

    // collection condition
    // void onCollectionConditionReceive(const android::sp<Buffer> buf);
    // void getCollectionCondition(const android::sp<Buffer> buf);
    // data store
    // void saveDataToFile(const android::sp<Buffer> buf);
private:
    static std::shared_ptr<RemoteDiagDatastore> instance;

    // void parseCollectionCondition(const android::sp<::Buffer> buf);
};
}
#endif /* REMOTEDIAG_REMOTEDIAGDATASTORE_H */
