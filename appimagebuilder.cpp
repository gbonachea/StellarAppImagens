#include "appimagebuilder.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTemporaryDir>
#include <QDateTime>
#include <QProcess>

AppImageBuilder::AppImageBuilder(QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_installProcess(nullptr)
{
}

AppImageBuilder::~AppImageBuilder()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        delete m_process;
    }
    if (m_installProcess) {
        m_installProcess->kill();
        m_installProcess->waitForFinished(3000);
        delete m_installProcess;
    }
}

bool AppImageBuilder::isAppimagetoolAvailable()
{
    return !appimagetoolPath().isEmpty();
}

QString AppImageBuilder::appimagetoolPath()
{
    QString path = QStandardPaths::findExecutable("appimagetool");
    if (!path.isEmpty())
        return path;

    QStringList searchPaths = {
        "/usr/bin/appimagetool",
        "/usr/local/bin/appimagetool",
        QDir::homePath() + "/bin/appimagetool",
        QDir::homePath() + "/.local/bin/appimagetool"
    };
    for (const QString &p : searchPaths) {
        if (QFileInfo::exists(p) && QFileInfo(p).isExecutable())
            return p;
    }
    return {};
}

void AppImageBuilder::onProcessReadyReadStandardOutput()
{
    if (m_process) {
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            emit logMessage(line.trimmed());
        }
    }
}

void AppImageBuilder::onProcessReadyReadStandardError()
{
    if (m_process) {
        QString output = QString::fromUtf8(m_process->readAllStandardError());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            emit logMessage(line.trimmed());
        }
    }
}

void AppImageBuilder::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit logMessage("Proceso finalizado con éxito (código: 0).");
        emit finished(true);
    } else {
        emit logError(QString("Proceso finalizado con error (código: %1).").arg(exitCode));
        emit finished(false);
    }

    cleanupTempDir(m_tempDir);
}

bool AppImageBuilder::buildAppImage(const AppImageConfig &config)
{
    QTemporaryDir tempDir("/tmp/StellarAppImagens_XXXXXX");
    if (!tempDir.isValid()) {
        emit logError("No se pudo crear directorio temporal.");
        emit finished(false);
        return false;
    }
    tempDir.setAutoRemove(false);
    m_tempDir = tempDir.path();
    emit logMessage("Directorio temporal: " + m_tempDir);

    QString appDirPath = m_tempDir + "/AppDir";
    QDir appDir(appDirPath);

    if (!createAppDirStructure(config, appDirPath)) {
        cleanupTempDir(m_tempDir);
        emit finished(false);
        return false;
    }
    if (!copyExecutable(config, appDirPath)) {
        cleanupTempDir(m_tempDir);
        emit finished(false);
        return false;
    }
    if (!copyIcon(config, appDirPath)) {
        cleanupTempDir(m_tempDir);
        emit finished(false);
        return false;
    }
    if (!generateDesktopFile(config, appDirPath)) {
        cleanupTempDir(m_tempDir);
        emit finished(false);
        return false;
    }
    if (!generateAppRun(config, appDirPath)) {
        cleanupTempDir(m_tempDir);
        emit finished(false);
        return false;
    }
    if (!copyIncludedFiles(config, appDirPath)) {
        cleanupTempDir(m_tempDir);
        emit finished(false);
        return false;
    }

    emit logMessage("Estructura AppDir creada correctamente.");
    emit logMessage("Invocando appimagetool...");

    return runAppimagetool(config, appDirPath);
}

bool AppImageBuilder::createAppDirStructure(const AppImageConfig &config, const QString &appDirPath)
{
    Q_UNUSED(config)
    QDir dir;

    QStringList subdirs = {
        "usr/bin",
        "usr/lib",
        "usr/share/icons/hicolor/256x256/apps",
        "usr/share/applications"
    };

    for (const QString &sub : subdirs) {
        if (!dir.mkpath(appDirPath + "/" + sub)) {
            emit logError("No se pudo crear directorio: " + sub);
            return false;
        }
    }
    emit logMessage("Estructura de directorios AppDir creada.");
    return true;
}

bool AppImageBuilder::copyExecutable(const AppImageConfig &config, const QString &appDirPath)
{
    QString dest = appDirPath + "/usr/bin/" + config.executableName;
    if (!QFile::copy(config.executablePath, dest)) {
        emit logError("No se pudo copiar el ejecutable: " + config.executablePath);
        return false;
    }

    QFile file(dest);
    file.setPermissions(file.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
    emit logMessage("Ejecutable copiado: " + config.executableName);
    return true;
}

bool AppImageBuilder::copyIncludedFiles(const AppImageConfig &config, const QString &appDirPath)
{
    for (auto it = config.filesByDest.constBegin(); it != config.filesByDest.constEnd(); ++it) {
        QString destDir = appDirPath + "/" + it.key();
        QDir().mkpath(destDir);

        for (const QString &srcPath : it.value()) {
            QFileInfo srcInfo(srcPath);
            if (srcInfo.isDir()) {
                QString destPath = destDir + "/" + srcInfo.fileName();
                QDir srcDir(srcPath);

                if (!QDir().mkpath(destPath)) {
                    emit logError("No se pudo crear directorio: " + destPath);
                    return false;
                }

                const QStringList entries = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString &entry : entries) {
                    QString srcEntry = srcPath + "/" + entry;
                    QString dstEntry = destPath + "/" + entry;
                    if (QFileInfo(srcEntry).isFile()) {
                        if (!QFile::copy(srcEntry, dstEntry)) {
                            emit logError("No se pudo copiar: " + srcEntry);
                            return false;
                        }
                    }
                }
                emit logMessage("Carpeta copiada: " + srcInfo.fileName() + " -> " + it.key());
            } else {
                QString destFile = destDir + "/" + srcInfo.fileName();
                if (!QFile::copy(srcPath, destFile)) {
                    emit logError("No se pudo copiar archivo: " + srcPath);
                    return false;
                }
                emit logMessage("Archivo copiado: " + srcInfo.fileName() + " -> " + it.key());
            }
        }
    }
    return true;
}

bool AppImageBuilder::copyIcon(const AppImageConfig &config, const QString &appDirPath)
{
    if (config.iconPath.isEmpty()) {
        emit logMessage("No se especificó icono, se omite.");
        return true;
    }

    QFileInfo iconInfo(config.iconPath);
    QString iconFileName = config.appName.toLower().replace(" ", "_") + "." + iconInfo.completeSuffix();

    QString destRoot = appDirPath + "/" + iconFileName;
    QString destHicolor = appDirPath + "/usr/share/icons/hicolor/256x256/apps/" + iconFileName;

    if (!QFile::copy(config.iconPath, destRoot)) {
        emit logError("No se pudo copiar icono a raíz del AppDir.");
        return false;
    }
    if (!QFile::copy(config.iconPath, destHicolor)) {
        emit logError("No se pudo copiar icono a hicolor.");
        return false;
    }

    emit logMessage("Icono copiado: " + iconFileName);
    return true;
}

bool AppImageBuilder::generateDesktopFile(const AppImageConfig &config, const QString &appDirPath)
{
    QString desktopPath = appDirPath + "/usr/share/applications/" +
        config.appName.toLower().replace(" ", "_") + ".desktop";

    QFile file(desktopPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logError("No se pudo crear archivo .desktop.");
        return false;
    }

    QString iconRef = config.appName.toLower().replace(" ", "_");

    QTextStream out(&file);
    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Name=" << config.appName << "\n";
    out << "Exec=" << config.executableName << " %F\n";
    out << "Icon=" << iconRef << "\n";
    out << "Categories=" << config.category << ";\n";
    out << "Terminal=false\n";
    out << "StartupNotify=true\n";
    out << "Comment=" << config.appName << " v" << config.version << "\n";
    file.close();

    QString rootDesktop = appDirPath + "/" + config.appName.toLower().replace(" ", "_") + ".desktop";
    if (!QFile::copy(desktopPath, rootDesktop)) {
        emit logError("No se pudo copiar .desktop a la raíz del AppDir.");
        return false;
    }

    emit logMessage("Archivo .desktop generado.");
    return true;
}

bool AppImageBuilder::generateAppRun(const AppImageConfig &config, const QString &appDirPath)
{
    QString appRunPath = appDirPath + "/AppRun";
    QFile file(appRunPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logError("No se pudo crear script AppRun.");
        return false;
    }

    QString script = config.apprunScript;
    if (script.contains("<NOMBRE_EJECUTABLE>")) {
        script.replace("<NOMBRE_EJECUTABLE>", config.executableName);
    }

    QTextStream out(&file);
    out << script << "\n";
    file.close();

    QFile::Permissions perms = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                               QFile::ReadGroup | QFile::ExeGroup |
                               QFile::ReadOther | QFile::ExeOther;
    if (!file.setPermissions(perms)) {
        emit logError("No se pudieron asignar permisos de ejecución a AppRun.");
        return false;
    }

    emit logMessage("Script AppRun generado con permisos de ejecución.");
    return true;
}

bool AppImageBuilder::runAppimagetool(const AppImageConfig &config, const QString &appDirPath)
{
    QString appimageTool = appimagetoolPath();
    if (appimageTool.isEmpty()) {
        emit logError("appimagetool no encontrado.");
        emit finished(false);
        return false;
    }

    QString appImageName = config.appName.toLower().replace(" ", "-") +
        "-" + config.version + ".AppImage";
    QString outputPath = config.outputDir + "/" + appImageName;

    QDir().mkpath(config.outputDir);

    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &AppImageBuilder::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &AppImageBuilder::onProcessReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AppImageBuilder::onProcessFinished);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("ARCH", "x86_64");
    m_process->setProcessEnvironment(env);

    emit logMessage("Ejecutando: " + appimageTool + " " + appDirPath + " " + outputPath);

    m_process->start(appimageTool, { appDirPath, outputPath });

    if (!m_process->waitForStarted(5000)) {
        emit logError("No se pudo iniciar appimagetool.");
        emit finished(false);
        return false;
    }

    return true;
}

void AppImageBuilder::cleanupTempDir(const QString &dirPath)
{
    if (dirPath.isEmpty())
        return;

    QDir dir(dirPath);
    if (dir.exists()) {
        dir.removeRecursively();
        emit logMessage("Directorio temporal eliminado: " + dirPath);
    }
}

bool AppImageBuilder::detectPackageManager(QString &cmd, QStringList &args)
{
    QStringList candidates = {
        "apt", "dnf", "pacman", "zypper", "xbps-install", "apk"
    };

    for (const QString &candidate : candidates) {
        QString path = QStandardPaths::findExecutable(candidate);
        if (!path.isEmpty()) {
            cmd = path;
            if (candidate == "apt") {
                args = {"install", "-y"};
            } else if (candidate == "dnf") {
                args = {"install", "-y"};
            } else if (candidate == "pacman") {
                args = {"-S", "--noconfirm"};
            } else if (candidate == "zypper") {
                args = {"install", "-y"};
            } else if (candidate == "xbps-install") {
                args = {"-y"};
            } else if (candidate == "apk") {
                args = {"add"};
            }
            return true;
        }
    }
    return false;
}

QString AppImageBuilder::detectDownloader(QString &cmd)
{
    QStringList candidates = {"curl", "wget"};
    for (const QString &c : candidates) {
        QString path = QStandardPaths::findExecutable(c);
        if (!path.isEmpty()) {
            cmd = path;
            return c;
        }
    }
    return {};
}

bool AppImageBuilder::downloadAppimagetoolWith(const QString &tool, const QString &url, const QString &dest)
{
    QStringList args;
    if (tool.contains("curl")) {
        args << "-L" << "-o" << dest << url;
    } else if (tool.contains("wget")) {
        args << "-O" << dest << url;
    } else {
        return false;
    }

    QProcess proc;
    proc.start(tool, args);
    if (!proc.waitForStarted(10000))
        return false;
    if (!proc.waitForFinished(120000))
        return false;

    return proc.exitCode() == 0 && QFileInfo::exists(dest);
}

void AppImageBuilder::setExecutableRecursive(const QString &path)
{
    QFileInfo info(path);
    if (info.isDir()) {
        QDir dir(path);
        const QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            setExecutableRecursive(path + "/" + entry);
        }
    } else if (info.isFile()) {
        QFile f(path);
        f.setPermissions(f.permissions() |
            QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
    }
}

void AppImageBuilder::onInstallProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    if (exitCode != 0) {
        emit logError(QString("Proceso de instalación terminó con código: %1").arg(exitCode));
        emit installFinished(false);
        return;
    }

    if (isAppimagetoolAvailable()) {
        emit logMessage("appimagetool instalado correctamente: " + appimagetoolPath());
        emit installFinished(true);
    } else {
        emit logError("La instalación terminó pero appimagetool no se encontró.");
        emit installFinished(false);
    }
}

bool AppImageBuilder::installDependencies()
{
    emit logMessage("=== Instalación de dependencias ===");

    QString installCmd;
    QStringList installArgs;
    bool hasPkgMgr = detectPackageManager(installCmd, installArgs);

    if (isAppimagetoolAvailable()) {
        emit logMessage("appimagetool ya está instalado: " + appimagetoolPath());
        emit installFinished(true);
        return true;
    }

    QString destDir = QDir::homePath() + "/.local/bin";
    QDir().mkpath(destDir);
    QString destFile = destDir + "/appimagetool";

    QString downloader;
    QString dlTool = detectDownloader(downloader);

    if (dlTool.isEmpty()) {
        emit logError("No se encontró curl ni wget para descargar.");

        if (hasPkgMgr) {
            emit logMessage("Intentando instalar curl con " + installCmd + "...");
            if (m_installProcess) {
                m_installProcess->kill();
                m_installProcess->waitForFinished(2000);
                delete m_installProcess;
            }
            m_installProcess = new QProcess(this);
            connect(m_installProcess,
                    QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, &AppImageBuilder::onInstallProcessFinished);
            connect(m_installProcess, &QProcess::readyReadStandardOutput, this, [this]() {
                QString out = QString::fromUtf8(m_installProcess->readAllStandardOutput());
                for (const QString &line : out.split('\n', Qt::SkipEmptyParts))
                    emit logMessage(line.trimmed());
            });
            connect(m_installProcess, &QProcess::readyReadStandardError, this, [this]() {
                QString out = QString::fromUtf8(m_installProcess->readAllStandardError());
                for (const QString &line : out.split('\n', Qt::SkipEmptyParts))
                    emit logError(line.trimmed());
            });

            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            m_installProcess->setProcessEnvironment(env);
            m_installProcess->start(installCmd, installArgs + QStringList{"curl"});
            if (!m_installProcess->waitForStarted(5000)) {
                emit logError("No se pudo iniciar la instalación de curl.");
                emit installFinished(false);
                return false;
            }
            return true;
        }

        emit installFinished(false);
        return false;
    }

    emit logMessage("Usando " + dlTool + " para descargar appimagetool...");
    emit logMessage("Descargando desde GitHub releases...");

    QString url = "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage";

    if (!downloadAppimagetoolWith(downloader, url, destFile)) {
        emit logError("Error al descargar appimagetool.");
        emit installFinished(false);
        return false;
    }

    emit logMessage("Descarga completada. Asignando permisos de ejecución...");
    setExecutableRecursive(destFile);

    QString appRun = destFile + ".sh";
    QFile appRunFile(appRun);
    if (appRunFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&appRunFile);
        ts << "#!/bin/bash\n";
        ts << "exec \"" << destFile << "\" \"$@\"\n";
        appRunFile.close();
        QFile::setPermissions(appRun,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
            QFileDevice::ReadOther | QFileDevice::ExeOther);
    }

    if (!isAppimagetoolAvailable()) {
        emit logMessage("Verificando que ~/.local/bin esté en PATH...");
        emit logMessage("Si appimagetool no se detecta, reinicie la sesión o ejecute:");
        emit logMessage("  export PATH=\"$HOME/.local/bin:$PATH\"");
    }

    if (isAppimagetoolAvailable()) {
        emit logMessage("appimagetool instalado correctamente: " + appimagetoolPath());
        emit installFinished(true);
        return true;
    }

    emit logMessage("appimagetool descargado en: " + destFile);
    emit logMessage("Puede agregar ~/.local/bin a su PATH para que se detecte automáticamente.");
    emit installFinished(true);
    return true;
}
