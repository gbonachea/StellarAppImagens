#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QTreeWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>


class AppImageBuilder;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSelectExecutable();
    void onSelectIcon();
    void onSelectOutputDir();
    void onAddFile();
    void onAddFolder();
    void onRemoveItem();
    void onGenerateAppImage();
    void onInstallDependencies();
    void onAbout();


private:
    void setupUi();
    QWidget *createInfoTab();
    QWidget *createFilesTab();
    QWidget *createConfigTab();
    void checkAppimagetool();
    void appendLog(const QString &msg, bool isError = false);

    QTabWidget *m_tabWidget;

    // Info tab
    QLineEdit *m_appNameEdit;
    QLineEdit *m_versionEdit;
    QComboBox *m_categoryCombo;
    QLineEdit *m_executablePath;
    QLineEdit *m_iconPath;

    // Files tab
    QTreeWidget *m_fileTree;
    QPushButton *m_btnAddFile;
    QPushButton *m_btnAddFolder;
    QPushButton *m_btnRemove;

    // Config tab
    QLineEdit *m_outputDirEdit;
    QTextEdit *m_apprunEditor;
    QPushButton *m_btnSelectOutput;

    // Log & generate
    QTextEdit *m_logArea;
    QPushButton *m_btnGenerate;
    QPushButton *m_btnInstallDeps;
    QPushButton *m_btnAbout;

    AppImageBuilder *m_builder;
};

#endif // MAINWINDOW_H
