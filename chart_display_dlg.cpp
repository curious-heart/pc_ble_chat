#include <QVBoxLayout>
#include <QFileDialog>
#include <QPixmap>
#include <QImage>

#include "ui_chart_display_dlg.h"
#include "chart_display_dlg.h"

ChartDisplayDlg::ChartDisplayDlg(QString init_save_pth): m_save_pth_str(init_save_pth)
{
    ui = new Ui::ChartDisplayDlg;
    ui->setupUi(this);
}

ChartDisplayDlg::~ChartDisplayDlg()
{
    delete ui;
}

void ChartDisplayDlg::on_saveImgBtn_clicked()
{
    QString img_format = ".png";
    QString save_filter = QString("Images (*%1)").arg(img_format);
    if(nullptr == m_gv)
    {
        return;
    }
    QString file_fpn;
    if(m_save_pth_str.isEmpty())
    {
        file_fpn = QFileDialog::getSaveFileName(this, tr("指定文件名"),
                                                QDir::currentPath(), save_filter);
    }
    else
    {
        file_fpn = QFileDialog::getSaveFileName(this, tr("指定文件名"),
                                                m_save_pth_str, save_filter);
    }
    if(file_fpn.isEmpty())
    {
        return;
    }
    QPixmap p = m_gv->grab();
    QImage image = p.toImage();
    image.save(file_fpn);
}

void ChartDisplayDlg::arrange_gv_display(QGraphicsView * gv)
{
    if(nullptr == gv)
    {
        return;
    }
    m_gv = gv;
    QVBoxLayout * v_layout = new QVBoxLayout(this);
    v_layout->addWidget(gv);
    v_layout->addWidget(ui->saveImgBtn);
}
