#include "icc_manager.hpp"
#include <QImageReader>
namespace photo26::core {
QColorSpace IccManager::resolveQtColorSpace(TargetColorSpace t) {
    switch(t) { case TargetColorSpace::SRgb:return QColorSpace(QColorSpace::SRgb); case TargetColorSpace::AdobeRgb:return QColorSpace(QColorSpace::AdobeRgb); case TargetColorSpace::DisplayP3:return QColorSpace(QColorSpace::DisplayP3); case TargetColorSpace::ProPhotoRgb:return QColorSpace(QColorSpace::ProPhotoRgb); default:return {}; }
}
IccProfileInfo IccManager::inspect(const QImage& image) { IccProfileInfo info; const auto cs=image.colorSpace(); if(!cs.isValid()) return info; info.rawIccBytes=cs.iccProfile(); info.hasProfile=!info.rawIccBytes.isEmpty(); info.description=cs.description(); info.colorSpaceName=info.description; info.byteSize=int(info.rawIccBytes.size()); return info; }
IccProfileInfo IccManager::inspectFile(const QString& path) { QImageReader r(path); r.setAutoTransform(false); return inspect(r.read()); }
Result<QImage> IccManager::convertToColorSpace(const QImage& image, TargetColorSpace target) {
    if(image.isNull()) return Result<QImage>::failure(ErrorCode::InvalidParameter,"Imagen vacía");
    if(target==TargetColorSpace::KeepOriginal) return Result<QImage>::success(image);
    if(!image.colorSpace().isValid()) return Result<QImage>::failure(ErrorCode::InvalidParameter,"La imagen no tiene un perfil de origen válido; asigne uno antes de convertir");
    auto result=image.convertedToColorSpace(resolveQtColorSpace(target));
    if(result.isNull()) return Result<QImage>::failure(ErrorCode::InternalError,"No se pudo transformar el espacio de color");
    return Result<QImage>::success(result);
}
void IccManager::applyColorSpace(QImage& image, TargetColorSpace target) { if(target!=TargetColorSpace::KeepOriginal) image.setColorSpace(resolveQtColorSpace(target)); }
}
