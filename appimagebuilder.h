#ifndef APPIMAGEBUILDER_H
#define APPIMAGEBUILDER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QMap>

struct AppImageConfig {
    QString appName;
    QString version;
    QString category;
    QString executablePath;
    QString executableName;
    QString iconPath;
    QString outputDir;
    QString apprunScript;
    QMap<QString,QStringList> filesByDest;
};

class AppImageBuilder : public QObject
{
    Q_OBJECT

public:
    explicit AppImageBuilder(QObject *parent = nullptr);
    ~AppImageBuilder();

    static bool isAppimagetoolAvailable();
    static QString appimagetoolPath();

    bool buildAppImage(const AppImageConfig &config);
    bool installDependencies();

signals:
    void logMessage(const QString &msg);
    void logError(const QString &msg);
    void finished(bool success);
    void installFinished(bool success);

private slots:
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onInstallProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool createAppDirStructure(const AppImageConfig &config, const QString &appDirPath);
    bool copyExecutable(const AppImageConfig &config, const QString &appDirPath);
    bool copyIncludedFiles(const AppImageConfig &config, const QString &appDirPath);
    bool copyIcon(const AppImageConfig &config, const QString &appDirPath);
    bool generateDesktopFile(const AppImageConfig &config, const QString &appDirPath);
    bool generateAppRun(const AppImageConfig &config, const QString &appDirPath);
    bool runAppimagetool(const AppImageConfig &config, const QString &appDirPath);
    void cleanupTempDir(const QString &dirPath);

    bool detectPackageManager(QString &cmd, QStringList &args);
    QString detectDownloader(QString &cmd);
    bool downloadAppimagetoolWith(const QString &tool, const QString &url, const QString &dest);
    void setExecutableRecursive(const QString &path);

    QProcess *m_process;
    QProcess *m_installProcess;
    QString m_tempDir;
};

#endif // APPIMAGEBUILDER_H
