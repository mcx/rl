//
// Copyright (c) 2009, Markus Rickert
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <QApplication>
#include <QDateTime>
#include <QHeaderView>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/Qt/SoQt.h>
#include <rl/kin/XmlFactory.h>
#include <rl/sg/XmlFactory.h>

#if QT_VERSION >= 0x060000
#include <QOpenGLWindow>
#else
#include <QGLWidget>
#endif

#include "ConfigurationDelegate.h"
#include "ConfigurationModel.h"
#include "MainWindow.h"
#include "OperationalDelegate.h"
#include "OperationalModel.h"
#include "Server.h"
#include "SoGradientBackground.h"

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags f) :
	QMainWindow(parent, f),
	configurationModels(),
	scene(),
	configurationDelegates(),
	configurationDockWidget(new QDockWidget(this)),
	configurationTabWidget(new QTabWidget(this)),
	configurationViews(),
	gradientBackground(),
	operationalDelegates(),
	operationalDockWidget(new QDockWidget(this)),
	operationalTabWidget(new QTabWidget(this)),
	operationalViews(),
	saveImageWithAlphaAction(new QAction(this)),
	saveImageWithoutAlphaAction(new QAction(this)),
	saveSceneAction(new QAction(this)),
	server(new Server(this)),
	viewer(nullptr)
{
	MainWindow::singleton = this;
	
	SoQt::init(this);
	SoDB::init();
	SoGradientBackground::initClass();
	
#if QT_VERSION >= 0x060000
	QSurfaceFormat format;
	format.setSamples(8);
	QSurfaceFormat::setDefaultFormat(format);
#else
	QGLFormat format;
	format.setAlpha(true);
	format.setSampleBuffers(true);
	QGLFormat::setDefaultFormat(format);
#endif
	
	this->scene = std::make_shared<rl::sg::so::Scene>();
	rl::sg::XmlFactory geometryFactory;
	geometryFactory.load(QApplication::arguments()[1].toStdString(), this->scene.get());
	rl::kin::XmlFactory kinematicFactory;
	
	for (int i = 2; i < QApplication::arguments().size(); ++i)
	{
		this->geometryModels.push_back(this->scene->getModel(i - 2));
		this->kinematicModels.push_back(kinematicFactory.create(QApplication::arguments()[i].toStdString()));
	}
	
	for (std::size_t i = 0; i < this->kinematicModels.size(); ++i)
	{
		ConfigurationDelegate* configurationDelegate = new ConfigurationDelegate(this);
		configurationDelegate->id = i;
		this->configurationDelegates.push_back(configurationDelegate);
		
		ConfigurationModel* configurationModel = new ConfigurationModel(this);
		configurationModel->id = i;
		this->configurationModels.push_back(configurationModel);
		
		QTableView* configurationView = new QTableView(this);
#if QT_VERSION >= 0x050000
		configurationView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else // QT_VERSION
		configurationView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif // QT_VERSION
		configurationView->horizontalHeader()->hide();
		configurationView->setAlternatingRowColors(true);
		configurationView->setItemDelegate(configurationDelegate);
		configurationView->setModel(configurationModel);
		this->configurationViews.push_back(configurationView);
		
		this->configurationTabWidget->addTab(configurationView, QString::number(i));
		
		OperationalDelegate* operationalDelegate = new OperationalDelegate(this);
		this->operationalDelegates.push_back(operationalDelegate);
		
		OperationalModel* operationalModel = new OperationalModel(this);
		operationalModel->id = i;
		this->operationalModels.push_back(operationalModel);
		
		QTableView* operationalView = new QTableView(this);
#if QT_VERSION >= 0x050000
		operationalView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else // QT_VERSION
		operationalView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif // QT_VERSION
		operationalView->setAlternatingRowColors(true);
		operationalView->setItemDelegate(operationalDelegate);
		operationalView->setModel(operationalModel);
		this->operationalViews.push_back(operationalView);
		
		this->operationalTabWidget->addTab(operationalView, QString::number(i));
		
		QObject::connect(
			configurationModel,
			SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
			operationalModel,
			SLOT(configurationChanged(const QModelIndex&, const QModelIndex&))
		);
		
		QObject::connect(
			operationalModel,
			SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
			configurationModel,
			SLOT(operationalChanged(const QModelIndex&, const QModelIndex&))
		);
		
		rl::math::Vector q(this->kinematicModels[i]->getDof());
		q.setZero();
		configurationModel->setData(q);
	}
	
	this->configurationDockWidget->resize(160, 320);
	this->configurationDockWidget->setWidget(this->configurationTabWidget);
	this->configurationDockWidget->setWindowTitle("Configuration");
	
	this->operationalDockWidget->resize(160, 320);
	this->operationalDockWidget->setWidget(this->operationalTabWidget);
	this->operationalDockWidget->setWindowTitle("Operational");
	
	this->addDockWidget(Qt::LeftDockWidgetArea, this->configurationDockWidget);
	this->addDockWidget(Qt::BottomDockWidgetArea, this->operationalDockWidget);
	
#if QT_VERSION >= 0x050600
	QList<QDockWidget*> resizeDocksWidgets;
	resizeDocksWidgets.append(this->operationalDockWidget);
	QList<int> resizeDocksSizes;
	resizeDocksSizes.append(1);
	this->resizeDocks(resizeDocksWidgets, resizeDocksSizes, Qt::Vertical);
#endif // QT_VERSION
	
	this->gradientBackground = new SoGradientBackground();
	this->gradientBackground->ref();
	
	if (this->palette().color(QPalette::Window).lightness() < 128)
	{
		this->gradientBackground->color0.setValue(0.0f, 0.0f, 0.0f);
		this->gradientBackground->color1.setValue(0.2f, 0.2f, 0.2f);
	}
	else
	{
		this->gradientBackground->color0.setValue(0.8f, 0.8f, 0.8f);
		this->gradientBackground->color1.setValue(1.0f, 1.0f, 1.0f);
	}
	
	this->scene->root->insertChild(this->gradientBackground, 0);
	
	this->viewer = new SoQtExaminerViewer(this, nullptr, true, SoQtFullViewer::BUILD_POPUP);
	this->viewer->setSceneGraph(this->scene->root);
	this->viewer->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
	this->viewer->viewAll();
	
	this->resize(1024, 768);
	this->setCentralWidget(this->viewer->getWidget());
	this->setWindowIconText("rlCoachKin");
	this->setWindowTitle("rlCoachKin");
	
	this->init();
	
	this->server->listen(QHostAddress::Any, 11235);
}

MainWindow::~MainWindow()
{
	this->gradientBackground->unref();
	MainWindow::singleton = nullptr;
}

void
MainWindow::changeEvent(QEvent* event)
{
	if (QEvent::PaletteChange == event->type())
	{
		if (this->palette().color(QPalette::Window).lightness() < 128)
		{
			this->gradientBackground->color0.setValue(0.0f, 0.0f, 0.0f);
			this->gradientBackground->color1.setValue(0.2f, 0.2f, 0.2f);
		}
		else
		{
			this->gradientBackground->color0.setValue(0.8f, 0.8f, 0.8f);
			this->gradientBackground->color1.setValue(1.0f, 1.0f, 1.0f);
		}
	}
	
	QMainWindow::changeEvent(event);
}

MainWindow*
MainWindow::instance()
{
	if (nullptr == MainWindow::singleton)
	{
		new MainWindow();
	}
	
	return MainWindow::singleton;
}

void
MainWindow::init()
{
	this->configurationDockWidget->toggleViewAction()->setShortcut(QKeySequence("F5"));
	this->addAction(this->configurationDockWidget->toggleViewAction());
	
	this->saveImageWithoutAlphaAction->setShortcut(QKeySequence("Return"));
	QObject::connect(this->saveImageWithoutAlphaAction, SIGNAL(triggered()), this, SLOT(saveImageWithoutAlpha()));
	this->addAction(this->saveImageWithoutAlphaAction);
	
	this->saveImageWithAlphaAction->setShortcut(QKeySequence("Shift+Return"));
	QObject::connect(this->saveImageWithAlphaAction, SIGNAL(triggered()), this, SLOT(saveImageWithAlpha()));
	this->addAction(this->saveImageWithAlphaAction);
	
	this->saveSceneAction->setShortcut(QKeySequence("Ctrl+Return"));
	QObject::connect(this->saveSceneAction, SIGNAL(triggered()), this, SLOT(saveScene()));
	this->addAction(this->saveSceneAction);
}

void
MainWindow::saveImage(bool withAlpha)
{
	if (withAlpha)
	{
		this->scene->root->removeChild(this->gradientBackground);
		this->viewer->render();
	}
	
	glReadBuffer(GL_FRONT);
	
#if QT_VERSION >= 0x060000
	QImage image = this->viewer->getGLWidget()->property("SoQtGLArea").value<QOpenGLWindow*>()->grabFramebuffer();
#else
	QImage image = static_cast<QGLWidget*>(this->viewer->getGLWidget())->grabFrameBuffer(withAlpha);
#endif
	
	if (withAlpha)
	{
		this->scene->root->insertChild(this->gradientBackground, 0);
		this->viewer->render();
	}
	
	image.save("rlCoachKin-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmsszzz") + ".png", "PNG");
}

void
MainWindow::saveImageWithAlpha()
{
	this->saveImage(true);
}

void
MainWindow::saveImageWithoutAlpha()
{
	this->saveImage(false);
}

void
MainWindow::saveScene()
{
	SoOutput output;
	
	if (!output.openFile(QString("coach-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmsszzz") + ".wrl").toStdString().c_str()))
	{
		return;
	}
	
	output.setHeaderString("#VRML V2.0 utf8");
	
	SoWriteAction writeAction(&output);
	writeAction.apply(this->scene->root);
	
	output.closeFile();
}
