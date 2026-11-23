#pragma once
#include "../common/types.hpp"
#include "../common/result.hpp"
#include "../common/cancellation.hpp"
#include "../common/safe_file_writer.hpp"
#include <QtCore/QRect>
#include <QtCore/QLineF>
#include <QtCore/QJsonObject>
#include <functional>
namespace photo26::core {
#ifndef PHOTO26_OP_SPRITESHEET_OP
#define PHOTO26_OP_SPRITESHEET_OP
struct SpriteInput {
    QString name;
    QImage image;
};
struct SpritesheetOptions {
    int columns{0};
    int rows{0};
    int spacingPx{2};
    int paddingPx{2};
    int forcedCellWidth{0};
    int forcedCellHeight{0};
    QColor backgroundColor{Qt::transparent};
};
struct SpriteFrameCoord {
    QString name;
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};
struct SpritesheetResult {
    QImage atlasImage;
    std::vector<SpriteFrameCoord> frames;
    QByteArray coordinatesJson;
};
class SpritesheetOp {
  public:
    static Result<SpritesheetResult> generate(const std::vector<SpriteInput> &,
                                              const SpritesheetOptions & = {});
    static StatusResult saveSpritesheetAndJson(const SpritesheetResult &, const QString &,
                                               const QString &, const QString & = "png",
                                               bool = true);
};
#endif
} // namespace photo26::core
