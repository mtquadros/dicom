//
// Created by dev on 25/02/2026.
//
#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QImage>
#include <QDir>

#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <gdcmFile.h>
#include <gdcmDataSet.h>
#include <gdcmTag.h>
#include <gdcmDataElement.h>
#include <gdcmByteValue.h>
#include <gdcmAttribute.h>

#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <array>

// -------------------- helpers DICOM tags --------------------
static bool TryGetDSString(const gdcm::DataSet& ds, uint16_t g, uint16_t e, std::string& out)
{
    gdcm::Tag tag(g, e);
    if (!ds.FindDataElement(tag)) return false;

    const gdcm::DataElement& de = ds.GetDataElement(tag);
    const gdcm::ByteValue* bv = de.GetByteValue();
    if (!bv) return false;

    out.assign(bv->GetPointer(), bv->GetLength());
    while (!out.empty() && (out.back() == '\0' || out.back() == ' ' || out.back() == '\r' || out.back() == '\n'))
        out.pop_back();
    return true;
}

static bool ParseDouble(const std::string& s, double& v)
{
    try {
        size_t idx = 0;
        v = std::stod(s, &idx);
        return idx > 0;
    } catch (...) { return false; }
}

// -------------------- DICOM -> QImage (Grayscale8) --------------------
static QImage DicomToQImage_Grayscale8(gdcm::ImageReader& ir)
{
    const gdcm::Image& img = ir.GetImage();
    const gdcm::File& file = ir.GetFile();
    const gdcm::DataSet& ds = file.GetDataSet();

    std::array<const unsigned int*,3> dims;
    dims.fill(img.GetDimensions());
    const int w = static_cast<int>(*dims[0]);
    const int h = static_cast<int>(*dims[1]);

    const size_t len = img.GetBufferLength();
    std::vector<char> buffer(len);
    if (!img.GetBuffer(buffer.data()))
        return QImage();

    const auto pf = img.GetPixelFormat();
    const int bitsAllocated = pf.GetBitsAllocated();
    const bool isSigned = pf.GetPixelRepresentation() == gdcm::PixelFormat::INT16;
    const int samplesPerPixel = pf.GetSamplesPerPixel();

    if (samplesPerPixel != 1) {
        // Este exemplo foca em grayscale (1 canal)
        return QImage();
    }
    if (!(bitsAllocated == 8 || bitsAllocated == 16)) {
        return QImage();
    }

    // Window Center/Width (0028,1050)/(0028,1051)
    double wc = 0.0, ww = 0.0;
    bool hasWL = false;
    {
        std::string sWC, sWW;
        if (TryGetDSString(ds, 0x0028, 0x1050, sWC) && TryGetDSString(ds, 0x0028, 0x1051, sWW)) {
            auto first = [](std::string x){
                auto p = x.find('\\');
                if (p != std::string::npos) x = x.substr(0, p);
                return x;
            };
            sWC = first(sWC);
            sWW = first(sWW);

            double tWC=0, tWW=0;
            if (ParseDouble(sWC, tWC) && ParseDouble(sWW, tWW) && tWW > 1e-9) {
                wc = tWC; ww = tWW;
                hasWL = true;
            }
        }
    }

    QImage out(w, h, QImage::Format_Grayscale8);
    if (out.isNull()) return QImage();

    auto clamp255 = [](int x){ return static_cast<uchar>(std::max(0, std::min(255, x))); };

    auto mapWL = [&](double p)->uchar {
        const double low  = wc - ww / 2.0;
        const double high = wc + ww / 2.0;
        if (p <= low) return 0;
        if (p >= high) return 255;
        const double t = (p - low) / (high - low);
        return clamp255(static_cast<int>(std::lround(t * 255.0)));
    };

    double minV = 0.0, maxV = 0.0;
    if (!hasWL) {
        minV =  1e300; maxV = -1e300;
        auto upd = [&](double v){ minV = std::min(minV, v); maxV = std::max(maxV, v); };

        if (bitsAllocated == 8) {
            const uint8_t* px = reinterpret_cast<const uint8_t*>(buffer.data());
            for (int i=0;i<w*h;++i) upd(px[i]);
        } else {
            if (isSigned) {
                const int16_t* px = reinterpret_cast<const int16_t*>(buffer.data());
                for (int i=0;i<w*h;++i) upd(px[i]);
            } else {
                const uint16_t* px = reinterpret_cast<const uint16_t*>(buffer.data());
                for (int i=0;i<w*h;++i) upd(px[i]);
            }
        }
        if (!(maxV > minV)) maxV = minV + 1.0;
    }

    auto mapMinMax = [&](double p)->uchar {
        const double t = (p - minV) / (maxV - minV);
        return clamp255(static_cast<int>(std::lround(t * 255.0)));
    };

    for (int y=0;y<h;++y) {
        uchar* row = out.scanLine(y);
        for (int x=0;x<w;++x) {
            const int idx = y*w + x;
            double p = 0.0;

            if (bitsAllocated == 8) {
                const uint8_t* px = reinterpret_cast<const uint8_t*>(buffer.data());
                p = px[idx];
            } else {
                if (isSigned) {
                    const int16_t* px = reinterpret_cast<const int16_t*>(buffer.data());
                    p = px[idx];
                } else {
                    const uint16_t* px = reinterpret_cast<const uint16_t*>(buffer.data());
                    p = px[idx];
                }
            }

            row[x] = hasWL ? mapWL(p) : mapMinMax(p);
        }
    }

    return out;
}

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

static std::string GetStringTag(const gdcm::DataSet& ds, uint16_t group, uint16_t element)
{
    gdcm::Tag tag(group, element);
    if (!ds.FindDataElement(tag)) return "";

    const gdcm::DataElement& de = ds.GetDataElement(tag);
    const gdcm::ByteValue* bv = de.GetByteValue();
    if (!bv) return "";

    return std::string(bv->GetPointer(), bv->GetLength());
}

// -------------------- main --------------------
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    QString path = QFileDialog::getOpenFileName(
        nullptr,
        argv[1],
        QDir::homePath(),
        "DICOM (*.dcm *.dicom *);;Todos (*)"
    );
    if (path.isEmpty()) return 0;

    gdcm::ImageReader ir;
    ir.SetFileName(path.toStdString().c_str());
    if (!ir.Read()) {
        QMessageBox::critical(nullptr, "Erro", "Falha ao ler o arquivo DICOM com GDCM.");
        return 1;
    }
//============================================================================================================
    const gdcm::File& file = ir.GetFile();
    const gdcm::DataSet& ds = file.GetDataSet();

    std::string metadata;
    metadata.append("== Tags comuns ==\n");
    metadata.append("PatientName (0010,0010): ");
    metadata.append(GetStringTag(ds, 0x0010, 0x0010));
    metadata.append("\n");
    metadata.append("PatientID   (0010,0020): ");
    metadata.append(GetStringTag(ds, 0x0010, 0x0020));
    metadata.append("\n");
    metadata.append("Modality    (0008,0060): ");
    metadata.append(GetStringTag(ds, 0x0008, 0x0060));
    metadata.append("\n");
    metadata.append("StudyDate   (0008,0020): ");
    metadata.append(GetStringTag(ds, 0x0008, 0x0020));
    metadata.append("\n");
    metadata.append("SeriesDesc  (0008,103E): ");
    metadata.append(GetStringTag(ds, 0x0008, 0x103E));
    metadata.append("\n");

    // Rows/Cols via Attribute (bom para tags numéricas)
    if (ds.FindDataElement(gdcm::Tag(0x0028, 0x0010)) && ds.FindDataElement(gdcm::Tag(0x0028, 0x0011))) {
        gdcm::Attribute<0x0028,0x0010> rows;
        gdcm::Attribute<0x0028,0x0011> cols;
        rows.SetFromDataSet(ds);
        cols.SetFromDataSet(ds);
        metadata.append("Rows x Cols (0028,0010/0011): ");
        metadata.append(std::to_string(rows.GetValue()));
        metadata.append(" x ");
        metadata.append(std::to_string(cols.GetValue()));
        metadata.append("\n");
    }

//============================================================================================================
    QImage img = DicomToQImage_Grayscale8(ir);
    if (img.isNull()) {
        QMessageBox::critical(nullptr, "Erro",
                              "Não consegui converter para imagem.\n"
                              "Este exemplo suporta principalmente DICOM grayscale (1 canal) 8/16-bit.");
        return 2;
    }

    // Texto do overlay (você pode colocar tags aqui também)
    QString overlay = QFileInfo(path).fileName();

    QString qmetadata = QString::fromStdString(metadata);
    QMainWindow win;
    win.setWindowTitle("DICOM Viewer (Qt5 + GDCM)");

    auto* viewer = new DicomViewWidget;
    viewer->setImage(img);
    viewer->setOverlayText(qmetadata);

    win.setCentralWidget(viewer);
    win.resize(1000, 800);
    win.show();

    return app.exec();
}