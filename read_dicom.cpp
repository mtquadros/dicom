#include <gdcmImageReader.h>
#include <gdcmReader.h>
#include <gdcmFile.h>
#include <gdcmDataSet.h>
#include <gdcmTag.h>
#include <gdcmDataElement.h>
#include <gdcmByteValue.h>
#include <gdcmAttribute.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>

static std::string GetStringTag(const gdcm::DataSet& ds, uint16_t group, uint16_t element)
{
    gdcm::Tag tag(group, element);
    if (!ds.FindDataElement(tag)) return "";

    const gdcm::DataElement& de = ds.GetDataElement(tag);
    const gdcm::ByteValue* bv = de.GetByteValue();
    if (!bv) return "";

    return std::string(bv->GetPointer(), bv->GetLength());
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " arquivo.dcm\n";
        return 1;
    }

    const std::string filename = argv[1];

   // --------- (A) Ler dataset (metadados / tags) ----------
    gdcm::Reader r;
    r.SetFileName(filename.c_str());
    if (!r.Read()) {
        std::cerr << "Falha ao ler DICOM (Reader): " << filename << "\n";
        return 2;
    }

    const gdcm::File& file = r.GetFile();
    const gdcm::DataSet& ds = file.GetDataSet();

    std::cout << "== Tags comuns ==\n";
    std::cout << "PatientName (0010,0010): " << GetStringTag(ds, 0x0010, 0x0010) << "\n";
    std::cout << "PatientID   (0010,0020): " << GetStringTag(ds, 0x0010, 0x0020) << "\n";
    std::cout << "Modality    (0008,0060): " << GetStringTag(ds, 0x0008, 0x0060) << "\n";
    std::cout << "StudyDate   (0008,0020): " << GetStringTag(ds, 0x0008, 0x0020) << "\n";
    std::cout << "SeriesDesc  (0008,103E): " << GetStringTag(ds, 0x0008, 0x103E) << "\n";

    // Rows/Cols via Attribute (bom para tags numéricas)
    if (ds.FindDataElement(gdcm::Tag(0x0028, 0x0010)) && ds.FindDataElement(gdcm::Tag(0x0028, 0x0011))) {
        gdcm::Attribute<0x0028,0x0010> rows;
        gdcm::Attribute<0x0028,0x0011> cols;
        rows.SetFromDataSet(ds);
        cols.SetFromDataSet(ds);
        std::cout << "Rows x Cols (0028,0010/0011): " << rows.GetValue() << " x " << cols.GetValue() << "\n";
    }

    // --------- (B) Ler como imagem (pixels) ----------
    gdcm::ImageReader ir;
    ir.SetFileName(filename.c_str());
    if (!ir.Read()) {
        std::cerr << "Falha ao ler DICOM como imagem (ImageReader): " << filename << "\n";
        return 3;
    }

    const gdcm::Image& img = ir.GetImage();

    std::array<unsigned int, 3> dims;
    dims.fill(img.GetNumberOfDimensions());
//    unsigned int dims[3] = {0,0,0};
//    dims = img.GetDimensions();

    const unsigned int cols = dims[0];
    const unsigned int rows = dims[1];
    const unsigned int slices = dims[2];

    std::cout << "\n== Imagem ==\n";
    std::cout << "Dimensoes (cols x rows x slices): " << cols << " x " << rows << " x " << slices << "\n";
    std::cout << "Num dims: " << img.GetNumberOfDimensions() << "\n";
    std::cout << "PixelFormat: " << img.GetPixelFormat() << "\n";
    std::cout << "Photometric: " << img.GetPhotometricInterpretation() << "\n";

    const size_t len = img.GetBufferLength();
    std::vector<char> buffer(len);

    if (!img.GetBuffer(buffer.data())) {
        std::cerr << "Falha ao obter buffer de pixels.\n";
        return 4;
    }

    std::cout << "Bytes de pixels no buffer: " << len << "\n";
    std::cout << "OK.\n";

    return 0;
}