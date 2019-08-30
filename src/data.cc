#include "data.hh"
#include "notify-icon-object.hh"

#include <map>
#include <mutex>
#include <optional>

static std::mutex env_datas_mutex;
// Must be map<>, not unordered_map<>, for the
// guarantee that insert and delete do not invalidate
// other elements.
static std::map<napi_env, EnvData> env_datas;

EnvData* get_env_data(napi_env env) {
  std::lock_guard lock{env_datas_mutex};
  if (auto it = env_datas.find(env); it != env_datas.end()) {
    return &it->second;
  }

  return nullptr;
}

napi_status EnvData::add_icon(int32_t id, napi_value value,
                              NotifyIconObject* object) {
  // std::lock_guard icons_lock{icons_mutex};

  if (icons.empty()) {
    NAPI_RETURN_IF_NOT_OK(icon_message_loop.init(this));
  }

  NapiRef ref;
  NAPI_RETURN_IF_NOT_OK(ref.create(env, value));
  auto insert = icons.insert({id,
                              {ref.ref, object->notify_icon.id.callback_id,
                               object->notify_icon.id.guid}});
  // If the insert succeeded, don't unref
  if (insert.second) {
    ref.release();
  }

  return napi_ok;
}

using UserCallParam = std::function<LRESULT(HWND, WPARAM)>;

bool EnvData::remove_icon(int32_t id) {
  {
    // std::lock_guard icons_lock{icons_mutex};
    if (auto it = icons.find(id); it == icons.end()) {
      return false;
    } else {
      icons.erase(it);
      if (icons.empty()) {
        icon_message_loop.quit();
        // ::PostMessageW(msg_hwnd, WM_USER_QUIT, 0, 0);
      }
    }
  }
  // msg_thread.join();
  return true;
}

void EnvData::notify_select(int32_t icon_id, NotifySelectArgs args) {
  icon_message_loop.run_on_env_thread.blocking([=](napi_env env, napi_value) {
    NAPI_FATAL_IF(this->env != env);

    napi_ref ref;
    {
      // std::lock_guard icons_lock{env_data->icons_mutex};
      if (auto it = icons.find(icon_id); it != icons.end()) {
        ref = it->second.ref;
      } else {
        return;
      }
    }

    napi_value value;
    NAPI_THROW_RETURN_VOID_IF_NOT_OK(
        env, napi_get_reference_value(env, ref, &value));

    NotifyIconObject* object;
    NAPI_THROW_RETURN_VOID_IF_NOT_OK(env, napi_get_value(env, value, &object));

    napi_value event;
    NAPI_THROW_RETURN_VOID_IF_NOT_OK(
        env, napi_create_object(env, &event,
                                {
                                    {"target", value},
                                    {"rightButton", args.right_button},
                                    {"mouseX", args.mouse_x},
                                    {"mouseY", args.mouse_y},
                                }));

    object->select_callback(value, {event});
  });
}

template <typename Fn>
napi_status napi_add_env_cleanup_hook(napi_env env, Fn fn) {
  auto fn_ptr = std::make_unique<Fn>(std::move(fn));
  NAPI_RETURN_IF_NOT_OK(napi_add_env_cleanup_hook(
      env, [](void* ptr) { (*static_cast<Fn*>(ptr))(); }, fn_ptr.get()));
  fn_ptr.release();
  return napi_ok;
}

std::tuple<napi_status, EnvData*> create_env_data(napi_env env) {
  // Lock for the whole method just so we know we'll get
  // a consistent EnvData.
  std::lock_guard lock{env_datas_mutex};
  auto data = &env_datas[env];
  data->env = env;

  if (auto status = data->icon_message_loop.run_on_env_thread.create(env);
      status != napi_ok) {
    return {status, nullptr};
  }

  if (auto status =
          napi_add_env_cleanup_hook(env,
                                    [=] {
                                      std::lock_guard lock{env_datas_mutex};
                                      // This will also fire all the required
                                      // destructors.
                                      env_datas.erase(env);
                                    });
      status != napi_ok) {
    return {status, nullptr};
  }

  return {napi_ok, data};
}

EnvData::~EnvData() {
  for (auto& pair : icons) {
    delete_notify_icon(
        {icon_message_loop.hwnd, pair.second.id, pair.second.guid});
  }
}
