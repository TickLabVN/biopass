#include "fingerprint_auth.h"

#include <gio/gio.h>

#include <iostream>
#include <thread>
#include <vector>

namespace facepass {

namespace {

const char *FPRINT_SERVICE = "net.reactivated.Fprint";
const char *FPRINT_MANAGER_PATH = "/net/reactivated/Fprint/Manager";
const char *FPRINT_MANAGER_INTERFACE = "net.reactivated.Fprint.Manager";
const char *FPRINT_DEVICE_INTERFACE = "net.reactivated.Fprint.Device";

struct AuthContext {
  GMainLoop *loop;
  AuthResult result;
  std::string error_msg;
};

void on_verify_status(GDBusConnection *connection, const gchar *sender_name,
                      const gchar *object_path, const gchar *interface_name,
                      const gchar *signal_name, GVariant *parameters, gpointer user_data) {
  AuthContext *ctx = static_cast<AuthContext *>(user_data);
  const gchar *result;
  gboolean done;

  g_variant_get(parameters, "(&sb)", &result, &done);
  std::string res_str = result;

  std::cout << "Fingerprint status: " << res_str << ", done: " << done << std::endl;

  if (res_str == "verify-match") {
    ctx->result = AuthResult::Success;
    g_main_loop_quit(ctx->loop);
  } else if (res_str == "verify-no-match") {
    // If not done, we continue waiting (retry).
    // If done, it's a failure.
    if (done) {
      ctx->result = AuthResult::Failure;
      g_main_loop_quit(ctx->loop);
    }
  } else if (res_str == "verify-unknown-error" || res_str == "verify-disconnected") {
    ctx->result = AuthResult::Unavailable;
    ctx->error_msg = res_str;
    g_main_loop_quit(ctx->loop);
  } else {
    // Retry scan, swipe too short, etc.
    // We stay in the loop unless done is true
    if (done) {
      ctx->result = AuthResult::Retry;  // Or Failure
      g_main_loop_quit(ctx->loop);
    }
  }
}

}  // namespace

bool FingerprintAuth::is_available() const {
  GError *error = nullptr;
  GDBusProxy *manager = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
                                                      nullptr, FPRINT_SERVICE, FPRINT_MANAGER_PATH,
                                                      FPRINT_MANAGER_INTERFACE, nullptr, &error);

  if (error) {
    std::cerr << "Failed to connect to fprintd manager: " << error->message << std::endl;
    g_error_free(error);
    return false;
  }

  GVariant *ret = g_dbus_proxy_call_sync(manager, "GetDefaultDevice", nullptr,
                                         G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

  bool available = false;
  if (ret) {
    // If we got a device path, it's potentially available.
    // We could check if there are enrolled fingers here, but strictly speaking
    // the hardware IS available. The specific user might not be enrolled (handled
    // in authenticate).
    available = true;
    g_variant_unref(ret);
  } else {
    // Usually means no device found
    if (error)
      g_error_free(error);
  }

  g_object_unref(manager);
  return available;
}

AuthResult FingerprintAuth::authenticate(const std::string &username, const AuthConfig &config) {
  GError *error = nullptr;
  GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!connection) {
    std::cerr << "Failed to get system bus: " << error->message << std::endl;
    g_error_free(error);
    return AuthResult::Unavailable;
  }

  // 1. Get Manager
  GDBusProxy *manager =
      g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, nullptr, FPRINT_SERVICE,
                            FPRINT_MANAGER_PATH, FPRINT_MANAGER_INTERFACE, nullptr, &error);

  if (error) {
    std::cerr << "Failed to get manager proxy: " << error->message << std::endl;
    g_error_free(error);
    g_object_unref(connection);
    return AuthResult::Unavailable;
  }

  // 2. Get Default Device
  GVariant *dev_ret = g_dbus_proxy_call_sync(manager, "GetDefaultDevice", nullptr,
                                             G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
  if (!dev_ret) {
    std::cerr << "No fingerprint device found: " << (error ? error->message : "Unknown")
              << std::endl;
    if (error)
      g_error_free(error);
    g_object_unref(manager);
    g_object_unref(connection);
    return AuthResult::Unavailable;
  }

  const gchar *device_path;
  g_variant_get(dev_ret, "(&o)", &device_path);
  std::string dev_path_str = device_path;
  g_variant_unref(dev_ret);

  // 3. Get Device Proxy
  GDBusProxy *device =
      g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, nullptr, FPRINT_SERVICE,
                            dev_path_str.c_str(), FPRINT_DEVICE_INTERFACE, nullptr, &error);

  if (error) {
    std::cerr << "Failed to get device proxy: " << error->message << std::endl;
    g_error_free(error);
    g_object_unref(manager);
    g_object_unref(connection);
    return AuthResult::Unavailable;
  }

  // 4. Check Enrolled Fingers
  GVariant *enrolled_ret =
      g_dbus_proxy_call_sync(device, "ListEnrolledFingers", g_variant_new("(s)", username.c_str()),
                             G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

  if (!enrolled_ret) {
    std::cerr << "Failed to list enrolled fingers (user might not exist or permission denied): "
              << (error ? error->message : "") << std::endl;
    if (error)
      g_error_free(error);
    g_object_unref(device);
    g_object_unref(manager);
    g_object_unref(connection);
    return AuthResult::Unavailable;  // Or Failure
  }

  GVariantIter *iter;
  gchar *finger_name;
  bool has_fingers = false;
  g_variant_get(enrolled_ret, "(as)", &iter);
  while (g_variant_iter_loop(iter, "s", &finger_name)) {
    has_fingers = true;
  }
  g_variant_iter_free(iter);
  g_variant_unref(enrolled_ret);

  if (!has_fingers) {
    std::cerr << "User " << username << " has no enrolled fingerprints." << std::endl;
    g_object_unref(device);
    g_object_unref(manager);
    g_object_unref(connection);
    return AuthResult::Unavailable;  // Should we return Failure? Unavailable seems appropriate if
                                     // not set up.
  }

  // 5. Claim Device
  GVariant *claim_ret =
      g_dbus_proxy_call_sync(device, "Claim", g_variant_new("(s)", username.c_str()),
                             G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

  if (!claim_ret) {
    std::cerr << "Failed to claim device: " << (error ? error->message : "") << std::endl;
    if (error)
      g_error_free(error);
    g_object_unref(device);
    g_object_unref(manager);
    g_object_unref(connection);
    return AuthResult::Unavailable;
  }
  g_variant_unref(claim_ret);

  // 6. Start Verification
  // "any" is typically used to accept any enrolled finger
  GVariant *verify_ret = g_dbus_proxy_call_sync(device, "VerifyStart", g_variant_new("(s)", "any"),
                                                G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

  if (!verify_ret) {
    std::cerr << "Failed to start verification: " << (error ? error->message : "") << std::endl;
    if (error)
      g_error_free(error);
    // Release
    GVariant *rel_ret = g_dbus_proxy_call_sync(device, "Release", nullptr, G_DBUS_CALL_FLAGS_NONE,
                                               -1, nullptr, nullptr);
    if (rel_ret)
      g_variant_unref(rel_ret);
    g_object_unref(device);
    g_object_unref(manager);
    g_object_unref(connection);
    return AuthResult::Failure;
  }
  g_variant_unref(verify_ret);

  // 7. Loop Loop
  AuthContext ctx;
  ctx.loop = g_main_loop_new(nullptr, FALSE);
  ctx.result = AuthResult::Failure;  // Default

  guint sub_id = g_dbus_connection_signal_subscribe(
      connection, FPRINT_SERVICE, FPRINT_DEVICE_INTERFACE, "VerifyStatus", dev_path_str.c_str(),
      nullptr, G_DBUS_SIGNAL_FLAGS_NONE, on_verify_status, &ctx, nullptr);

  std::cout << "Waiting for fingerprint..." << std::endl;
  g_main_loop_run(ctx.loop);

  g_dbus_connection_signal_unsubscribe(connection, sub_id);
  g_main_loop_unref(ctx.loop);

  // Stop Verification
  GVariant *stop_ret = g_dbus_proxy_call_sync(device, "VerifyStop", nullptr, G_DBUS_CALL_FLAGS_NONE,
                                              -1, nullptr, nullptr);
  if (stop_ret)
    g_variant_unref(stop_ret);

  // Release
  GVariant *release_ret = g_dbus_proxy_call_sync(device, "Release", nullptr, G_DBUS_CALL_FLAGS_NONE,
                                                 -1, nullptr, nullptr);
  if (release_ret)
    g_variant_unref(release_ret);

  g_object_unref(device);
  g_object_unref(manager);
  g_object_unref(connection);

  return ctx.result;
}

}  // namespace facepass
