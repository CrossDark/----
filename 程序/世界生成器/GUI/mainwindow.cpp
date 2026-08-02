#include "mainwindow.h"
#include "worldgen_core.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

#include <QImageReader>
#include <QPixmap>

/* 核心日志回调:把 worldgen_core 的消息队列投递到主线程显示 */
static MainWindow *g_win = nullptr;

static void coreLog(const char *msg)
{
    if (g_win) {
        const QString s = QString::fromUtf8(msg);
        QMetaObject::invokeMethod(g_win, [s]() {
            g_win->postLog(s.trimmed());
        }, Qt::QueuedConnection);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("地狱之下 - 地图生成器"));

    /* ---------- 左侧:参数面板 ---------- */
    auto *paramBox = new QGroupBox(QStringLiteral("参数"), this);
    auto *form = new QFormLayout(paramBox);

    m_seed = new QSpinBox(paramBox);
    m_seed->setRange(0, 2000000000);
    m_seed->setValue(7);
    m_seed->setSpecialValueText(QStringLiteral("随机(0)"));

    m_faults = new QSpinBox(paramBox);
    m_faults->setRange(0, 5000);
    m_faults->setValue(0);
    m_faults->setSpecialValueText(QStringLiteral("自动(0)"));

    m_water = new QSpinBox(paramBox);
    m_water->setRange(0, 100);
    m_water->setValue(65);
    m_water->setSuffix(QStringLiteral(" %"));

    m_width = new QSpinBox(paramBox);
    m_width->setRange(128, 8192);
    m_width->setSingleStep(256);
    m_width->setValue(2560);

    m_height = new QSpinBox(paramBox);
    m_height->setRange(64, 4096);
    m_height->setSingleStep(128);
    m_height->setValue(1440);

    m_lineWidth = new QSpinBox(paramBox);
    m_lineWidth->setRange(1, 20);
    m_lineWidth->setValue(3);

    m_slices = new QSpinBox(paramBox);
    m_slices->setRange(0, 99);
    m_slices->setValue(0);
    m_slices->setSpecialValueText(QStringLiteral("关闭(0)"));

    m_grid = new QCheckBox(QStringLiteral("经纬网格"), paramBox);
    m_fill = new QCheckBox(QStringLiteral("分层设色"), paramBox);

    m_output = new QLineEdit(paramBox);
    m_output->setText(QStringLiteral("图片/地图_生成.png"));
    auto *browseBtn = new QPushButton(QStringLiteral("浏览…"), paramBox);
    connect(browseBtn, &QPushButton::clicked, this, &MainWindow::browseOutput);
    auto *outRow = new QHBoxLayout;
    outRow->addWidget(m_output, 1);
    outRow->addWidget(browseBtn);

    form->addRow(QStringLiteral("种子"), m_seed);
    form->addRow(QStringLiteral("故障次数"), m_faults);
    form->addRow(QStringLiteral("水占比"), m_water);
    form->addRow(QStringLiteral("宽度"), m_width);
    form->addRow(QStringLiteral("高度"), m_height);
    form->addRow(QStringLiteral("线宽"), m_lineWidth);
    form->addRow(QStringLiteral("等高线切片"), m_slices);
    form->addRow(m_grid);
    form->addRow(m_fill);
    form->addRow(QStringLiteral("输出路径"), outRow);

    auto *genBtn = new QPushButton(QStringLiteral("生成地图"), paramBox);
    connect(genBtn, &QPushButton::clicked, this, &MainWindow::generate);

    auto *left = new QVBoxLayout;
    left->addWidget(paramBox);
    left->addWidget(genBtn);
    left->addStretch(1);

    /* ---------- 右侧:地图预览 ---------- */
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    m_mapLabel = new QLabel(scroll);
    m_mapLabel->setAlignment(Qt::AlignCenter);
    m_mapLabel->setText(QStringLiteral("尚未生成"));
    m_mapLabel->setMinimumSize(400, 300);
    scroll->setWidget(m_mapLabel);

    /* ---------- 底部:日志 ---------- */
    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);

    auto *central = new QWidget(this);
    auto *mainLayout = new QHBoxLayout;      /* 无父,交由 root 管理 */
    mainLayout->addLayout(left);
    mainLayout->addWidget(scroll, 1);

    auto *root = new QVBoxLayout(central);
    root->addLayout(mainLayout, 1);
    root->addWidget(m_log, 0);

    setCentralWidget(central);
    resize(1100, 760);

    /* ---------- 异步生成(worldgen 核心内嵌,后台线程执行) ---------- */
    m_watcher = new QFutureWatcher<int>(this);
    connect(m_watcher, &QFutureWatcher<int>::finished,
            this, &MainWindow::onGenerationFinished);

    g_win = this;
    worldgen_set_log(coreLog);
}

MainWindow::~MainWindow()
{
    if (g_win == this)
        g_win = nullptr;
}

void MainWindow::postLog(const QString &msg)
{
    appendLog(msg);
}

void MainWindow::appendLog(const QString &msg)
{
    m_log->appendPlainText(msg);
    m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
}

void MainWindow::generate()
{
    if (m_watcher->isRunning()) {
        appendLog(QStringLiteral("已在生成中,请稍候…"));
        return;
    }

    QString outPath = m_output->text().trimmed();
    if (outPath.isEmpty()) {
        appendLog(QStringLiteral("请先填写输出路径。"));
        return;
    }

    /* 相对路径 -> 相对当前工作目录转绝对;目录不存在则创建,
     * 不可写时(如从 Finder 启动 .app 时 cwd 为 /)回退到主目录 */
    if (!QDir::isAbsolutePath(outPath))
        outPath = QDir::current().absoluteFilePath(outPath);
    const QString parentPath = QFileInfo(outPath).absolutePath();
    if (!QDir(parentPath).exists() && !QDir().mkpath(parentPath)) {
        const QString alt = QDir::home().absoluteFilePath(QFileInfo(outPath).fileName());
        appendLog(QStringLiteral("无法创建目录 %1,改用 %2").arg(parentPath, alt));
        outPath = alt;
        if (!QDir().mkpath(QFileInfo(outPath).absolutePath())) {
            appendLog(QStringLiteral("仍然无法创建目录,已取消生成。"));
            return;
        }
    }
    m_output->setText(outPath);

    const int seed      = m_seed->value();
    const int faults    = m_faults->value();
    const int water     = m_water->value();
    const int w         = m_width->value();
    const int h         = m_height->value();
    const int lw        = m_lineWidth->value();
    const int grid      = m_grid->isChecked() ? 1 : 0;
    const int slices    = m_slices->value();
    const int fill      = m_fill->isChecked() ? 1 : 0;

    QStringList tags;
    if (grid) tags << QStringLiteral("[经纬网格]");
    if (slices > 0) tags << QStringLiteral("切片%1").arg(slices);
    if (fill) tags << QStringLiteral("[分层设色]");
    appendLog(QStringLiteral("开始生成: 种子=%1 故障=%2 水=%3% %4x%5 线宽=%6 -> %7 %8")
                  .arg(seed)
                  .arg(faults)
                  .arg(water)
                  .arg(w)
                  .arg(h)
                  .arg(lw)
                  .arg(outPath, tags.join(QLatin1Char(' '))));

    m_pendingImage = outPath;
    /* UTF-8 路径必须在此线程内转成字节并随 lambda 存活 */
    QByteArray out8 = outPath.toUtf8();

    auto future = QtConcurrent::run([seed, faults, water, w, h, lw,
                                     grid, slices, fill, out8]() {
        return worldgen_run(seed, faults, water, w, h, lw,
                            grid, slices, fill, out8.constData());
    });
    m_watcher->setFuture(future);
}

void MainWindow::browseOutput()
{
    const QString dir = QDir::currentPath();
    const QString file = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存地图"), dir,
        QStringLiteral("PNG 图片 (*.png);;所有文件 (*)"));
    if (!file.isEmpty())
        m_output->setText(file);
}

void MainWindow::onGenerationFinished()
{
    const int code = m_watcher->result();
    appendLog(QStringLiteral("生成结束,退出码 %1").arg(code));
    if (code == 0 && !m_pendingImage.isEmpty())
        loadImage(m_pendingImage);
}

void MainWindow::loadImage(const QString &path)
{
    QImage img(path);
    if (img.isNull()) {
        appendLog(QStringLiteral("无法加载图片: %1").arg(path));
        return;
    }
    /* 按预览区等比例缩放(不拉伸) */
    const int viewW = m_mapLabel->width();
    const int viewH = m_mapLabel->height();
    QImage scaled = img.scaled(viewW, viewH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_mapLabel->setPixmap(QPixmap::fromImage(scaled));
    m_mapLabel->setText(QString());
    appendLog(QStringLiteral("已加载: %1 (%2x%3)").arg(path).arg(img.width()).arg(img.height()));
}
