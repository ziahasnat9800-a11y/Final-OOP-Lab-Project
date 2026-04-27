#include <QApplication>
#include <QSqlDatabase>
#include <QMessageBox>
#include "database.h"
#include "loginwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SmartHome");
    app.setOrganizationName("OOPLab");

    // Add SQLite driver
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        QMessageBox::critical(nullptr, "Error", "SQLite driver not available!");
        return 1;
    }

    if (!Database::instance().initialize()) {
        QMessageBox::critical(nullptr, "Database Error",
                              "Failed to initialize database.");
        return 1;
    }

    LoginWindow loginWindow;
    loginWindow.show();

    return app.exec();
}