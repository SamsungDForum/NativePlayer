/*!
 * native_player.h (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_NATIVE_PLAYER_H_
#define NATIVE_PLAYER_INC_NATIVE_PLAYER_H_

#include <memory>

#include "ppapi/c/pp_rect.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/text_input_controller.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"

#include "communicator/message_receiver.h"
#include "communicator/message_sender.h"
#include "player/player_provider.h"


/// @class NativePlayer
/// This is a main class for this application. It holds a classes responsible
/// for communication and for controlling the player.

class NativePlayer : public pp::Instance {
 public:
  /// A constructor.
  /// @param[in] instance a class identifier for NaCl engine.
  explicit NativePlayer(PP_Instance instance)
      : pp::Instance(instance),
        player_thread_(this),
        cc_factory_(this),
        text_input_controller_(this) {}

  /// ~NativePlayer()
  /// @private This method don't have to be included in Doxygen documentation
  ~NativePlayer() override;

  /// DidChangeView() handles initialization or changing viewport
  /// @param[in] view new representation of vewport
  /// @see pp::Instance
  void DidChangeView(const pp::View& view) override;

  /// HandleMessage() is a default method for handling messages from another
  /// modules in this application this functionality is redirected to
  /// UiMessageReceiver.
  /// @param[in] var_message a container with received message
  /// @see pp::Instance.
  void HandleMessage(const pp::Var& var_message) override;

  /// Method called for instancce initialization.
  /// @see pp::Instance
  bool Init(uint32_t, const char**, const char**) override;

 private:
  /// Method called for initialize IO stream.
  void InitNaClIO();

  void DispatchMessage(pp::Var message);

  void DispatchMessageMessageOnSideThread(int32_t, pp::Var message);

  std::shared_ptr<Communication::MessageReceiver> message_receiver_;
  pp::SimpleThread player_thread_;
  pp::CompletionCallbackFactory<NativePlayer> cc_factory_;
  pp::Rect rect_;
  pp::TextInputController text_input_controller_;
};

/// NativePlayerModule creation
class NativePlayerModule : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) override {
    return new NativePlayer(instance);
  }
};

/// Module creation, very first call in NaCl
/// @see HelloWorld demo
namespace pp {
Module* CreateModule() { return new NativePlayerModule(); }
}

#endif  // NATIVE_PLAYER_INC_NATIVE_PLAYER_H_
