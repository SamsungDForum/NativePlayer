/*!
 * content_protection_visitor.h (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Adam Bujalski
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_CONTENT_PROTECTION_VISITOR_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_CONTENT_PROTECTION_VISITOR_H_

#include <memory>
#include <vector>

/// @file
/// @brief This file defines <code>ContentProtectionDescriptor</code> and
///   <code>ContentProtectionVisitor</code> classes.

namespace dash {
namespace mpd {
class IDescriptor;
}
}

/// @class ContentProtectionDescriptor
/// @brief This is an interface for all DRM handling classes provided by
///   the application.
///
/// Those classes are needed to hold information about DRM
/// provided in the DASH manifest.\n
/// <code>ContentProtectionDescriptor</code> is created by the
/// <code>ContentProtectionVisitor</code> basing on DASH manifest.
/// <code>ContentProtectionDescriptor</code> is held in
/// <code>CommonStreamDescription</code>.
/// @see DashManifest
/// @see CommonStreamDescription
class ContentProtectionDescriptor {
 public:
  /// Destroys <code>ContentProtectionDescriptor</code> object.
  virtual ~ContentProtectionDescriptor() = default;

 protected:
  ContentProtectionDescriptor() = default;
  ContentProtectionDescriptor(const ContentProtectionDescriptor&) = default;
  ContentProtectionDescriptor(ContentProtectionDescriptor&&) = default;
  ContentProtectionDescriptor& operator=(const ContentProtectionDescriptor&) =
      default;
  ContentProtectionDescriptor& operator=(ContentProtectionDescriptor&&) =
      default;
};

/// @class ContentProtectionVisitor
/// @brief This is an interface for classes provided by the application
///   parsing DRM information from the DASH manifest.
///
/// This class is used to create <code>ContentProtectionDescriptor</code>
/// objects basing on the DASH manifest.\n
/// The DASH manifest can have node <code><ContentProtection></code> describing
/// DRM.
/// @note Format of the node describing ContentProtection is DRM system
///   dependent.
///
/// <code>ContentProtectionVisitor</code> needs to be passed in
/// DashManifest::ParseMPD method in order to be used.
/// When the DASH parser encounters <code><ContentProtection></code>
/// ContentProtectionVisitor::Visit is called.
/// Application has to define by itself which fields - subnodes of
/// <code><ContentProtection></code> node are needed to be parsed to find DRM
/// information.
/// @see ContentProtectionDescriptor
class ContentProtectionVisitor {
 public:
  /// Destroys <code>ContentProtectionVisitor</code> object.
  virtual ~ContentProtectionVisitor() = default;

  /// This method is called when DASH parser - <code>DashManifest</code>
  /// discovers <code><ContentProtection></code> node in DASH manifest and
  /// passes vector of it's contents.\n
  /// This method performs a search for the DRM-dependent
  /// <code><ContentProtection></code> node and builds
  /// <code>ContentProtectionDescription</code> for this specific DRM.
  ///
  /// @param[in] cp A vector of <code><ContentProtection></code> node content
  ///   in which DRM information will be looked for.
  /// @return ContentProtectionDescriptor builded by acquired information from
  ///   <code><ContentProtection></code> node for specified DRM.\n An
  ///   <code>empty shared_ptr</code> if no information has been found.
  virtual std::shared_ptr<ContentProtectionDescriptor> Visit(
      const std::vector<dash::mpd::IDescriptor*>& cp) = 0;

 protected:
  ContentProtectionVisitor() = default;
  ContentProtectionVisitor(const ContentProtectionVisitor&) = default;
  ContentProtectionVisitor(ContentProtectionVisitor&&) = default;
  ContentProtectionVisitor& operator=(const ContentProtectionVisitor&) =
      default;
  ContentProtectionVisitor& operator=(ContentProtectionVisitor&&) =
      default;
};

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_CONTENT_PROTECTION_VISITOR_H_
