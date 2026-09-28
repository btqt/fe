#ifndef REMOTEDIAG_DATA_MODEL_H
#define REMOTEDIAG_DATA_MODEL_H

#include "ParamsDef.h"
#include <memory>
#include <utils/RefBase.h>
#include "FileUtil.h"
#include "services/HttpManagerAdapter.h"

namespace rdgapp {

template<class  T>
class DataModel {
    public:
        static error_t save(const std::string& name, const T& aT) noexcept;
        static auto get(const std::string& name) -> std::shared_ptr<T>;
        static error_t clear(const std::string& name) noexcept;
        static bool checkExist(const std::string& name) noexcept;
};
}
#endif // REMOTEDIAG_DATA_MODEL_H
