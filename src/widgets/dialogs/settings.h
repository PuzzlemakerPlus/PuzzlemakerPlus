// settings.h
// Declares the settings dialog. (Currently, this just lists loaded packages.)
#pragma once
#include <qdialog.h>
#include <QListWidget>

#include "../../package/package_loader.h"

namespace PMP
{
class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(const std::vector<std::unique_ptr<Package>>& packageList, QWidget* parent = nullptr);


};
}