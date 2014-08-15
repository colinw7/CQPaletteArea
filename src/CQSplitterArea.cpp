#include <CQSplitterArea.h>
#include <CQPaletteArea.h>

#include <QMainWindow>
#include <QSplitter>
#include <QStylePainter>
#include <QStyleOption>
#include <QMouseEvent>

#include <cassert>
#include <iostream>

CQSplitterArea::
CQSplitterArea(CQPaletteArea *palette) :
 QWidget(0), palette_(palette), dockArea_(Qt::LeftDockWidgetArea),
 resizable_(true), floating_(false)
{
  splitter_ = new QSplitter(this);

  splitter_->setObjectName("splitter");

  handle_ = new CQSplitterHandle(this);

  handle_->setObjectName(QString("%1_handle").arg(palette_->objectName()));

  if (isVisible())
    updateLayout();
}

void
CQSplitterArea::
setDockArea(Qt::DockWidgetArea dockArea)
{
  dockArea_ = dockArea;

  if (isVerticalDockArea())
    splitter()->setOrientation(Qt::Vertical);
  else
    splitter()->setOrientation(Qt::Horizontal);

  handle_->updateState();

  if (isVisible())
    updateLayout();
}

void
CQSplitterArea::
setResizable(bool resize)
{
  resizable_ = resize;

  if (isVisible())
    updateLayout();
}

void
CQSplitterArea::
setFloating(bool floating)
{
  floating_ = floating;

  if (isVisible())
    updateLayout();
}

void
CQSplitterArea::
showEvent(QShowEvent *)
{
  updateLayout();
}

void
CQSplitterArea::
updateLayout()
{
  QMainWindow *mw = palette()->mgr()->window();

  if (mw)
    handle_->setParent(mw);

  handle_->setVisible(mw && palette_->isVisible() && isResizable() && ! isFloating());

  if (handle_->isVisible()) {
    QWidget *cw = mw->centralWidget();

//std::cerr << "CW: " << cw->x() << " " << cw->y() << " " << cw->width() << " " << cw->height() << std::endl;

    int hs = (isVerticalDockArea() ? handle_->width() : handle_->height());

    if      (dockArea() == Qt::LeftDockWidgetArea) {
      handle_->move  (cw->x() - hs, cw->y());
      handle_->resize(hs, height());
    }
    else if (dockArea() == Qt::RightDockWidgetArea) {
      handle_->move  (cw->x() + cw->width(), cw->y());
      handle_->resize(hs, height());
    }
    else if (dockArea() == Qt::TopDockWidgetArea) {
      handle_->move  (0, cw->y() - hs);
      handle_->resize(width(), hs);
    }
    else if (dockArea() == Qt::BottomDockWidgetArea) {
      handle_->move  (0, cw->y() + cw->height());
      handle_->resize(width(), hs);
    }

    handle_->raise();
  }

  splitter_->move(0, 0);
  splitter_->resize(width(), height());
}

//------

CQSplitterHandle::
CQSplitterHandle(CQSplitterArea *area) :
 QWidget(0), area_(area)
{
  setObjectName("handle");

  setFixedHeight(5);

  setCursor(Qt::SplitHCursor);
}

void
CQSplitterHandle::
updateState()
{
  if (area_->isVerticalDockArea()) {
    setFixedWidth(5);
    setMinimumHeight(0); setMaximumHeight(QWIDGETSIZE_MAX);

    setCursor(Qt::SplitHCursor);
  }
  else {
    setFixedHeight(5);
    setMinimumWidth(0); setMaximumWidth(QWIDGETSIZE_MAX);

    setCursor(Qt::SplitVCursor);
  }
}

void
CQSplitterHandle::
paintEvent(QPaintEvent *)
{
  QStylePainter ps(this);

  QStyleOption opt;

#if 0
  ps.fillRect(rect(), QBrush(QColor(100,100,150)));
#else
  opt.initFrom(this);

  opt.rect  = rect();
  opt.state = (! area_->isVerticalDockArea() ? QStyle::State_None : QStyle::State_Horizontal);

  if (mouseState_.pressed)
    opt.state |= QStyle::State_Sunken;

  if (mouseOver_)
    opt.state |= QStyle::State_MouseOver;

  ps.drawControl(QStyle::CE_Splitter, opt);
#endif
}

void
CQSplitterHandle::
mousePressEvent(QMouseEvent *e)
{
  mouseState_.pressed  = true;
  mouseState_.pressPos = e->globalPos();

  update();
}

void
CQSplitterHandle::
mouseMoveEvent(QMouseEvent *e)
{
  if (! mouseState_.pressed) return;

  if (area_->isVerticalDockArea()) {
    int dx = e->globalPos().x() - mouseState_.pressPos.x();

    if (dx) {
      if (area()->palette()->moveSplitter(dx))
        mouseState_.pressPos = e->globalPos();
    }
  }
  else {
    int dy = e->globalPos().y() - mouseState_.pressPos.y();

    if (dy) {
      if (area()->palette()->moveSplitter(dy))
        mouseState_.pressPos = e->globalPos();
    }
  }

  update();
}

void
CQSplitterHandle::
mouseReleaseEvent(QMouseEvent *)
{
  mouseState_.pressed = false;

  update();
}

void
CQSplitterHandle::
enterEvent(QEvent *)
{
  mouseOver_ = true;

  update();
}

void
CQSplitterHandle::
leaveEvent(QEvent *)
{
  mouseOver_ = false;

  update();
}
