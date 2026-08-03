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

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    typedef HRESULT(WINAPI *SetCurrentProcessExplicitAppUserModelIDProc)(PCWSTR);
    HMODULE shell32 = LoadLibraryW(L"shell32.dll");
    if (shell32) {
        auto proc = reinterpret_cast<SetCurrentProcessExplicitAppUserModelIDProc>(
            GetProcAddress(shell32, "SetCurrentProcessExplicitAppUserModelID"));
        if (proc) {
            proc(L"net.blumia.pineapple-music");
        }
        FreeLibrary(shell32);
    }
#endif

    QApplication a(argc, argv);

    QTranslator translator;
    if (translator.load(QLocale(), QLatin1String("pineapple-music"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        a.installTranslator(&translator);
    }

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
