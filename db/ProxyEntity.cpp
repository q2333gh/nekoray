#include "db/ProxyEntity.hpp"

#include "fmt/ChainBean.hpp"
#include "fmt/CustomBean.hpp"
#include "fmt/NaiveBean.hpp"
#include "fmt/QUICBean.hpp"
#include "fmt/ShadowSocksBean.hpp"
#include "fmt/SocksHttpBean.hpp"
#include "fmt/TrojanVLESSBean.hpp"
#include "fmt/VMessBean.hpp"

namespace NekoGui {

NekoGui_fmt::ChainBean *ProxyEntity::ChainBean() const {
    return static_cast<NekoGui_fmt::ChainBean *>(bean.get());
}

NekoGui_fmt::SocksHttpBean *ProxyEntity::SocksHTTPBean() const {
    return static_cast<NekoGui_fmt::SocksHttpBean *>(bean.get());
}

NekoGui_fmt::ShadowSocksBean *ProxyEntity::ShadowSocksBean() const {
    return static_cast<NekoGui_fmt::ShadowSocksBean *>(bean.get());
}

NekoGui_fmt::VMessBean *ProxyEntity::VMessBean() const {
    return static_cast<NekoGui_fmt::VMessBean *>(bean.get());
}

NekoGui_fmt::TrojanVLESSBean *ProxyEntity::TrojanVLESSBean() const {
    return static_cast<NekoGui_fmt::TrojanVLESSBean *>(bean.get());
}

NekoGui_fmt::NaiveBean *ProxyEntity::NaiveBean() const {
    return static_cast<NekoGui_fmt::NaiveBean *>(bean.get());
}

NekoGui_fmt::QUICBean *ProxyEntity::QUICBean() const {
    return static_cast<NekoGui_fmt::QUICBean *>(bean.get());
}

NekoGui_fmt::CustomBean *ProxyEntity::CustomBean() const {
    return static_cast<NekoGui_fmt::CustomBean *>(bean.get());
}

} // namespace NekoGui

