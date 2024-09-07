// SPDX-FileCopyrightText: 2024 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

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
    if (translator.load(QLocale(), QLatin1String("pineapple-music"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        a.installTranslator(&translator);
    }
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
