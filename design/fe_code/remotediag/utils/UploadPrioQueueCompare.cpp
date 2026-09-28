#include "UploadPrioQueueCompare.h"

namespace rdgapp {

bool UploadPrioQueueCompare::operator()(const android::sp<UploadTask> lhs, const android::sp<UploadTask> rhs) const
{
   bool res{false};
   if (lhs->getUploadPrio() == rhs->getUploadPrio()) {
      res = (lhs->getUploadId() < rhs->getUploadId()) ? true : false;
   }
   else{
      res = (lhs->getUploadPrio() < rhs->getUploadPrio()) ? true : false;
   }
   return res;
}
}
