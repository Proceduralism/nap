#include <QStyleOptionButton>
#include <QPainter>
#include <QStylePainter>
#include <QtGui>
#include "ruler.h"

napkin::Ruler::Ruler(QWidget* parent) : QWidget(parent)
{
	mTimeConfig = new SMPTETimeDisplay();
//	mTimeConfig = new FloatTimeDisplay();
//	mTimeConfig = new GeneralTimeDisplay();

	mFont.setFamily("Monospace");
	mFont.setStyleHint(QFont::TypeWriter);
	setHatchFont(mFont);
}


void napkin::Ruler::setRange(const napkin::Range& range)
{
	if (mRange == range)
		return;

	mRange.set(range);
	update();
}

void napkin::Ruler::paintEvent(QPaintEvent* event)
{
	QStylePainter painter;
	painter.begin(this);

	painter.setBrush(palette().background());
	painter.setPen(Qt::NoPen);
	painter.setFont(mFont);
	painter.drawRect(rect());
	{
		painter.setPen(QPen(palette().foreground().color()));

		drawHatches(painter, mMinorHatchLength, mTimeConfig->minorHatchSpacing(), false);
		drawHatches(painter, mMajorHatchLength, mTimeConfig->majorHatchSpacing(), true);
	}
	painter.end();
}


void napkin::Ruler::drawHatches(QPainter& painter, int hatchLength, qreal minStepSize, bool drawLabels) const
{
	qreal start = mRange.start();
	qreal end = mRange.end();
	if (start > end)
	{
		start = mRange.end();
		end = mRange.start();
	}
	qreal length = end - start;

	qreal viewScale = width() / length;
	qreal stepInterval = mTimeConfig->calcStepInterval(length, width(), minStepSize);

	qreal stepSize = viewScale * stepInterval;
	qreal startOffset = -start * viewScale;
	qreal localOffset = fmod(startOffset, stepSize);
	int timeOffset = qFloor(start / stepInterval);

	if (timeOffset < 0)
		timeOffset++;

	int stepCount = qCeil(length / stepInterval) + 1;

	int y = height() - hatchLength;
	int textY = mTextHeight;
	for (int i = 0; i < stepCount; i++)
	{
		// floor instead of round, matches QGraphicsView aliasing
		int x = qFloor(localOffset + (qreal) i * stepSize);

		painter.drawLine(x, y, x, height());

		if (drawLabels)
		{
			qreal time = (timeOffset + i) * stepInterval;

			const QString timestr = mTimeConfig->timeToString(stepInterval, time);
			painter.drawText(x + mLabelOffset.x(),
							 textY + mLabelOffset.y(),
							 timestr);
		}
	}
}



void napkin::Ruler::setHeight(int height)
{
	setMinimumHeight(height);
	setMaximumHeight(height);
}

void napkin::Ruler::setHatchFont(const QFont& font)
{
	mFont = font;

	QFontMetrics metrics(mFont);

	mTextHeight = metrics.height();
}

void napkin::Ruler::resizeEvent(QResizeEvent* event)
{
	update();
}




