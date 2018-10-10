#include <nap/logger.h>
#include <generic/randomnames.h>
#include "timelinepanel.h"
#include <QList>

using namespace napkin;

TimelinePanel::TimelinePanel() : QWidget()
{
	// Main layout
	setLayout(&mLayout);
	mLayout.setContentsMargins(0, 0, 0, 0);
	mLayout.setSpacing(0);

	// TimelineOutline
	mSplitter.addWidget(&mOutline);

	// Timeline layout
	mTimelineLayout.setSpacing(0);
	mTimelineLayout.setContentsMargins(0, 0, 0, 0);
	mTimelineLayout.addWidget(&mRuler);
	mTimelineLayout.addWidget(&mView);
	mTimelineWidget.setLayout(&mTimelineLayout);
	mSplitter.addWidget(&mTimelineWidget);

	// Splitter
	mSplitter.setSizes({300, 1000});
	mSplitter.setStretchFactor(0, 0);
	mSplitter.setStretchFactor(1, 1);
	mLayout.addWidget(&mSplitter);

	// Data
	mView.setScene(&mScene);
	mView.setPanBounds(QRectF(0, 0, std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::max()));
	connect(&mSplitter, &QSplitter::splitterMoved, this, &TimelinePanel::onTimelineViewTransformed);
	connect(&mView, &GridView::viewTransformed, this, &TimelinePanel::onTimelineViewTransformed);

	connect(&mOutline, &TimelineOutline::verticalScrollChanged, [this](int value) {
		mView.setVerticalScroll(value);
	});

	connect(&mOutline, &TimelineOutline::trackVisibilityChanged, [this]() {
		mScene.setVisibleTracks(mOutline.getVisibleTracks());
		// Constrain scrolling according to outline
		int rectHeight = qMax(0, mOutline.overflowHeight());
		mView.setPanBounds(0, 0, std::numeric_limits<qreal>::infinity(), rectHeight);
	});

	int rulerHeight = 30;
	mRuler.setHeight(rulerHeight);
	mOutline.setHeaderHeight(rulerHeight);
//	mView.setGridEnabled(false);

	setTimeScale(10);

	mScene.setVisibleTracks(mOutline.getVisibleTracks());

	mTimeDisplays.emplace_back(std::make_unique<SMPTETimeDisplay>());
	mTimeDisplays.emplace_back(std::make_unique<GeneralTimeDisplay>());
	mTimeDisplays.emplace_back(std::make_unique<FloatTimeDisplay>());
	mTimeDisplays.emplace_back(std::make_unique<AnimationTimeDisplay>());

//    initOutlineModelHandlers();
	createTimeFormatActionGroup();

	setupRulerContextMenu();

	demo();
}

void TimelinePanel::setTimeScale(qreal scale)
{
	mView.setTimeScale(scale);
}

void TimelinePanel::showEvent(QShowEvent* event)
{
}


void TimelinePanel::setTimeline(Timeline* timeline)
{
	mScene.setTimeline(timeline);
	mOutline.setTimeline(timeline);
}

void TimelinePanel::onTimelineViewTransformed()
{
	auto scroll = getTranslation(mView.transform());
	int s = qRound(-scroll.y());
	mOutline.setVerticalScroll(s);
	mRuler.setRange(mView.getViewRange());
}

void TimelinePanel::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	onTimelineViewTransformed();
}


void TimelinePanel::demo()
{
	namegen::NameGen gen;

	auto timeline = new Timeline(this);
	timeline->setMinEventLength(1.0/timeline->framerate());

	{
		auto track = timeline->addTrack("Event Track One");
		track->addEvent("FirstEvent", 0, 100);
		track->addEvent("Event Zwei", 100, 150);
		track->addEvent("Derde Event", 160, 250);
	}
	{
		auto track = timeline->addTrack("Tick Track");
		track->addTick(0);
		track->addTick(100);
		track->addTick(150);
		track->addTick(200);
	}
	{
		auto track = timeline->addTrack("Second Event Track");
		track->setHeight(20);
		auto child1 = track->addTrack("A Child Track");
		child1->addEvent("Pookie", 50, 90);
		child1->addEvent("Wookie", 100, 140);
		child1->addEvent("Dookie", 150, 190);
		auto child2 = track->addTrack("Another Child Track");
		child2->addEvent("Rob", 20, 65);
		child2->addEvent("Knob", 70, 115);
		child2->addEvent("Bob", 120, 135);
	}
	{
		auto track = timeline->addTrack("Track number Three");
		track->addEvent("0", 0, 10);
		track->addEvent("10", 10, 20);
		track->addEvent("20", 20, 30);
		track->addEvent("30", 30, 40);
		track->addEvent("40", 40, 50);
		track->addEvent("50", 50, 60);
		track->addEvent("60", 60, 70);
		track->addEvent("70", 70, 80);
		track->addEvent("80", 80, 90);
		track->addEvent("90", 90, 100);
	}
	{
		auto extraTrack = timeline->addTrack("Extra Tracks");
		extraTrack->setHeight(20);
		for (int i=0; i<10; i++) {
			int offset = 30 * i;
			auto track = extraTrack->addTrack(QString("Extra Track Whee %1").arg(i));
			track->addEvent(QString("Empty Event %1").arg(i), offset, 300 + offset);
		}
	}

//	int trackCount = 30;
//	int eventCount = 10;
//
//	auto framestep = 1.0 / timeline->framerate();
//
//	for (int i = 0; i < trackCount; i++)
//	{
//		auto trackname = QString::fromStdString(gen.multiple());
//		auto track = timeline->addTrack(trackname);
//
//		qreal t = 0;
//		for (int e = 0; e < eventCount; e++)
//		{
//			t += namegen::randint(0, 40) * framestep;
//			qreal len = namegen::randint(20, 300) * framestep;
//			auto eventname = QString::fromStdString(gen.multiple());
//			track->addEvent(eventname, t, t + len);
//			t += len;
//		}
//	}

	setTimeline(timeline);

}

QActionGroup& TimelinePanel::createTimeFormatActionGroup()
{
	auto actionGroupTimeFormat = new QActionGroup(this);
	for (const auto& timedisplay : mTimeDisplays)
	{
		auto action = actionGroupTimeFormat->addAction(timedisplay->name());
		connect(action, &QAction::triggered, [this, &timedisplay]() {
			mRuler.setDisplayFormat(timedisplay.get());
		});
	}

	return *actionGroupTimeFormat;
}
void TimelinePanel::setupRulerContextMenu()
{
	for (auto& disp : mTimeDisplays)
	{
		auto a = new QAction(disp->name(), &mRuler);
		connect(a, &QAction::triggered, [this, &disp]() {
			mRuler.setDisplayFormat(disp.get());
		});
		mRuler.addAction(a);
	}
	mRuler.setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
}







