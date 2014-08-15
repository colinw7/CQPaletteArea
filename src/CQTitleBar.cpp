#include <CQTitleBar.h>

#include <QPainter>
#include <QStylePainter>
#include <QStyleOptionToolButton>

// create title bar
CQTitleBar::
CQTitleBar(Qt::Orientation orient, QWidget *parent) :
 QWidget(parent), title_(), icon_(), orient_(orient), border_(2),
 bgColor_(160,160,160), barColor_(120,120,120), buttons_()
{
  setObjectName("title");
}

// create title bar (swapped args)
CQTitleBar::
CQTitleBar(QWidget *parent, Qt::Orientation orient) :
 QWidget(parent), title_(), icon_(), orient_(orient), border_(2),
 bgColor_(160,160,160), barColor_(120,120,120), buttons_()
{
  setObjectName("title");
}

// set title
void
CQTitleBar::
setTitle(const QString &title)
{
  title_ = title;

  if (isVisible())
    update();
}

// set icon
void
CQTitleBar::
setIcon(const QIcon &icon)
{
  icon_ = icon;

  if (isVisible())
    update();
}

// set orientation
void
CQTitleBar::
setOrientation(Qt::Orientation orient)
{
  orient_ = orient;

  if (isVisible())
    updateLayout();
}

// set border
void
CQTitleBar::
setBorder(int border)
{
  border_ = border;

  if (isVisible())
    updateLayout();
}

// set background color
void
CQTitleBar::
setBackgroundColor(const QColor &color)
{
  bgColor_ = color;

  if (isVisible())
    update();
}

// set title bar line color
void
CQTitleBar::
setBarColor(const QColor &color)
{
  barColor_ = color;

  if (isVisible())
    update();
}

// add icon button
CQTitleBarButton *
CQTitleBar::
addButton(const QIcon &icon)
{
  CQTitleBarButton *button = new CQTitleBarButton;

  button->setIcon(icon);

  addButton(button);

  return button;
}

// add button widget
void
CQTitleBar::
addButton(CQTitleBarButton *button)
{
  button->setParent(this);

  button->setTitleBar(this);

  buttons_.push_back(button);

  updateLayout();
}

// handle show event
void
CQTitleBar::
showEvent(QShowEvent *)
{
  updateLayout();
}

// handle paint event
void
CQTitleBar::
paintEvent(QPaintEvent *)
{
  int b  = border();
  int b1 = std::max(b, 1);

  QPainter p(this);

  QColor bgColor = this->backgroundColor();

  p.fillRect(rect(), bgColor);

  QColor barColor = this->barColor();

  drawTitleBarLines(&p, rect(), barColor);

  QIcon   icon  = this->icon();
  QString title = this->title();

  int x = 0;

  if (orientation() == Qt::Horizontal) {
    if (! icon.isNull()) {
      int ps = height() - 2*b1;

      p.fillRect(QRect(x, 0, ps + 2*b1, height()), bgColor);

      p.drawPixmap(x + 2, 2, icon.pixmap(QSize(ps,ps)));

      x += ps + 2*b1;
    }

    if (! title.isEmpty()) {
      QFontMetrics fm(font());

      int tw = fm.width(title);
      int th = fm.boundingRect(title).height();

      p.fillRect(QRect(x, 0, tw + 8, height()), bgColor);

      p.setPen(palette().color(QPalette::Text));

      p.drawText(x + 2, (height() - th)/2 + fm.ascent(), title);
    }
  }
  else {
    int y = height() - 1;

    if (! icon.isNull()) {
      int ps = width() - 2*b1;

      p.fillRect(QRect(0, y - ps - 2*b1, width(), ps + 2*b1), bgColor);

      p.save();

      p.translate(0, height());
      p.rotate(-90);

      //p.drawPixmap(2, y - ps - 2, icon.pixmap(QSize(ps,ps)));
      p.drawPixmap(x + 2, 2, icon.pixmap(QSize(ps,ps)));

      p.restore();

      y -= ps + 2*b1;
      x += ps + 2*b1;
    }

    if (! title.isEmpty()) {
      QFontMetrics fm(font());

      int tw = fm.width(title);
      int th = fm.boundingRect(title).height();

      p.save();

      p.fillRect(QRect(0, y - tw - 8, width(), tw + 8), bgColor);

      p.setPen(palette().color(QPalette::Text));

      p.translate(0, height());
      p.rotate(-90);

      p.drawText(x + 2, (width() - th)/2 + fm.ascent(), title);

      p.restore();
    }
  }

  int bw = (orientation() == Qt::Horizontal ? height() - 2*b1 : width() - 2*b1);

  int nb = 0;

  for (uint i = 0; i < buttons_.size(); ++i)
    if (buttons_[i]->isVisible())
      ++nb;

  if (orientation() == Qt::Horizontal)
    p.fillRect(QRect(width() - nb*bw, 0, nb*bw, height()), bgColor);
  else
    p.fillRect(QRect(0, 0, width(), nb*bw), bgColor);
}

// handle resize event
void
CQTitleBar::
resizeEvent(QResizeEvent *)
{
  updateLayout();
}

// update layout
void
CQTitleBar::
updateLayout()
{
  if (! isVisible()) return;

  int b  = border();
  int b1 = std::max(b, 1);

  QFontMetrics fm(font());

  if (orientation() == Qt::Horizontal) {
    setFixedHeight(fm.height() + 2*b);
    setMinimumWidth(0); setMaximumWidth(QWIDGETSIZE_MAX);
  }
  else {
    setFixedWidth(fm.height() + 2*b);
    setMinimumHeight(0); setMaximumHeight(QWIDGETSIZE_MAX);
  }

  //------

  int bw = (orientation() == Qt::Horizontal ? height() - 2*b1 : width() - 2*b1);

  int nb = buttons_.size();

  int pos = (orientation() == Qt::Horizontal ? width() - 2 : 2);

  for (int i = nb - 1; i >= 0; --i) {
    CQTitleBarButton *button = buttons_[i];

    if (! button->isVisible())
      continue;

    if (orientation() == Qt::Horizontal) {
      int dh = height() - button->height();

      button->move(pos - bw, dh/2);

      pos -= bw;
    }
    else {
      int dw = width() - button->width();

      button->move(dw/2, pos);

      pos += bw;
    }
  }
}

// draw title lines
void
CQTitleBar::
drawTitleBarLines(QPainter *p, const QRect &r, const QColor &c)
{
  p->setPen(c);

  if (orientation() == Qt::Horizontal) {
    int left  = r.left () + 2;
    int right = r.right() - 2;

    int y = r.center().y() - 3;

    for (int i = 0; i < 4; ++i) {
      int y1 = y + 2*i;

      p->drawLine(left, y1, right, y1);
    }
  }
  else {
    int top    = r.top   () + 2;
    int bottom = r.bottom() - 2;

    int x = r.center().x() - 3;

    for (int i = 0; i < 4; ++i) {
      int x1 = x + 2*i;

      p->drawLine(x1, top, x1, bottom);
    }
  }
}

// check if point inside title (not in button area)
bool
CQTitleBar::
insideTitle(const QPoint &pos) const
{
  int b  = border();
  int b1 = std::max(b, 1);

  int bw = (orientation() == Qt::Horizontal ? height() - 2*b1 : width() - 2*b1);

  int nb = 0;

  for (uint i = 0; i < buttons_.size(); ++i)
    if (buttons_[i]->isVisible())
      ++nb;

  if (orientation() == Qt::Horizontal)
    return (pos.x() < width() - nb*bw - 2*b1);
  else
    return (pos.y() > nb*bw + 2*b1);
}

// return size hint
QSize
CQTitleBar::
sizeHint() const
{
  int b = border();

  QFontMetrics fm(font());

  int h = fm.height() + 2*b;
  int w = 0;

  int nb = 0;

  for (uint i = 0; i < buttons_.size(); ++i)
    if (buttons_[i]->isVisible())
      ++nb;

  w += nb*h;

  if (! icon().isNull()) w += h;

  if (! title().isEmpty())
    w += fm.width(title());

  w = std::max(w, h);

  if (orientation() != Qt::Horizontal)
    std::swap(w, h);

  return QSize(w, h);
}

// return minimum size hint
QSize
CQTitleBar::
minimumSizeHint() const
{
  int b = border();

  QFontMetrics fm(font());

  int w, h;

  if (orientation() == Qt::Horizontal) {
    h = fm.height() + 2*b;
    w = h;
  }
  else {
    w = fm.height() + 2*b;
    h = w;
  }

  return QSize(w, h);
}

//------

// create title button
CQTitleBarButton::
CQTitleBarButton(QWidget *parent) :
 QToolButton(parent), bar_(0)
{
  setObjectName("button");

  setIconSize(QSize(10,10));

  setAutoRaise(true);

  setFocusPolicy(Qt::NoFocus);

  setCursor(Qt::ArrowCursor);
}

// handle draw
void
CQTitleBarButton::
paintEvent(QPaintEvent *)
{
  QStylePainter p(this);

  QStyleOptionToolButton opt;

#if 0
  opt.initFrom(this);

  opt.iconSize = iconSize(); //default value

  opt.icon = icon();

  if (isDown())
    opt.state |= QStyle::State_Sunken;
  else
    opt.state |= QStyle::State_Raised;

  opt.state |= QStyle::State_AutoRaise;

  opt.subControls = QStyle::SC_ToolButton;

  if (isDown())
    opt.activeSubControls = QStyle::SC_ToolButton;
  else
    opt.activeSubControls = QStyle::SC_None;

  opt.features = QStyleOptionToolButton::None;

  opt.toolButtonStyle = Qt::ToolButtonIconOnly;

  opt.pos  = pos();
  opt.font = font();
#else
  initStyleOption(&opt);
#endif

  if (bar_ && bar_->orientation() == Qt::Vertical) {
    p.translate(0, height());
    p.rotate(-90);
  }

  p.drawComplexControl(QStyle::CC_ToolButton, opt);
}
