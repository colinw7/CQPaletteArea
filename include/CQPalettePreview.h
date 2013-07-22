#ifndef CQPalettePreview_H
#define CQPalettePreview_H

#include <QWidget>
#include <QSet>

class CQPaletteArea;

class CQPalettePreview : public QObject {
  Q_OBJECT

 public:
  CQPalettePreview();

  bool active() const { return active_; }

  void setActive(bool active);

  void addWidget(QWidget *w);

  void addRect(const QRect &r);

  void clear();

 signals:
  void stopPreview();

 private:
  bool eventFilter(QObject *obj, QEvent *event);

  bool processEvent(QObject *obj, QEvent *event);

  bool isPreviewValid() const;

  bool isPreviewValid(QWidget *w, const QPoint &gp, bool press=false) const;

  bool checkPreviewRects(const QPoint &gp, int tol=0) const;

  bool isPreviewWidget(QWidget *w) const;

  bool isModalDialogWidget(QWidget *w) const;

 private:
  typedef QSet<QWidget *> Widgets;
  typedef QList<QRect>    Rects;

  bool    active_;
  bool    stopOnRelease_;
  Widgets widgets_;
  Rects   rects_;
};

#endif
