//-*-c++-*-
#pragma once

#include <memory>
#include <map>
#include <set>
#include <array>
#include "base/memory/ref_counted.h"
#include "base/files/file_path.h"
#include "base/synchronization/read_write_lock.h"

#include "api/api_binding.h"

namespace base {
class Thread;
class DictionaryValue;
}
namespace meson {
class API;
class APIServer : public base::RefCountedThreadSafe<APIServer> {
public:
  class Client;

 public:
  APIServer(void) = delete;
  explicit APIServer(API& api);
  virtual ~APIServer(void);

 public:
  void Start();
  void Stop();

 private:
  void StartHandlerThread();
  void ResetHandlerThread();
  void StopHandlerThread();
  void ThreadRun();
  void ThreadTearDown();

 private:
  API& api_;
  std::unique_ptr<base::Thread> thread_;
  scoped_refptr<Client> client_;
  DISALLOW_COPY_AND_ASSIGN(APIServer);
};
}
