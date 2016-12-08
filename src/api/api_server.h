//-*-c++-*-
#pragma once

#include <memory>
#include <map>
#include <set>
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
  class Client : public base::RefCountedThreadSafe<Client> {
   public:
    Client(API& api);
    virtual ~Client(void);

   public:
    void ProcessMessage(std::string msg);

    class Remote : public APIBindingRemote {
     public:
      Remote(APIServer::Client& client, unsigned int target);

      virtual void InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) override;
      virtual void EmitEvent(const std::string& type, std::unique_ptr<base::ListValue> event) override;
      virtual std::unique_ptr<base::Value> EmitEventWithResult(const std::string& type, std::unique_ptr<base::ListValue> event) override;

      int RegisterEvents(const std::string& event);
      void UnregisterEvents(int id);
      std::pair<int, std::string> RegisterTemporaryEvents();
      int GetEventID(const std::string& eventName) const;

     private:
      mutable base::subtle::ReadWriteLock eventLock_;
      APIServer::Client& client_;
      unsigned int target_;
      std::map<std::string, int> registeredEvents_;
      std::set<int> freeIDs_;
      DISALLOW_COPY_AND_ASSIGN(Remote);
    };

   protected:
   private:
    void PerformUIAction(MESON_ACTION_TYPE type, std::unique_ptr<base::DictionaryValue> action);
    void PerformIOAction(MESON_ACTION_TYPE type, std::unique_ptr<base::DictionaryValue> action);
    void DoCreate(base::DictionaryValue& message);
    void PostCreate(int actionId, scoped_refptr<APIBinding> binding);
    void DoCall(base::DictionaryValue& message);
    void DoRegisterEvent(base::DictionaryValue& message);
    void ReplyToAction(const unsigned int id, const std::string& error, std::unique_ptr<base::Value> result);
    void SendReply(const unsigned int id, const std::string& error, std::unique_ptr<base::Value> result);
    void SendEvent(const unsigned int target, int event_id, std::unique_ptr<base::ListValue> args);
    std::unique_ptr<base::Value> SendEventWithResult(const unsigned int target, int event_id, std::unique_ptr<base::ListValue> args);

   private:
    std::map<unsigned int, scoped_refptr<Remote>> remotes_;
    API& api_;
    DISALLOW_COPY_AND_ASSIGN(Client);
  };

 public:
  APIServer(void) = delete;
  explicit APIServer(API& api);

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
