#include <CQTabBar.h>

#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QStylePainter>
#include <QStyleOptionTab>
#include <QMouseEvent>
#include <QToolTip>
#include <QDrag>
#include <QMimeData>

#include <cassert>

namespace {
static const char *dragId     = "CQTabBarDragId";
static const char *mimeId     = "CQTabBarMimeId";
static const char *mimeNameId = "CQTabBarMimeNameId";
static const char *mimeTabId  = "CQTabBarMimeTabId";
}

// create tab bar
CQTabBar::
CQTabBar(QWidget *parent) :
 QWidget(parent), currentIndex_(-1), position_(North), allowNoTab_(false),
 buttonStyle_(Qt::ToolButtonIconOnly), iconSize_(16,16), iw_(0), w_(0), h_(0),
 clipNum_(-1), offset_(0), pressed_(false), pressIndex_(-1), moveIndex_(-1)
{
  setObjectName("tabBar");

  setAcceptDrops(true);

  // add scroll buttons if tab bar is clipped
  lscroll_ = new CQTabBarScrollButton(this, "lscroll");
  rscroll_ = new CQTabBarScrollButton(this, "rscroll");

  connect(lscroll_, SIGNAL(clicked()), this, SLOT(lscrollSlot()));
  connect(rscroll_, SIGNAL(clicked()), this, SLOT(rscrollSlot()));

  lscroll_->hide();
  rscroll_->hide();
}

// delete tab bar
CQTabBar::
~CQTabBar()
{
  for (TabButtons::iterator p = buttons_.begin(); p != buttons_.end(); ++p)
    delete *p;
}

// add tab for widget with specified text
int
CQTabBar::
addTab(const QString &text, QWidget *w)
{
  return addTab(QIcon(), text, w);
}

// add tab for widget with specified text
int
CQTabBar::
addTab(const QIcon &icon, const QString &text, QWidget *w)
{
  // create button
  CQTabBarButton *button = new CQTabBarButton(this);

  button->setText  (text);
  button->setIcon  (icon);
  button->setWidget(w);

  return addTab(button);
}

// add tab for button
int
CQTabBar::
addTab(CQTabBarButton *button)
{
  int ind = count();

  return insertTab(ind, button);
}

// insert tab for widget with specified text
int
CQTabBar::
insertTab(int ind, const QString &text, QWidget *w)
{
  return insertTab(ind, QIcon(), text, w);
}

// insert tab for widget with specified text
int
CQTabBar::
insertTab(int ind, const QIcon &icon, const QString &text, QWidget *w)
{
  // create button
  CQTabBarButton *button = new CQTabBarButton(this);

  button->setText  (text);
  button->setIcon  (icon);
  button->setWidget(w);

  return insertTab(ind, button);
}

int
CQTabBar::
insertTab(int ind, CQTabBarButton *button)
{
  buttons_.push_back(0);

  for (int i = buttons_.size() - 1; i > ind; --i)
    buttons_[i] = buttons_[i - 1];

  buttons_[ind] = button;

  int index = buttons_.size();

  button->setIndex(index);

  // update current
  if (! allowNoTab() && currentIndex() < 0)
    setCurrentIndex(index);

  // update display
  updateSizes();

  update();

  return index;
}

// remove tab for widget
void
CQTabBar::
removeTab(QWidget *widget)
{
  // get tab index
  int ind = getTabIndex(widget);
  assert(ind >= 0);

  removeTab(ind);
}

// remove tab for index
void
CQTabBar::
removeTab(int ind)
{
  int pos = tabButtonPos(ind);

  CQTabBarButton *button = buttons_[pos];

  buttons_[pos] = 0;

  delete button;

  // reset current if deleted is current
  if (currentIndex() == ind)
    setCurrentIndex(-1);

  // update display
  updateSizes();

  update();
}

// get number of tabs
int
CQTabBar::
count() const
{
  int n = 0;

  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;
    if (! button) continue;

    ++n;
  }

  return n;
}

// get tab at count
int
CQTabBar::
tabInd(int i) const
{
  int n = 0;

  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;
    if (! button) continue;

    if (n == i)
      return button->index();

    ++n;
  }

  return -1;
}

// set current tab
void
CQTabBar::
setCurrentIndex(int ind)
{
  // button array can contain empty slots so ensure the requested one is valid
  CQTabBarButton *button = tabButton(ind);
  if (! button) return;

  // process if changed
  if (ind != currentIndex_) {
    currentIndex_ = ind;

    // if one tab must be active and nothing active then use first non-null button
    if (! allowNoTab() && currentIndex_ < 0 && count() > 0)
      currentIndex_ = 0;

    update();

    emit currentChanged(currentIndex_);
  }
}

// get tab index for specified widget
int
CQTabBar::
getTabIndex(QWidget *w) const
{
  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;

    if (button && button->widget() == w)
      return button->index();
  }

  return -1;
}

// set tab location (relative to contents)
void
CQTabBar::
setPosition(const Position &position)
{
  // assert position is valid (just in case)
  assert(position == North || position == South || position == West || position == East);

  position_ = position;

  update();
}

// set allow no current tab
void
CQTabBar::
setAllowNoTab(bool allow)
{
  allowNoTab_ = allow;

  if (! allowNoTab() && currentIndex() < 0 && count() > 0)
    setCurrentIndex(0);
}

// set tab button style
void
CQTabBar::
setButtonStyle(const Qt::ToolButtonStyle &buttonStyle)
{
  buttonStyle_ = buttonStyle;

  updateSizes();

  update();
}

// set tab text
void
CQTabBar::
setTabText(int ind, const QString &text)
{
  CQTabBarButton *button = tabButton(ind);

  if (button)
    button->setText(text);

  updateSizes();

  update();
}

// set tab icon
void
CQTabBar::
setTabIcon(int ind, const QIcon &icon)
{
  CQTabBarButton *button = tabButton(ind);

  if (button)
    button->setIcon(icon);

  updateSizes();

  update();
}

// set tab tooltip
void
CQTabBar::
setTabToolTip(int ind, const QString &tip)
{
  CQTabBarButton *button = tabButton(ind);

  if (button)
    button->setToolTip(tip);
}

// set tab visible
void
CQTabBar::
setTabVisible(int ind, bool visible)
{
  CQTabBarButton *button = tabButton(ind);

  if (button)
    button->setVisible(visible);

  updateSizes();

  update();
}

// set tab pending state
void
CQTabBar::
setTabPending(int ind, bool pending)
{
  CQTabBarButton *button = tabButton(ind);

  if (button)
    button->setPending(pending);

  update();
}

// set tab data
void
CQTabBar::
setTabData(int ind, const QVariant &data)
{
  CQTabBarButton *button = tabButton(ind);

  if (button)
    button->setData(data);
}

// get tab data
QVariant
CQTabBar::
tabData(int ind) const
{
  CQTabBarButton *button = tabButton(ind);

  if (button)
    return button->data();
  else
    return QVariant();
}

// get button for tab
CQTabBarButton *
CQTabBar::
tabButton(int ind) const
{
  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;

    if (button && button->index() == ind)
      return button;
  }

  return 0;
}

// get array pos for tab
int
CQTabBar::
tabButtonPos(int ind) const
{
  for (int i = 0; i < int(buttons_.size()); ++i) {
    CQTabBarButton *button = buttons_[i];

    if (button && button->index() == ind)
      return i;
  }

  assert(false);

  return -1;
}

// get widget for tab
QWidget *
CQTabBar::
tabWidget(int ind) const
{
  CQTabBarButton *button = tabButton(ind);

  if (button)
    return button->widget();

  return 0;
}

// draw tab buttons
void
CQTabBar::
paintEvent(QPaintEvent *)
{
  QStylePainter stylePainter(this);

  //------

  // calculate width and height of region
  int xo = 0;

  if (offset_ > 0) {
    int offset = offset_;

    for (TabButtons::const_iterator p = buttons_.begin(); offset > 0 && p != buttons_.end(); ++p) {
      CQTabBarButton *button = *p;

      if (! button || ! button->visible()) continue;

      xo += button->width();

      --offset;
    }
  }

  int w = width ();
  int h = height();

  // set tab style
  QStyleOptionTabV2 tabStyle;

  tabStyle.initFrom(this);

  tabStyle.shape = getTabShape();

  int overlap = style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &tabStyle, this);

  // set tab base style
  QStyleOptionTabBarBaseV2 baseStyle;

  baseStyle.initFrom(this);

  // calculate button geometry and first/last tab buttons
  CQTabBarButton *firstButton = 0;
  CQTabBarButton *lastButton  = 0;

  int x = -xo;

  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;

    if (! button || ! button->visible()) continue;

    if (firstButton == 0)
      firstButton = button;
    else
      lastButton = button;

    // calc button width
    int w1 = button->width();

    // calculate and store button rectangle
    QRect r;

    if (isVertical())
      r = QRect(0, x, h_, w1);
    else
      r = QRect(x, 0, w1, h_);

    button->setRect(r);

    // update base line rectangle
    if (button->index() == currentIndex())
      baseStyle.selectedTabRect = r;

    //-----

    x += w1;
  }

  // draw tab base
  if      (position_ == North)
    baseStyle.rect = QRect(0, h_ - overlap, w, overlap);
  else if (position_ == South)
    baseStyle.rect = QRect(0, 0, w, overlap);
  else if (position_ == West)
    baseStyle.rect = QRect(h_ - overlap, 0, overlap, h);
  else if (position_ == East)
    baseStyle.rect = QRect(0, 0, overlap, h);

  baseStyle.shape = getTabShape();

  stylePainter.drawPrimitive(QStyle::PE_FrameTabBarBase, baseStyle);

  //------

  // draw buttons
  x = -xo;

  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;

    if (! button || ! button->visible()) continue;

    //----

    // set button style
    tabStyle.initFrom(this);

    tabStyle.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);

    tabStyle.rect = button->rect();

    tabStyle.row = 0;

    if (button->index() == pressIndex_)
      tabStyle.state |= QStyle::State_Sunken;
    else
      tabStyle.state &= ~QStyle::State_Sunken;

    if (button->index() == currentIndex())
      tabStyle.state |=  QStyle::State_Selected;
    else
      tabStyle.state &= ~QStyle::State_Selected;

    if (button->index() == moveIndex_)
      tabStyle.state |=  QStyle::State_MouseOver;
    else
      tabStyle.state &= ~QStyle::State_MouseOver;

    tabStyle.shape = getTabShape();

    if (buttonStyle_ == Qt::ToolButtonTextOnly || buttonStyle_ == Qt::ToolButtonTextBesideIcon)
      tabStyle.text = button->text();

    if (buttonStyle_ == Qt::ToolButtonIconOnly || buttonStyle_ == Qt::ToolButtonTextBesideIcon)
      tabStyle.icon = button->positionIcon(position_);

    tabStyle.iconSize = iconSize();

    if      (button == firstButton)
      tabStyle.position = QStyleOptionTab::Beginning;
    else if (button == lastButton)
      tabStyle.position = QStyleOptionTab::End;
    else
      tabStyle.position = QStyleOptionTab::Middle;

    if (button->pending())
      tabStyle.palette.setColor(QPalette::Button, QColor("#0000FF"));

    // draw button
    stylePainter.drawControl(QStyle::CE_TabBarTab, tabStyle);

    //-----

    x += button->width();
  }

  // update scroll buttons
  lscroll_->setEnabled(offset_ > 0);
  rscroll_->setEnabled(offset_ < clipNum_);
}

// handle resize
void
CQTabBar::
resizeEvent(QResizeEvent *)
{
  updateSizes();
}

// update size of tab area
void
CQTabBar::
updateSizes()
{
  // calculate width and height of region
  QFontMetrics fm(font());

  int iw = iconSize().width();
  int h  = qMax(iw, fm.height()) + TAB_BORDER;

  // remove resize width
  if (isVertical())
    h = qMin(h, width () - RESIZE_WIDTH);
  else
    h = qMin(h, height() - RESIZE_WIDTH);

  int w = 0;

  clipNum_ = 0;

  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;

    if (! button || ! button->visible()) continue;

    w += button->width();

    if (isVertical()) {
      if (w > height())
        ++clipNum_;
    }
    else {
      if (w > width())
        ++clipNum_;
    }
  }

  iw_ = iw;
  w_  = w;
  h_  = h;

  //-----

  // update scroll if clipped
  showScrollButtons(clipNum_ > 0);

  if (offset_ > clipNum_)
    offset_ = clipNum_;
}

// update scroll buttons
void
CQTabBar::
showScrollButtons(bool show)
{
  lscroll_->setVisible(show);
  rscroll_->setVisible(show);

  if (show) {
    // position scroll buttons depending in tab position
    if (isVertical()) {
      int xs = iconWidth() + 6;
      int ys = iconWidth();

      int d = h_ - xs;

      lscroll_->setFixedSize(xs, ys);
      rscroll_->setFixedSize(xs, ys);

      lscroll_->move(d, height() - 2*ys);
      rscroll_->move(d, height() -   ys);

      lscroll_->setArrowType(Qt::UpArrow);
      rscroll_->setArrowType(Qt::DownArrow);
    }
    else {
      int xs = iconWidth();
      int ys = iconWidth() + 6;

      int d = h_ - ys;

      lscroll_->setFixedSize(xs, ys);
      rscroll_->setFixedSize(xs, ys);

      lscroll_->move(width() - 2*xs, d);
      rscroll_->move(width() -   xs, d);

      lscroll_->setArrowType(Qt::LeftArrow);
      rscroll_->setArrowType(Qt::RightArrow);
    }
  }
  else
    offset_ = 0;
}

// called when left/bottom scroll is pressed
void
CQTabBar::
lscrollSlot()
{
  // scroll to previous tab (if any)
  --offset_;

  if (offset_ < 0)
    offset_ = 0;

  update();
}

// called when right/top scroll is pressed
void
CQTabBar::
rscrollSlot()
{
  // scroll to next tab (if any)
  ++offset_;

  if (offset_ > clipNum_)
    offset_ = clipNum_;

  update();
}

// handle tool tip event
bool
CQTabBar::
event(QEvent *e)
{
  if (e->type() == QEvent::ToolTip) {
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);

    int ind = tabAt(helpEvent->pos());

    if (ind != -1) {
      CQTabBarButton *button = tabButton(ind);

      if (button)
        QToolTip::showText(helpEvent->globalPos(), button->toolTip());
    }
    else {
      QToolTip::hideText();

      e->ignore();
    }

    return true;
  }

  return QWidget::event(e);
}

// get preferred size
QSize
CQTabBar::
sizeHint() const
{
  QFontMetrics fm(font());

  int iw = iconSize().width();
  int h  = qMax(iw, fm.height()) + TAB_BORDER + RESIZE_WIDTH;

  int w = 0;

  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;

    if (! button || ! button->visible()) continue;

    w += button->width();
  }

  if (isVertical())
    return QSize(h, w);
  else
    return QSize(w, h);
}

// get minimum size
QSize
CQTabBar::
minimumSizeHint() const
{
  QSize s = sizeHint();

  if (isVertical())
    return QSize(s.width(), 0);
  else
    return QSize(0, s.height());
}

// set press point
void
CQTabBar::
setPressPoint(const QPoint &p)
{
  pressed_    = true;
  pressPos_   = p;
  pressIndex_ = tabAt(pressPos_);
}

// handle mouse press
void
CQTabBar::
mousePressEvent(QMouseEvent *e)
{
  // init press state and redraw
  setPressPoint(e->pos());

  update();
}

// handle mouse move (while pressed)
void
CQTabBar::
mouseMoveEvent(QMouseEvent *e)
{
  // update press state and redraw
  if (! pressed_)
    setPressPoint(e->pos());

  // If left button pressed check for drag
  if (e->buttons() & Qt::LeftButton) {
    // check drag distance
    if ((e->pos() - pressPos_).manhattanLength() >= QApplication::startDragDistance()) {
      CQTabBarButton *button = tabButton(pressIndex_);

      QIcon icon = (button ? button->icon() : QIcon());

      // initiate drag
      QDrag *drag = new QDrag(this);

      drag->setPixmap(icon.pixmap(iconSize()));

      QMimeData *mimeData = new QMimeData;

      // use unique id for our mime data and store source palette type
      QString numStr = QString("%1").arg(pressIndex_);

      mimeData->setData(mimeId    , dragId);
      mimeData->setData(mimeNameId, objectName().toLatin1());
      mimeData->setData(mimeTabId , numStr.toLatin1());

      drag->setMimeData(mimeData);

      drag->exec();
    }
  }

  update();
}

// handle mouse release
void
CQTabBar::
mouseReleaseEvent(QMouseEvent *e)
{
  // reset pressed state
  pressed_ = false;

  // check if new tab button is pressed
  pressIndex_ = tabAt(e->pos());

  bool isCurrent = (pressIndex_ != -1 && pressIndex_ == currentIndex());

  if (pressIndex_ != -1) {
    if (! isCurrent)
      setCurrentIndex(pressIndex_); // will send currentChanged signal
    else
      emit currentPressed(pressIndex_);
  }

  // signal tab button pressed
  emit tabPressedSignal(pressIndex_, ! isCurrent);

  // redraw
  update();
}

// handle drag enter event
void
CQTabBar::
dragEnterEvent(QDragEnterEvent *event)
{
  QString name;
  int     fromIndex;

  if (! dragValid(event->mimeData(), name, fromIndex)) {
    event->ignore();
    return;
  }

  event->acceptProposedAction();
}

// handle drag move
void
CQTabBar::
dragMoveEvent(QDragMoveEvent *event)
{
  QString name;
  int     fromIndex;

  if (! dragValid(event->mimeData(), name, fromIndex)) {
    event->ignore();
    return;
  }

  if (dragPosValid(name, event->pos()))
    event->acceptProposedAction();
  else
    event->ignore();
}

// handle drop event
void
CQTabBar::
dropEvent(QDropEvent *event)
{
  QString name;
  int     fromIndex;

  if (! dragValid(event->mimeData(), name, fromIndex)) {
    event->ignore();
    return;
  }

  if (! dragPosValid(name, event->pos()))
    return;

  // get index at release position
  int toIndex = tabAt(event->pos());

  if (name == objectName()) {
    // skip invalid and do nothing drops
    if (fromIndex < 0 || toIndex < 0 || fromIndex == toIndex)
      return;

    int pos1 = tabButtonPos(fromIndex);
    int pos2 = tabButtonPos(toIndex  );

    CQTabBarButton *button1 = buttons_[pos1];
    CQTabBarButton *button2 = buttons_[pos2];

    buttons_[pos2] = button1;
    buttons_[pos1] = button2;

    button1->setIndex(toIndex);
    button2->setIndex(fromIndex);

    if      (fromIndex == currentIndex()) setCurrentIndex(toIndex);
    else if (toIndex   == currentIndex()) setCurrentIndex(fromIndex);

    emit tabMoved(fromIndex, toIndex);

    event->acceptProposedAction();
  }
  else {
    emit tabMovePageSignal(name, fromIndex, this->objectName(), toIndex);

    event->acceptProposedAction();
  }
}

// is drag valid
bool
CQTabBar::
dragValid(const QMimeData *m, QString &name, int &num) const
{
  // Only accept if it's our request
  QStringList formats = m->formats();

  if (! formats.contains(mimeId) || m->data(mimeId) != dragId)
    return false;

  assert(formats.contains(mimeTabId));
  assert(formats.contains(mimeNameId));

  name = m->data(mimeNameId).data();

  QString numStr = m->data(mimeTabId).data();

  bool ok;

  num = numStr.toInt(&ok);

  assert(ok);

  return true;
}

// is drag position valid
bool
CQTabBar::
dragPosValid(const QString &name, const QPoint &pos) const
{
  // drag in same tab bar must be to another tab
  if (name == this->objectName()) {
    int ind = tabAt(pos);

    if (ind < 0)
      return false;
  }

  return true;
}

// get tab at specified point
int
CQTabBar::
tabAt(const QPoint &point) const
{
  for (TabButtons::const_iterator p = buttons_.begin(); p != buttons_.end(); ++p) {
    CQTabBarButton *button = *p;

    if (! button || ! button->visible()) continue;

    if (button->rect().contains(point))
      return button->index();
  }

  return -1;
}

// get icon size
QSize
CQTabBar::
iconSize() const
{
  return iconSize_;
}

// set icon size
void
CQTabBar::
setIconSize(const QSize &size)
{
  iconSize_ = size;

  update();
}

// handle context menu request
void
CQTabBar::
contextMenuEvent(QContextMenuEvent *e)
{
  emit showContextMenuSignal(e->globalPos());
}

//! get tab shape for position
QTabBar::Shape
CQTabBar::
getTabShape() const
{
  switch (position_) {
    case North: return QTabBar::RoundedNorth;
    case South: return QTabBar::RoundedSouth;
    case West : return QTabBar::RoundedWest;
    case East : return QTabBar::RoundedEast;
    default   : assert(false); return QTabBar::RoundedNorth;
  }
}

bool
CQTabBar::
isVertical() const
{
  return (position_ == West || position_ == East);
}

//-------

// create tab button
CQTabBarButton::
CQTabBarButton(CQTabBar *bar) :
 bar_(bar), index_(0), text_(), icon_(), positionIcon_(),
 iconPosition_(CQTabBar::North), toolTip_(), w_(0), visible_(true),
 pending_(false), r_()
{
}

// set button text
void
CQTabBarButton::
setText(const QString &text)
{
  text_ = text;
}

// set button icon
void
CQTabBarButton::
setIcon(const QIcon &icon)
{
  icon_ = icon;

  // ensure new icon causes recalc
  if (iconPosition_ != CQTabBar::North && iconPosition_ != CQTabBar::South)
    iconPosition_ = CQTabBar::North;
}

// set button data
void
CQTabBarButton::
setData(const QVariant &data)
{
  data_ = data;
}

// get icon for tab position
const QIcon &
CQTabBarButton::
positionIcon(CQTabBar::Position pos) const
{
  if (pos == CQTabBar::North || pos == CQTabBar::South)
    return icon_;

  if (pos == iconPosition_)
    return positionIcon_;

  iconPosition_ = pos;

  QTransform t;

  QPixmap p = pixmap();

  t.rotate(iconPosition_ == CQTabBar::West ? 90 : -90);

  positionIcon_ = QIcon(p.transformed(t));

  return positionIcon_;
}

// get tab tooltip
const QString &
CQTabBarButton::
toolTip() const
{
  if (toolTip_ != "")
    return toolTip_;
  else
    return text_;
}

// set tab tooltip
void
CQTabBarButton::
setToolTip(const QString &tip)
{
  toolTip_ = tip;
}

// set tab widget
void
CQTabBarButton::
setWidget(QWidget *w)
{
  w_ = w;
}

// set tab visible
void
CQTabBarButton::
setVisible(bool visible)
{
  visible_ = visible;
}

// set tab pending
void
CQTabBarButton::
setPending(bool pending)
{
  pending_ = pending;
}

// set tab rectangle
void
CQTabBarButton::
setRect(const QRect &r)
{
  r_ = r;
}

// get tab icon pixmap
QPixmap
CQTabBarButton::
pixmap() const
{
  return icon_.pixmap(bar_->iconSize());
}

// get button width depending on button style
int
CQTabBarButton::
width() const
{
  QFontMetrics fm(bar_->font());

  //------

  Qt::ToolButtonStyle buttonStyle = bar_->buttonStyle();

  int w = 0;

  if      (buttonStyle == Qt::ToolButtonTextOnly)
    w = fm.width(text()) + 24;
  else if (buttonStyle == Qt::ToolButtonIconOnly)
    w = bar_->iconWidth() + 24;
  else
    w = bar_->iconWidth() + fm.width(text()) + 32;

  return w;
}

//---------

CQTabBarScrollButton::
CQTabBarScrollButton(CQTabBar *bar, const char *name) :
 QToolButton(bar)
{
  setObjectName(name);

  setAutoRepeat(true);

  setFocusPolicy(Qt::NoFocus);
}
