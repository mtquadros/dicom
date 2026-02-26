//
// Created by dev on 26/02/2026.
//

#ifndef READ_DICOM_GDCM_DICOM_LIB_H
#define READ_DICOM_GDCM_DICOM_LIB_H
#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <gdcmFile.h>
#include <gdcmDataSet.h>
#include <gdcmTag.h>
#include <gdcmDataElement.h>
#include <gdcmByteValue.h>
#include <gdcmAttribute.h>

#include <QImage>
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
    const bool isSigned = pf.GetPixelRepresentation() == gdcm::PixelFormat::INT64;
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


static std::string GetStringTag(const gdcm::DataSet& ds, uint16_t group, uint16_t element)
{
    gdcm::Tag tag(group, element);
    if (!ds.FindDataElement(tag)) return "";

    const gdcm::DataElement& de = ds.GetDataElement(tag);
    const gdcm::ByteValue* bv = de.GetByteValue();
    if (!bv) return "";

    return std::string(bv->GetPointer(), bv->GetLength());
}
#endif //READ_DICOM_GDCM_DICOM_LIB_H