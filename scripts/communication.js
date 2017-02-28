/*!
 * communication.js (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file communication.js
 * @brief NaCl application communication handler.
 * All NaCl <-> JS communication should be implemented in this file.
 */

var ClipTypeEnum = {
  kUnknown : 0,
  kUrl : 1,
  kDash : 2,
}

var clips = [
  {
    title: 'Google DASH Car',
    url: 'http://yt-dash-mse-test.commondatastorage.googleapis.com/media/car-20120827-manifest.mpd',
    subtitles: [
      { subtitle: './subs/sample_cyrilic_utf8.srt' },
    ],
    type: ClipTypeEnum.kDash,
    poster: 'resources/car.jpg',
    describe: 'This is clip with DASH content. User can choose desired video/audio representation',
  },
  {
    title: 'Google DASH encrypted',
    url: 'http://yt-dash-mse-test.commondatastorage.googleapis.com/media/oops_cenc-20121114-signedlicenseurl-manifest.mpd',
    type: ClipTypeEnum.kDash,
    poster: 'resources/oops.jpg',
    describe: 'This is clip with DASH content with DRM. User can choose desired video/audio representation',
  },
  {
    title: 'Big Buck Bunny mp4',
    url: 'http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4',
    subtitles: [
      { subtitle: './subs/sample_cyrilic.srt', encoding: 'windows-1251' },
      { subtitle: './subs/sample_cyrilic_utf8.srt' },
    ],
    encoding: 'windows-1251',
    type: ClipTypeEnum.kUrl,
    poster: 'resources/bunny.jpg',
    describe: 'This is clip played directly from URL',
  },
  {
    title: 'HEVC Single Resolution Multi-Rate',
    url: 'http://dash.akamaized.net/dash264/TestCasesHEVC/1a/1/TOS_OnDemand_HEVC_MultiRate.mpd',
    type: ClipTypeEnum.kDash,
    poster: 'resources/tos-poster.jpg',
    describe: 'http://testassets.dashif.org/#testvector/list',
  },
  {
    title: 'HEVC Multi-Resolution Multi-Rate',
    url: 'http://dash.akamaized.net/dash264/TestCasesHEVC/2a/1/TOS_OnDemand_HEVC_MultiRes.mpd',
    type: ClipTypeEnum.kDash,
    poster: 'resources/tos-poster.jpg',
    describe: 'http://testassets.dashif.org/#testvector/list',
  },
];

var TvKeyEnum = {
  kKey0: 48,
  kKey1: 49,
  kKey2: 50,
  kKey3: 51,
  kKey4: 52,
  kKey5: 53,
  kKey6: 54,
  kKey7: 55,
  kKey8: 56,
  kKey9: 57,
  kKeyLeft: 37,
  kKeyUp: 38,
  kKeyRight: 39,
  kKeyDown: 40,
  kKeyEnter: 13,
  kKeyReturn: 10009,
  kKeyGreen: 404,
  kKeyYellow: 405,
  kKeyBlue: 406,
  kKeyRed: 403,
  kKeyPlay: 415,
  kKeyPause: 19,
  kKeyStop: 413,
  kKeyFF: 417,
  kKeyRW: 412,
  kKeyPlayPause: 10252,
};

var MessageToPlayerEnum = {
  kClosePlayer : 0,
  kLoadMedia : 1,
  kPlay : 2,
  kPause : 3,
  kSeek : 4,
  kChangeRepresentation : 5,
  kChangeSubtitlesRepresentation : 7,
  kChangeSubtitlesVisibility : 8,
  kSetLogLevel : 90,
};

var MessageFromPlayerEnum = {
  kTimeUpdate : 100,
  kSetDuration : 101,
  kBufferingCompleted : 102,
  kAudioRepresentation : 103,
  kVideoRepresentation : 104,
  kSubtitlesRepresentation : 105,
  kRepresentationChanged : 106,
  kSubtitles : 107,
  kStreamEnded : 108,
};

var StreamTypeEnum = {
  kInvalid : -1,
  kVideo : 0,
  kAudio : 1
};

var ControlButtonEnum = {
  kSubtitleRep : 0,
  kSubtitles : 1,
  kFullscreen : 2,
  kRewind : 3,
  kPlay : 4,
  kForward : 5,
  kExit : 6,
  kVideoRep : 7,
  kAudioRep : 8,
};

var Selecting = {
  kAudio : 0,
  kVideo : 1,
  kSubtitles : 2,
};

var ControlButtons = [
  {
    action : sendRepChange.bind(this, Selecting.kSubtitles),
    id : 'subtitles_select',
  },
  {
    action : onSubsClick,
    id : 'subs_btn',
  },
  {
    action : onFullscreenClick,
    id : 'fullscreen_btn',
  },
  {
    action : onRewindClick,
    id : 'rewind_btn',
  },
  {
     action : onPlayPauseClick,
     id : 'play_btn',
  },
  {
    action : onForwardClick,
    id : 'forward_btn',
  },
  {
    action : onCloseClick,
    id : 'close_btn',
  },
  {
    action : sendRepChange.bind(this, Selecting.kVideo),
    id : 'video_select',
  },
  {
    action : sendRepChange.bind(this, Selecting.kAudio),
    id : 'audio_select',
  },
];

var kInit = 0;
var kNext = 1;
var kPauseSymbol = '&#10074&#10074';
var kPlaySymbol = '&#9654';
var kPrevious = -1;
var kSubsOn = 'Subs ON';
var kSubsOff = 'Subs OFF';
var kUrlButtonControls = 7;
var kSendSeekTimeout = 2000;
var kMilisecondsInSecond = 1000;

var clip_duration;
var current_time;
var to_seek = 0;

var button_timeout;
var seek_timeout;
var subtitle_timeout;

var playing = false;
var show_subtitles = true;
var ui_enabled = false;

var selected_clip = kInit;
var selected_subs = kInit;
var selected_control = ControlButtonEnum.kPlay;

function modulo(dividend, divisor) {
  return ((dividend % divisor) + divisor) % divisor;
};

function Timer(callback, delay) {
  var timerId, start, remaining = delay;

  this.hold = function() {
    window.clearTimeout(timerId);
    remaining -= new Date() - start;
  };

  this.resume = function() {
    start = new Date();
    window.clearTimeout(timerId);
    timerId = window.setTimeout(callback, remaining);
  };

  this.reset = function(new_delay) {
    remaining = new_delay;
    this.resume();
  }

  this.execute = function() {
    window.clearTimeout(timerId);
    callback();
  };

  this.resume();
}

function updateCurrentTime(time) {
  current_time = parseFloat(time);
  var current_time_box = document.getElementById('current_time');
  current_time_box.innerHTML = current_time;
  if (clip_duration < current_time)
    clip_duration = current_time;
  var played_bar = document.getElementById('played_bar');
  played_bar.style.width = parseInt(100.0 * current_time / clip_duration) + '%';
  document.getElementById('current_time').innerHTML = current_time;
  return;
}

function showSubtitles(data) {
  if (data.subtitle) {
    var element = document.getElementById('subtitle');
    element.style.display = 'block';
    element.innerHTML = data.subtitle;
    if (!subtitle_timeout)
      subtitle_timeout = new Timer(
          function() {
            element.style.display = 'none';
            element.innerHTML = '';
         }, data.duration * kMilisecondsInSecond);
    else
      subtitle_timeout.reset(data.duration * kMilisecondsInSecond);
  }
}

/**
 * This function is called when a message from NaCl arrives.
 */
function handleNaclMessage(message_event) {
  var message = message_event.data;
  if (printIfLog(message)) {  // function defined in common.js
    return;   // this was a log or error message, so we can finish this handling
  }

  if (!message_event.data.messageFromPlayer)
    return;

  switch (message_event.data.messageFromPlayer) {
  case MessageFromPlayerEnum.kTimeUpdate:
    updateCurrentTime( message_event.data.time)
    break;
  case MessageFromPlayerEnum.kSetDuration:
    clip_duration = message_event.data.time;
    clip_duration = parseFloat(clip_duration);
    document.getElementById('total_time').innerHTML = clip_duration;
    break;
  case MessageFromPlayerEnum.kBufferingCompleted:
    ui_enabled = true;
    document.getElementById('loading').style.display = 'none';
    break;
  case MessageFromPlayerEnum.kAudioRepresentation:
    document.getElementById('audio_reps').style.display = 'inline-block';
    var select = document.getElementById('audio_select');
    var option = document.createElement('option');
    option.text = message_event.data.bitrate + ' ' +
        message_event.data.language;
    select.add(option, select[message_event.data.id]);
    break;
  case MessageFromPlayerEnum.kVideoRepresentation:
    document.getElementById('video_reps').style.display = 'inline-block';
    var select = document.getElementById('video_select');
    var option = document.createElement('option');
    option.text = message_event.data.bitrate + ' ' +
        message_event.data.width + 'x' + message_event.data.height;
    select.add(option, select[message_event.data.id]);
    break;
  case MessageFromPlayerEnum.kRepresentationChanged:
    var select;
    if (message_event.data.type == StreamTypeEnum.kAudio)
      select = document.getElementById('audio_select');
    else if (message_event.data.type == StreamTypeEnum.kVideo)
      select = document.getElementById('video_select');
    select.selectedIndex = message_event.data.id;
    break;
  case MessageFromPlayerEnum.kSubtitles:
    showSubtitles(message_event.data);
    break;
  case MessageFromPlayerEnum.kSubtitlesRepresentation:
    document.getElementById('subs_btn').disabled = false;
    document.getElementById('subs_btn').innerHTML = kSubsOff;
    document.getElementById('subtitles_reps').style.display = 'inline-block';
    var select = document.getElementById('subtitles_select');
    select.disabled = false;
    var option = document.createElement('option');
    option.text = message_event.data.id + '. ' + message_event.data.language;
    select.remove(message_event.data.id);
    select.add(option, select[message_event.data.id]);
    break;
  case MessageFromPlayerEnum.kStreamEnded:
    ui_enabled = false;
    document.getElementById('ended').style.display = 'block';
    break;
  }
}

function getSeekS(e) {
  var element = e.currentTarget;
  var parent_x = e.pageX -  e.offsetX;
  var total_bar_width = document.getElementById('total_bar').offsetWidth;
  var seek_percent = (e.clientX - parent_x) / total_bar_width;
  return parseFloat(clip_duration * seek_percent);
}

function onSeekClicked(e) {
  var seek_to_s = getSeekS(e);
  sendSeek(seek_to_s);
  e.stopPropagation();
  e.preventDefault();
}

function onSeekHover(e) {
  var seek_to_s = parseInt(getSeekS(e));
  var seek_to_box = document.getElementById('seek_to_box');
  seek_to_box.innerHTML = '<i>Seek to ' + seek_to_s + ' s</i>';
  e.stopPropagation();
  e.preventDefault();
}

function onSeekOut(e) {
  var seek_to_box = document.getElementById('seek_to_box');
  seek_to_box.innerHTML = '';
  e.stopPropagation();
  e.preventDefault();
}

function onCloseClick() {
  ui_enabled = false;
  nacl_module.postMessage(
      {'messageToPlayer': MessageToPlayerEnum.kClosePlayer});
  showMenu();
}

function onSubsClick() {
  nacl_module.postMessage({'messageToPlayer':
       MessageToPlayerEnum.kChangeSubtitlesVisibility});
  var element = document.getElementById('subtitle');
  if (show_subtitles) {
    show_subtitles = false;
    element.style.display = 'none';
    document.getElementById('subs_btn').innerHTML = kSubsOn;
  } else {
    show_subtitles = true;
    element.style.display = 'block';
    document.getElementById('subs_btn').innerHTML = kSubsOff;
  }
}

function onLoadClick() {
  ui_enabled = false;

  var subtitles;
  var encoding;
  var element = document.getElementById('subtitles_menu');
  if (clips[selected_clip].subtitles && selected_subs > 0) {
    var index = selected_subs - 1;
    subtitles = clips[selected_clip].subtitles[index].subtitle;
    encoding = clips[selected_clip].subtitles[index].encoding;
  }

  var message = {
    'messageToPlayer': MessageToPlayerEnum.kLoadMedia,
    'type' : clips[selected_clip].type,
    'subtitle': subtitles,
    'encoding': encoding,
    'url': clips[selected_clip].url
  };

  if (clips[selected_clip].hasOwnProperty('drm_license_url'))
    message.drm_license_url = clips[selected_clip].drm_license_url;

  if (clips[selected_clip].hasOwnProperty('drm_key_request_properties')) {
    message.drm_key_request_properties =
        clips[selected_clip].drm_key_request_properties;
  }

  nacl_module.postMessage(message);
}

function onPlayPauseClick() {
  if (!ui_enabled)
    return;

  if (playing) {
    playing = false;
    if (subtitle_timeout)
      subtitle_timeout.hold();
    nacl_module.postMessage({'messageToPlayer': MessageToPlayerEnum.kPause});
    document.getElementById('play_btn').innerHTML = kPlaySymbol;
  } else {
    playing = true;
    if (subtitle_timeout)
      subtitle_timeout.resume();
    nacl_module.postMessage({'messageToPlayer': MessageToPlayerEnum.kPlay});
    document.getElementById('play_btn').innerHTML = kPauseSymbol;
  }
}

function onRewindClick() {
  if (!ui_enabled)
    return;

  var dummyRemote = {keyCode : TvKeyEnum.kKeyRW};
  keyHandler(dummyRemote);
}

function onForwardClick() {
  if (!ui_enabled)
    return;

  var dummyRemote = {keyCode : TvKeyEnum.kKeyFF};
  keyHandler(dummyRemote);
}

function onPlayClick() {
  if (!ui_enabled)
    return;

  playing = true;
  if (subtitle_timeout)
    subtitle_timeout.resume();
  nacl_module.postMessage({'messageToPlayer': MessageToPlayerEnum.kPlay});
  document.getElementById('play_btn').innerHTML = kPauseSymbol;
}

function onPauseClick() {
  if (!ui_enabled)
    return;

  playing = false;
  if (subtitle_timeout)
    subtitle_timeout.hold();
  nacl_module.postMessage({'messageToPlayer': MessageToPlayerEnum.kPause});
  document.getElementById('play_btn').innerHTML = kPlaySymbol;
}

function onReturn() {
  if (document.webkitFullscreenElement)
    document.webkitCancelFullScreen();
  else
    onCloseClick();
}

function onEnter() {
  ControlButtons[selected_control].action();
}

function sendSeek(to_time, from_current_time) {
  ui_enabled = false;
  if (subtitle_timeout)
    subtitle_timeout.execute();
  if (from_current_time) {
    to_time += current_time;
    nacl_module.postMessage({'messageToPlayer': MessageToPlayerEnum.kSeek,
                           'time': to_time});   // float
  } else {
    nacl_module.postMessage({'messageToPlayer': MessageToPlayerEnum.kSeek,
                           'time': to_time});   // float
  }
}

function sendChangeRepresentation(type, id) {
  nacl_module.postMessage({'messageToPlayer':
                               MessageToPlayerEnum.kChangeRepresentation,
                           'type': type,   // StreamTypeEnum
                           'id': id});     // integer
}

function sendChangeSubtitles(id) {
  nacl_module.postMessage(
      {'messageToPlayer': MessageToPlayerEnum.kChangeSubtitlesRepresentation,
       'id': id});     // integer
}

function seekHelper(seek_step) {
  clearTimeout(seek_timeout);
  to_seek += seek_step;
  var seek_to_box = document.getElementById('seek_to_box');
  seek_to_box.innerHTML = '<i>Seek ' + to_seek + ' seconds</i>';
  seek_timeout = setTimeout(
      function() {
        sendSeek(to_seek, true);
        to_seek = 0;
        var seek_to_box = document.getElementById('seek_to_box');
        seek_to_box.innerHTML = '';
      }, kSendSeekTimeout);
}

function sendRepChange(action) {
  var rep_select;
  var type = null;
  switch (action) {
  case Selecting.kAudio:
    rep_select = 'audio_select';
    type = StreamTypeEnum.kAudio;
    break;
  case Selecting.kVideo:
    rep_select = 'video_select';
    type = StreamTypeEnum.kVideo;
    break;
  case Selecting.kSubtitles:
    rep_select = 'subtitles_select';
    break;
  default:
    return;
  }

  document.getElementById(rep_select).size = 0;
  document.getElementById(rep_select).blur();
  var index = document.getElementById(rep_select).selectedIndex;
  if (type != null)
    sendChangeRepresentation(type, index);
  else
    sendChangeSubtitles(index);
}

function updateSelectedIndex(step) {
  var element = document.getElementById(ControlButtons[selected_control].id);
  element.selectedIndex = modulo(element.selectedIndex + step, element.size);
}

function showLogs() {
  var logs = document.getElementById('logs_area');
  if (logs.style.display == 'block')
    logs.style.display = 'none';
  else
    logs.style.display = 'block';
}

function onFullscreenClick() {
  var videoContent = document.getElementById('listener');
  if (!document.webkitFullscreenElement) {
    if (videoContent.webkitRequestFullscreen)
      videoContent.webkitRequestFullscreen();
      document.getElementById('controls').className = 'inactive';
      document.getElementById('listener').addEventListener(
          'mousemove', showButtons, false);
  } else if (document.webkitCancelFullScreen) {
    document.webkitCancelFullScreen();
    document.getElementById('controls').className = '';
    document.getElementById('listener').removeEventListener(
        'mousemove', keyHandler);
  }
}

function updateLogLevel(level) {
  logs_level = level;
  nacl_module.postMessage(
      {'messageToPlayer': MessageToPlayerEnum.kSetLogLevel,
       'level': logs_level});

}
