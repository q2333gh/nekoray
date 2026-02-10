#pragma once

#include <QJsonObject>
#include <QJsonArray>

#include "main/NekoGui.hpp"

namespace NekoGui_fmt {
    struct CoreObjOutboundBuildResult {
    public:
        QJsonObject outbound;
        QString error;
    };

    struct ExternalBuildResult {
    public:
        QString program;
        QStringList env;
        QStringList arguments;
        //
        QString tag;
        //
        QString error;
        QString config_export;
    };

    class AbstractBean : public JsonStore {
    public:
        // Sentinel version indicating an invalid/unknown bean type
        static constexpr int INVALID_BEAN_VERSION = -114514;

        int version;

        QString name = "";
        QString serverAddress = "127.0.0.1";
        int serverPort = 1080;

        QString custom_config = "";
        QString custom_outbound = "";

        explicit AbstractBean(int version);

        //

        QString ToNekorayShareLink(const QString &type);

        void ResolveDomainToIP(const std::function<void()> &onFinished);

        //

        [[nodiscard]] virtual QString DisplayAddress();

        [[nodiscard]] virtual QString DisplayName();

        virtual QString DisplayCoreType() { return software_core_name; };

        virtual QString DisplayType() { return {}; };

        virtual QString DisplayTypeAndName();

        //

        virtual int NeedExternal(bool isFirstProfile) { return 0; };

        virtual CoreObjOutboundBuildResult BuildCoreObjSingBox() { return {}; };

        virtual ExternalBuildResult BuildExternal(int mapping_port, int socks_port, int external_stat) { return {}; };

        virtual QString ToShareLink() { return {}; };
    };

} // namespace NekoGui_fmt
