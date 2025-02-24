//--
// This file is part of Sonic Pi: http://sonic-pi.net
// Full project source: https://github.com/samaaron/sonic-pi
// License: https://github.com/samaaron/sonic-pi/blob/main/LICENSE.md
//
// Copyright 2022 by Sam Aaron (http://sam.aaron.name).
// All rights reserved.
//
// Permission is granted for use, copying, modification, and
// distribution of modified versions of this work as long as this
// notice is included.
//++

#include "sonicpimetro.h"
#include <QVBoxLayout>
#include "qt_api_client.h"
#include <QStyleOption>
#include <QPainter>

SonicPiMetro::SonicPiMetro(std::shared_ptr<SonicPi::QtAPIClient> spClient, std::shared_ptr<SonicPi::SonicPiAPI> spAPI, SonicPiTheme *theme, QWidget* parent)
  : QWidget(parent)
  , m_spClient(spClient)
  , m_spAPI(spAPI)
{
  this->theme = theme;
  mutex = new QMutex;
  enableLinkButton = new QPushButton(tr("Link"));
  enableLinkButton->setAutoFillBackground(true);
  enableLinkButton->setObjectName("enableLinkButton");
  enableLinkButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  enableLinkButton->setFlat(true);
  #ifdef Q_OS_MAC
  QString link_shortcut = QKeySequence("Ctrl+t").toString(QKeySequence::NativeText);
#else
  QString link_shortcut = QKeySequence("alt+t").toString(QKeySequence::NativeText);
#endif
  enableLinkButton->setToolTip(tr("Enable/Disable network sync.\nThis controls whether the Link metronome will synchronise with other Link metronomes on the local network.") + "\n(" + link_shortcut + ")");

  tapButton = new QPushButton(tr("Tap"));
  tapButton->setAutoFillBackground(true);
  tapButton->setObjectName("tapButton");
  tapButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  tapButton->setFlat(true);

  tapButton->setToolTip(tr("Tap tempo.\nClick repeatedly to the beat to set the BPM manually.\nAccuracy increases with every additional click.") + "\n(" + QKeySequence("Shift+Return").toString(QKeySequence::NativeText) + ")");


  QHBoxLayout* metro_layout  = new QHBoxLayout;
  QWidget* spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  setLayout(metro_layout);

  bpmScrubWidget = new BPMScrubWidget(m_spClient, m_spAPI, theme);
  bpmScrubWidget->setObjectName("bpmScrubber");
  bpmScrubWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  bpmScrubWidget->setToolTip(tr("Current Link BPM. Edit or drag to modify."));
  metro_layout->addWidget(enableLinkButton);
  metro_layout->addWidget(tapButton);
  metro_layout->addWidget(bpmScrubWidget);
  metro_layout->addWidget(spacer);

  connect(enableLinkButton, &QPushButton::clicked, [=]() {
    this->toggleLink();
  });

  connect(tapButton, &QPushButton::clicked, [=]() {
    this->tapTempo();
  });

  connect(m_spClient.get(), &SonicPi::QtAPIClient::UpdateNumActiveLinks, this, &SonicPiMetro::updateActiveLinkCount);

  connect(m_spClient.get(), &SonicPi::QtAPIClient::UpdateBPM, this, &SonicPiMetro::setBPM);



  updateLinkButtonDisplay();
}

void SonicPiMetro::linkEnable()
{
  mutex->lock();

  if(!m_linkEnabled) {
    m_spAPI.get()->LinkEnable();
    m_linkEnabled = true;
  }

  emit linkEnabled();
  updateLinkButtonDisplay();
  mutex->unlock();
}

void SonicPiMetro::linkDisable()
{

  mutex->lock();

  if(m_linkEnabled) {
    m_spAPI.get()->LinkDisable();
    m_linkEnabled = false;
  }

  emit linkDisabled();
  updateLinkButtonDisplay();
  mutex->unlock();
}

void SonicPiMetro::toggleLink()
{
  mutex->lock();
  m_linkEnabled = !m_linkEnabled;

  if(m_linkEnabled) {
    m_spAPI.get()->LinkEnable();
    emit linkEnabled();
  }
  else {
    m_spAPI.get()->LinkDisable();
    emit linkDisabled();
  }

  updateLinkButtonDisplay();
  mutex->unlock();
}

void SonicPiMetro::updateActiveLinkCount(int count)
{
  numActiveLinks = count;
  updateLinkButtonDisplay();
}

void SonicPiMetro::updateActiveLinkText()
{
  if(numActiveLinks == 1) {
    enableLinkButton->setText("1 Link");
  } else {
    enableLinkButton->setText(QString("%1 Links").arg(numActiveLinks));
  }
}

void SonicPiMetro::updateLinkButtonDisplay()
{
  QString qss;
  if(m_linkEnabled) {
    updateActiveLinkText();
    qss = QString("\nQPushButton {\nbackground-color: %1;}\nQPushButton::hover:!pressed {\nbackground-color: %2}\n").arg(theme->color("PressedButton").name()).arg(theme->color("PressedButton").name());
    enableLinkButton->setStyleSheet(theme->getAppStylesheet() + qss);

    qss = QString("\nQLineEdit#bpmScrubber {\nborder-color: %1;}\n \nQLineEdit#bpmScrubber::hover:!pressed {\nbackground-color: %2;}\n").arg(theme->color("PressedButton").name()).arg(theme->color("PressedButton").name());
    bpmScrubWidget->setStyleSheet(theme->getAppStylesheet() + qss);

  } else {
    enableLinkButton->setText("Link");
    qss = QString("\nQPushButton {\nbackground-color: %1;}\nQPushButton::hover:!pressed {\nbackground-color: %2}\n").arg(theme->color("Button").name()).arg(theme->color("HoverButton").name());
    enableLinkButton->setStyleSheet(theme->getAppStylesheet() + qss);

    qss = QString("\nQLineEdit#bpmScrubber {\nborder-color: %1;}\n \nQLineEdit#bpmScrubber::hover:!pressed {\nbackground-color: %2;}\n").arg(theme->color("HoverButton").name()).arg(theme->color("HoverButton").name());

    bpmScrubWidget->setStyleSheet(theme->getAppStylesheet() + qss);
  }
}

void SonicPiMetro::setBPM(double bpm)
{
  bpmScrubWidget->setAndDisplayBPM(bpm);
}

void SonicPiMetro::updateColourTheme()
{

  updateLinkButtonDisplay();
}

 void SonicPiMetro::paintEvent(QPaintEvent *)
 {
     QStyleOption opt;
     opt.initFrom(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
 }

void SonicPiMetro::tapTempo()
{
    QString qss = QString("\nQPushButton#tapButton\n {\nbackground-color: %1;\ncolor: %2;\n}\n").arg(theme->color("PressedButton").name()).arg(theme->color("ButtonText").name());
    tapButton->setStyleSheet(theme->getAppStylesheet() + qss);

  QTimer::singleShot(250, this, [=]() {
    tapButton->setStyleSheet(theme->getAppStylesheet());
  });


  qint64 timeStamp = QDateTime::currentMSecsSinceEpoch();
  numTaps = numTaps + 1;

  if(numTaps == 1) {
    firstTap = timeStamp;
  } else {

    double timeSinceLastTap = (double)(timeStamp - lastTap);
    double totalTapDistance = (double)(timeStamp - firstTap);
    double avgDistance = totalTapDistance / (numTaps - 1);

    //make sure the first three taps are similarly spaced
    if (((numTaps < 3) &&
         ((timeSinceLastTap > (avgDistance + 30)) ||
          (timeSinceLastTap < (avgDistance - 30)))) ||

        //drop the accuracy requirement for later taps as the error
        //introduced by input timing jitter of the last tap reduces as
        //the tap count increases.
        ((timeSinceLastTap > (avgDistance + 50)) ||
         (timeSinceLastTap < (avgDistance - 50))))
      {
        bpmScrubWidget->displayResetVisualCue();
        numTaps = 1;
        firstTap = timeStamp;

      } else if (numTaps > 2) {
      double newBpm = round(60.0 / (avgDistance / 1000.0));
      if(newBpm != bpmScrubWidget->getBPM()) {
        bpmScrubWidget->setDisplayAndSyncBPM(newBpm);
        bpmScrubWidget->displayBPMChangeVisualCue();
      }
    }
  }
  lastTap = timeStamp;
}
