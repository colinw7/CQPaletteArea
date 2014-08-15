#ifndef CQTitleBar_H
#define CQTitleBar_H

#include <QToolButton>

class CQTitleBarButton;

// Title bar widget with title, orientation and a number of buttons
class CQTitleBar : public QWidget {
  Q_OBJECT

  Q_PROPERTY(QString         title           READ title           WRITE setTitle          )
  Q_PROPERTY(QIcon           icon            READ icon            WRITE setIcon           )
  Q_PROPERTY(Qt::Orientation orientation     READ orientation     WRITE setOrientation    )
  Q_PROPERTY(int             border          READ border          WRITE setBorder         )
  Q_PROPERTY(QColor          backgroundColor READ backgroundColor WRITE setBackgroundColor)
  Q_PROPERTY(QColor          barColor        READ barColor        WRITE setBarColor       )

 public:
  CQTitleBar(Qt::Orientation orient=Qt::Horizontal, QWidget *parent=0);
  CQTitleBar(QWidget *parent, Qt::Orientation orient=Qt::Horizontal);

  virtual ~CQTitleBar() { }

  // set/get title
  virtual QString title() const { return title_; }
  virtual void setTitle(const QString &title);

  // set/get icon
  virtual QIcon icon() const { return icon_; }
  virtual void setIcon(const QIcon &icon);

  // set/get orientation
  Qt::Orientation orientation() const { return orient_; }
  void setOrientation(Qt::Orientation orient);

  // set/get border
  virtual int border() const { return border_; }
  virtual void setBorder(int border);

  // set/get background color
  virtual QColor backgroundColor() const { return bgColor_; }
  void setBackgroundColor(const QColor &color);

  // set/get bar line color
  virtual QColor barColor() const { return barColor_; }
  void setBarColor(const QColor &color);

  // add button
  CQTitleBarButton *addButton(const QIcon &icon);

  // add button
  void addButton(CQTitleBarButton *button);

  bool insideTitle(const QPoint &pos) const;

 protected:
  void showEvent(QShowEvent *);

  void paintEvent(QPaintEvent *);

  void resizeEvent(QResizeEvent *);

  void updateLayout();

  void drawTitleBarLines(QPainter *p, const QRect &r, const QColor &c);

 public:
  QSize sizeHint() const;

  QSize minimumSizeHint() const;

 private:
  typedef std::vector<CQTitleBarButton *> Buttons;

  QString         title_;
  QIcon           icon_;
  Qt::Orientation orient_;
  int             border_;
  QColor          bgColor_;
  QColor          barColor_;
  Buttons         buttons_;
};

// title bar button
class CQTitleBarButton : public QToolButton {
 public:
  CQTitleBarButton(QWidget *parent=0);

  void setTitleBar(CQTitleBar *bar) { bar_ = bar; }

 private:
  void paintEvent(QPaintEvent *);

 private:
  CQTitleBar *bar_;
};

#endif
