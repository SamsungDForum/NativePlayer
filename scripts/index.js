/*!
 * index.js (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @file index.js
 * @brief Handles main menu.
 */

function keyMenuHandler(e) {
  var key_code = e.keyCode;
  switch (key_code) {
  case TvKeyEnum.kKeyEnter:
    showPlayer();
    document.body.addEventListener('keydown', keyHandler);
    break;
  case TvKeyEnum.kKeyRight:
    changeClip(kNext);
    break;
  case TvKeyEnum.kKeyLeft:
    changeClip(kPrevious);
    break;
  }
}

function init() {
  selected_clip = kInit;
  changeClip(kInit);
  document.body.addEventListener('keydown', keyMenuHandler);
}

function setBackground(object, index) {
  object.style.backgroundImage = 'url(' + clips[index].poster + ')';
}

function setTitle(object, index) {
  object.innerHTML = '<h2>' + clips[index].title + '</h2>';
}

function setDescription(object, index) {
  object.innerHTML += '<br>' + clips[index].describe
      + '<br><br>Source:&nbsp<font color="grey">' + clips[selected_clip].url
      + '</font>';
}

function changeClip(step) {
  selected_clip += step;
  selected_clip = modulo(selected_clip, clips.length);
  var previous = modulo(selected_clip - 1, clips.length);

  var inputObject = document.getElementById('left');
  var parent = inputObject.parentNode;

  parent.removeChild(inputObject);
  if (step > 0 && inputObject.style.webkitAnimationName !== 'center_left')
    inputObject.style.webkitAnimationName = 'center_left';
  else if (step < 0 && inputObject.style.webkitAnimationName !== 'right_left')
    inputObject.style.webkitAnimationName = 'right_left';
  inputObject.style.webkitAnimationDuration = '1s';

  parent.appendChild(inputObject);
  inputObject.className = 'other';
  setBackground(inputObject, previous);
  setTitle(inputObject, previous);

  inputObject = document.getElementById('center');
  parent.removeChild(inputObject);
  if (step > 0 && inputObject.style.webkitAnimationName !== 'right_center')
    inputObject.style.webkitAnimationName = 'right_center';
  else if (step < 0 && inputObject.style.webkitAnimationName !== 'left_center')
    inputObject.style.webkitAnimationName = 'left_center';
  inputObject.style.webkitAnimationDuration = '1s';

  parent.appendChild(inputObject);
  inputObject.className = 'center';
  setBackground(inputObject, selected_clip);
  setTitle(inputObject, selected_clip);
  setDescription(inputObject, selected_clip);

  var next = modulo(selected_clip + 1, clips.length);
  inputObject = document.getElementById('right');
  parent.removeChild(inputObject);

  if (step > 0 && inputObject.style.webkitAnimationName !== 'left_right')
    inputObject.style.webkitAnimationName = 'left_right';
  else if (step < 0 && inputObject.style.webkitAnimationName !== 'center_right')
    inputObject.style.webkitAnimationName = 'center_right';
  inputObject.style.webkitAnimationDuration = '1s';

  parent.appendChild(inputObject);
  inputObject.className = 'other';
  setBackground(inputObject, next);
  setTitle(inputObject, next);
}

function showPlayer() {
  window.location.href = 'player.html?clip=' + selected_clip;
}
