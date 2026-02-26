//
// Created by dev on 26/02/2026.
//

#ifndef READ_DICOM_GDCM_DICOMVIEWWIDGET_H
#define READ_DICOM_GDCM_DICOMVIEWWIDGET_H

// -------------------- Viewer widget: desenha imagem + overlay --------------------
class DicomViewWidget : public QWidget
{
public:
    explicit DicomViewWidget(QWidget* parent=nullptr) : QWidget(parent)
    {
        setMinimumSize(800, 600);
        setAutoFillBackground(true);
    }

    void setImage(const QImage& img) { m_img = img; update(); }
    void setOverlayText(const QString& t) { m_overlay = t; update(); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);

        if (!m_img.isNull()) {
            // calcula retângulo com aspecto preservado
            QSize imgSz = m_img.size();
            QSize wndSz = size();
            QSize scaled = imgSz.scaled(wndSz, Qt::KeepAspectRatio);

            QRect target((wndSz.width() - scaled.width())/2,
                         (wndSz.height() - scaled.height())/2,
                         scaled.width(), scaled.height());

            p.setRenderHint(QPainter::SmoothPixmapTransform, true);
            p.drawImage(target, m_img);

            // Overlay no canto inferior direito (com sombra)
            if (!m_overlay.isEmpty()) {
                const int margin = 12;

                QFont f = p.font();
                f.setPointSize(14);
                f.setBold(true);
                p.setFont(f);

                QRect overlayRect = rect().adjusted(margin, margin, -margin, -margin);

                // sombra
                p.setPen(QColor(0,0,0,200));
                p.drawText(overlayRect.adjusted(2,2,2,2),
                           Qt::AlignRight | Qt::AlignBottom,
                           m_overlay);

                // texto
                p.setPen(QColor(255,255,255,230));
                p.drawText(overlayRect,
                           Qt::AlignRight | Qt::AlignBottom,
                           m_overlay);
            }
        } else {
            p.setPen(Qt::white);
            p.drawText(rect(), Qt::AlignCenter, "Nenhuma imagem carregada");
        }
    }

private:
    QImage  m_img;
    QString m_overlay;
};


#endif //READ_DICOM_GDCM_DICOMVIEWWIDGET_H