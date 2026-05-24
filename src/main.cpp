#include <QApplication>
#include <QIcon>
#include "app/MainWindow.h"
#include "app/SingleInstanceService.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("MD Backpack Reader");
    app.setApplicationName("MD Backpack Reader");
    app.setWindowIcon(QIcon(":/icons/md-backpack-reader.ico"));

    const QString serverName = QStringLiteral("md-backpack-reader-single-instance");
    QStringList paths = QApplication::arguments().mid(1);

    if (SingleInstanceService::tryRouteToPrimary(serverName, paths))
        return 0;

    SingleInstanceService service(serverName);

    MainWindow window;
    QObject::connect(&service, &SingleInstanceService::filesRequested,
                     &window, &MainWindow::openDocumentsFromPaths);
    window.show();

    if (!paths.isEmpty())
        window.openDocumentsFromPaths(paths);

    return app.exec();
}
