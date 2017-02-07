#include "api/api_server.h"
#include "api/api.h"

#include "base/bind.h"
#include "base/values.h"
#include "base/logging.h"
#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread.h"
#include "base/message_loop/message_loop.h"
#include "content/public/browser/browser_thread.h"
#include "api/meson.h"

#ifdef DEBUG
#define CHECK_CURRENTLY_ON(thread_identifier) \
  (CHECK(::content::BrowserThread::CurrentlyOn(thread_identifier)) << ::content::BrowserThread::GetDCheckCurrentlyOnErrorMessage(thread_identifier))
#else
#define CHECK_CURRENTLY_ON(thread_identifier)
#endif

namespace {
bool isNeedRunUIThread(MESON_ACTION_TYPE type) {
  switch (type) {
    case MESON_ACTION_TYPE_CREATE:
      return true;
    case MESON_ACTION_TYPE_DELETE:
      return false;
    case MESON_ACTION_TYPE_CALL:
      return true;
    case MESON_ACTION_TYPE_REGISTER_EVENT:
      return false;
    case MESON_ACTION_TYPE_REPLY:
      return true;
    case MESON_ACTION_TYPE_EVENT:
    default:
      assert(false);
      return false;
  }
}
}
namespace meson {
namespace {
class APIClientRemote : public APIBindingRemote {
 public:
  APIClientRemote(APIServer::Client& client, scoped_refptr<APIBinding> binding);

 public:
  void RemoveBinding(APIBinding* binding) override;

  virtual bool InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) override;
  virtual void EmitEvent(scoped_refptr<api::EventArg> event) override;
  virtual std::unique_ptr<base::Value> EmitEventWithResult(scoped_refptr<api::EventArg> event) override;

  int RegisterEvents(const std::string& event);
  void UnregisterEvents(int id);
  std::pair<int, std::string> RegisterTemporaryEvents();
  int GetEventID(const std::string& eventName) const;

 private:
  mutable base::subtle::ReadWriteLock eventLock_;
  APIServer::Client& client_;
  std::map<std::string, int> registeredEvents_;
  std::set<int> freeIDs_;
  DISALLOW_COPY_AND_ASSIGN(APIClientRemote);
};
}

class APIServer::Client : public base::RefCountedThreadSafe<Client> {
 public:
  Client(API& api);
  virtual ~Client(void);

 public:
  void ProcessMessage(std::string msg);

 public:
  void DoSendEvent(const MESON_OBJECT_TYPE type, api::ObjID target, int event_id, scoped_refptr<api::EventArg> event) {
    content::BrowserThread::PostTask(content::BrowserThread::IO,
                                     FROM_HERE,
                                     base::Bind(&APIServer::Client::SendEvent, this, type, target, event_id, base::RetainedRef(event)));
  }
  std::unique_ptr<base::Value> DoSendEventWithResult(const MESON_OBJECT_TYPE type, api::ObjID target, int event_id, scoped_refptr<api::EventArg> event) {
    return SendEventWithResult(type, target, event_id, event);
  }

 private:
  void PerformUIAction(MESON_ACTION_TYPE type, scoped_refptr<api::CommandArg> action);
  void PerformIOAction(MESON_ACTION_TYPE type, scoped_refptr<api::CommandArg> action);
  void DoRegisterEvent(base::DictionaryValue& message);
  void ReplyToAction(api::ActionID id, scoped_refptr<api::MethodResultBody> result);
  void PostCreateToAction(api::ActionID id, scoped_refptr<api::MethodResultBody> result);
  void SendReply(api::ActionID id, scoped_refptr<api::MethodResultBody> result);
  void SendEvent(MESON_OBJECT_TYPE type, api::ObjID target, int event_id, scoped_refptr<api::EventArg> event);
  std::unique_ptr<base::Value> SendEventWithResult(MESON_OBJECT_TYPE type, api::ObjID target, int event_id, scoped_refptr<api::EventArg> args);

 private:
  std::map<std::pair<MESON_OBJECT_TYPE, api::ObjID>, base::WeakPtr<APIClientRemote>> remotes_;
  API& api_;
  //std::array<scoped_refptr<APIBindingRemote>, MESON_OBJECT_TYPE_NUM> klasses_;
  DISALLOW_COPY_AND_ASSIGN(Client);
};

const char kAPIServerThreadName[] = "meson__api_server_thread";

APIServer::Client::Client(API& api)
    : api_(api) {
  for (size_t idx = 0; idx < MESON_OBJECT_TYPE_NUM; idx++) {
    auto type = static_cast<MESON_OBJECT_TYPE>(idx);
    auto binding = api_.GetBinding(type, MESON_OBJID_STATIC);
    if (binding) {
      scoped_refptr<APIClientRemote> remote = new APIClientRemote(*this, binding);
      remotes_[std::make_pair(type, binding->GetID())] = base::AsWeakPtr(remote.get());
      binding->SetRemote(binding->GetID(), remote.get());
    }
  }
#if 0
  for (size_t idx = 0; idx < klasses_.size(); idx++) {
    auto type = static_cast<MESON_OBJECT_TYPE>(idx);
    if (type != MESON_OBJECT_TYPE_NULL) {
      klasses_[idx] = new APIClientRemote(*this, api_.GetBinding(type, MESON_OBJID_STATIC));
    }
  }
#endif
}

APIServer::Client::~Client(void) {
}

// running in IO Thread
void APIServer::Client::ProcessMessage(std::string msg) {
  auto val = base::JSONReader::Read(msg);
  scoped_refptr<api::CommandArg> cmd(new api::CommandArg);
  cmd->arg = base::DictionaryValue::From(std::move(val));

  int iaction;
  cmd->arg->GetInteger("_action", &iaction);

  auto action = static_cast<MESON_ACTION_TYPE>(iaction);

  if (isNeedRunUIThread(action)) {
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     base::Bind(&APIServer::Client::PerformUIAction, this, action, base::RetainedRef(cmd)));
  } else {
    content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                     base::Bind(&APIServer::Client::PerformIOAction, this, action, base::RetainedRef(cmd)));
  }
}

void APIServer::Client::PerformUIAction(MESON_ACTION_TYPE action, scoped_refptr<api::CommandArg> message) {
  CHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int type, actionID, id;
  std::string method;
  const base::ListValue* args;
  message->arg->GetInteger("_actionId", &actionID);
  message->arg->GetInteger("_type", &type);
  message->arg->GetInteger("_id", &id);
  message->arg->GetString("_method", &method);
  message->arg->GetList("_args", &args);
  LOG(INFO) << "UIAction:" << action << " method:" << method << " type:" << type << ", id:" << id;
  //TODO: error check
  auto binding = API::Get()->GetBinding(static_cast<MESON_OBJECT_TYPE>(type), static_cast<api::ObjID>(id));
  CHECK(binding);
  //TODO: error check
  if (MESON_ACTION_TYPE_CREATE == action) {
    binding->CallLocalMethod("_create", *args, base::Bind(&APIServer::Client::PostCreateToAction, this, actionID));
  } else {
    //TODO: error check
    binding->CallLocalMethod(method, *args, base::Bind(&APIServer::Client::ReplyToAction, this, actionID));
  }
}

void APIServer::Client::PerformIOAction(MESON_ACTION_TYPE action, scoped_refptr<api::CommandArg> message) {
  CHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (MESON_ACTION_TYPE_REGISTER_EVENT == action) {
    DoRegisterEvent(*message->arg);
    return;
  }
  int type, actionID, id;
  std::string method;
  const base::ListValue* args;
  message->arg->GetInteger("_actionId", &actionID);
  message->arg->GetInteger("_type", &type);
  message->arg->GetInteger("_id", &id);
  message->arg->GetString("_method", &method);
  message->arg->GetList("_args", &args);
  //TODO: error check
  LOG(INFO) << "IOAction:" << action << " method:" << method << " type:" << type << ", id:" << id;
  auto binding = API::Get()->GetBinding(static_cast<MESON_OBJECT_TYPE>(type), static_cast<api::ObjID>(id));
  binding->CallLocalMethod(method, *args, base::Bind(&APIServer::Client::ReplyToAction, this, actionID));
}

void APIServer::Client::PostCreateToAction(api::ActionID id, scoped_refptr<api::MethodResultBody> result) {
  CHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto binding = result->instance_;
  scoped_refptr<api::MethodResultBody> cre_rslt;
  if (binding) {
    auto target = binding->GetID();
    scoped_refptr<APIClientRemote> remote = new APIClientRemote(*this, binding);
    remotes_[std::make_pair(binding->Type(), target)] = base::AsWeakPtr(remote.get());
    binding->SetRemote(target, remote.get());
    cre_rslt = new api::MethodResultBody(binding->GetTypeID());
    LOG(INFO) << "PostCreateToAction: " << binding->Type() << " / " << binding->GetID();
  } else {
    cre_rslt = result;
  }
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::Bind(&APIServer::Client::SendReply, this, id, base::RetainedRef(cre_rslt)));
}

void APIServer::Client::DoRegisterEvent(base::DictionaryValue& message) {
  CHECK_CURRENTLY_ON(content::BrowserThread::IO);
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << message;
  int id = -1;
  int t = -1;
  int actionId = -1;
  message.GetInteger("_type", &t);
  message.GetInteger("_actionId", &actionId);
  message.GetInteger("_id", &id);
  const base::DictionaryValue* opt = nullptr;
  message.GetDictionary("_args", &opt);
  auto fiter = remotes_.find(std::make_pair(static_cast<MESON_OBJECT_TYPE>(t), static_cast<api::ObjID>(id)));
  if (fiter == remotes_.end()) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : object not found : " << message;
    ReplyToAction(actionId, make_scoped_refptr(new api::MethodResultBody("invalid id")));
    return;
  }
  scoped_refptr<APIClientRemote> remote = (*fiter).second.get();
  if (!remote) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : object already destroyed : " << message;
    ReplyToAction(actionId, make_scoped_refptr(new api::MethodResultBody("inalid id")));
    return;
  }

  bool f = false;
  if (opt && opt->GetBoolean("delete", &f) && f) {
    // delete event
    int eventID = -1;
    f = opt->GetInteger("number", &eventID);
    DCHECK(f);
    remote->UnregisterEvents(eventID);
    ReplyToAction(actionId, make_scoped_refptr(new api::MethodResultBody()));  // no need reply
  } else if (!opt || !opt->GetBoolean("temporary", &f) || !f) {
    // permanent
    std::string event;
    opt->GetString("eventName", &event);
    int id = remote->RegisterEvents(event);
    ReplyToAction(actionId, make_scoped_refptr(new api::MethodResultBody(std::unique_ptr<base::Value>(new base::FundamentalValue(id)))));
  } else {
    // temporary
    int numRegist = -1;
    opt->GetInteger("number", &numRegist);
    std::unique_ptr<base::ListValue> ret(new base::ListValue());
    for (int idx = 0; idx < numRegist; idx++) {
      auto r = new base::DictionaryValue();
      auto idAndName = remote->RegisterTemporaryEvents();
      r->SetInteger("eventId", idAndName.first);
      r->SetString("eventName", idAndName.second);
      ret->Append(r);
    }
    ReplyToAction(actionId, make_scoped_refptr(new api::MethodResultBody(std::move(ret))));
  }
}

void APIServer::Client::ReplyToAction(api::ActionID id, scoped_refptr<api::MethodResultBody> result) {
  /* Runs on UI Thread. */
  if (id != 0) {
    content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                     base::Bind(&APIServer::Client::SendReply, this, id, base::RetainedRef(result)));
  }
}

void APIServer::Client::SendReply(api::ActionID id, scoped_refptr<api::MethodResultBody> result) {
  /* Runs on IO Thread. */
  base::DictionaryValue action;
  action.SetInteger("_action", MESON_ACTION_TYPE_REPLY);
  action.SetInteger("_actionId", id);
  action.SetString("_error", result->error_);
  if (result->value_) {
    action.Set("_result", result->value_->DeepCopy());
  }

  std::string payload;
  base::JSONWriter::Write(action, &payload);
  DLOG(INFO) << "**Reply: " << payload;
  mesonApiPostServerResponseHandler(id, payload.c_str());
}

void APIServer::Client::SendEvent(MESON_OBJECT_TYPE type, api::ObjID target, int event_id, scoped_refptr<api::EventArg> event) {
  /* Runs on IO Thread. */
  base::DictionaryValue action;
  action.SetInteger("_action", MESON_ACTION_TYPE_EVENT);
  action.SetInteger("_actionId", 0);  // no reply need.
  action.SetInteger("_type", type);
  action.SetInteger("_id", target);
  action.SetInteger("_eventId", event_id);
  if (event) {
    action.Set("_result", event->event_->DeepCopy());
  }

  std::string payload;
  base::JSONWriter::Write(action, &payload);
  DLOG(INFO) << "**Event: " << payload;
  mesonApiPostServerResponseHandler(target, payload.c_str());
}

std::unique_ptr<base::Value> APIServer::Client::SendEventWithResult(MESON_OBJECT_TYPE type, api::ObjID target, int event_id, scoped_refptr<api::EventArg> event) {
  /* Runs on IO Thread. */
  base::DictionaryValue action;
  action.SetInteger("_action", MESON_ACTION_TYPE_EVENT);
  action.SetInteger("_actionId", 0);  // no reply need.
  action.SetInteger("_type", type);
  action.SetInteger("_id", target);
  action.SetInteger("_eventId", event_id);
  if (event) {
    action.Set("_result", event->event_->DeepCopy());
  }

  std::string payload;
  base::JSONWriter::Write(action, &payload);
  DLOG(INFO) << "**Event: " << payload;
  char* ret = mesonApiPostServerResponseHandler(target, payload.c_str(), true);

  std::unique_ptr<base::Value> result;
  if (ret) {
    result = base::JSONReader::Read(ret);
    free(ret);
  }
  return result;
}

APIClientRemote::APIClientRemote(APIServer::Client& client, scoped_refptr<APIBinding> binding)
    : APIBindingRemote(binding), client_(client) {
}

void APIClientRemote::RemoveBinding(meson::APIBinding* binding) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (binding_.get() != binding) {
    return;
  }
  std::unique_ptr<base::DictionaryValue> event;
  EmitEvent(make_scoped_refptr(new api::EventArg("-deleted", std::move(event))));
  binding_ = nullptr;
}

bool APIClientRemote::InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) {
  if (!binding_) {
    return false;
  }
  LOG(INFO) << "Remote::InvokeMethod(" << binding_->GetID() << ", " << method << ")";
  //TODO: not implement yet.
  return false;
}

void APIClientRemote::EmitEvent(scoped_refptr<api::EventArg> event) {
  if (!binding_) {
    // already deleted
    return;
  }
  auto target = binding_->GetID();
  int eventID = GetEventID(event->name_);
  if (eventID < 0) {
    LOG(INFO) << "Remote::EmitEvent(" << binding_->Type() << ", " << target << ", " << event->name_ << ") : event unregisterd.";
    return;
  }
  client_.DoSendEvent(binding_->Type(), target, eventID, event);
}

std::unique_ptr<base::Value> APIClientRemote::EmitEventWithResult(scoped_refptr<api::EventArg> event) {
  if (!binding_) {
    return std::unique_ptr<base::Value>();
  }
  auto target = binding_->GetID();
  int eventID = GetEventID(event->name_);
  if (eventID < 0) {
    LOG(INFO) << "Remote::EmitEventWithResult(" << binding_->Type() << ", " << target << ", " << event->name_ << ") : event unregisterd.";
    return std::unique_ptr<base::Value>();
  }
  return client_.DoSendEventWithResult(binding_->Type(), target, eventID, event);
}

int APIClientRemote::RegisterEvents(const std::string& event) {
  CHECK_CURRENTLY_ON(content::BrowserThread::IO);
  base::subtle::AutoWriteLock locker(eventLock_);
  auto fiter = registeredEvents_.find(event);
  if (fiter != registeredEvents_.end()) {
    return (*fiter).second;
  }
  auto id = static_cast<int>(registeredEvents_.size()) + 1;
  if (!freeIDs_.empty()) {
    auto top = freeIDs_.begin();
    id = (*top);
    freeIDs_.erase(top);
  }
  registeredEvents_[event] = id;
  return id;
}

void APIClientRemote::UnregisterEvents(int id) {
  CHECK_CURRENTLY_ON(content::BrowserThread::IO);
  base::subtle::AutoWriteLock locker(eventLock_);
  for (auto iter = registeredEvents_.begin(); iter != registeredEvents_.end(); iter++) {
    if ((*iter).second == id) {
      (*iter).second = -1;
      freeIDs_.insert(id);
      break;
    }
  }
}

int APIClientRemote::GetEventID(const std::string& eventName) const {
  base::subtle::AutoReadLock locker(eventLock_);
  auto fiter = registeredEvents_.find(eventName);
  return (fiter != registeredEvents_.end()) ? (*fiter).second : -1;
}

std::pair<int, std::string> APIClientRemote::RegisterTemporaryEvents() {
  base::subtle::AutoWriteLock locker(eventLock_);
  auto id = static_cast<int>(registeredEvents_.size()) + 1;
  if (!freeIDs_.empty()) {
    auto top = freeIDs_.begin();
    id = (*top);
    freeIDs_.erase(top);
  }
  auto name = base::StringPrintf("%%temp_event_%d%%", id);
  registeredEvents_[name] = id;
  return std::make_pair(id, name);
}

APIServer::APIServer(API& api)
    : api_(api), client_(new Client(api_)) {}

APIServer::~APIServer() {}

void APIServer::Start() {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  if (thread_)
    return;
  thread_.reset(new base::Thread(kAPIServerThreadName));

  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE, base::Bind(&APIServer::StartHandlerThread, this));
}

void APIServer::Stop() {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  if (!thread_)
    return;
  content::BrowserThread::PostTaskAndReply(content::BrowserThread::IO, FROM_HERE,
                                           base::Bind(&APIServer::StopHandlerThread, this),
                                           base::Bind(&APIServer::ResetHandlerThread, this));
}

void APIServer::StartHandlerThread() {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  base::Thread::Options opt;
  opt.message_loop_type = base::MessageLoop::TYPE_IO;
  if (!thread_->StartWithOptions(opt) || !mesonApiCheckInitHandler()) {
    LOG(INFO) << __PRETTY_FUNCTION__ << ": mesonApi not registerd.";
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE, base::Bind(&APIServer::ResetHandlerThread, this));
    return;
  }

  DLOG(INFO) << __PRETTY_FUNCTION__;
  thread_->message_loop()->PostTask(FROM_HERE, base::Bind(&APIServer::ThreadRun, this));
  // call start first run
  mesonApiCallInitHandler();
}
void APIServer::ResetHandlerThread() {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  thread_.reset();
}
void APIServer::StopHandlerThread() {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  if (!thread_->message_loop())
    return;
  thread_->message_loop()->PostTask(FROM_HERE, base::Bind(&APIServer::ThreadTearDown, this));
  thread_->Stop();
}

void APIServer::ThreadRun() {
  char* request = mesonApiCallWaitServerRequestHandler();
  CHECK(request) << "mesonApiCallWaitServerRequestHandler() return null!";
  if (request) {
    DLOG(INFO) << __PRETTY_FUNCTION__ << " : " << request;
    client_->ProcessMessage(std::string(request));
    free(request);
  }

  /* Finally we loop */
  thread_->message_loop()->PostTask(FROM_HERE, base::Bind(&APIServer::ThreadRun, this));
}

void APIServer::ThreadTearDown() {
  DLOG(INFO) << __PRETTY_FUNCTION__;
}
}
