#include "mainwindow.h"

#include "singleapplicationmanager.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QUrl>
#include <QDebug>
#include <QTranslator>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    QString qmDir;
#ifdef _WIN32
    qmDir = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("translations");
#else
    qmDir = QT_STRINGIFY(QM_FILE_INSTALL_DIR);
#endif
    translator.load(QString("pineapple-music_%1").arg(QLocale::system().name()), qmDir);
    a.installTranslator(&translator);

    // parse commandline arguments
    QCommandLineParser parser;
    parser.addPositionalArgument("File list", QCoreApplication::translate("main", "File list."));
    parser.addHelpOption();

    parser.process(a);

    QStringList urlStrList = parser.positionalArguments();

    SingleApplicationManager sam("_pineapple_music_owo_");
    if (sam.checkSingleInstance(QVariant::fromValue(urlStrList))) {
        return 0;
    } else {
        sam.createSingleInstance();
    }

    MainWindow w;
    w.show();

    if (!urlStrList.isEmpty()) {
        w.commandlinePlayAudioFiles(urlStrList);
    }

    QObject::connect(&sam, &SingleApplicationManager::dataReached, &w, &MainWindow::localSocketPlayAudioFiles);

    return a.exec();
}
