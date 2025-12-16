#include "ApplicationModel.h"
#include "LabelFactory.h"
#include "LocalFilesystem.h"

using namespace std;

namespace  {
bool IsClockwise(QVector<QPointF> points) {
    double sum = 0;
    for (int i = 0; i < points.size(); ++i) {
        auto p1 = points[i];
        auto p2 = (i < points.size() - 1) ? points[i + 1] : points[0];
        sum += (p2.x() - p1.x()) * (p2.y() + p1.y());
    }
    return sum > 0;
}

qreal Area(QVector<QPointF> points) {
    qreal sum = 0;
    for (int i = 0; i < points.size(); ++i) {
        auto p1 = points[i];
        auto p2 = (i < points.size() - 1) ? points[i + 1] : points[0];
        sum += p1.x() * p2.y() - p1.y() * p2.x();
    }
    sum *= 0.5;
    return sum > 0 ? sum : -sum;
}

const double world_imw = 3840;
const double world_imh = 1920;
}

void ApplicationModel::OpenBorders(QString folder) {
    ClearProject();

    pictures_path_original_ = folder;

    set_filesystem(std::make_shared<LocalFilesystem>(folder));

    auto definitions = std::vector<std::shared_ptr<LabelDefinition>>();

    // Collect all countries
    QStringList countries;

    QDirIterator it(folder);
    while (it.hasNext()) {
        QFileInfo info(it.next());
        if (info.isFile() && info.baseName().length() >= 2) {
            if (info.baseName() != "XA" && info.baseName() != "XB") continue;

            countries << info.baseName();

            auto def = make_shared<LabelDefinition>(LabelType::polygon);
            def->set_type_name(info.baseName() + "X");

            LabelDefinition::CreateCategory(def, 0, "Plus", Qt::green);
            LabelDefinition::CreateCategory(def, 1, "Minus", Qt::blue);

            definitions.push_back(def);
        }
    }

    // Add cropper definition
    auto cropper = make_shared<LabelDefinition>(LabelType::rect);
    cropper->set_type_name("CROPPER");
    LabelDefinition::CreateCategory(cropper, 0, "cropper", Qt::red);
    definitions.push_back(cropper);

    set_label_definitions(std::make_shared<LabelDefinitionsTreeModel>(this, definitions));
    connect(get_label_definitions().get(), &LabelDefinitionsTreeModel::Changed, this, &ApplicationModel::SetModified);


    auto file = GetFileModel("_.png");

    // add cropper label
    auto cropper_label = LabelFactory::CreateLabel(LabelType::rect);
    cropper_label->SetCategory(cropper->GetCategory(0));
    cropper_label->FromStringsList({"10 10 100 100"});
    file->labels_.push_back(cropper_label);

    for (auto code: countries) {
        auto def = get_label_definitions()->FindDefinition(code + "X");
        auto plus_cat = def->GetCategory(0);
        auto minus_cat = def->GetCategory(1);

        auto filename = folder + "/" + code + ".json";

        QFile jsonFile(filename);
        jsonFile.open(QFile::ReadOnly);
        auto json_content = jsonFile.readAll();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument().fromJson(json_content, &error);
        if (document.isNull()) {
            continue;
        }

        def->set_description(document.object()["name"].toString());
        auto borders = document.object()["borders"].toArray();

        for (auto border : borders) {
            auto jpoints = border.toObject()["points"].toArray();
            QVector<QPointF> points;
            for (auto jp : jpoints) {
                QPointF p(jp.toArray()[0].toDouble(), jp.toArray()[1].toDouble());

                p.setX((p.x() + 180.0) * world_imw / 360.0);
                p.setY(world_imh - (p.y() + 90.0) * world_imh / 180.0);

                points.push_back(p);
            }

            auto label = LabelFactory::CreateLabel(LabelType::polygon);
            label->SetCategory(!IsClockwise(points) ? plus_cat : minus_cat);

            QStringList points_sl;
            for (auto p: points) {
                points_sl << QString("%0 %1")
                .arg(QString::number(p.x(), 'g', 12))
                    .arg(QString::number(p.y(), 'g', 12));
            }

            label->FromStringsList(QStringList() << points_sl.join(" "));
            file->labels_.push_back(label);
        }
    }
}

void ApplicationModel::SaveBorders(QString folder, std::shared_ptr<LabelDefinition> def) {
    QJsonObject json;
    json.insert("name", def->get_description());

    auto file = GetFileModel("_.png");

    QJsonArray borders;
    for (auto label: file->labels_) {
        if (label->GetDefinition() != def) {
            continue;
        }

        QJsonArray points;
        auto text = label->ToStringsList()[0];
        QTextStream stream(&(QString&)text);
        while (!stream.atEnd()) {
            qreal x, y;
            stream >> x >> y;

            qreal _x = x * 360.0 / world_imw - 180.0;
            qreal _y = (world_imh - y) * 180.0 / world_imh - 90.0;

            QJsonArray point;
            point << _x << _y;
            points << point;
        }
        QJsonObject border;
        border.insert("points", points);
        borders << border;
    }
    json.insert("borders", borders);

    QFile jsonFile(folder + "/" + def->get_type_name() + ".json");
    if (jsonFile.open(QFile::WriteOnly)) {
        jsonFile.write(QJsonDocument(json).toJson());
    }
}

void ApplicationModel::CropBorders() {
    // find cropper
    auto file = GetFileModel("_.png");
    shared_ptr<Label> cropper_label;
    for (auto l: file->labels_) {
        if (l->GetDefinition()->get_type_name() == "CROPPER") {
            cropper_label = l;
            break;
        }
    }

    if (!cropper_label) {
        return;
    }

    auto text = cropper_label->ToStringsList()[0];
    QTextStream stream(&(QString&)text);
    qreal minx, miny, maxx, maxy;
    stream >> minx >> miny >> maxx >> maxy;
    if (minx > maxx) std::swap(minx, maxx);
    if (miny > maxy) std::swap(miny, maxy);

    vector<shared_ptr<Label>> to_delete;

    for (auto l: file->labels_) {
        if (l->GetDefinition()->get_type_name() == "CROPPER") {
            continue;
        }

        if (auto poly = dynamic_cast<PolygonLabel*>(l.get())) {
            if (!poly->CropBy(minx, miny, maxx, maxy)) {
                to_delete.push_back(l);
            }
        }
    }

    for (auto l: to_delete) {
        file->DeleteLabel(l);
    }
}

QColor randomBrightColor()
{
    int hue = QRandomGenerator::global()->bounded(360);
    int saturation = 200 + QRandomGenerator::global()->bounded(56); // 200–255
    int value = 200 + QRandomGenerator::global()->bounded(56);       // 200–255

    return QColor::fromHsv(hue, saturation, value);
}

#define USE_CATEGORIES_FOR_COUNTRIES 0

void ApplicationModel::Open0Map(QString filename) {
    ClearProject();

    pictures_path_original_ = QFileInfo(filename).absoluteDir().path();;

    set_filesystem(std::make_shared<LocalFilesystem>(pictures_path_original_));

    auto definitions = std::vector<std::shared_ptr<LabelDefinition>>();

    // open file
    QFile jsonFile(filename);
    jsonFile.open(QFile::ReadOnly);
    auto json_content = jsonFile.readAll();

    QJsonParseError error;
    QJsonDocument document = QJsonDocument().fromJson(json_content, &error);
    if (document.isNull()) {
        return;
    }

#if USE_CATEGORIES_FOR_COUNTRIES
    // Create label per country
    map<QString, shared_ptr<LabelDefinition>> defs_map;
    for (auto border: document.array()) {
        auto code = border.toObject()["code"].toString();
        if (defs_map.count(code) == 0) {
            auto def = make_shared<LabelDefinition>(LabelType::polygon);
            def->set_type_name(code);
            LabelDefinition::CreateCategory(def, 0, "", Qt::green);
            definitions.push_back(def);
            defs_map[code] = def;
        }
    }
#else
    auto borders_def = make_shared<LabelDefinition>(LabelType::polygon);
    definitions.push_back(borders_def);
    map<QString, shared_ptr<LabelCategory>> cats_map;

    // Add Taiwan (hotfix)
    cats_map["TW"] = LabelDefinition::CreateCategory(borders_def, 1, "TW", randomBrightColor());

    // Create category per country code
    for (auto border: document.array()) {
        auto code = border.toObject()["code"].toString();
        if (cats_map.count(code) == 0) {
            cats_map[code] = LabelDefinition::CreateCategory(borders_def, cats_map.size() + 1, code, randomBrightColor());
        }
    }
#endif

    // Add cropper definition
    auto cropper = make_shared<LabelDefinition>(LabelType::rect);
    cropper->set_type_name("CROPPER");
    LabelDefinition::CreateCategory(cropper, 0, "cropper", Qt::red);
    definitions.push_back(cropper);

    set_label_definitions(std::make_shared<LabelDefinitionsTreeModel>(this, definitions));
    connect(get_label_definitions().get(), &LabelDefinitionsTreeModel::Changed, this, &ApplicationModel::SetModified);

    auto file = GetFileModel("_.png");

    // add cropper label
    auto cropper_label = LabelFactory::CreateLabel(LabelType::rect);
    cropper_label->SetCategory(cropper->GetCategory(0));
    cropper_label->FromStringsList({"10 10 100 100"});
    file->labels_.push_back(cropper_label);

    // add borders
    for (auto border: document.array()) {
        auto code = border.toObject()["code"].toString();

#if USE_CATEGORIES_FOR_COUNTRIES
        if (!defs_map.count(code)) {
            continue;
        }
        auto def = defs_map[code];
        int category = 0;
#else
        if (!cats_map.count(code)) {
            continue;
        }

        auto def = borders_def;
        int category = cats_map[code]->get_value();
#endif
        auto jpoints = border.toObject()["points"].toArray();
        QVector<QPointF> points;
        for (auto jp : jpoints) {
            QPointF p(jp.toArray()[0].toDouble(), jp.toArray()[1].toDouble());

            p.setX((p.x() + 180.0) * world_imw / 360.0);
            p.setY(world_imh - (p.y() + 90.0) * world_imh / 180.0);

            points.push_back(p);
        }

        auto label = LabelFactory::CreateLabel(LabelType::polygon);
        label->SetCategory(def->GetCategory(category));

        QStringList points_sl;
        for (auto p: points) {
            points_sl << QString("%0 %1")
            .arg(QString::number(p.x(), 'g', 14))
                .arg(QString::number(p.y(), 'g', 14));
        }

        label->FromStringsList(QStringList() << points_sl.join(" "));
        file->labels_.push_back(label);
    }
}

void ApplicationModel::Save0Map(QString filename) {
    QJsonArray borders;

    auto file = GetFileModel("_.png");
    for (auto label: file->labels_) {
        if (label->GetDefinition()->get_type_name() == "CROPPER") {
            continue;
        }

        QVector<QPointF> pointsf;
        QJsonArray points;
        auto text = label->ToStringsList()[0];
        QTextStream stream(&(QString&)text);
        while (!stream.atEnd()) {
            qreal x, y;
            stream >> x >> y;

            qreal _x = x * 360.0 / world_imw - 180.0;
            qreal _y = (world_imh - y) * 180.0 / world_imh - 90.0;

            QJsonArray point;
            point << _x << _y;
            points << point;

            pointsf.push_back(QPointF(_x, _y));
        }

        QJsonObject border;
        border.insert("points", points);
#if USE_CATEGORIES_FOR_COUNTRIES
        border.insert("code", label->GetDefinition()->get_type_name());
#else
        border.insert("code", label->GetCategory()->get_name());
#endif

        qreal area = Area(pointsf);
        if (!IsClockwise(pointsf)) {
            area *= -1.0;
        }
        border.insert("area", area);

        qreal minx = pointsf[0].x();
        qreal miny = pointsf[0].y();
        qreal maxx = pointsf[0].x();
        qreal maxy = pointsf[0].y();
        for (auto p: pointsf) {
            minx = std::min<qreal>(minx, p.x());
            miny = std::min<qreal>(miny, p.y());
            maxx = std::max<qreal>(maxx, p.x());
            maxy = std::max<qreal>(maxy, p.y());
        }

        QJsonArray min_point;
        QJsonArray max_point;
        min_point << minx << miny;
        max_point << maxx << maxy;
        border.insert("min", min_point);
        border.insert("max", max_point);

        borders << border;
    }

    QFile jsonFile(filename);
    if (jsonFile.open(QFile::WriteOnly)) {
#if true
        // Compact version
        jsonFile.write(QJsonDocument(borders).toJson(QJsonDocument::Compact));
#else
        // Pretty format, for readability
        jsonFile.write(QJsonDocument(borders).toJson());
#endif
    }
}
