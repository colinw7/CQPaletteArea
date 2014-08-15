#include <CQDockArea.h>
#include <CQSplitterArea.h>
#include <CQPaletteArea.h>

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
 QDockWidget(), window_(window), dockArea_(Qt::RightDockWidgetArea), ignoreSize_(false),
 dockWidth_(100), dockHeight_(100), fixed_(false), splitterPressed_(false)
{
  setObjectName("dockArea");

  setFocusPolicy(Qt::NoFocus);

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
applyDockWidth(int width, bool fixed)
{
  // move so tab bar stays in a constant position
  if (isFloating() && dockArea() == Qt::RightDockWidgetArea) {
    int dx = this->width() - (width + EXTRA_FLOAT_WIDTH);

    if (dx) move(this->pos() + QPoint(dx, 0));
  }

  //------

  bool oldIgnoreSize = setIgnoreSize(true);

  //------

  if      (! isFixed() && fixed) // not fixed to fixed -> save width
    setDockWidth(this->width());
  else if (! fixed) // to not fixed update width
    setDockWidth(width);

  //------

  if (this->isFloating())
    width += EXTRA_FLOAT_WIDTH;

  // force size
  if (this->width() != width) {
    this->setMinimumWidth(width);
    this->setMaximumWidth(width);
  }

  (void) setIgnoreSize(oldIgnoreSize);

  fixed_ = fixed;

  // signal size changed
  if (! fixed_)
    emit dockWidthChanged(dockWidth());
}

// set dock height
void
CQDockArea::
applyDockHeight(int height, bool fixed)
{
  // move so tab bar stays in a constant position
  if (isFloating() && dockArea() == Qt::BottomDockWidgetArea) {
    int dy = this->height() - (height + EXTRA_FLOAT_HEIGHT);

    if (dy) move(this->pos() + QPoint(0, dy));
  }

  //------

  bool oldIgnoreSize = setIgnoreSize(true);

  //------

  if      (! isFixed() && fixed) // not fixed to fixed -> save height
    setDockHeight(this->height());
  else if (! fixed) // to not fixed update height
    setDockHeight(height);

  //------

  if (this->isFloating())
    height += EXTRA_FLOAT_HEIGHT;

  // force size
  if (this->height() != height) {
    this->setMinimumHeight(height);
    this->setMaximumHeight(height);
  }

  (void) setIgnoreSize(oldIgnoreSize);

  fixed_ = fixed;

  // signal size changed
  if (! fixed_)
    emit dockHeightChanged(dockHeight());
}

//------

// handle resize
void
CQDockArea::
resizeEvent(QResizeEvent *)
{
  if (isFixed() || ignoreSize()) return;

  if (isVerticalDockArea()) {
    setDockWidth(width());

    emit dockWidthChanged(dockWidth());
  }
  else {
    setDockHeight(height());

    emit dockHeightChanged(dockHeight());
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
      CQSplitterHandle *handle = qobject_cast<CQSplitterHandle *>(obj);

      if (! handle) return;

      CQPaletteArea *palette = handle->area()->palette();

      if (palette == this)
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
  else if (type == QEvent::Resize) {
    QWidget *mw = window_->centralWidget();

    if (mw == obj)
      emit centralWidgetResized();
  }
}

//------

bool
CQDockArea::
isInsideFloatingBorder(const QPoint &p) const
{
  QRect r = childrenRect();

  return (p.x() < r.left () || p.y() < r.top   () ||
          p.x() > r.right() || p.y() > r.bottom());
}
