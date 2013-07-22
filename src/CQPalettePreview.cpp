#include <CQPalettePreview.h>
#include <CQPaletteArea.h>

#include <QApplication>
#include <QMouseEvent>
#include <QMainWindow>
#include <QSizeGrip>
#include <QDialog>

CQPalettePreview::
CQPalettePreview() :
 active_(false), stopOnRelease_(false)
{
}

void
CQPalettePreview::
setActive(bool active)
{
  if (active_ == active)
    return;

  active_ = active;

  if (active_)
    qApp->installEventFilter(this);
  else
    qApp->removeEventFilter(this);
}

void
CQPalettePreview::
clear()
{
  widgets_.clear();
  rects_  .clear();
}

void
CQPalettePreview::
addWidget(QWidget *w)
{
  widgets_.insert(w);
}

void
CQPalettePreview::
addRect(const QRect &r)
{
  rects_.push_back(r);
}

bool
CQPalettePreview::
eventFilter(QObject *obj, QEvent *event)
{
  bool ok = processEvent(obj, event);

  if (! ok)
    emit stopPreview();

  return QObject::eventFilter(obj, event);
}

bool
CQPalettePreview::
processEvent(QObject *obj, QEvent *event)
{
  QEvent::Type type = event->type();

  // ignore if not a widget
  QWidget *w = qobject_cast<QWidget *>(obj);
  if (! w) return true;

  bool previewValid = true;

  // ignore if not a button event
  if      (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick) {
    // stop preview if event is not in an allowable widget or rectangle
    QMouseEvent *me = static_cast<QMouseEvent *>(event);

    stopOnRelease_ = false;

    if (type == QEvent::MouseButtonPress)
      stopOnRelease_ = ! isPreviewValid(w, me->globalPos(), true);
    else
      previewValid = isPreviewValid(w, me->globalPos(), true);
  }
  else if (type == QEvent::MouseButtonRelease) {
    // stop preview if event is not in an allowable widget or rectangle
    QMouseEvent *me = static_cast<QMouseEvent *>(event);

    if (stopOnRelease_) {
      previewValid   = false;
      stopOnRelease_ = false;
    }
    else
      previewValid = isPreviewValid(w, me->globalPos(), false);
  }
  else
    return true;

  if (! previewValid)
    return false;

  return true;
}

// check if mouse in an allowable widget or rectangle
bool
CQPalettePreview::
isPreviewValid() const
{
  QPoint gp = QCursor::pos();

  return isPreviewValid(qApp->widgetAt(gp), gp, false);
}

// check if event widget and position are allowable
bool
CQPalettePreview::
isPreviewValid(QWidget *w, const QPoint &gp, bool press) const
{
  if (! press) {
    const int tol = 16;

    // end preview on release on main window resize bar in another dock widget
    if (qobject_cast<QMainWindow *>(w) != NULL) {
      if (! checkPreviewRects(gp, tol))
        return false;
    }

    return true;
  }

  //-----

  // mouse is grab (menu) so assume ok
  if (QWidget::mouseGrabber())
    return true;

  // ignore size grid
  if (qobject_cast<QSizeGrip *>(w) != NULL)
    return true;

  // ignore main window press (resize bar)
  if (qobject_cast<QMainWindow *>(w) != NULL)
    return true;

  //-----

  // check widgets and rects
  bool previewValid = true;

  bool found = false;

  if (isPreviewWidget(w))
    found = true;

  if (! found)
    found = checkPreviewRects(gp);

  if (! found && isModalDialogWidget(w))
    found = true;

  if (! found)
    previewValid = false;

  return previewValid;
}

bool
CQPalettePreview::
checkPreviewRects(const QPoint &gp, int tol) const
{
  bool found = false;

  int n = rects_.size();

  for (int i = 0; i < n; ++i) {
    const QRect &r = rects_[i];

    QRect r1 = r.adjusted(-tol, -tol, tol, tol);

    if (r1.contains(gp)) {
      found = true;
      break;
    }
  }

  return found;
}

// check if widget is a preview widget or a child of it
bool
CQPalettePreview::
isPreviewWidget(QWidget *w) const
{
  QWidget *w1 = w;

  while (w1) {
    if (widgets_.contains(w1))
      return true;

    w1 = w1->parentWidget();
  }

  return false;
}

// check if widget is a child of a modal dialog
bool
CQPalettePreview::
isModalDialogWidget(QWidget *w) const
{
  QWidget *w1 = w;

  while (w1) {
    QDialog *dialog = qobject_cast<QDialog *>(w1);

    if (dialog && dialog->isModal())
      return true;

    w1 = w1->parentWidget();
  }

  return false;
}
