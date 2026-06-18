#include "MainWindow.h"
#include <QFileInfo>
#include "messagebox.h"

using namespace std;
using namespace urobots::qt_helpers;

#define BORDERS_FOLDER "TB1"
#define MAP_0_FOLDER "TB2"

void MainWindow::OpenBorders() {
    if (!CloseActiveProject()) {
        return;
    }

    // select folder with files
    auto files_folder = QSettings().value(BORDERS_FOLDER).toString();
    QFileDialog dialog(this, tr("Select folder with images"), files_folder);
    dialog.setFileMode(QFileDialog::ExistingFiles);

    //dialog.setOption(QFileDialog::DontUseNativeDialog);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    auto selected_files = dialog.selectedFiles();

    files_folder = QFileInfo(selected_files.first()).absolutePath();
    QSettings().setValue(BORDERS_FOLDER, files_folder);

    model_.OpenBorders(selected_files, files_folder);
}

void MainWindow::SaveBorders() {
    auto category = ui.definition_editor->GetSelectedCategory();
    if (!category) {
        messagebox::Critical("Please select country category");
        return;
    }

    auto dir_value = QSettings().value(BORDERS_FOLDER).toString();
    auto filename = QFileDialog::getSaveFileName(
        this,
        tr("Save border"),
        dir_value,
        tr("All files (*.*);;JSON files (*.json)"));

    model_.SaveBorders(filename, category);
}

void MainWindow::Open0Map() {
    auto dir_value = QSettings().value(MAP_0_FOLDER).toString();
    auto filename = QFileDialog::getOpenFileName(
        this,
        tr("Open 0 map"),
        dir_value,
        tr("All files (*.*);;JSON files (*.json)"));

    if (!filename.isNull()) {
        dir_value = QFileInfo(filename).absoluteDir().path();
        QSettings().setValue(MAP_0_FOLDER, filename);
        model_.Open0Map(filename);
    }
}

void MainWindow::Save0Map() {
    auto dir_value = QSettings().value(MAP_0_FOLDER).toString();
    auto filename = QFileDialog::getSaveFileName(
        this,
        tr("Save project as"),
        dir_value % "/" + "1",
        tr("JSON files (*.json);;All Files (*.*)"));

    if (filename.isNull())
        return;

    QSettings().setValue(MAP_0_FOLDER, QFileInfo(filename).absolutePath());
    model_.Save0Map(filename);
}

void MainWindow::RemoveAllLabelsExceptX() {
    for (auto i: model_.get_label_definitions()->GetDefinitions()) {
        std::vector<std::shared_ptr<LabelCategory>> categories = i->categories;
        for (auto j: categories) {
            if (!j->get_name().startsWith("X")) {
                model_.Delete(j, false);
            }
        }
    }
}

