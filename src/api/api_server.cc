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
const char kAPIServerThreadName[] = "meson__api_server_thread";

APIServer::Client::Client(API& api)
    : api_(api) {
  api_.CreateAppBinding();
  scoped_refptr<Remote> remote = new Remote(*this, MESON_OBJID_APP);
  remotes_[MESON_OBJID_APP] = remote;
  api_.SetRemote(MESON_OBJID_APP, remote);
}

APIServer::Client::~Client(void) {
}
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
  scoped_refptr<Remote> remote = new Remote(*this, target);
  remotes_[target] = remote;
  api_.SetRemote(target, remote);
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
  DCHECK((*fiter).second);
  auto remote = (*fiter).second;

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

APIServer::Client::Remote::Remote(APIServer::Client& client, unsigned int target)
    : client_(client), target_(target) {
}

void APIServer::Client::Remote::InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) {
  LOG(INFO) << "Remote::InvokeMethod(" << target_ << ", " << method << ")";
}

void APIServer::Client::Remote::EmitEvent(const std::string& type, std::unique_ptr<base::ListValue> event) {
  DLOG(INFO) << "Remote::EmitEvent(" << target_ << ", " << type << ")";
  int eventID = GetEventID(type);
  if (eventID < 0) {
    LOG(INFO) << "Remote::EmitEvent(" << target_ << ", " << type << ") : event unregisterd.";
    return;
  }
  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE,
                                   base::Bind(&APIServer::Client::SendEvent, &client_, target_, eventID, base::Passed(&event)));
}

std::unique_ptr<base::Value> APIServer::Client::Remote::EmitEventWithResult(const std::string& type, std::unique_ptr<base::ListValue> event) {
  int eventID = GetEventID(type);
  if (eventID < 0) {
    LOG(INFO) << "Remote::EmitEventWithResult(" << target_ << ", " << type << ") : event unregisterd.";
    return std::unique_ptr<base::Value>();
  }
  return client_.SendEventWithResult(target_, eventID, std::move(event));
}

int APIServer::Client::Remote::RegisterEvents(const std::string& event) {
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

void APIServer::Client::Remote::UnregisterEvents(int id) {
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

int APIServer::Client::Remote::GetEventID(const std::string& eventName) const {
  base::subtle::AutoReadLock locker(eventLock_);
  auto fiter = registeredEvents_.find(eventName);
  return (fiter != registeredEvents_.end()) ? (*fiter).second : -1;
}

std::pair<int, std::string> APIServer::Client::Remote::RegisterTemporaryEvents() {
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
