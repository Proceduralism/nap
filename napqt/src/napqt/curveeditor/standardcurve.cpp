
#include "standardcurve.h"

#include <cassert>

using namespace napqt;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StandardCurveModel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int StandardCurveModel::curveCount() const
{
	return static_cast<int>(mCurves.size());
}

AbstractCurve* StandardCurveModel::curve(int index)
{
	return mCurves.at(index);
}

int StandardCurveModel::curveIndex(AbstractCurve* curve) const
{
	for (int i = 0; i < mCurves.size(); i++)
		if (mCurves[i] == curve)
			return i;
	return -1;
}

StandardCurve* StandardCurveModel::addCurve()
{
	auto curve = new StandardCurve(this);
	connect(curve, &AbstractCurve::changed, this, &StandardCurveModel::onCurveChanged);

	int index = mCurves.size();
	mCurves << curve;
	curvesAdded({index});
	return curve;
}

void StandardCurveModel::removeCurve(AbstractCurve* curve)
{
	removeCurve(curveIndex(curve));
}

void StandardCurveModel::removeCurve(int index)
{
	auto idx = static_cast<size_t>(index);
	mCurves.erase(mCurves.begin() + idx);
	curvesRemoved({index});
}

void StandardCurveModel::onCurveChanged(AbstractCurve* curve)
{
	curvesChanged({curveIndex(curve)});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StandardCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StandardCurve::StandardCurve(StandardCurveModel* parent) : AbstractCurve(parent)
{

}


int napqt::StandardCurve::pointCount() const
{
	return mPoints.size();
}

void StandardCurve::setName(const QString& name)
{
	mName = name;
	changed(this);
}

QVariant napqt::StandardCurve::data(int index, int role) const
{
	auto& p = mPoints.at(index);
	switch (role)
	{
		case datarole::POS:
			return p.pos;
		case datarole::IN_TAN:
			return p.inTan;
		case datarole::OUT_TAN:
			return p.outTan;
		case datarole::INTERP:
			return QVariant::fromValue(p.interp);
		default:
			return QVariant();
	}
}

void napqt::StandardCurve::setData(int index, int role, QVariant value)
{
	auto& p = mPoints[index];
	bool ok;
	switch (role)
	{
		case datarole::POS:
			p.pos = value.toPointF();
			return;
		case datarole::IN_TAN:
			p.inTan = value.toPointF();
			return;
		case datarole::OUT_TAN:
			p.outTan = value.toPointF();
			return;
		case datarole::INTERP:
			p.interp = value.value<InterpType>();
			return;
		default:
			break;
	}
	pointsChanged({index});
}

void napqt::StandardCurve::addPoint(qreal time, qreal value, InterpType interp)
{
	mPoints << StandardPoint(QPointF(time, value), interp);
	pointsAdded({mPoints.size()});
}

void napqt::StandardCurve::removePoint(int index)
{
	mPoints.removeAt(index);
	pointsRemoved({index});
}

StandardCurveModel* StandardCurve::model()
{
	return dynamic_cast<StandardCurveModel*>(parent());
}
