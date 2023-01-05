#ifndef CHART_DISPLAY_DLG_H
#define CHART_DISPLAY_DLG_H

#include <QtWidgets/qdialog.h>
#include <QGraphicsView>

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui {
    class ChartDisplayDlg;
}
QT_END_NAMESPACE


class ChartDisplayDlg: public QDialog
{
    Q_OBJECT

private:
    Ui::ChartDisplayDlg * ui;
    QGraphicsView * m_gv = nullptr;
    QString m_save_pth_str = "";

public:
    ChartDisplayDlg(QString init_save_pth = "");
    ~ChartDisplayDlg();

    void arrange_gv_display(QGraphicsView * gv);

private slots:
    void on_saveImgBtn_clicked();
};

#endif // CHART_DISPLAY_DLG_H
