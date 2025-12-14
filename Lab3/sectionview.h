#ifndef SECTIONVIEW_H
#define SECTIONVIEW_H

#include <QWidget>

namespace Ui {
class SectionView;
}

class SectionView : public QWidget
{
    Q_OBJECT

public:
    explicit SectionView(QWidget *parent = nullptr);
    ~SectionView();

private:
    Ui::SectionView *ui;
};

#endif // SECTIONVIEW_H
