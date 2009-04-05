#include "mainwindow.h"
#include "portgrouplist.h"

#include "ui_about.h"

PortGroupList	*pgl;

MainWindow::MainWindow(QWidget *parent) 
	: QMainWindow (parent)
{
	pgl = new PortGroupList;

	portsWindow = new PortsWindow(pgl, this);
	statsWindow = new PortStatsWindow(pgl, this);
	portsDock = new QDockWidget(tr("Ports"), this);
	statsDock = new QDockWidget(tr("Stats"), this);

	setupUi(this);

	statsDock->setWidget(statsWindow);
    addDockWidget(Qt::BottomDockWidgetArea, statsDock);
	portsDock->setWidget(portsWindow);
    addDockWidget(Qt::TopDockWidgetArea, portsDock);

	connect(actionFileExit, SIGNAL(triggered()), this, SLOT(close()));
}

MainWindow::~MainWindow()
{
	delete statsDock;
	delete portsDock;
	delete statsWindow;
	delete portsWindow;
}

void MainWindow::on_actionHelpAbout_triggered()
{
	QDialog *aboutDialog = new QDialog;

	Ui::About about;
	about.setupUi(aboutDialog);

	aboutDialog->exec();

	delete aboutDialog;
}

