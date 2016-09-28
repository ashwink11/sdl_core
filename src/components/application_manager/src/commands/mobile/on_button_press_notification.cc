/*

 Copyright (c) 2013, Ford Motor Company
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the
 distribution.

 Neither the name of the Ford Motor Company nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "application_manager/commands/mobile/on_button_press_notification.h"
#include "application_manager/application_impl.h"
#include "interfaces/MOBILE_API.h"

#include "utils/make_shared.h"

namespace application_manager {

namespace commands {

namespace mobile {

OnButtonPressNotification::OnButtonPressNotification(
    const MessageSharedPtr& message, ApplicationManager& application_manager)
    : CommandNotificationImpl(message, application_manager) {}

OnButtonPressNotification::~OnButtonPressNotification() {}

void OnButtonPressNotification::Run() {
  SDL_AUTO_TRACE();

  const uint32_t btn_id = static_cast<uint32_t>(
      (*message_)[strings::msg_params][hmi_response::button_name].asInt());

  // CUSTOM_BUTTON notification
  if (static_cast<uint32_t>(mobile_apis::ButtonName::CUSTOM_BUTTON) == btn_id) {
    // app_id is mandatory for CUSTOM_BUTTON notification
    if (false == (*message_)[strings::msg_params].keyExists(strings::app_id)) {
      SDL_ERROR("CUSTOM_BUTTON OnButtonPress without app_id.");
      return;
    }

    // custom_button_id is mandatory for CUSTOM_BUTTON notification
    if (false ==
        (*message_)[strings::msg_params].keyExists(
            hmi_response::custom_button_id)) {
      SDL_ERROR("CUSTOM_BUTTON OnButtonPress without custom_button_id.");
      return;
    }

    ApplicationSharedPtr app = application_manager_.application(
        (*message_)[strings::msg_params][strings::app_id].asUInt());

    if (false == app.valid()) {
      SDL_ERROR("Application doesn't exist.");
      return;
    }

    uint32_t custom_btn_id = 0;
    custom_btn_id =
        (*message_)[strings::msg_params][hmi_response::custom_button_id]
            .asUInt();

    if (false == app->IsSubscribedToSoftButton(custom_btn_id)) {
      SDL_ERROR("Application doesn't subscribed to this custom_button_id.");
      return;
    }

    SendButtonPress(app);
    return;
  }

  const std::vector<ApplicationSharedPtr>& subscribedApps =
      application_manager_.applications_by_button(btn_id);

  std::vector<ApplicationSharedPtr>::const_iterator it = subscribedApps.begin();
  for (; subscribedApps.end() != it; ++it) {
    ApplicationSharedPtr subscribed_app = *it;
    if (!subscribed_app) {
      SDL_WARN("Null pointer to subscribed app.");
      continue;
    }

    // Send ButtonPress notification only in HMI_FULL or HMI_LIMITED mode
    if ((mobile_api::HMILevel::HMI_FULL != subscribed_app->hmi_level()) &&
        (mobile_api::HMILevel::HMI_LIMITED != subscribed_app->hmi_level())) {
      SDL_WARN("OnButtonPress notification is allowed only"
               << "in FULL or LIMITED hmi level");
      continue;
    }

    // Send ButtonPress notification for OK button only in HMI_FULL mode
    if ((static_cast<uint32_t>(mobile_apis::ButtonName::OK) == btn_id) &&
        (mobile_api::HMILevel::HMI_FULL != subscribed_app->hmi_level())) {
      SDL_WARN("OnButtonPress notification for OK button"
               << "is allowed only in FULL hmi level");
      continue;
    }

    SendButtonPress(subscribed_app);
  }
}

void OnButtonPressNotification::SendButtonPress(ApplicationConstSharedPtr app) {
  smart_objects::SmartObjectSPtr on_btn_press =
      utils::MakeShared<smart_objects::SmartObject>();

  (*on_btn_press)[strings::params][strings::connection_key] = app->app_id();

  (*on_btn_press)[strings::params][strings::function_id] =
      static_cast<int32_t>(mobile_apis::FunctionID::eType::OnButtonPressID);

  (*on_btn_press)[strings::msg_params][strings::button_name] =
      (*message_)[strings::msg_params][hmi_response::button_name];
  (*on_btn_press)[strings::msg_params][strings::button_press_mode] =
      (*message_)[strings::msg_params][hmi_response::button_mode];

  if ((*message_)[strings::msg_params].keyExists(
          hmi_response::custom_button_id)) {
    (*on_btn_press)[strings::msg_params][strings::custom_button_id] =
        (*message_)[strings::msg_params][strings::custom_button_id];
  }

  message_ = on_btn_press;
  SendNotification();
}

}  // namespace mobile

}  // namespace commands

}  // namespace application_manager
