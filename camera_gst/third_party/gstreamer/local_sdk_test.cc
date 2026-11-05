#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video.h>

int main(int argc, char** argv) {
  gst_init(&argc, &argv);
  const gchar* version = gst_version_string();
  return (version != nullptr && version[0] != '\0') ? 0 : 1;
}
