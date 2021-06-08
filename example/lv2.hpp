#pragma once
#include <concepts>

#include "lv2/core/lv2.h"

struct LV2Plugin {
  virtual void connect_port(uint32_t port, float* data) = 0;
  virtual void activate() {}
  virtual void deactivate() {}
  virtual void run(uint32_t n_samples) = 0;
};

template<std::derived_from<LV2Plugin> Plugin>
LV2_Descriptor make_descriptor(const char* uri)
{
  auto instantiate = [](const LV2_Descriptor* descriptor, double rate, const char* bundle_path,
                        const LV2_Feature* const* features) { //
    return static_cast<LV2_Handle>(new Plugin());
  };
  auto connect_port = [](LV2_Handle instance, uint32_t port, void* data) { //
    static_cast<Plugin*>(instance)->connect_port(port, static_cast<float*>(data));
  };
  auto activate = [](LV2_Handle instance) { //
    static_cast<Plugin*>(instance)->activate();
  };
  auto run = [](LV2_Handle instance, uint32_t n_samples) { //
    static_cast<Plugin*>(instance)->run(n_samples);
  };
  auto deactivate = [](LV2_Handle instance) { //
    static_cast<Plugin*>(instance)->deactivate();
  };
  auto cleanup = [](LV2_Handle instance) { //
    delete static_cast<Plugin*>(instance);
  };
  auto extension_data = [](const char* uri) -> const void* { return nullptr; };
  return {
    uri,            //
    instantiate,    //
    connect_port,   //
    activate,       //
    run,            //
    deactivate,     //
    cleanup,        //
    extension_data, //
  };
}

