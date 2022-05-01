/**
 * @file playercontroller.cpp
 * @author Mikra Selene
 * @version 1.0
 * @date 2022.04.05
 *
 * @section LICENSE
 *
 * Copyright (C) 2022 Mikra Selene
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "PlayerController.h"

#include <QMediaMetaData>
#include <QDockWidget>
#include <QTextEdit>

#include "ui_Player.h"

PlayerController::PlayerController(const QPointer<Player> &player) {
  player_ = player;
  auto ui = player_->ui();

  // connect clicks on buttons to handlers
#define on_click_connection(sender, slot) \
  connect(ui->sender, &QPushButton::clicked, this, &PlayerController::slot)
  on_click_connection(prev, onClickPrev);
  on_click_connection(play, onClickPlay);
  on_click_connection(next, onClickNext);
  on_click_connection(stop, onClickStop);
  on_click_connection(info, onClickInfo);
  on_click_connection(btn_volume, onClickBtnVolume);
  on_click_connection(btn_play_order, onClickBtnPlayOrder);
#undef on_click_connection

  // connect changes of sliders to handlers
#define on_change_slider_connection(sender, signal, slot) \
  connect(ui->sender, &QSlider::signal, this, &PlayerController::slot)
  on_change_slider_connection(progress_slider, sliderMoved, onChangeProgress);
  on_change_slider_connection(progress_slider, sliderPressed, onChangeProgress);
  on_change_slider_connection(volume_slider, valueChanged, onChangeVolume);
#undef on_change_slider_connection

  // connect changes of the checkbox and the combobox to handlers
  connect(ui->rewind, &QCheckBox::stateChanged, this,
          &PlayerController::onChangeRate);
  connect(ui->rate, &QComboBox::currentTextChanged, this,
          &PlayerController::onChangeRate);

  // connect signals to Qt media controllers
#define media_control_connection(signal, slot) \
  connect(this, &PlayerController::signal, player_->mediaPlayer(), slot)
  media_control_connection(play, &QMediaPlayer::play);
  media_control_connection(pause, &QMediaPlayer::pause);
  media_control_connection(stop, &QMediaPlayer::stop);
  // todo: connect to MediaList
  connect(this, &PlayerController::prev, this, []() -> void {});
  connect(this, &PlayerController::prev, this, []() -> void {});
  // todo: connect to metaData
  connect(this, &PlayerController::info, this, []() -> void {});
  media_control_connection(changeRate, &QMediaPlayer::setPlaybackRate);
  media_control_connection(changeProgress, &QMediaPlayer::setPosition);
  connect(this, &PlayerController::stop, ui->video_widget,
          QOverload<>::of(&QVideoWidget::update));
  connect(this, &PlayerController::changeVolume, player_->audioOutput(),
          &QAudioOutput::setVolume);
  connect(player_->mediaPlayer(), &QMediaPlayer::mediaStatusChanged, this,
          &PlayerController::atEnd);
#undef control_connection
}

#pragma region  // region: controller slots

/**
 * @brief Handler called when the `play` button is clicked. It emits signal
 * `pause()`/`play` and change the label of `play` button to "PLAY"/"PAUSE"
 * depending on current playing state. It also set main window title to artist
 * - title format.
 * @xxx It ineffectively set the total progress of the media each time `play`
 * is clicked. The fallback of artist and title name is inelegant.
 */
void PlayerController::onClickPlay() {
  qDebug() << "clicked: play";
  bool playing = player_->state() == QMediaPlayer::PlayingState;
  emit playing ? pause() : play();
  player_->setButtonLabelPlay(playing);

  player_->setProgressSliderMax(static_cast<int>(player_->duration()));

  // default values of artist and title will be "Untitled" and "V/A"
  auto metadata = player_->metaData();
  auto artist = metadata.value(QMediaMetaData::AlbumArtist).toString();
  auto title = metadata.value(QMediaMetaData::Title).toString();
  if (artist == "") {
    artist = metadata.value(QMediaMetaData::ContributingArtist).toString();
  }  // todo!
  player_->setWindowTitle(QString("%1 - %2").arg(
      artist == "" ? "V/A" : artist, title == "" ? "Untitled" : title));
}

/**
 * @brief Handler called when the `stop` button is clicked. The label of `play`
 * button should be reset to "PLAY".
 */
void PlayerController::onClickStop() {
  qDebug() << "clicked: stop";
  emit stop();
  player_->setButtonLabelPlay(true);
}

/**
 * @brief Handler called when the `prev` button is clicked.
 */
void PlayerController::onClickPrev() {
  qDebug() << "clicked: prev";
  emit prev();
}

/**
 * @brief Handler called when the `next` button is clicked.
 */
void PlayerController::onClickNext() {
  qDebug() << "clicked: next";
  emit next();
}

/**
 * @brief Handler called when the `info` button is clicked. It gets the metadata
 * of the media.
 * @todo It just print the metadata of the media file to debug stream for now.
 */
void PlayerController::onClickInfo() {
  qDebug() << "clicked: info";
  qDebug() << "media metadata:";
  auto metadata = player_->metaData();
  QString text = "";
  for (auto k : metadata.keys()) {
    qDebug() << k << ":" << metadata[k];
    text += metadata.metaDataKeyToString(k) + ":" + metadata[k].toString() + '\n';
  }
  player_->addFloatTable(0, 0, text);

  emit info();
}

/**
 * @brief Handler called when the value of the progress slider is changed.
 */
void PlayerController::onChangeProgress() {
  qDebug() << "changed: progress";
  emit changeProgress(player_->sliderProgress());
}

/**
 * @brief Handler called when the value of the volume slider is changed.
 * If the previous volume or current volume is 0, change then volume icon
 * matching the current volume.
 */
void PlayerController::onChangeVolume() {
  qDebug() << "changed: volume";
  float volumeCur = player_->sliderVolume();
  float volumePre = player_->audioOutput()->volume();
  if(volumeCur && !volumePre){
      player_->setButtonVolume(1);
  } else if(!volumeCur && volumePre){
      player_->setButtonVolume(0);
  }
  emit changeVolume(volumeCur);
}

/**
 * @brief Handler called when the value of `rate` combobox is changed.
 */
void PlayerController::onChangeRate() {
  qDebug() << "changed: rate";
  player_->setLoopLock(player_->rewind());
  emit changeRate((player_->rewind() ? -1.0 : 1.0) * player_->comboBoxRate());
}

/*!
 * @brief Handler called when the media is at it's end. If the `loop` checkbox
 * is checked, it emits a `play()` signal to replay the media, otherwise it
 * emits `stop()`.
 */
void PlayerController::atEnd() {
  if (player_->endOfMedia()) {
    qDebug() << "end of media";
//    emit player_->loop() ? play() : stop();
//    player_->setButtonLabelPlay(!player_->loop());
    bool continuePlay = playOrder!= onlyOnce;
    emit continuePlay ? play() : stop();
    player_->setButtonLabelPlay(!continuePlay);
  }
}

/**
 * @brief Handler called when the value of `btn_volume` button is clicked.
 * It set the volume 0 or previous value.
 */
void PlayerController::onClickBtnVolume(){
  qDebug() << "clicked: btn_volume";
  float volume = player_->audioOutput()->volume();
  if(volume){
    player_->setButtonVolume(0);
    emit changeVolume(0);
  } else {
    player_->setButtonVolume(1);
    emit changeVolume(player_->sliderVolume());
  }
}

void PlayerController::onClickBtnPlayOrder(){
  switch(playOrder){
    case onlyOnce:{
      qDebug() << "clicked: btn_play_order change to play onlyOnce";
      playOrder = inOrder;
      player_->setPlayOrderIcon(1);
      break;
    };
    case inOrder:{
      qDebug() << "clicked: btn_play_order change to play randomLoop";
      playOrder = randomLoop;
      player_->setPlayOrderIcon(2);
      break;
    };
    case randomLoop:{
      qDebug() << "clicked: btn_play_order change to play singleLoop";
      playOrder = singleLoop;
      player_->setPlayOrderIcon(3);
      break;
    };
    case singleLoop:{
      qDebug() << "clicked: btn_play_order change to play onlyOnce";
      playOrder = onlyOnce;
      player_->setPlayOrderIcon(0);
      break;
    };
    default:{
      qDebug() << "error!!";
    };
  }
}

#pragma endregion
