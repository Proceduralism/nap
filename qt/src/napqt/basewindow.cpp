#include "basewindow.h"

#include <QApplication>

#include "autosettings.h"

using namespace nap::qt;

BaseWindow::BaseWindow()
{
	setWindowTitle(QApplication::applicationName());
	setDockNestingEnabled(true);
	mWindowMenu = new QMenu("Window");
	menuBar()->addMenu(mWindowMenu);
}

QDockWidget* BaseWindow::addDock(const QString& name, QWidget* widget, Qt::DockWidgetArea area)
{
	QDockWidget* dock = new QDockWidget(this);
	dock->setObjectName(name);
	dock->setWidget(widget);
	dock->setWindowTitle(name);

	if (widget->objectName().isEmpty())
		widget->setObjectName(QString("%1_Widget").arg(name));

	mWindowMenu->addAction(dock->toggleViewAction());
	addDockWidget(area, dock);
	return dock;
}

void BaseWindow::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	AutoSettings::get().restore(*this);
}

void BaseWindow::closeEvent(QCloseEvent* event)
{
	AutoSettings::get().store(*this);
	QWidget::closeEvent(event);
}

