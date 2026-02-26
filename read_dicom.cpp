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
#include "dicom_lib.h"
#include "DicomViewWidget.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <array>


// -------------------- main --------------------
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    QString path = QFileDialog::getOpenFileName(
        nullptr,
        argv[1],
        QDir::currentPath(),
        "*.dcm;;*.dicom"
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
    metadata.append("PatientName: ");
    metadata.append(GetStringTag(ds, 0x0010, 0x0010));
    metadata.append("\n");
    metadata.append("PatientID:   ");
    metadata.append(GetStringTag(ds, 0x0010, 0x0020));
    metadata.append("\n");
    metadata.append("Modality:    ");
    metadata.append(GetStringTag(ds, 0x0008, 0x0060));
    metadata.append("\n");
    metadata.append("StudyDate:   ");
    metadata.append(GetStringTag(ds, 0x0008, 0x0020));
    metadata.append("\n");
    metadata.append("SeriesDesc:  ");
    metadata.append(GetStringTag(ds, 0x0008, 0x103E));
    metadata.append("\n");

    // Rows/Cols via Attribute (bom para tags numéricas)
    if (ds.FindDataElement(gdcm::Tag(0x0028, 0x0010)) && ds.FindDataElement(gdcm::Tag(0x0028, 0x0011))) {
        gdcm::Attribute<0x0028,0x0010> rows;
        gdcm::Attribute<0x0028,0x0011> cols;
        rows.SetFromDataSet(ds);
        cols.SetFromDataSet(ds);
        metadata.append("Rows x Cols: ");
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
    win.setWindowTitle("Visualizador de imagens DICOM");

    auto* viewer = new DicomViewWidget;
    viewer->setImage(img);
    viewer->setOverlayText(qmetadata);

    win.setCentralWidget(viewer);
    win.resize(1000, 800);
    win.show();

    return app.exec();
}