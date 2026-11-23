#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QApplication>
#include <cmath>
#include <atomic>
#include <stdexcept>
#include "../src/desktop/main_window.hpp"
#include "../src/core/io/heif_codec.hpp"
using namespace photo26::core;
using namespace photo26::desktop;
static void check(bool b, const char *label) { if (!b) throw std::runtime_error(label); }
static QImage solid(int w, int h, QColor c) { QImage i(w,h,QImage::Format_ARGB32); i.fill(c); return i; }
static QByteArray readFile(const QString &path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return f.readAll(); }
class Photo26TestSuite : public QObject {
    Q_OBJECT
private slots:
    void operationsRegression() {
        try {
        QImage image = solid(120, 80, Qt::red);
        auto crop =
            CropTransformOp::computeCenteredAspectCrop(image.size(), AspectRatioPreset::Ratio1_1);
        check(crop == QRect(20, 0, 80, 80), "center crop");
        check(CropTransformOp::clampCropRect(QRect(-10, -10, 20, 20), image.size()) ==
                  QRect(0, 0, 10, 10),
              "clamp crop");
        check(!CropTransformOp::crop(image, {}).isOk(), "reject empty crop");
        auto thirds = CropTransformOp::computeRuleOfThirds(QRectF(0, 0, 90, 60));
        check(thirds.vertical1.x1() == 30 && thirds.horizontal2.y1() == 40, "thirds");
        TransformOptions t;
        t.rotateQuadrants90 = 90;
        auto rotated = CropTransformOp::transform(image, t);
        check(rotated.isOk() && rotated.value->size() == QSize(80, 120), "quarter rotation");
        t.rotateQuadrants90 = 3;
        check(!CropTransformOp::transform(image, t).isOk(), "invalid rotation");
        t.rotateQuadrants90 = 0;
        t.freeAngleDegrees = 30;
        t.backgroundColor = Qt::blue;
        auto angled = CropTransformOp::transform(image, t);
        check(angled.isOk() && angled.value->pixelColor(0, 0).blue() == 255, "rotation background");
        ResizeOptions r;
        r.targetWidth = 60;
        auto size = ResizeOp::computeTargetSize(image.size(), r);
        check(size.isOk() && *size.value == QSize(60, 40), "aspect resize");
        r.targetWidth = 240;
        r.preventUpscaling = true;
        check(*ResizeOp::computeTargetSize(image.size(), r).value == image.size(),
              "prevent upscaling");
        r.mode = ResizeMode::ByPercentage;
        r.percentage = -1;
        check(!ResizeOp::computeTargetSize(image.size(), r).isOk(), "invalid percentage");
        QImage edge = solid(2, 1, Qt::transparent);
        edge.setPixelColor(0, 0, QColor(255, 0, 0, 255));
        edge.setPixelColor(1, 0, QColor(0, 0, 255, 0));
        r = {};
        r.targetWidth = 17;
        r.targetHeight = 1;
        r.keepAspectRatio = false;
        for (auto method : {InterpolationMethod::Nearest, InterpolationMethod::Bilinear,
                            InterpolationMethod::Bicubic, InterpolationMethod::Lanczos3}) {
            r.interpolation = method;
            auto scaled = ResizeOp::resize(edge, r);
            check(scaled.isOk() && scaled.value->size() == QSize(17, 1), "resampler output");
            auto c = scaled.value->pixelColor(7, 0);
            check(c.red() >= 250 && c.blue() <= 2, "premultiplied alpha halo");
        }
        QImage gradient(31, 17, QImage::Format_RGBA64);
        for (int y = 0; y < 17; ++y)
            for (int x = 0; x < 31; ++x)
                gradient.setPixelColor(x, y, QColor::fromRgbF(x / 30., y / 16., .2, .6));
        r = {};
        r.targetWidth = 7;
        r.targetHeight = 5;
        r.keepAspectRatio = false;
        auto high = ResizeOp::resize(gradient, r);
        check(high.isOk() && high.value->depth() == 64, "16 bit resampling");
        r = {};
        r.targetWidth = 300;
        r.targetHeight = 1;
        r.keepAspectRatio = false;
        auto narrow = ResizeOp::resize(solid(1, 300, Qt::red), r);
        check(narrow.isOk() && narrow.value->size() == QSize(300, 1) &&
                  narrow.value->pixelColor(150, 0) == QColor(Qt::red),
              "vertical-first resampling");
        ColorAdjustOptions color;
        color.grayscale = true;
        auto gray = ColorAdjustOp::apply(solid(1, 1, Qt::red), color);
        check(gray.isOk() && std::abs(gray.value->pixelColor(0, 0).redF() - .2126) < .0001,
              "Rec709 red luminance");
        color = {};
        color.invert = true;
        auto inv = ColorAdjustOp::apply(solid(1, 1, QColor(30, 60, 90, 77)), color);
        check(inv.isOk() && inv.value->pixelColor(0, 0) == QColor(225, 195, 165, 77),
              "invert alpha");
        color = {};
        color.gamma = 2;
        auto gamma = ColorAdjustOp::apply(solid(1, 1, QColor(64, 64, 64)), color);
        check(gamma.value->pixelColor(0, 0).red() > 120, "gamma formula");
        color = {};
        color.blackAndWhite = true;
        color.bwDither = true;
        auto bw = ColorAdjustOp::apply(gradient, color);
        check(bw.isOk(), "dithering");
        for (int y = 0; y < bw.value->height(); ++y)
            for (int x = 0; x < bw.value->width(); ++x) {
                auto c = bw.value->pixelColor(x, y);
                check((c.red() == 0 || c.red() == 255) && c.red() == c.green() &&
                          c.green() == c.blue(),
                      "binary dither values");
            }
        color.gamma = 0;
        check(!ColorAdjustOp::apply(image, color).isOk(), "reject gamma");
        CombineOptions co;
        co.marginPx = 0;
        co.spacingPx = 0;
        co.targetCellWidth = 100;
        co.targetCellHeight = 100;
        auto combined = CombineOp::combine({solid(100, 50, Qt::red), solid(50, 100, Qt::blue)}, co);
        check(combined.isOk() && combined.value->size() == QSize(200, 100), "combine size");
        check(combined.value->pixelColor(0, 0) == QColor(Qt::white) &&
                  combined.value->pixelColor(0, 30) == QColor(Qt::red),
              "combine preserve ratio");
        co.layout = CombineLayout::Mosaic;
        check(CombineOp::combine({image, image, image}, co).isOk(), "mosaic");
        CompressionOptions compression;
        compression.format = "png";
        auto compressed = CompressWebOp::compressInMemory(image, compression, 20000);
        check(compressed.isOk() &&
                  QImage::fromData(compressed.value->encodedBytes).size() == image.size(),
              "PNG compression roundtrip");
        check(compressed.value->compressedBytes ==
                  std::uint64_t(compressed.value->encodedBytes.size()),
              "compression metrics");
        compression.format = "not-a-format";
        check(!CompressWebOp::compressInMemory(image, compression).isOk(), "unsupported encoder");
        compression.format = "jpg";
        compression.lossless = true;
        check(!CompressWebOp::compressInMemory(image, compression).isOk(),
              "honest lossless JPEG rejection");
        compression.lossless = false;
        compression.targetMaxBytes = 1000;
        auto targeted = CompressWebOp::compressInMemory(gradient, compression);
        check(targeted.isOk() && targeted.value->compressedBytes <= 1000, "target compression");
        WebOptimizerOptions web;
        web.format = "png";
        web.widths = {32, 64, 320};
        auto variants = CompressWebOp::generateWebVariants(image, "sample.png", web);
        check(variants.isOk() && variants.value->size() == 3 &&
                  variants.value->back().actualSize == image.size(),
              "web variants no upscale");
        web.namingPattern = "../escape.png";
        check(!CompressWebOp::generateWebVariants(image, "sample", web).isOk(),
              "web path traversal rejection");
        WatermarkOptions watermark;
        watermark.useImageWatermark = true;
        watermark.watermarkImage = solid(10, 10, Qt::blue);
        watermark.opacity = 1;
        watermark.marginX = 0;
        watermark.marginY = 0;
        auto marked = WatermarkTextOp::applyWatermark(image, watermark);
        check(marked.isOk() && marked.value->pixelColor(119, 79).blue() == 255, "image watermark");
        watermark.useImageWatermark = false;
        watermark.fontPointSize = 12;
        check(WatermarkTextOp::applyWatermark(image, watermark).isOk(), "text watermark");
        TextAndBorderOptions border;
        border.paddingPx = 5;
        border.borderWidthPx = 2;
        border.cornerRadiusPx = 10;
        border.text = "Photo26";
        auto bordered = WatermarkTextOp::applyTextAndBorders(image, border);
        check(bordered.isOk() && bordered.value->size() == QSize(134, 94) &&
                  bordered.value->pixelColor(0, 0).alpha() == 0,
              "text rounded border");
        auto atlas = SpritesheetOp::generate({{"a", image}, {"b", solid(20, 30, Qt::blue)}});
        check(atlas.isOk() && atlas.value->frames.size() == 2 &&
                  !QJsonDocument::fromJson(atlas.value->coordinatesJson).isNull(),
              "spritesheet coordinates");
        SpritesheetOptions so;
        so.columns = 1;
        so.rows = 1;
        check(!SpritesheetOp::generate({{"a", image}, {"b", image}}, so).isOk(), "atlas capacity");
        QImage two = solid(100, 20, Qt::red);
        for (int y = 0; y < 20; ++y)
            for (int x = 75; x < 100; ++x)
                two.setPixelColor(x, y, Qt::blue);
        auto palette = PaletteExtractorOp::extractDominantColors(two, 2);
        auto again = PaletteExtractorOp::extractDominantColors(two, 2);
        check(palette.isOk() && palette.value->size() == 2 &&
                  palette.value->front().color == QColor(Qt::red) &&
                  std::abs(palette.value->front().dominanceRatio - .75) < 1e-9,
              "palette dominance");
        check(again.value->front().hex == palette.value->front().hex, "deterministic palette");
        for (auto f : {PaletteExportFormat::Json, PaletteExportFormat::CssVariables,
                       PaletteExportFormat::GimpGpl, PaletteExportFormat::SwatchPng})
            check(PaletteExtractorOp::exportPalette(*palette.value, f).isOk(), "palette export");
        auto same = ImageCompareOp::compare(image, image);
        check(same.isOk() && same.value->rmse == 0 && std::isinf(same.value->psnrDb),
              "identical comparison");
        auto different = ImageCompareOp::compare(image, solid(120, 80, Qt::blue));
        check(different.value->diffPixelPercent == 100 && different.value->rmse > 0,
              "difference metrics");
        for (auto m : {ComparisonMode::SideBySide, ComparisonMode::SplitSlider,
                       ComparisonMode::OverlayBlend})
            check(ImageCompareOp::compare(image, edge, m).isOk(),
                  "comparison modes different sizes");
        QImage subject = solid(40, 40, Qt::white);
        for (int y = 10; y < 30; ++y)
            for (int x = 10; x < 30; ++x)
                subject.setPixelColor(x, y, Qt::red);
        subject.setPixelColor(20, 20, Qt::white);
        BgRemovalOptions bg;
        bg.featherRadius = 0;
        auto removed = BackgroundRemovalOp::removeBackground(subject, bg);
        check(removed.isOk() && removed.value->pixelColor(0, 0).alpha() == 0 &&
                  removed.value->pixelColor(15, 15).alpha() == 255 &&
                  removed.value->pixelColor(20, 20).alpha() == 255,
              "exterior connected segmentation");
        bg.method = BgRemovalMethod::ChromaKeyColor;
        auto chroma = BackgroundRemovalOp::removeBackground(subject, bg);
        check(chroma.value->pixelColor(20, 20).alpha() == 0, "chroma key enclosed color");
        bg.method = BgRemovalMethod::LuminanceThreshold;
        bg.replaceWithSolidColor = true;
        bg.replacementColor = Qt::blue;
        auto replaced = BackgroundRemovalOp::removeBackground(subject, bg);
        check(replaced.isOk() && replaced.value->pixelColor(0, 0) == QColor(Qt::blue),
              "luminance replacement");
        check(!BackgroundRemovalOp::isOnnxRuntimeAvailable(), "honest ONNX availability");
        // Keep all artifacts inside the exclusively assigned directory.
        QTemporaryDir temp;
        check(temp.isValid(), "temporary test directory");
        QString p1 = temp.filePath("one.png"), p2 = temp.filePath("two.png");
        check(image.save(p1) && subject.save(p2), "create test images");
        MassRenameOptions rn;
        rn.pattern = "img-{num}-{width}x{height}";
        auto preview = MassRenameOp::previewRename({p1, p2}, rn, temp.filePath("renamed"));
        check(preview.size() == 2 && preview[0].proposedName == "img-001-120x80.png",
              "rename tokens");
        check(MassRenameOp::executeRenameOrCopy(preview).isOk() && QFileInfo::exists(p1) &&
                  QFileInfo::exists(preview[0].proposedPath),
              "copy non-destructive");
        rn.pattern = "same";
        auto duplicate = MassRenameOp::previewRename({p1, p2}, rn, temp.filePath("conflicts"));
        check(duplicate[0].hasConflict && duplicate[1].hasConflict, "duplicate rename detection");
        check(!MassRenameOp::executeRenameOrCopy(duplicate, true, true).isOk(),
              "duplicate override rejected");
        BatchPreset preset;
        preset.outputFormat = "png";
        preset.enableResize = true;
        preset.resizeOptions.targetWidth = 60;
        preset.enableColorAdjust = true;
        preset.colorOptions.grayscale = true;
        preset.renameOptions.pattern = "{name}-{num}";
        preset.watermarkOptions.watermarkImage = subject;
        preset.watermarkOptions.text = "roundtrip";
        auto json = BatchProcessor::presetToJson(preset);
        auto round = BatchProcessor::presetFromJson(json);
        check(round.isOk() && BatchProcessor::presetToJson(*round.value) == json,
              "all preset fields JSON roundtrip");
        json["quality"] = "bad";
        check(!BatchProcessor::presetFromJson(json).isOk(), "JSON type rejection");
        std::atomic<int> progress{0};
        auto batch = BatchProcessor::runBatch(
            {p1, p2, temp.filePath("missing.png")}, temp.filePath("batch"), preset, false, {},
            [&](int done, int, const QString &) { progress = done; }, 2);
        check(batch.succeeded == 2 && batch.failed == 1 && batch.itemResults.size() == 3 &&
                  progress == 3,
              "batch independent errors and progress");
        check(QImage(batch.itemResults[0].outputPath).size() == QSize(60, 40), "batch output size");
        CancellationToken cancel;
        cancel.cancel();
        auto cancelled =
            BatchProcessor::runBatch({p1, p2}, temp.filePath("cancelled"), preset, false, cancel);
        check(cancelled.cancelled == 2 && cancelled.succeeded == 0, "batch cancellation");
        auto custom = BatchProcessor::runBatch(
            {p1, p2}, temp.filePath("custom"), preset, false, {}, {}, 2,
            [](const QImage &, const QString &, const QString &, const BatchPreset &) {
                throw std::runtime_error("saver error");
                return StatusResult::success();
            });
        check(custom.failed == 2, "custom saver failure isolation");
        check(SpritesheetOp::saveSpritesheetAndJson(*atlas.value, temp.filePath("atlas.png"),
                                                    temp.filePath("atlas.json"))
                  .isOk(),
              "save atlas and JSON");
        web.namingPattern = "{name}-{width}w.{ext}";
        check(CompressWebOp::saveWebVariantsToFolder(image, "sample", temp.filePath("web"), web,
                                                     false)
                  .isOk(),
              "save web variants");
        check(!CompressWebOp::saveWebVariantsToFolder(image, "sample", temp.filePath("web"), web,
                                                      false)
                   .isOk(),
              "save web conflict");
        preset.renameOptions.pattern = "collision";
        auto colliding =
            BatchProcessor::runBatch({p1, p2}, temp.filePath("duplicates"), preset, true);
        check(colliding.failed == 2 && colliding.succeeded == 0,
              "batch duplicate outputs never overwritten");
        } catch (const std::exception &e) { QFAIL(e.what()); }
    }
    void operationVariants() {
        QImage image = solid(120, 80, Qt::red);
        for (auto preset : {AspectRatioPreset::Ratio1_1, AspectRatioPreset::Ratio3_4,
                            AspectRatioPreset::Ratio4_3, AspectRatioPreset::Ratio16_9,
                            AspectRatioPreset::Ratio9_16, AspectRatioPreset::Ratio2_3,
                            AspectRatioPreset::Ratio3_2, AspectRatioPreset::Ratio21_9,
                            AspectRatioPreset::Custom}) {
            QRect rect = CropTransformOp::computeCenteredAspectCrop(image.size(), preset, 5, 4);
            QVERIFY(!rect.isEmpty()); QVERIFY(QRect(QPoint(0,0), image.size()).contains(rect));
            double expected = CropTransformOp::aspectRatioValue(preset, 5, 4);
            QVERIFY(std::abs(double(rect.width()) / rect.height() - expected) < 0.08);
        }
        CropOptions crop; crop.cropRect=QRect(5, 6, 20, 30);
        auto cropped=CropTransformOp::crop(image,crop); QVERIFY(cropped.isOk()); QCOMPARE(cropped.value->size(),QSize(20,30));
        for (int angle : {90, 180, 270}) { TransformOptions t; t.rotateQuadrants90=angle;
            auto rotated=CropTransformOp::transform(image,t); QVERIFY(rotated.isOk());
            QCOMPARE(rotated.value->size(), angle == 180 ? QSize(120,80) : QSize(80,120)); }
        QImage corners=solid(2,2,Qt::red); corners.setPixelColor(0,0,Qt::blue);
        TransformOptions flip; flip.flipHorizontal=true; auto h=CropTransformOp::transform(corners,flip);
        QVERIFY(h.isOk()); QCOMPARE(h.value->pixelColor(1,0),QColor(Qt::blue));
        flip={}; flip.flipVertical=true; auto v=CropTransformOp::transform(corners,flip);
        QVERIFY(v.isOk()); QCOMPARE(v.value->pixelColor(0,1),QColor(Qt::blue));
        ResizeOptions resize; resize.mode=ResizeMode::ByPercentage; resize.percentage=50;
        auto half=ResizeOp::computeTargetSize(image.size(),resize); QVERIFY(half.isOk()); QCOMPARE(*half.value,QSize(60,40));
        resize.mode=ResizeMode::FitMaxDimensions; resize.maxWidth=60; resize.maxHeight=60;
        auto fitted=ResizeOp::computeTargetSize(image.size(),resize); QVERIFY(fitted.isOk()); QCOMPARE(*fitted.value,QSize(60,40));
        for (auto layout : {CombineLayout::Horizontal,CombineLayout::Vertical,CombineLayout::Grid,CombineLayout::Mosaic}) {
            CombineOptions opts; opts.layout=layout; opts.spacingPx=5; opts.marginPx=7; opts.transparentBackground=true;
            auto combined=CombineOp::combine({image,image,image},opts); QVERIFY(combined.isOk());
            QCOMPARE(combined.value->pixelColor(0,0).alpha(),0);
        }
        for (int mode=0;mode<6;++mode) { ColorAdjustOptions opt;
            if(mode==0) opt.sepia=true; if(mode==1) opt.brightness=0.2;
            if(mode==2) opt.contrast=0.2; if(mode==3) opt.saturation=-0.5;
            if(mode==4) opt.exposureEv=1; if(mode==5) { opt.blackAndWhite=true; opt.bwThreshold=100; }
            auto result=ColorAdjustOp::apply(solid(8,8,QColor(100,80,60)),opt);
            QVERIFY(result.isOk()); QVERIFY(result.value->pixelColor(0,0)!=QColor(100,80,60));
        }
        WebOptimizerOptions web; web.format="png";
        auto variants=CompressWebOp::generateWebVariants(image,"sample",web); QVERIFY(variants.isOk());
        QCOMPARE(variants.value->size(),size_t(5));
        const int widths[]={320,640,960,1280,1920};
        for (size_t i=0;i<5;++i) { QCOMPARE(variants.value->at(i).targetWidth,widths[i]); QVERIFY(variants.value->at(i).actualSize.width()<=image.width()); }
    }
    void formatsAndIo() {
        QVERIFY(!FormatRegistry::allFormats().empty());
        const std::vector<std::pair<QByteArray, QString>> headers = {
            {QByteArray::fromHex("89504e470d0a1a0a"), "png"},
            {QByteArray::fromHex("ffd8ff"), "jpg"}, {"RIFF1234WEBP", "webp"},
            {"GIF89a", "gif"}, {"BM", "bmp"}, {QByteArray::fromHex("49492a00"), "tiff"},
            {QByteArray::fromHex("00000100"), "ico"}, {"<svg xmlns='http://www.w3.org/2000/svg'/>", "svg"},
            {QByteArray::fromHex("00000010667479706865696300000000"), "heic"},
            {QByteArray::fromHex("00000010667479706176696600000000"), "avif"}};
        for (const auto &h : headers) QCOMPARE(FormatRegistry::detectFormatFromHeader(h.first), h.second);
        QTemporaryDir dir; QVERIFY(dir.isValid());
        QImage img = solid(40, 30, QColor(255, 0, 0, 128));
        for (const QString &fmt : {QString("png"), QString("jpg"), QString("webp"), QString("bmp"), QString("tiff"), QString("gif"), QString("ico"), QString("ppm")}) {
            SaveOptions opt; opt.format = fmt; opt.backgroundColor = Qt::blue;
            QString path = dir.filePath("image." + fmt);
            auto saved = ImageIo::saveImage(img, path, opt);
            QVERIFY2(saved.isOk(), qPrintable(fmt + ": " + saved.error.message));
            auto loaded = ImageIo::loadImage(path);
            QVERIFY2(loaded.isOk(), qPrintable(fmt + ": " + loaded.error.message));
            QVERIFY(!loaded.value->image.isNull());
            if (fmt != "ico") QCOMPARE(loaded.value->image.size(), img.size());
            if (fmt == "jpg" || fmt == "bmp") {
                QColor c = loaded.value->image.pixelColor(10, 10);
                QVERIFY(c.alpha() == 255 && c.red() > 100 && c.blue() > 100 && c.green() < 20);
            }
        }
        SaveOptions svg; svg.format = "svg";
        auto bad = ImageIo::saveImage(img, dir.filePath("fake.svg"), svg);
        QCOMPARE(bad.error.code, ErrorCode::UnsupportedFormat);
        QVERIFY(SafeFileWriter::writeBytes(dir.filePath("vector.svg"), "<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10'><rect width='10' height='10' fill='red'/></svg>").isOk());
        LoadOptions opts; opts.svgWidth = 60; opts.svgHeight = 40;
        auto raster = ImageIo::loadImage(dir.filePath("vector.svg"), opts);
        QVERIFY(raster.isOk()); QCOMPARE(raster.value->image.size(), QSize(60, 40));
        QCOMPARE(raster.value->image.pixelColor(30, 20), QColor(Qt::red));
        opts = {}; opts.limits.maxWidth = 10;
        QVERIFY(!ImageIo::loadImage(dir.filePath("image.png"), opts).isOk());
        QVERIFY(!ImageIo::loadImage(dir.filePath("missing.png")).isOk());
        QVERIFY(SafeFileWriter::writeBytes(dir.filePath("broken.png"), "broken").isOk());
        QVERIFY(!ImageIo::loadImage(dir.filePath("broken.png")).isOk());
    }
    void safeZipAndDocuments() {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        QString path = dir.filePath("foto.png");
        QVERIFY(SafeFileWriter::writeBytes(path, "original").isOk());
        auto conflict = SafeFileWriter::writeBytes(path, "new");
        QCOMPARE(conflict.error.code, ErrorCode::ConflictDetected);
        QCOMPARE(readFile(path), QByteArray("original"));
        QCOMPARE(QFileInfo(SafeFileWriter::suggestOutputFileName(path, "recortada", "png")).fileName(), QString("foto_recortada.png"));
        QVERIFY(SafeFileWriter::writeWithCallback(dir.filePath("callback"), [](const QString &p) { return SafeFileWriter::writeBytes(p, "atomic", {true, true}); }).isOk());
        QCOMPARE(readFile(dir.filePath("callback")), QByteArray("atomic"));
        QString zip = dir.filePath("archive.zip");
        QVERIFY(ZipWriter::createZipFromEntries(zip, {{"test.txt", "123456789"}}).isOk());
        QByteArray z = readFile(zip); QVERIFY(z.startsWith(QByteArray::fromHex("504b0304")));
        // CRC-32 of the canonical test vector, little endian in the ZIP local header.
        QCOMPARE(z.mid(14, 4), QByteArray::fromHex("2639f4cb"));
        QImage img = solid(64, 48, Qt::red);
        QString pdf = dir.filePath("single.pdf"), multi = dir.filePath("multi.pdf");
        QVERIFY(PdfExporter::exportSingleImageToPdf(img, pdf).isOk());
        QVERIFY(PdfExporter::exportImagesToPdf({img, solid(48, 64, Qt::blue)}, multi).isOk());
        QVERIFY(readFile(pdf).startsWith("%PDF-")); QVERIFY(readFile(multi).startsWith("%PDF-"));
        QVERIFY(QFileInfo(pdf).size() > 100); QVERIFY(QFileInfo(multi).size() > 100);
    }
    void iconAndAnimation() {
        QTemporaryDir dir; QVERIFY(dir.isValid()); QImage img = solid(64, 64, Qt::red);
        QString ico = dir.filePath("favicon.ico");
        QVERIFY(IcoCodec::saveMultiResolutionIco(img, ico).isOk());
        auto entries = IcoCodec::readIcoEntries(ico); QVERIFY(entries.isOk()); QCOMPARE(entries.value->size(), size_t(3));
        const int sizes[] = {16, 32, 48};
        for (int i = 0; i < 3; ++i) QCOMPARE((*entries.value)[i].size(), QSize(sizes[i], sizes[i]));
        auto bundle = IcoCodec::exportFaviconBundle(img, dir.filePath("bundle"));
        QVERIFY(bundle.isOk()); QCOMPARE(bundle.value->pngFilePaths.size(), 6);
        QVERIFY(QFileInfo::exists(bundle.value->icoFilePath));
        QVERIFY(QJsonDocument::fromJson(readFile(bundle.value->manifestFilePath)).isObject());
        for (const auto &p : bundle.value->pngFilePaths) QVERIFY(!QImage(p).isNull());
        QString gif = dir.filePath("animation.gif");
        QVERIFY(GifCodec::saveAnimatedGif({{img, 100}, {solid(64,64,Qt::blue), 200}}, gif).isOk());
        QVERIFY(readFile(gif).startsWith("GIF89a"));
        auto frames = GifCodec::extractGifFrames(gif); QVERIFY(frames.isOk()); QCOMPARE(frames.value->size(), size_t(2));
        QCOMPARE(frames.value->at(0).image.pixelColor(0,0), QColor(Qt::red));
        QCOMPARE(frames.value->at(1).image.pixelColor(0,0), QColor(Qt::blue));
        auto exported = GifCodec::exportGifFramesToFolder(gif, dir.filePath("frames"));
        QVERIFY(exported.isOk()); QCOMPARE(exported.value->size(), 2);
    }
    void metadata() {
        QTemporaryDir dir; QVERIFY(dir.isValid());
        ImageMetadata m; m.author="Ada"; m.copyright="Copyright"; m.title="Title";
        m.description="Description"; m.captureDate="2024-01-02T03:04:05";
        m.modificationDate="2024-02-03T04:05:06"; m.cameraMakeModel="Camera";
        m.lensModel="Lens"; m.software="Photo26"; m.orientation=3; m.dpiX=144; m.dpiY=144;
        m.keywords={"one", "two"}; m.license="CC0"; m.gps=GpsCoordinate{40.4,-3.7,{},true};
        m.iccProfileBytes=QColorSpace(QColorSpace::SRgb).iccProfile(); m.colorProfileName="sRGB";
        auto json=MetadataEngine::exportToJson(m); QVERIFY(json.isOk());
        QCOMPARE(QJsonDocument::fromJson(*json.value).object().value("schema").toString(), QString("photo26.metadata.v1"));
        auto restored=MetadataEngine::importFromJson(*json.value); QVERIFY(restored.isOk());
        QCOMPARE(restored.value->author,m.author); QCOMPARE(restored.value->keywords,m.keywords);
        QCOMPARE(restored.value->iccProfileBytes,m.iccProfileBytes); QVERIFY(restored.value->gps.has_value());
        QCOMPARE(restored.value->copyright,m.copyright); QCOMPARE(restored.value->title,m.title);
        QCOMPARE(restored.value->description,m.description); QCOMPARE(restored.value->captureDate,m.captureDate);
        QCOMPARE(restored.value->modificationDate,m.modificationDate); QCOMPARE(restored.value->cameraMakeModel,m.cameraMakeModel);
        QCOMPARE(restored.value->lensModel,m.lensModel); QCOMPARE(restored.value->software,m.software);
        QCOMPARE(restored.value->orientation,m.orientation); QCOMPARE(restored.value->dpiX,m.dpiX);
        QCOMPARE(restored.value->dpiY,m.dpiY); QCOMPARE(restored.value->license,m.license);
        QCOMPARE(restored.value->gps->latitude,m.gps->latitude); QCOMPARE(restored.value->gps->longitude,m.gps->longitude);
        QCOMPARE(restored.value->colorProfileName,m.colorProfileName);
        QCOMPARE(MetadataEngine::importFromJson("{broken").error.code, ErrorCode::SchemaValidationError);
        QCOMPARE(MetadataEngine::importFromJson("{\"schema\":\"wrong\"}").error.code, ErrorCode::SchemaValidationError);
        QVERIFY(!MetadataEngine::checkFormatCompatibilityWarnings("gif", m).isEmpty());
        QStringList files;
        for (const QString &fmt : {QString("jpg"), QString("png"), QString("webp"), QString("tiff")}) {
            QString p=dir.filePath("metadata."+fmt); SaveOptions opt; opt.format=fmt;
            QVERIFY2(ImageIo::saveImage(solid(32,24,Qt::red),p,opt).isOk(), qPrintable(fmt));
            QStringList warnings; auto write=MetadataEngine::writeMetadataToFile(p,m,&warnings);
            QVERIFY2(write.isOk(), qPrintable(fmt+": "+write.error.message));
            auto inspect=MetadataEngine::inspectFile(p); QVERIFY(inspect.isOk());
            QCOMPARE(inspect.value->fileProps.width,32); QCOMPARE(inspect.value->fileProps.height,24);
            if (fmt != "tiff") {
                QCOMPARE(inspect.value->embedded.author,m.author);
                QCOMPARE(inspect.value->embedded.title,m.title);
                QCOMPARE(inspect.value->embedded.description,m.description);
                files << p;
            } else QVERIFY(!warnings.isEmpty());
        }
        QString copy=dir.filePath("copy.png"); SaveOptions opt; opt.format="png";
        QVERIFY(ImageIo::saveImage(solid(32,24,Qt::blue),copy,opt).isOk());
        QVERIFY(MetadataEngine::copyMetadataBetweenFiles(files.front(),copy).isOk());
        auto copied=MetadataEngine::inspectFile(copy); QVERIFY(copied.isOk()); QCOMPARE(copied.value->embedded.author,m.author);
        ImageMetadata patch; patch.title="Batch title";
        QVERIFY(MetadataEngine::batchUpdateMetadata(files,patch,{"title"}).isOk());
        for (const auto &p : files) { auto result=MetadataEngine::inspectFile(p); QVERIFY(result.isOk()); QCOMPARE(result.value->embedded.title,patch.title); }
        QString clean=dir.filePath("clean.png");
        QVERIFY(MetadataEngine::stripAllMetadata(copy,clean).isOk());
        auto stripped=MetadataEngine::inspectFile(clean); QVERIFY(stripped.isOk()); QVERIFY(stripped.value->embedded.author.isEmpty());
        MetadataStripOptions strip; strip.stripExif=false; strip.stripXmp=true; strip.stripIptc=false; strip.stripGps=false; strip.stripIcc=false;
        QVERIFY(MetadataEngine::stripSelectedMetadata(copy,dir.filePath("selected.png"),strip).isOk());
        QVERIFY(MetadataEngine::inspectFile(dir.filePath("selected.png")).isOk());
    }
    void colorProfiles() {
        QImage image=solid(20,20,QColor(100,150,200));
        IccManager::applyColorSpace(image,TargetColorSpace::SRgb);
        QVERIFY(IccManager::inspect(image).hasProfile);
        for (auto target : {TargetColorSpace::SRgb,TargetColorSpace::AdobeRgb,TargetColorSpace::DisplayP3,TargetColorSpace::ProPhotoRgb}) {
            auto converted=IccManager::convertToColorSpace(image,target);
            QVERIFY(converted.isOk()); QVERIFY(converted.value->colorSpace().isValid());
            QCOMPARE(converted.value->size(),image.size());
        }
    }
    void appleAndHeifFormats() {
        QVERIFY(HeifCodec::isAvailable());
        QVERIFY(HeifCodec::canReadHeic());
        QVERIFY(HeifCodec::canWriteHeic());
        QVERIFY(HeifCodec::canReadAvif());
        QVERIFY(HeifCodec::canWriteAvif());

        QCOMPARE(FormatRegistry::detectFormatFromHeader("icns\0\0\0\x08", "icon.icns"), QString("icns"));
        QCOMPARE(FormatRegistry::detectFormatFromHeader(QByteArray("II*\0", 4), "IMG_0001.DNG"), QString("dng"));

        QTemporaryDir dir; QVERIFY(dir.isValid());
        QImage alphaImg = solid(64, 64, QColor(45, 95, 138, 180));
        alphaImg.setColorSpace(QColorSpace(QColorSpace::SRgb));

        // 1. Round-trip HEIC, HEIF, AVIF, and Apple ICNS (preserving size & alpha)
        for (const QString& fmt : {QString("heic"), QString("heif"), QString("avif"), QString("icns")}) {
            SaveOptions opt;
            opt.format = fmt;
            opt.quality = 85;
            opt.preserveTransparency = true;
            const QString outPath = dir.filePath("apple_test." + fmt);
            auto saved = ImageIo::saveImage(alphaImg, outPath, opt);
            QVERIFY2(saved.isOk(), qPrintable(fmt + ": " + saved.error.message));
            QVERIFY(QFileInfo(outPath).size() > 0);
            auto loaded = ImageIo::loadImage(outPath);
            QVERIFY2(loaded.isOk(), qPrintable(fmt + " load: " + loaded.error.message));
            QVERIFY(!loaded.value->image.isNull());
            if (fmt != "icns") {
                QCOMPARE(loaded.value->image.size(), alphaImg.size());
                QVERIFY(loaded.value->image.hasAlphaChannel());
            }
        }

        // 2. Metadata EXIF + XMP + ICC round-trip and stripping on HEIC and AVIF
        ImageMetadata meta;
        meta.author = "Photo26 Apple Test";
        meta.title = "iPhone Shot";
        meta.description = "HEIC/AVIF metadata round-trip";
        meta.copyright = "Copyright Photo26";
        meta.gps = GpsCoordinate{40.4168, -3.7038, {}, true};
        meta.iccProfileBytes = QColorSpace(QColorSpace::SRgb).iccProfile();
        meta.colorProfileName = "sRGB";
        for (const QString& fmt : {QString("heic"), QString("avif")}) {
            const QString p = dir.filePath("meta_apple." + fmt);
            SaveOptions opt; opt.format = fmt; opt.quality = 90; opt.preserveMetadata = true; opt.preserveIccProfile = true;
            QVERIFY2(ImageIo::saveImage(alphaImg, p, opt, &meta).isOk(), qPrintable(fmt));
            auto inspected = MetadataEngine::inspectFile(p);
            QVERIFY2(inspected.isOk(), qPrintable(fmt + " inspect: " + inspected.error.message));
            QCOMPARE(inspected.value->embedded.author, meta.author);
            QCOMPARE(inspected.value->embedded.title, meta.title);
            QVERIFY(inspected.value->embedded.gps.has_value());
            QVERIFY(inspected.value->embedded.hasIcc);

            const QString clean = dir.filePath("clean_apple." + fmt);
            QVERIFY2(MetadataEngine::stripAllMetadata(p, clean).isOk(), qPrintable(fmt + " strip"));
            auto afterStrip = MetadataEngine::inspectFile(clean);
            QVERIFY(afterStrip.isOk());
            QVERIFY(afterStrip.value->embedded.author.isEmpty());
            QVERIFY(!afterStrip.value->embedded.gps.has_value());
        }

        // 3. Compression, Web Optimizer, Mass Rename {width}x{height}, and Batch on HEIC -> AVIF
        const QString srcHeic = dir.filePath("iphone_photo.heic");
        SaveOptions heicOpt; heicOpt.format = "heic"; heicOpt.quality = 85;
        QVERIFY(ImageIo::saveImage(solid(128, 96, QColor(180, 90, 40)), srcHeic, heicOpt).isOk());

        CompressionOptions compOpt; compOpt.format = "heic"; compOpt.quality = 75;
        auto compHeic = CompressWebOp::compressInMemory(alphaImg, compOpt);
        QVERIFY2(compHeic.isOk(), qPrintable(compHeic.error.message));
        QVERIFY(compHeic.value->compressedBytes > 0);

        WebOptimizerOptions webOpt; webOpt.format = "avif"; webOpt.widths = {32, 64}; webOpt.quality = 75;
        auto avifVariants = CompressWebOp::generateWebVariants(alphaImg, "hero.heic", webOpt);
        QVERIFY2(avifVariants.isOk(), qPrintable(avifVariants.error.message));
        QCOMPARE(avifVariants.value->size(), size_t(2));

        MassRenameOptions rn; rn.pattern = "apple-{width}x{height}";
        auto rnPrev = MassRenameOp::previewRename({srcHeic}, rn, dir.filePath("renamed"));
        QCOMPARE(rnPrev.size(), size_t(1));
        QCOMPARE(rnPrev[0].proposedName, QString("apple-128x96.heic"));

        BatchPreset bp; bp.outputFormat = "avif"; bp.quality = 75; bp.enableResize = true; bp.resizeOptions.targetWidth = 64;
        auto batchRes = BatchProcessor::runBatch({srcHeic}, dir.filePath("batch_apple"), bp, false);
        QCOMPARE(batchRes.succeeded, 1);
        QVERIFY(QFileInfo(batchRes.itemResults[0].outputPath).exists());

        // 4. Apple ProRAW / DNG (TIFF container) reading via ImageIo
        const QString dngPath = dir.filePath("proraw_sample.dng");
        QVERIFY(solid(80, 60, QColor(30, 140, 90)).save(dngPath, "TIFF"));
        auto dngLoaded = ImageIo::loadImage(dngPath);
        QVERIFY2(dngLoaded.isOk(), qPrintable(dngLoaded.error.message));
        QCOMPARE(dngLoaded.value->detectedFormat, QString("dng"));
        QCOMPARE(dngLoaded.value->image.size(), QSize(80, 60));

        // 5. MainWindow loading HEIC and exporting to AVIF & ICNS
        MainWindow window;
        QVERIFY(window.loadFilesIntoWorkspace({srcHeic}));
        window.activateTool("convert");
        window.applyCurrentToolPreview();
        SaveOptions avifOut; avifOut.format = "avif"; avifOut.quality = 80;
        auto expAvif = window.exportSingleProcessedToFile(dir.filePath("from_ui.avif"), avifOut);
        QVERIFY2(expAvif.isOk(), qPrintable(expAvif.error.message));
        SaveOptions icnsOut; icnsOut.format = "icns";
        auto expIcns = window.exportSingleProcessedToFile(dir.filePath("from_ui.icns"), icnsOut);
        QVERIFY2(expIcns.isOk(), qPrintable(expIcns.error.message));
    }
    void widgetsAndWindow() {
        ToolCardGridWidget grid; auto tools=ToolCardGridWidget::allTools(); QCOMPARE(tools.size(),size_t(19));
        for (const auto &tool : tools) QVERIFY(ToolCardGridWidget::findTool(tool.id).has_value());
        grid.setSearchFilter("resize"); QCOMPARE(grid.searchFilter(),QString("resize"));
        grid.refreshIconsForTheme(AppTheme::LightClean);
        QVERIFY(!ThemeManager::buildStyleSheet(AppTheme::DarkModern).isEmpty());
        InteractiveCanvasWidget canvas; QImage img=solid(64,48,Qt::red);
        canvas.setOriginalImage(img); canvas.setPreviewImage(img); QVERIFY(canvas.hasImage());
        canvas.setSplitComparisonEnabled(true); QVERIFY(canvas.isSplitComparisonEnabled());
        canvas.setCropOverlayEnabled(true); canvas.setCropRectImageCoords(QRect(2,3,20,10));
        QCOMPARE(canvas.cropRectImageCoords(), QRect(2,3,20,10)); canvas.clearImages(); QVERIFY(!canvas.hasImage());
        ExportBarWidget bar; bar.setCurrentToolSuffix("recortada");
        QCOMPARE(bar.currentToolSuffix(),QString("recortada"));
        QCOMPARE(ExportBarWidget::suggestDescriptiveFileName("foto.png","recortada","png"),QString("foto_recortada.png"));
        QTemporaryDir dir; QVERIFY(dir.isValid()); QString first=dir.filePath("first.png"), second=dir.filePath("second.png");
        QVERIFY(img.save(first)); QVERIFY(solid(64,48,Qt::blue).save(second));
        MainWindow window; QVERIFY(window.homeGridWidget()); QVERIFY(window.canvasWidget()); QVERIFY(window.exportBarWidget());
        for (const auto &tool : tools) { window.activateTool(tool.id); QCOMPARE(window.activeToolId(),tool.id); QVERIFY(!window.isOnHomeScreen()); }
        window.showHomeScreen(); QVERIFY(window.isOnHomeScreen());
        QVERIFY(window.loadFilesIntoWorkspace({first,second})); QCOMPARE(window.loadedFilePaths().size(),2);
        window.activateTool("convert"); window.applyCurrentToolPreview();
        SaveOptions opt; opt.format="png";
        auto single=window.exportSingleProcessedToFile(dir.filePath("result.png"),opt);
        QVERIFY2(single.isOk(),qPrintable(single.error.message)); QVERIFY(!QImage(dir.filePath("result.png")).isNull());
        BatchFolderSaveRequest request; request.targetDirectory=dir.filePath("out"); request.saveOptions=opt;
        auto folder=window.exportMultipleProcessedToFolder(request); QVERIFY2(folder.isOk(),qPrintable(folder.error.message));
        auto zip=window.exportMultipleProcessedToZip(dir.filePath("out.zip"),opt); QVERIFY2(zip.isOk(),qPrintable(zip.error.message));
        QVERIFY(readFile(dir.filePath("out.zip")).startsWith(QByteArray::fromHex("504b0304")));
        QString report; QVERIFY2(window.runSelfTestRoutine(&report),qPrintable(report));
    }
};
QTEST_MAIN(Photo26TestSuite)
#include "test_photo26_all.moc"
