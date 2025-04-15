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

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "ConfigurationModel.h"
#include "ConfigurationSpaceScene.h"
#include "ConfigurationSpaceThread.h"
#include "MainWindow.h"
#include "Thread.h"
#include "Viewer.h"

ConfigurationSpaceScene::ConfigurationSpaceScene(QObject* parent) :
	QGraphicsScene(parent),
	axis(),
	delta(),
	maximum(),
	minimum(),
	model(nullptr),
	range(),
	steps(),
	collisions(true),
	configuration(nullptr),
	data(),
	edges(nullptr),
	image(),
	path(nullptr),
	thread(new ConfigurationSpaceThread(this))
{
	this->axis[0] = 0;
	this->axis[1] = 1;
	this->delta[0] = 1;
	this->delta[1] = 1;
	
	this->thread->scene = this;
	
	this->configuration = this->addEllipse(-3, -3, 6, 6, QPen(Qt::NoPen), QBrush((QColor(247, 127, 7))));
	this->configuration->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	this->configuration->setVisible(false);
	this->configuration->setZValue(4);
	
	this->edges = this->createItemGroup(QList<QGraphicsItem*>());
	this->edges->setZValue(2);
	
	QPen pathPen(QColor(55, 176, 55), 2);
	pathPen.setCosmetic(true);
	this->path = this->addPath(QPainterPath(), pathPen);
	this->path->setZValue(3);
	
	QObject::connect(
		this->thread,
		SIGNAL(addCollision(const int&, const int&, const unsigned char&)),
		this,
		SLOT(addCollision(const int&, const int&, const unsigned char&))
	);
	
	QObject::connect(this->thread, SIGNAL(finished()), this, SIGNAL(evalFinished()));
}

ConfigurationSpaceScene::~ConfigurationSpaceScene()
{
	this->thread->stop();
}

void
ConfigurationSpaceScene::addCollision(const int& x, const int& y, const unsigned char& rgb)
{
	this->data[y * this->steps[0] + x] = rgb;
	this->invalidate(
		this->minimum[0] + x * this->delta[0],
		this->minimum[1] + y * this->delta[1],
		this->delta[0],
		this->delta[1],
		QGraphicsScene::BackgroundLayer
	);
}

void
ConfigurationSpaceScene::clear()
{
	this->reset();
	this->resetCollisions();
}

void
ConfigurationSpaceScene::drawBackground(QPainter* painter, const QRectF& rect)
{
	painter->fillRect(rect, this->palette().color(QPalette::Window));
	
	if (this->collisions)
	{
		QRectF target = rect.intersected(this->sceneRect());
		QRectF source(
			(target.x() - this->minimum[0]) / this->delta[0],
			(target.y() - this->minimum[1]) / this->delta[1],
			target.width() / this->delta[0],
			target.height() / this->delta[1]
		);
		painter->drawImage(target, image, source, Qt::NoFormatConversion);
	}
}

void
ConfigurationSpaceScene::drawConfiguration(const rl::math::Vector& q)
{
	this->configuration->setPos(
		q(this->axis[0]),
		q(this->axis[1])
	);
}

void
ConfigurationSpaceScene::drawConfigurationEdge(const rl::math::Vector& u, const rl::math::Vector& v, const bool& free)
{
	QGraphicsLineItem* line = this->addLine(
		u(this->axis[0]),
		u(this->axis[1]),
		v(this->axis[0]),
		v(this->axis[1]),
		QPen(QBrush(QColor(96, 96, 96)), 0)
	);
	
	this->edges->addToGroup(line);
}

void
ConfigurationSpaceScene::drawConfigurationVertex(const rl::math::Vector& q, const bool& free)
{
}

void
ConfigurationSpaceScene::drawConfigurationPath(const rl::plan::VectorList& path)
{
	this->resetPath();
	
	if (path.empty())
	{
		return;
	}
	
	QPainterPath painterPath;
	painterPath.moveTo(path.front()(this->axis[0]), path.front()(this->axis[1]));
	
	for (rl::plan::VectorList::const_iterator i = ++path.begin(); i != path.end(); ++i)
	{
		painterPath.lineTo((*i)(this->axis[0]), (*i)(this->axis[1]));
	}
	
	this->path->setPath(painterPath);
}

void
ConfigurationSpaceScene::drawLine(const rl::math::Vector& xyz0, const rl::math::Vector& xyz1)
{
}

void
ConfigurationSpaceScene::drawPoint(const rl::math::Vector& xyz)
{
}

void
ConfigurationSpaceScene::drawSphere(const rl::math::Vector& center, const rl::math::Real& radius)
{
}

void
ConfigurationSpaceScene::drawSweptVolume(const rl::plan::VectorList& path)
{
}

void
ConfigurationSpaceScene::drawWork(const rl::math::Transform& t)
{
}

void
ConfigurationSpaceScene::drawWorkEdge(const rl::math::Vector& u, const rl::math::Vector& v)
{
}

void
ConfigurationSpaceScene::drawWorkPath(const rl::plan::VectorList& path)
{
}

void
ConfigurationSpaceScene::drawWorkVertex(const rl::math::Vector& q)
{
}

void
ConfigurationSpaceScene::eval()
{
	if (nullptr == this->model)
	{
		return;
	}
	
	if (this->model->getDofPosition() < 2)
	{
		return;
	}
	
	this->resetCollisions();
	this->thread->start();
}

void
ConfigurationSpaceScene::init()
{
	this->clear();
	
	if (nullptr == this->model)
	{
		return;
	}
	
	if (this->model->getDofPosition() < 2)
	{
		return;
	}
	
	rl::math::Vector maximum = this->model->getMaximum();
	rl::math::Vector minimum = this->model->getMinimum();
	
	this->maximum[0] = maximum(this->axis[0]);
	this->maximum[1] = maximum(this->axis[1]);
	
	this->minimum[0] = minimum(this->axis[0]);
	this->minimum[1] = minimum(this->axis[1]);
	
	this->range[0] = std::abs(this->maximum[0] - this->minimum[0]);
	this->range[1] = std::abs(this->maximum[1] - this->minimum[1]);
	
	this->steps[0] = static_cast<int>(std::ceil(this->range[0] / this->delta[0]));
	this->steps[1] = static_cast<int>(std::ceil(this->range[1] / this->delta[1]));
	
	this->configuration->setPos(
		(*MainWindow::instance()->q)(this->axis[0]),
		(*MainWindow::instance()->q)(this->axis[1])
	);
	
	this->data.assign(static_cast<std::size_t>(this->steps[0]) * this->steps[1], 128);
	
#if QT_VERSION >= 0x050500
	this->image = QImage(this->data.data(), this->steps[0], this->steps[1], this->steps[0], QImage::Format_Grayscale8);
#else
	this->image = QImage(this->data.data(), this->steps[0], this->steps[1], this->steps[0], QImage::Format_Indexed8);
	QVector<QRgb> colors;
	colors.reserve(256);
	for (int i = 0; i < 256; ++i)
	{
		colors.push_back(qRgb(i, i, i));
	}
	this->image.setColorTable(colors);
#endif
	
	this->setSceneRect(this->minimum[0], this->minimum[1], this->range[0], this->range[1]);
}

void
ConfigurationSpaceScene::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	this->mousePressEvent(mouseEvent);
}

void
ConfigurationSpaceScene::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	if (nullptr == this->model)
	{
		return;
	}
	
	if (this->model->getDofPosition() < 2)
	{
		return;
	}
	
	if (Qt::LeftButton == mouseEvent->buttons())
	{
		if (!MainWindow::instance()->thread->isRunning())
		{
			qreal x = qBound(this->minimum[0], mouseEvent->scenePos().x(), this->maximum[0]);
			qreal y = qBound(this->minimum[1], mouseEvent->scenePos().y(), this->maximum[1]);
			
			(*MainWindow::instance()->q)(this->axis[0]) = x;
			(*MainWindow::instance()->q)(this->axis[1]) = y;
			
			MainWindow::instance()->configurationModel->invalidate();
			this->drawConfiguration(*MainWindow::instance()->q);
			MainWindow::instance()->viewer->drawConfiguration(*MainWindow::instance()->q);
		}
	}
}

void
ConfigurationSpaceScene::reset()
{
	this->thread->stop();
	this->resetEdges();
	this->resetLines();
	this->resetPath();
}

void
ConfigurationSpaceScene::resetCollisions()
{
	std::fill(this->data.begin(), this->data.end(), 128);
	this->invalidate(this->sceneRect(), QGraphicsScene::BackgroundLayer);
}

void
ConfigurationSpaceScene::resetEdges()
{
	qDeleteAll(this->edges->childItems());
}

void
ConfigurationSpaceScene::resetLines()
{
}

void
ConfigurationSpaceScene::resetPath()
{
	this->path->setPath(QPainterPath());
}

void
ConfigurationSpaceScene::resetPoints()
{
}

void
ConfigurationSpaceScene::resetSpheres()
{
}

void
ConfigurationSpaceScene::resetVertices()
{
}

void
ConfigurationSpaceScene::showMessage(const std::string& message)
{
}

void
ConfigurationSpaceScene::toggleCollisions(const bool& doOn)
{
	this->collisions = doOn;
	this->invalidate(this->sceneRect(), QGraphicsScene::BackgroundLayer);
}

void
ConfigurationSpaceScene::toggleConfiguration(const bool& doOn)
{
	this->configuration->setVisible(doOn);
}

void
ConfigurationSpaceScene::toggleConfigurationEdges(const bool& doOn)
{
	this->edges->setVisible(doOn);
}

void
ConfigurationSpaceScene::togglePathEdges(const bool& doOn)
{
	this->path->setVisible(doOn);
}
