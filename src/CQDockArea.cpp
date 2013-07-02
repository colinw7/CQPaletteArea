#include <CQDockArea.h>

#include <QApplication>
#include <QMainWindow>
#include <QTimer>
#include <QMouseEvent>

enum { EXTRA_FLOAT_WIDTH  = 8 };
enum { EXTRA_FLOAT_HEIGHT = 8 };

CQDockArea::
CQDockArea(QMainWindow *mainWindow) :
 QDockWidget(), mainWindow_(mainWindow), dragResize_(false), resizeTimeout_(10),
 resizeTimer_(0), ignoreSize_(false), oldMinSize_(), oldMaxSize_()
{
  setObjectName("dockArea");

  setFocusPolicy(Qt::NoFocus);

  // create timer to resize palette
  resizeTimer_ = new QTimer(this);

  resizeTimer_->setSingleShot(true);

  connect(resizeTimer_, SIGNAL(timeout()), this, SLOT(resetMinMaxSizes()));
}

void
CQDockArea::
setWidget(QWidget *w)
{
  QDockWidget::setWidget(w);

  updateWidgetTitle();
}

void
CQDockArea::
updateWidgetTitle()
{
  setWindowTitle(widget()->windowTitle());
  setWindowIcon (widget()->windowIcon ());
}

void
CQDockArea::
setDockWidth(int width)
{
  // try drag (no animate)
  if (dragResize())
    dragToDockWidth(width);

  bool oldIgnoreSize = setIgnoreSize(true);

  //------

  // set palette width by setting min or max width (which makes it fixed sized)
  // and use timer to reset to min/max values after main window has finished its
  // layout processing (which would otherwise has reset this width)
  oldMinSize_ = this->minimumSize();
  oldMaxSize_ = this->maximumSize();

  if (this->isFloating())
    width += EXTRA_FLOAT_WIDTH;

  if (this->width() == width) {
    (void) setIgnoreSize(oldIgnoreSize);
    return;
  }

  if (this->width() < width)
    this->setMinimumWidth(width);
  else
    this->setMaximumWidth(width);

  (void) setIgnoreSize(oldIgnoreSize);

  resizeTimer_->start(resizeTimeOut());
}

void
CQDockArea::
setDockHeight(int height)
{
  // try drag (no animate)
  if (dragResize())
    dragToDockHeight(height);

  bool oldIgnoreSize = setIgnoreSize(true);

  //------

  // set palette height by setting min or max height (which makes it fixed sized)
  // and use timer to reset to min/max values after main window has finished its
  // layout processing (which would otherwise has reset this height)
  oldMinSize_ = this->minimumSize();
  oldMaxSize_ = this->maximumSize();

  if (this->isFloating())
    height += EXTRA_FLOAT_HEIGHT;

  if (this->height() == height) {
    (void) setIgnoreSize(oldIgnoreSize);
    return;
  }

  if (this->height() < height)
    this->setMinimumHeight(height);
  else
    this->setMaximumHeight(height);

  resizeTimer_->start(resizeTimeOut());
}

void
CQDockArea::
resetMinMaxSizes()
{
  this->setMinimumSize(oldMinSize_);
  this->setMaximumSize(oldMaxSize_);

  //resetWidgetMinMaxWidth (this);
  //resetWidgetMinMaxHeight(this);

  //resetWidgetMinMaxWidth (this->widget());
  //resetWidgetMinMaxHeight(this->widget());
}

//------

void
CQDockArea::
dragToDockWidth(int width)
{
  // calc delta
  if (this->isFloating())
    width += EXTRA_FLOAT_WIDTH;

  int dw = width - this->width();
  if (! dw) return;

  //------

  Qt::DockWidgetArea dockArea = mainWindow_->dockWidgetArea(this);

  resetWidgetMinMaxWidth(this);
  resetWidgetMinMaxWidth(this->widget());

  if (! this->isFloating()) {
    QWidget *w = mainWindow_->centralWidget();
    if (! w) return;

    // calculate start point (inside splitter)
    QPoint startPos(0,0);

    if      (dockArea == Qt::LeftDockWidgetArea)
      startPos = w->mapToGlobal(w->rect().bottomLeft()) - QPoint(2,2);
    else if (dockArea == Qt::RightDockWidgetArea)
      startPos = w->mapToGlobal(w->rect().topRight  ()) + QPoint(2,2);
    else
      return;

    startPos = mainWindow_->mapFromGlobal(startPos);

    // calculate end point (startPoint + delta)
    QPoint endPos;

    if (dockArea == Qt::LeftDockWidgetArea)
      endPos = startPos + QPoint( dw,0);
    else
      endPos = startPos + QPoint(-dw,0);

    // build events
    QMouseEvent pressEvent  (QEvent::MouseButtonPress  , startPos, Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);
    QMouseEvent motionEvent (QEvent::MouseMove         , endPos  , Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, endPos  , Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);

    // send events
    bool oldIgnoreSize = setIgnoreSize(true);

    QApplication::sendEvent(mainWindow_, &pressEvent  );
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::sendEvent(mainWindow_, &motionEvent );
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::sendEvent(mainWindow_, &releaseEvent);
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    (void) setIgnoreSize(oldIgnoreSize);
  }
  else {
    QRect rect = this->geometry();

    if      (dockArea == Qt::LeftDockWidgetArea)
      rect.adjust(0,0,dw,0);
    else if (dockArea == Qt::RightDockWidgetArea)
      rect.adjust(-dw,0,0,0);
    else
      return;

    bool oldIgnoreSize = setIgnoreSize(true);

    this->setGeometry(rect);

    (void) setIgnoreSize(oldIgnoreSize);
  }
}

void
CQDockArea::
dragToDockHeight(int height)
{
  // calc delta
  if (this->isFloating())
    height += EXTRA_FLOAT_HEIGHT;

  int dh = height - this->height();
  if (! dh) return;

  //------

  Qt::DockWidgetArea dockArea = mainWindow_->dockWidgetArea(this);

  resetWidgetMinMaxHeight(this);
  resetWidgetMinMaxHeight(this->widget());

  if (! this->isFloating()) {
    QWidget *w = mainWindow_->centralWidget();
    if (! w) return;

    // calculate start point (inside splitter)
    QPoint startPos(0,0);

    if      (dockArea == Qt::BottomDockWidgetArea)
      startPos = w->mapToGlobal(w->rect().bottomLeft()) + QPoint(2,2);
    else if (dockArea == Qt::TopDockWidgetArea)
      startPos = w->mapToGlobal(w->rect().topRight  ()) - QPoint(2,2);
    else
      return;

    startPos = mainWindow_->mapFromGlobal(startPos);

    // calculate end point (startPoint + delta)
    QPoint endPos;

    if (dockArea == Qt::BottomDockWidgetArea)
      endPos = startPos + QPoint(0,-dh);
    else
      endPos = startPos + QPoint(0, dh);

    // build events
    QMouseEvent pressEvent  (QEvent::MouseButtonPress  , startPos, Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);
    QMouseEvent motionEvent (QEvent::MouseMove         , endPos  , Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, endPos  , Qt::LeftButton,
                             Qt::LeftButton, Qt::NoModifier);

    // send events
    bool oldIgnoreSize = setIgnoreSize(true);

    QApplication::sendEvent(mainWindow_, &pressEvent  );
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::sendEvent(mainWindow_, &motionEvent );
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::sendEvent(mainWindow_, &releaseEvent);
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    (void) setIgnoreSize(oldIgnoreSize);
  }
  else {
    QRect rect = this->geometry();

    if      (dockArea == Qt::BottomDockWidgetArea)
      rect.adjust(0,-dh,0,0);
    else if (dockArea == Qt::TopDockWidgetArea)
      rect.adjust(0,0,0,dh);
    else
      return;

    bool oldIgnoreSize = setIgnoreSize(true);

    this->setGeometry(rect);

    (void) setIgnoreSize(oldIgnoreSize);
  }
}

void
CQDockArea::
resetWidgetMinMaxWidth(QWidget *w)
{
  w->setMinimumWidth(0); w->setMaximumWidth(QWIDGETSIZE_MAX);
}

void
CQDockArea::
resetWidgetMinMaxHeight(QWidget *w)
{
  w->setMinimumHeight(0); w->setMaximumHeight(QWIDGETSIZE_MAX);
}
