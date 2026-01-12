#include "QmlTypes.hpp"
#include "ProfileModel.hpp"
#include "GroupModel.hpp"
#include "MainWindowController.hpp"
#include "BasicSettingsController.hpp"
#include "EditProfileController.hpp"
#include <QtQml/qqml.h>
#include "db/Database.hpp"

void registerQmlTypes() {
    qmlRegisterType<ProfileModel>("Nekoray", 1, 0, "ProfileModel");
    qmlRegisterType<GroupModel>("Nekoray", 1, 0, "GroupModel");
    qmlRegisterType<MainWindowController>("Nekoray", 1, 0, "MainWindowController");
    qmlRegisterType<BasicSettingsController>("Nekoray", 1, 0, "BasicSettingsController");
    qmlRegisterType<EditProfileController>("Nekoray", 1, 0, "EditProfileController");
}
