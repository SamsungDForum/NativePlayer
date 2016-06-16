/*!
 * player.js (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @file player.js
 * @brief Handles player UI.
 * @see common.js for more details.
 */

var demo_name = 'Native Player';
var demo_description = '';
var nmf_path_name = 'CurrentBin/NativePlayer.nmf';
var nacl_width = 1280;
var nacl_height = 720;
var uses_logging = true;
logs_level = 'log';

function getQueryVariable(variable) {
  var query = window.location.search.substring(1);
  var vars = query.split('&');
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split('=');
    if (pair[0] == variable) {
      if (variable == 'set')
        return Math.floor(pair[1]);
      else
        return pair[1];
    }
  }
  return false;
}

function initPlayer() {
  selected_clip = getQueryVariable('clip');
  if (clips[selected_clip].type == ClipTypeEnum.kUrl)
    ControlButtons.length = kUrlButtonControls;
}

function exampleSpecificActionAfterNaclLoad() {
  onLoadClick();
  document.getElementById('total_bar').addEventListener('click', onSeekClicked, false);
  document.getElementById('total_bar').addEventListener('mousemove', onSeekHover, false);
  document.getElementById('total_bar').addEventListener('mouseout', onSeekOut, false);
  document.getElementById('played_bar').addEventListener('click', onSeekClicked, false);

  var i, key_code = {}, supported_keys;
  if (typeof window['tizen'] != 'undefined')
    supported_keys = tizen.tvinputdevice.getSupportedKeys();
  if (supported_keys) {
    for (i = 0; i < supported_keys.length; i++)
      key_code[supported_keys[i].name] = supported_keys[i].code;

    var buttons_to_register = [
      'ColorF0Red', 'ColorF1Green', 'ColorF2Yellow', 'ColorF3Blue',
      'MediaPlay', 'MediaPause', 'MediaStop', 'MediaFastForward', 'MediaRewind',
      'MediaPlayPause', 'Return'
    ];
    buttons_to_register.forEach(function (button) {
      if (key_code.hasOwnProperty(button))
        tizen.tvinputdevice.registerKey(button);
    });
  }

  document.body.addEventListener('keydown', keyHandler);
  var element = document.getElementById('play_btn');
  element.classList.add('selected');
}

function showMenu() {
  window.location.href = 'index.html';
  exitNaCl();
}

function exitNaCl() {
  var nacl = document.getElementById('nacl_module');
  var container = document.getElementById('listener');
  container.removeChild(nacl);
  nacl = null;
}

function exitSelect(element) {
  element.size = 0;
}

function enterSelect(element) {
  element.size = element.length;
}

function changeControl(step) {
  var element = document.getElementById(ControlButtons[selected_control].id);
  if (selected_control == ControlButtonEnum.kVideoRep ||
      selected_control == ControlButtonEnum.kAudioRep ||
      selected_control == ControlButtonEnum.kSubtitleRep)
    exitSelect(element);
  else
    element.classList.remove('selected');
  selected_control  += step;
  selected_control = modulo(selected_control, ControlButtons.length);
  element = document.getElementById(ControlButtons[selected_control].id);
  if (selected_control == ControlButtonEnum.kVideoRep ||
      selected_control == ControlButtonEnum.kAudioRep ||
      selected_control == ControlButtonEnum.kSubtitleRep)
    enterSelect(element);
  else
    element.classList.add('selected');
}

function showButtons() {
  if (document.webkitFullscreenElement) {
    clearTimeout(button_timeout);
    document.getElementById('controls').className = '';
    button_timeout = setTimeout(
        function() {
          if (document.webkitFullscreenElement)
            document.getElementById('controls').className = 'inactive';
        }, 5000);
  }
}

function keyHandler(e) {
  var key_code = e.keyCode;
  showButtons();
  switch (key_code) {
  case TvKeyEnum.kKeyPlay:
    onPlayClick();
    break;
  case TvKeyEnum.kKeyPause:
    onPauseClick();
    break;
  case TvKeyEnum.kKeyPlayPause:
    onPlayPauseClick();
    break;
  case TvKeyEnum.kKeyRight:
    changeControl(kNext);
    break;
  case TvKeyEnum.kKeyLeft:
    changeControl(kPrevious);
    break;
  case TvKeyEnum.kKeyEnter:
    onEnter();
    break;
  case TvKeyEnum.kKeyStop:
    onCloseClick();
    break;
  case TvKeyEnum.kKeyFF:
    seekHelper(5);
    break;
  case TvKeyEnum.kKeyRW:
    seekHelper(-5);
    break;
  case TvKeyEnum.kKeyUp:
    updateSelectedIndex(kPrevious);
    break;
  case TvKeyEnum.kKeyDown:
    updateSelectedIndex(kNext);
    break;
  case TvKeyEnum.kKeyRed:
    selectSubtitles();
    break;
  case TvKeyEnum.kKeyGreen:
    selectVideo();
    break;
  case TvKeyEnum.kKeyYellow:
    selectAudio();
    break;
  case TvKeyEnum.kKeyBlue:
    showLogs();
    break;
  case TvKeyEnum.kKeyReturn:
    onReturn();
    break;
  }
}
