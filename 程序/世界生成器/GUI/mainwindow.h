#pragma once

#include <QMainWindow>
#include <QFutureWatcher>

class QSpinBox;
class QCheckBox;
class QLineEdit;
class QLabel;
class QPlainTextEdit;

/* 地图生成 GUI:worldgen 核心直接内嵌编译,可调整参数并预览地图 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void postLog(const QString &msg);   /* 供工作线程回调(线程安全) */

private slots:
    void generate();
    void browseOutput();
    void onGenerationFinished();
    void loadImage(const QString &path);

private:
    void appendLog(const QString &msg);

    QSpinBox      *m_seed;
    QSpinBox      *m_faults;
    QSpinBox      *m_water;
    QSpinBox      *m_width;
    QSpinBox      *m_height;
    QSpinBox      *m_lineWidth;
    QSpinBox      *m_slices;
    QCheckBox     *m_grid;
    QCheckBox     *m_fill;
    QLineEdit     *m_output;
    QLabel        *m_mapLabel;
    QPlainTextEdit *m_log;
    QFutureWatcher<int> *m_watcher;
    QString        m_pendingImage;
};
