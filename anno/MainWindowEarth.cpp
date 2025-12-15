#include "MainWindow.h"

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
    dialog.setFileMode(QFileDialog::Directory);
    //dialog.setOption(QFileDialog::DontUseNativeDialog);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    files_folder = dialog.selectedFiles().first();
    QSettings().setValue(BORDERS_FOLDER, files_folder);

    model_.OpenBorders(files_folder);
}

void MainWindow::SaveBorders() {
    /*
    auto def = ui.definition_editor->GetDefinition();
    if (!def) {
        messagebox::Critical("Please select marker type");
        return;
    }

    auto files_folder = QSettings().value(TB_1_IMAGE_FOLDER).toString();
    model_.SaveBorders(files_folder, def);
   */
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
