/*!
 * native_player.cc (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Michal Murgrabia
 */

#include "native_player.h"

#include <cstring>

#include <nacl_io/nacl_io.h>
#include <ppapi/c/pp_macros.h>
#include <ppapi/cpp/var_dictionary.h>

#if (PPAPI_RELEASE >= 47)
#include <ppapi/cpp/text_input_controller.h>
#endif

#include "communicator/messages.h"
#include "logger.h"

using Samsung::NaClPlayer::Rect;

const char* kLogCmd = "logs";
const char* kLogDebug = "debug";

NativePlayer::~NativePlayer() { UnregisterMessageHandler(); }

void NativePlayer::DidChangeView(const pp::View& view) {
  const pp::Rect pp_r{view.GetRect().size()};
  if (rect_ == pp_r) return;

  rect_ = pp_r;
  pp::VarDictionary message;
  message.Set(Communication::kKeyMessageToPlayer,
            static_cast<int>(Communication::MessageToPlayer::kChangeViewRect));

  message.Set(Communication::kKeyXCoordination, pp_r.x());
  message.Set(Communication::kKeyYCoordination, pp_r.y());
  message.Set(Communication::kKeyWidth, pp_r.width());
  message.Set(Communication::kKeyHeight, pp_r.height());

  LOG_DEBUG("View changed to: (x:%d, y: %d), (w:%d, h:%d)", pp_r.x(), pp_r.y(),
            pp_r.width(), pp_r.height());

  DispatchMessage(message);
}

void NativePlayer::HandleMessage(const pp::Var& message) {
  DispatchMessage(message);
}

bool NativePlayer::Init(uint32_t argc, const char** argn, const char** argv) {
  Logger::InitializeInstance(this);
  LOG_INFO("Start Init");
  for (uint32_t i = 0; i < argc; i++) {
    if (strcmp(argn[i], kLogCmd) == 0 && strcmp(argv[i], kLogDebug) == 0)
      Logger::SetStdLogLevel(LogLevel::kDebug);
  }

#if (PPAPI_RELEASE >= 47)
  // Prevents showing on-screen keyboard in Tizen 3.0
  pp::TextInputController text_input_controller(this);
  text_input_controller.SetTextInputType(PP_TEXTINPUT_TYPE_NONE);
#endif

  std::shared_ptr<Communication::MessageSender> ui_message_sender =
      std::make_shared<Communication::MessageSender>(this);

  std::shared_ptr<PlayerProvider> player_provider =
      std::make_shared<PlayerProvider>(this, std::move(ui_message_sender));

  message_receiver_ =
      std::make_shared<Communication::MessageReceiver>(player_provider);

  InitNaClIO();
  player_thread_.Start();
  RegisterMessageHandler(message_receiver_.get(),
                         player_thread_.message_loop());

  LOG_INFO("Finished Init");

  return true;
}

void NativePlayer::InitNaClIO() {
  nacl_io_init_ppapi(pp_instance(), pp::Module::Get()->get_browser_interface());
}

void NativePlayer::DispatchMessage(pp::Var message) {
  player_thread_.message_loop().PostWork(
    cc_factory_.NewCallback(
        &NativePlayer::DispatchMessageMessageOnSideThread, message));
}

void NativePlayer::DispatchMessageMessageOnSideThread(int32_t,
    pp::Var message) {
  message_receiver_->HandleMessage(this, message);
}
