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
  virtual void EmitEvent(const std::string& type, std::unique_ptr<base::ListValue> event) override;
  virtual std::unique_ptr<base::Value> EmitEventWithResult(const std::string& type, std::unique_ptr<base::ListValue> event) override;

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
  void DoSendEvent(const unsigned int target, int event_id, std::unique_ptr<base::ListValue> args) {
    content::BrowserThread::PostTask(content::BrowserThread::IO,
                                     FROM_HERE,
                                     base::Bind(&APIServer::Client::SendEvent, this, target, event_id, base::Passed(&args)));
  }
  std::unique_ptr<base::Value> DoSendEventWithResult(const unsigned int target, int event_id, std::unique_ptr<base::ListValue> args) {
    return SendEventWithResult(target, event_id, std::move(args));
  }

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
  std::map<unsigned int, base::WeakPtr<APIClientRemote>> remotes_;
  API& api_;
  DISALLOW_COPY_AND_ASSIGN(Client);
};

const char kAPIServerThreadName[] = "meson__api_server_thread";

APIServer::Client::Client(API& api)
    : api_(api) {
  auto binding = api_.CreateAppBinding();
  scoped_refptr<APIClientRemote> remote = new APIClientRemote(*this, binding);
  api_.SetRemote(MESON_OBJID_APP, remote.get());
  remotes_[MESON_OBJID_APP] = base::AsWeakPtr(remote.get());
}

APIServer::Client::~Client(void) {
}

// running in IO Thread
void APIServer::Client::ProcessMessage(std::string msg) {
  auto val = base::JSONReader::Read(msg);
  auto json = base::DictionaryValue::From(std::move(val));

  int iaction;
  json->GetInteger("_action", &iaction);

  MESON_ACTION_TYPE action = static_cast<MESON_ACTION_TYPE>(iaction);

  if (isNeedRunUIThread(action)) {
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     base::Bind(&APIServer::Client::PerformUIAction, this, action, base::Passed(&json)));
  } else {
    content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                     base::Bind(&APIServer::Client::PerformIOAction, this, action, base::Passed(&json)));
  }
}

void APIServer::Client::PerformUIAction(MESON_ACTION_TYPE action, std::unique_ptr<base::DictionaryValue> message) {
  CHECK_CURRENTLY_ON(content::BrowserThread::UI);
  switch (action) {
    case MESON_ACTION_TYPE_CREATE:
      DoCreate(*message);
      break;
    case MESON_ACTION_TYPE_CALL:
      DoCall(*message);
      break;
    default:
      LOG(ERROR) << __PRETTY_FUNCTION__ << " : *** invalid UI action! ***";
  }
}

void APIServer::Client::PerformIOAction(MESON_ACTION_TYPE action, std::unique_ptr<base::DictionaryValue> message) {
  CHECK_CURRENTLY_ON(content::BrowserThread::IO);
  switch (action) {
    case MESON_ACTION_TYPE_REGISTER_EVENT:
      DoRegisterEvent(*message);
      break;
    default:
      LOG(ERROR) << __PRETTY_FUNCTION__ << " : *** invalid IO action! ***";
  }
}

void APIServer::Client::DoCreate(base::DictionaryValue& message) {
  CHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int type;
  if (!message.GetInteger("_type", &type)) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : _type not found";
    return;
  }

  int actionId;
  message.GetInteger("_actionId", &actionId);
  if (actionId == 0) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : _actionId is ZERO!";
    return;
  }

  base::DictionaryValue dummy;
  const base::DictionaryValue* opt = nullptr;
  const base::ListValue* arg = nullptr;
  if (message.GetListWithoutPathExpansion("_args", &arg)) {
    // for dummy opt
    const base::DictionaryValue* adic = nullptr;
    if (arg->GetDictionary(0, &adic)) {
      opt = adic;
    }
  }
  if (!arg) {
    opt = &dummy;
  }

  auto binding = api_.Create(static_cast<MESON_OBJECT_TYPE>(type), *opt);
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::Bind(&APIServer::Client::PostCreate, this, actionId, binding));
}
void APIServer::Client::PostCreate(int actionId, scoped_refptr<APIBinding> binding) {
  CHECK_CURRENTLY_ON(content::BrowserThread::IO);
  auto target = binding->GetID();
  scoped_refptr<APIClientRemote> remote = new APIClientRemote(*this, binding);
  remotes_[target] = base::AsWeakPtr(remote.get());
  api_.SetRemote(target, remote.get());
  std::unique_ptr<base::FundamentalValue> res(new base::FundamentalValue(static_cast<int>(target)));
  SendReply(actionId, "", std::move(res));
}

void APIServer::Client::DoCall(base::DictionaryValue& message) {
  int id = -1;
  int actionId = -1;
  message.GetInteger("_actionId", &actionId);
  message.GetInteger("_id", &id);
  const base::ListValue* p;
  std::unique_ptr<const api::APIArgs> args;
  if (message.GetListWithoutPathExpansion("_args", &p)) {
    args.reset(p->DeepCopy());
  }
  if (!args) {
    args.reset(new api::APIArgs());
  }
  std::string method;
  message.GetString("_method", &method);
  api_.CallMethod(id, method, std::move(args), base::Bind(&APIServer::Client::ReplyToAction, this, actionId));
}

void APIServer::Client::DoRegisterEvent(base::DictionaryValue& message) {
  CHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DLOG(INFO) << __PRETTY_FUNCTION__ << " : " << message;
  int id = -1;
  int actionId = -1;
  message.GetInteger("_actionId", &actionId);
  message.GetInteger("_id", &id);
  const base::DictionaryValue* opt = nullptr;
  message.GetDictionary("_args", &opt);
  auto fiter = remotes_.find(id);
  if (fiter == remotes_.end()) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : object not found : " << message;
    ReplyToAction(actionId, "invalid id", std::unique_ptr<base::Value>());
    return;
  }
  scoped_refptr<APIClientRemote> remote = (*fiter).second.get();
  if (!remote) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : object already destroyed : " << message;
    ReplyToAction(actionId, "inalid id", std::unique_ptr<base::Value>());
    return;
  }

  bool f = false;
  if (opt && opt->GetBoolean("delete", &f) && f) {
    // delete event
    int eventID = -1;
    f = opt->GetInteger("number", &eventID);
    DCHECK(f);
    remote->UnregisterEvents(eventID);
    ReplyToAction(actionId, "", std::unique_ptr<base::Value>());
    // no need reply
  } else if (!opt || !opt->GetBoolean("temporary", &f) || !f) {
    // permanent
    std::string event;
    message.GetString("_method", &event);
    int id = remote->RegisterEvents(event);
    ReplyToAction(actionId, "", std::unique_ptr<base::Value>(new base::FundamentalValue(id)));
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
    ReplyToAction(actionId, "", std::unique_ptr<base::Value>(ret.release()));
  }
}

void APIServer::Client::ReplyToAction(const unsigned int id, const std::string& error, std::unique_ptr<base::Value> result) {
  /* Runs on UI Thread. */
  if (id != 0) {
    content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                     base::Bind(&APIServer::Client::SendReply, this, id, error, base::Passed(&result)));
  }
}

void APIServer::Client::SendReply(const unsigned int id, const std::string& error, std::unique_ptr<base::Value> result) {
  /* Runs on IO Thread. */
  base::DictionaryValue action;
  action.SetInteger("_action", MESON_ACTION_TYPE_REPLY);
  action.SetInteger("_actionId", id);
  action.SetString("_error", error);
  if (result) {
    action.Set("_result", result.release());
  }

  std::string payload;
  base::JSONWriter::Write(action, &payload);
  DLOG(INFO) << "**Reply: " << payload;
  mesonApiPostServerResponseHandler(id, payload.c_str());
}

void APIServer::Client::SendEvent(const unsigned int target, int event_id, std::unique_ptr<base::ListValue> event) {
  /* Runs on IO Thread. */
  base::DictionaryValue action;
  action.SetInteger("_action", MESON_ACTION_TYPE_EVENT);
  action.SetInteger("_actionId", 0);  // no reply need.
  action.SetInteger("_id", target);
  action.SetInteger("_eventId", event_id);
  if (event) {
    action.Set("_result", event.release());
  }

  std::string payload;
  base::JSONWriter::Write(action, &payload);
  DLOG(INFO) << "**Event: " << payload;
  mesonApiPostServerResponseHandler(target, payload.c_str());
}

std::unique_ptr<base::Value> APIServer::Client::SendEventWithResult(const unsigned int target, int event_id, std::unique_ptr<base::ListValue> event) {
  /* Runs on IO Thread. */
  base::DictionaryValue action;
  action.SetInteger("_action", MESON_ACTION_TYPE_EVENT);
  action.SetInteger("_actionId", 0);  // no reply need.
  action.SetInteger("_id", target);
  action.SetInteger("_eventId", event_id);
  if (event) {
    action.Set("_result", event.release());
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
  std::unique_ptr<base::ListValue> event;
  EmitEvent("-deleted", std::move(event));
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

void APIClientRemote::EmitEvent(const std::string& type, std::unique_ptr<base::ListValue> event) {
  if (!binding_) {
    // already deleted
    return;
  }
  auto target = binding_->GetID();
  DLOG(INFO) << "Remote::EmitEvent(" << target << ", " << type << ")";
  int eventID = GetEventID(type);
  if (eventID < 0) {
    LOG(INFO) << "Remote::EmitEvent(" << target << ", " << type << ") : event unregisterd.";
    return;
  }
  client_.DoSendEvent(target, eventID, std::move(event));
}

std::unique_ptr<base::Value> APIClientRemote::EmitEventWithResult(const std::string& type, std::unique_ptr<base::ListValue> event) {
  if (!binding_) {
    return std::unique_ptr<base::Value>();
  }
  auto target = binding_->GetID();
  int eventID = GetEventID(type);
  if (eventID < 0) {
    LOG(INFO) << "Remote::EmitEventWithResult(" << target << ", " << type << ") : event unregisterd.";
    return std::unique_ptr<base::Value>();
  }
  return client_.DoSendEventWithResult(target, eventID, std::move(event));
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
