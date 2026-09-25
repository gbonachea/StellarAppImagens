#include "mainwindow.h"
#include "appimagebuilder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QFont>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_builder(new AppImageBuilder(this))
{
    setupUi();
    checkAppimagetool();

    connect(m_builder, &AppImageBuilder::logMessage, this, [this](const QString &msg) {
        appendLog(msg, false);
    });
    connect(m_builder, &AppImageBuilder::logError, this, [this](const QString &msg) {
        appendLog(msg, true);
    });
    connect(m_builder, &AppImageBuilder::finished, this, [this](bool success) {
        m_btnGenerate->setEnabled(true);
        if (success) {
            appendLog("¡AppImage generado con éxito!");
            QMessageBox::information(this, "Éxito",
                "El AppImage se generó correctamente.\n"
                "Ubicación: " + m_outputDirEdit->text());
        } else {
            appendLog("Error al generar el AppImage.", true);
            QMessageBox::critical(this, "Error",
                "Ocurrió un error al generar el AppImage.\n"
                "Revise la consola de log para más detalles.");
        }
    });
    connect(m_builder, &AppImageBuilder::installFinished, this, [this](bool success) {
        m_btnInstallDeps->setEnabled(true);
        if (success) {
            appendLog("Dependencias instaladas correctamente.");
            QMessageBox::information(this, "Instalación completada",
                "Las dependencias se instalaron correctamente.\n"
                "appimagetool: " + AppImageBuilder::appimagetoolPath());
        } else {
            appendLog("Error al instalar dependencias.", true);
            QMessageBox::warning(this, "Error de instalación",
                "No se pudieron instalar todas las dependencias.\n"
                "Revise la consola de log para más detalles.");
        }
    });
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle("Stellar AppImagens - Generador de AppImage");
    resize(820, 700);
    setMinimumSize(700, 550);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createInfoTab(), "Información del AppImage");
    m_tabWidget->addTab(createFilesTab(), "Archivos y Librerías");
    m_tabWidget->addTab(createConfigTab(), "Configuración y Salida");
    mainLayout->addWidget(m_tabWidget, 1);

    QGroupBox *logGroup = new QGroupBox("Consola de Estado / Log");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

    m_logArea = new QTextEdit(this);
    m_logArea->setReadOnly(true);
    QFont monoFont("Monospace", 9);
    monoFont.setStyleHint(QFont::Monospace);
    m_logArea->setFont(monoFont);
    m_logArea->setStyleSheet("QTextEdit { border: 1px solid #555; }");
    m_logArea->setMinimumHeight(120);
    logLayout->addWidget(m_logArea);

    QHBoxLayout *actionLayout = new QHBoxLayout();

    m_btnInstallDeps = new QPushButton("Instalar Dependencias", this);
    m_btnInstallDeps->setMinimumHeight(40);
    QFont depFont = m_btnInstallDeps->font();
    depFont.setBold(true);
    depFont.setPointSize(10);
    m_btnInstallDeps->setFont(depFont);
    m_btnInstallDeps->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; border-radius: 4px; padding: 0 16px; }"
        "QPushButton:hover { background-color: #1976d2; }"
        "QPushButton:pressed { background-color: #0d47a1; }"
    );
    connect(m_btnInstallDeps, &QPushButton::clicked, this, &MainWindow::onInstallDependencies);
    actionLayout->addWidget(m_btnInstallDeps);

    m_btnGenerate = new QPushButton("Generar AppImage", this);
    m_btnGenerate->setMinimumHeight(40);
    QFont btnFont = m_btnGenerate->font();
    btnFont.setBold(true);
    btnFont.setPointSize(11);
    m_btnGenerate->setFont(btnFont);
    m_btnGenerate->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; border-radius: 4px; padding: 0 24px; }"
        "QPushButton:hover { background-color: #388e3c; }"
        "QPushButton:pressed { background-color: #1b5e20; }"
    );
    connect(m_btnGenerate, &QPushButton::clicked, this, &MainWindow::onGenerateAppImage);
    actionLayout->addWidget(m_btnGenerate);

    m_btnAbout = new QPushButton("?", this);
    m_btnAbout->setFixedSize(32, 32);
    m_btnAbout->setToolTip("Acerca de Stellar AppImagens");
    m_btnAbout->setStyleSheet(
        "QPushButton { background-color: #444; color: white; border-radius: 16px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #666; }"
    );
    connect(m_btnAbout, &QPushButton::clicked, this, &MainWindow::onAbout);
    actionLayout->addWidget(m_btnAbout);

    logLayout->addLayout(actionLayout);

    mainLayout->addWidget(logGroup);
}

QWidget *MainWindow::createInfoTab()
{
    QWidget *tab = new QWidget();
    QGridLayout *grid = new QGridLayout(tab);
    grid->setSpacing(10);

    grid->addWidget(new QLabel("Nombre de la aplicación:"), 0, 0);
    m_appNameEdit = new QLineEdit(tab);
    m_appNameEdit->setPlaceholderText("Mi Aplicación");
    grid->addWidget(m_appNameEdit, 0, 1);

    grid->addWidget(new QLabel("Versión:"), 1, 0);
    m_versionEdit = new QLineEdit(tab);
    m_versionEdit->setPlaceholderText("1.0.0");
    m_versionEdit->setText("1.0.0");
    grid->addWidget(m_versionEdit, 1, 1);

    grid->addWidget(new QLabel("Categoría:"), 2, 0);
    m_categoryCombo = new QComboBox(tab);
    m_categoryCombo->addItems({
        "Utility",
        "Development",
        "Graphics",
        "AudioVideo",
        "Audio",
        "Video",
        "Network",
        "Office",
        "Science",
        "Education",
        "Game",
        "System",
        "Settings",
        "FileManager",
        "TextEditor"
    });
    grid->addWidget(m_categoryCombo, 2, 1);

    grid->addWidget(new QLabel("Ejecutable principal:"), 3, 0);
    QHBoxLayout *execLayout = new QHBoxLayout();
    m_executablePath = new QLineEdit(tab);
    m_executablePath->setReadOnly(true);
    m_executablePath->setPlaceholderText("Seleccionar binario...");
    execLayout->addWidget(m_executablePath);
    QPushButton *btnSelectExec = new QPushButton("Examinar...", tab);
    connect(btnSelectExec, &QPushButton::clicked, this, &MainWindow::onSelectExecutable);
    execLayout->addWidget(btnSelectExec);
    grid->addLayout(execLayout, 3, 1);

    grid->addWidget(new QLabel("Icono de la aplicación:"), 4, 0);
    QHBoxLayout *iconLayout = new QHBoxLayout();
    m_iconPath = new QLineEdit(tab);
    m_iconPath->setReadOnly(true);
    m_iconPath->setPlaceholderText("Seleccionar icono (PNG/SVG)...");
    iconLayout->addWidget(m_iconPath);
    QPushButton *btnSelectIcon = new QPushButton("Examinar...", tab);
    connect(btnSelectIcon, &QPushButton::clicked, this, &MainWindow::onSelectIcon);
    iconLayout->addWidget(btnSelectIcon);
    grid->addLayout(iconLayout, 4, 1);

    grid->setRowStretch(5, 1);
    return tab;
}

QWidget *MainWindow::createFilesTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    m_fileTree = new QTreeWidget(tab);
    m_fileTree->setHeaderLabels({"Archivo / Carpeta", "Destino en AppDir"});
    m_fileTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_fileTree->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_fileTree, 1);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnAddFile = new QPushButton("Añadir Archivo", tab);
    m_btnAddFolder = new QPushButton("Añadir Carpeta", tab);
    m_btnRemove = new QPushButton("Eliminar", tab);

    m_btnAddFile->setStyleSheet("QPushButton { padding: 6px 12px; }");
    m_btnAddFolder->setStyleSheet("QPushButton { padding: 6px 12px; }");
    m_btnRemove->setStyleSheet("QPushButton { padding: 6px 12px; background-color: #b71c1c; color: white; }"
                               "QPushButton:hover { background-color: #c62828; }");

    connect(m_btnAddFile, &QPushButton::clicked, this, &MainWindow::onAddFile);
    connect(m_btnAddFolder, &QPushButton::clicked, this, &MainWindow::onAddFolder);
    connect(m_btnRemove, &QPushButton::clicked, this, &MainWindow::onRemoveItem);

    btnLayout->addWidget(m_btnAddFile);
    btnLayout->addWidget(m_btnAddFolder);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnRemove);
    layout->addLayout(btnLayout);

    return tab;
}

QWidget *MainWindow::createConfigTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QGroupBox *outGroup = new QGroupBox("Directorio de salida");
    QHBoxLayout *outLayout = new QHBoxLayout(outGroup);
    m_outputDirEdit = new QLineEdit(tab);
    m_outputDirEdit->setPlaceholderText("Seleccionar directorio de salida...");
    m_outputDirEdit->setText(QDir::homePath());
    outLayout->addWidget(m_outputDirEdit);
    m_btnSelectOutput = new QPushButton("Examinar...", tab);
    connect(m_btnSelectOutput, &QPushButton::clicked, this, &MainWindow::onSelectOutputDir);
    outLayout->addWidget(m_btnSelectOutput);
    layout->addWidget(outGroup);

    QGroupBox *apprunGroup = new QGroupBox("Script AppRun personalizado");
    QVBoxLayout *apprunLayout = new QVBoxLayout(apprunGroup);

    m_apprunEditor = new QTextEdit(tab);
    QFont monoFont("Monospace", 9);
    monoFont.setStyleHint(QFont::Monospace);
    m_apprunEditor->setFont(monoFont);
    m_apprunEditor->setStyleSheet("QTextEdit { border: 1px solid #555; }");
    m_apprunEditor->setPlainText(
        "#!/bin/bash\n"
        "HERE=\"$(dirname \"$(readlink -f \"$0\")\")\"\n"
        "export LD_LIBRARY_PATH=\"$HERE/usr/lib:$LD_LIBRARY_PATH\"\n"
        "exec \"$HERE/usr/bin/<NOMBRE_EJECUTABLE>\" \"$@\""
    );
    m_apprunEditor->setMinimumHeight(150);
    apprunLayout->addWidget(m_apprunEditor);
    layout->addWidget(apprunGroup, 1);

    return tab;
}

void MainWindow::checkAppimagetool()
{
    if (AppImageBuilder::isAppimagetoolAvailable()) {
        appendLog("appimagetool encontrado: " + AppImageBuilder::appimagetoolPath());
    } else {
        appendLog("ADVERTENCIA: appimagetool no se encontró en el sistema.", true);
        appendLog("Descárguelo desde: https://github.com/AppImage/AppImageKit/releases", false);
        QMessageBox::warning(this, "Herramienta no encontrada",
            "No se encontró 'appimagetool' en el PATH del sistema.\n\n"
            "Puede descargarlo desde:\n"
            "https://github.com/AppImage/AppImageKit/releases\n\n"
            "La generación de AppImage no funcionará sin esta herramienta.");
    }
}

void MainWindow::appendLog(const QString &msg, bool isError)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString color = isError ? "#ff6b6b" : "#a8e6a3";
    QString prefix = isError ? "[ERROR]" : "[INFO]";

    m_logArea->append(
        QString("<span style='color:#888'>[%1]</span> "
                "<span style='color:%2'>%3 %4</span>")
            .arg(timestamp, color, prefix, msg.toHtmlEscaped())
    );

    QTextCursor cursor = m_logArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logArea->setTextCursor(cursor);
}

void MainWindow::onSelectExecutable()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Seleccionar ejecutable principal", QDir::homePath(),
        "Ejecutables (*)");
    if (!path.isEmpty()) {
        m_executablePath->setText(path);
    }
}

void MainWindow::onSelectIcon()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Seleccionar icono", QDir::homePath(),
        "Imágenes PNG (*.png);;Imágenes SVG (*.svg);;Todos (*)");
    if (!path.isEmpty()) {
        m_iconPath->setText(path);
    }
}

void MainWindow::onSelectOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Seleccionar directorio de salida", QDir::homePath());
    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
    }
}

void MainWindow::onAddFile()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Seleccionar archivos a incluir", QDir::homePath());
    for (const QString &file : files) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_fileTree);
        item->setText(0, file);
        item->setText(1, "usr/lib/");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_fileTree->addTopLevelItem(item);
    }
}

void MainWindow::onAddFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Seleccionar carpeta a incluir", QDir::homePath());
    if (!dir.isEmpty()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_fileTree);
        item->setText(0, dir);
        item->setText(1, "usr/share/");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_fileTree->addTopLevelItem(item);
    }
}

void MainWindow::onRemoveItem()
{
    QTreeWidgetItem *item = m_fileTree->currentItem();
    if (item) {
        delete item;
    }
}

void MainWindow::onGenerateAppImage()
{
    if (m_appNameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Campo requerido", "Ingrese el nombre de la aplicación.");
        m_tabWidget->setCurrentIndex(0);
        return;
    }
    if (m_executablePath->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Campo requerido", "Seleccione el ejecutable principal.");
        m_tabWidget->setCurrentIndex(0);
        return;
    }
    if (m_outputDirEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Campo requerido", "Seleccione el directorio de salida.");
        m_tabWidget->setCurrentIndex(2);
        return;
    }
    if (!AppImageBuilder::isAppimagetoolAvailable()) {
        QMessageBox::critical(this, "Error",
            "appimagetool no está disponible.\n"
            "Descárguelo desde: https://github.com/AppImage/AppImageKit/releases");
        return;
    }

    AppImageConfig config;
    config.appName = m_appNameEdit->text().trimmed();
    config.version = m_versionEdit->text().trimmed();
    config.category = m_categoryCombo->currentText();
    config.executablePath = m_executablePath->text().trimmed();
    config.executableName = QFileInfo(config.executablePath).fileName();
    config.iconPath = m_iconPath->text().trimmed();
    config.outputDir = m_outputDirEdit->text().trimmed();
    config.apprunScript = m_apprunEditor->toPlainText();

    for (int i = 0; i < m_fileTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_fileTree->topLevelItem(i);
        QString src = item->text(0);
        QString dest = item->text(1);
        config.filesByDest[dest].append(src);
    }

    m_btnGenerate->setEnabled(false);
    m_logArea->clear();
    appendLog("Iniciando generación de AppImage...");
    appendLog("Aplicación: " + config.appName + " v" + config.version);

    m_builder->buildAppImage(config);
}

void MainWindow::onInstallDependencies()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Instalar dependencias",
        "Se descargará e instalará appimagetool desde GitHub releases.\n"
        "Se guardará en ~/.local/bin/.\n\n"
        "¿Continuar?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        appendLog("Instalación cancelada por el usuario.");
        return;
    }

    m_btnInstallDeps->setEnabled(false);
    appendLog("Iniciando instalación de dependencias...");
    m_builder->installDependencies();
}

void MainWindow::onAbout()
{
    QMessageBox about(this);
    about.setWindowTitle("Acerca de Stellar AppImagens");
    about.setIconPixmap(QIcon(":/icons/stellarappImagens_128.png").pixmap(64, 64));
    about.setText(
        "<h2>Stellar AppImagens</h2>"
        "<p><b>Versión 1.0.0</b></p>"
        "<p>Generador de paquetes AppImage para Linux.</p>"
        "<p>Permite convertir cualquier aplicación o binario en un paquete "
        ".AppImage portátil, incluyendo sus librerías, iconos y archivos "
        "necesarios para ejecutarse en cualquier distribución Linux sin "
        "necesidad de instalación.</p>"
        "<hr>"
        "<p style='color:gray; font-size:small;'>"
        "C++17 / Qt 6 | "
        "Requiere <a href='https://github.com/AppImage/AppImageKit/releases'>appimagetool</a>"
        "</p>"
    );
    about.exec();
}
