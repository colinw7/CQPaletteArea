#include <CQDockArea.h>
#include <CQWidgetUtil.h>

#include <QApplication>
#include <QMainWindow>
#include <QTimer>
#include <QMouseEvent>

#include <iostream>

enum { EXTRA_FLOAT_WIDTH  = 8 };
enum { EXTRA_FLOAT_HEIGHT = 8 };

// create dock area
CQDockArea::
CQDockArea(QMainWindow *window) :
 QDockWidget(), window_(window), dockArea_(Qt::RightDockWidgetArea), dragResize_(false),
 resizeTimeout_(10), resizeTimer_(0), ignoreSize_(false), dockWidth_(100), dockHeight_(100),
 oldMinSize_(), oldMaxSize_(), fixed_(false), splitterPressed_(false)
{
  setObjectName("dockArea");

  setFocusPolicy(Qt::NoFocus);

  // create timer to resize palette
  resizeTimer_ = new QTimer(this);

  resizeTimer_->setSingleShot(true);

  connect(resizeTimer_, SIGNAL(timeout()), this, SLOT(resetMinMaxSizes()));

  qApp->installEventFilter(this);
}

// get whether on vertical dock area (left or right)
bool
CQDockArea::
isVerticalDockArea() const
{
  return (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea);
}

// get whether on horizontal dock area (top or bottom)
bool
CQDockArea::
isHorizontalDockArea() const
{
  return (dockArea() == Qt::TopDockWidgetArea || dockArea() == Qt::BottomDockWidgetArea);
}

// set dock area widget
void
CQDockArea::
setWidget(QWidget *w)
{
  QDockWidget::setWidget(w);

  updateWidgetTitle();
}

// set title
void
CQDockArea::
updateWidgetTitle()
{
  setWindowTitle(widget()->windowTitle());
  setWindowIcon (widget()->windowIcon ());
}

// set dock width
void
CQDockArea::
setDockWidth(int width, bool fixed)
{
  // move so tab bar stays in a constant position
  if (isFloating() && dockArea() == Qt::RightDockWidgetArea) {
    int dx = this->width() - (width + EXTRA_FLOAT_WIDTH);

    if (dx) move(this->pos() + QPoint(dx, 0));
  }

  //------

  // try drag (no animate)
  if (dragResize())
    dragToDockWidth(width);

  //------

  bool oldIgnoreSize = setIgnoreSize(true);

  //------

  if      (! isFixed() && fixed) // not fixed to fixed -> save width
    dockWidth_ = this->width();
  else if (! fixed) // to not fixed update width
    dockWidth_ = width;

  //------

  // set palette width by setting min or max width (which makes it fixed sized)
  // and use timer to reset to min/max values after main window has finished its
  // layout processing (which would otherwise has reset this width)
  if (! isFixed()) {
    oldMinSize_ = this->minimumSize();
    oldMaxSize_ = this->maximumSize();
  }

  if (this->isFloating())
    width += EXTRA_FLOAT_WIDTH;

  // already at correct non-fixed size so we are done
  if (! fixed && this->width() == width) {
    (void) setIgnoreSize(oldIgnoreSize);

    fixed_ = fixed;

    // signal size changed
    emit dockWidthChanged(dockWidth_);

    // signal resize done
    emit resizeFinished();

    return;
  }

  // non fixed needs two step process
  if (! fixed) {
    // constraint to force required width
    if (this->width() < width)
      this->setMinimumWidth(width);
    else
      this->setMaximumWidth(width);

    (void) setIgnoreSize(oldIgnoreSize);

    // delay for animation to reset size
    resizeTimer_->start(resizeTimeOut());
  }
  // fixed just needs to force size
  else {
    this->setMinimumWidth(width);
    this->setMaximumWidth(width);

    (void) setIgnoreSize(oldIgnoreSize);

    fixed_ = true;
  }
}

// set dock height
void
CQDockArea::
setDockHeight(int height, bool fixed)
{
  // move so tab bar stays in a constant position
  if (isFloating() && dockArea() == Qt::BottomDockWidgetArea) {
    int dy = this->height() - (height + EXTRA_FLOAT_HEIGHT);

    if (dy) move(this->pos() + QPoint(0, dy));
  }

  //------

  // try drag (no animate)
  if (dragResize())
    dragToDockHeight(height);

  //------

  bool oldIgnoreSize = setIgnoreSize(true);

  //------

  if      (! isFixed() && fixed) // not fixed to fixed save height
    dockHeight_ = this->height();
  else if (! fixed) // to not fixed update height
    dockHeight_ = height;

  //------

  // set palette height by setting min or max height (which makes it fixed sized)
  // and use timer to reset to min/max values after main window has finished its
  // layout processing (which would otherwise has reset this height)
  if (! isFixed()) {
    oldMinSize_ = this->minimumSize();
    oldMaxSize_ = this->maximumSize();
  }

  if (this->isFloating())
    height += EXTRA_FLOAT_HEIGHT;

  // already at correct non-fixed size so we are done
  if (! fixed && this->height() == height) {
    (void) setIgnoreSize(oldIgnoreSize);

    fixed_ = fixed;

    // signal size changed
    emit dockHeightChanged(dockHeight_);

    // signal resize done
    emit resizeFinished();

    return;
  }

  // non fixed needs two step process
  if (! fixed) {
    // constraint to force required height
    if (this->height() < height)
      this->setMinimumHeight(height);
    else
      this->setMaximumHeight(height);

    (void) setIgnoreSize(oldIgnoreSize);

    // delay for animation to reset size
    resizeTimer_->start(resizeTimeOut());
  }
  // fixed just needs to force size
  else {
    this->setMinimumHeight(height);
    this->setMaximumHeight(height);

    (void) setIgnoreSize(oldIgnoreSize);

    fixed_ = true;
  }
}

void
CQDockArea::
resetMinMaxSizes()
{
  // restore old size constraints
  this->setMinimumSize(oldMinSize_);
  this->setMaximumSize(oldMaxSize_);

  fixed_ = false;

  // signal size changed
  if (isVerticalDockArea())
    emit dockWidthChanged(dockWidth_);
  else
    emit dockHeightChanged(dockHeight_);

  // signal resize done
  emit resizeFinished();
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

  Qt::DockWidgetArea dockArea = window_->dockWidgetArea(this);

  CQWidgetUtil::resetWidgetMinMaxWidth(this);
  CQWidgetUtil::resetWidgetMinMaxWidth(this->widget());

  if (! this->isFloating()) {
    QWidget *w = window_->centralWidget();
    if (! w) return;

    // calculate start point (inside splitter)
    QPoint startPos(0,0);

    if      (dockArea == Qt::LeftDockWidgetArea)
      startPos = w->mapToGlobal(w->rect().bottomLeft()) - QPoint(2,2);
    else if (dockArea == Qt::RightDockWidgetArea)
      startPos = w->mapToGlobal(w->rect().topRight  ()) + QPoint(2,2);
    else
      return;

    startPos = window_->mapFromGlobal(startPos);

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

    QApplication::sendEvent(window_, &pressEvent  );
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::sendEvent(window_, &motionEvent );
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::sendEvent(window_, &releaseEvent);
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

  Qt::DockWidgetArea dockArea = window_->dockWidgetArea(this);

  CQWidgetUtil::resetWidgetMinMaxHeight(this);
  CQWidgetUtil::resetWidgetMinMaxHeight(this->widget());

  if (! this->isFloating()) {
    QWidget *w = window_->centralWidget();
    if (! w) return;

    // calculate start point (inside splitter)
    QPoint startPos(0,0);

    if      (dockArea == Qt::BottomDockWidgetArea)
      startPos = w->mapToGlobal(w->rect().bottomLeft()) + QPoint(2,2);
    else if (dockArea == Qt::TopDockWidgetArea)
      startPos = w->mapToGlobal(w->rect().topRight  ()) - QPoint(2,2);
    else
      return;

    startPos = window_->mapFromGlobal(startPos);

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

    QApplication::sendEvent(window_, &pressEvent  );
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::sendEvent(window_, &motionEvent );
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::sendEvent(window_, &releaseEvent);
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

//------

// handle resize
void
CQDockArea::
resizeEvent(QResizeEvent *)
{
  if (isFixed() || ignoreSize()) return;

  if (isVerticalDockArea()) {
    dockWidth_ = width();

    emit dockWidthChanged(dockWidth_);
  }
  else {
    dockHeight_ = height();

    emit dockHeightChanged(dockHeight_);
  }
}

//------

// handle mouse press events on splitter and detached border
bool
CQDockArea::
eventFilter(QObject *obj, QEvent *event)
{
  handleEvent(obj, event);

  return QObject::eventFilter(obj, event);
}

void
CQDockArea::
handleEvent(QObject *obj, QEvent *event)
{
  QEvent::Type type = event->type();

  if      (type == QEvent::MouseButtonPress) {
    if (! isVisible()) return;

    QMouseEvent *me = static_cast<QMouseEvent *>(event);

    if (! isFloating()) {
      if (qobject_cast<QMainWindow *>(obj) != window_) return;

      if (isInsideSplitter(me->globalPos()))
        splitterPressed_ = true;
    }
    else {
      if (qobject_cast<CQDockArea *>(obj) != this) return;

      if (isInsideFloatingBorder(me->pos()))
        splitterPressed_ = true;
    }
  }
  else if (type == QEvent::MouseButtonRelease) {
    if (splitterPressed_)
      splitterPressed_ = false;
  }
}

//------

bool
CQDockArea::
isInsideSplitter(const QPoint &gpos) const
{
  QRect r = getSplitterRect();

  return r.contains(gpos);
}

bool
CQDockArea::
isInsideFloatingBorder(const QPoint &p) const
{
  QRect r = childrenRect();

  return (p.x() < r.left () || p.y() < r.top   () ||
          p.x() > r.right() || p.y() > r.bottom());
}

//------

QRect
CQDockArea::
getSplitterRect() const
{
  QWidget *w = window_->centralWidget();
  if (! w) return QRect();

  QRect tr = QRect(   mapToGlobal(   rect().topLeft()),    rect().size());
  QRect wr = QRect(w->mapToGlobal(w->rect().topLeft()), w->rect().size());

  int x1, y1, x2, y2;

  if      (dockArea() == Qt::LeftDockWidgetArea) {
    x1 = tr.right() + 1, y1 = tr.top   ();
    x2 = wr.left () - 1, y2 = tr.bottom();
  }
  else if (dockArea() == Qt::RightDockWidgetArea) {
    x1 = wr.right() + 1, y1 = tr.top   ();
    x2 = tr.left () - 1, y2 = tr.bottom();
  }
  else if (dockArea() == Qt::BottomDockWidgetArea) {
    x1 = tr.left (), y1 = wr.bottom() + 1;
    x2 = tr.right(), y2 = tr.top   () - 1;
  }
  else if (dockArea() == Qt::TopDockWidgetArea) {
    x1 = tr.left (), y1 = tr.bottom() + 1;
    x2 = tr.right(), y2 = wr.top   () - 1;
  }
  else
    return QRect();

  return QRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}
