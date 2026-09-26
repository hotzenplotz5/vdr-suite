#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "suitebridge_control_plane.h"
#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <vector>
namespace { using namespace vdrsuite::agent::control;
bool configure(int fd){int df=fcntl(fd,F_GETFD,0),sf=fcntl(fd,F_GETFL,0);return df>=0&&sf>=0&&fcntl(fd,F_SETFD,df|FD_CLOEXEC)==0&&fcntl(fd,F_SETFL,sf|O_NONBLOCK)==0;}
bool safePath(const std::string&p){return !p.empty()&&p.size()<sizeof(sockaddr_un::sun_path);}
}
SuiteBridgeControlPlane::SuiteBridgeControlPlane(std::string p,std::size_t q):socketPath_(std::move(p)),queueCapacity_(std::max<std::size_t>(1,q)){}
SuiteBridgeControlPlane::~SuiteBridgeControlPlane(){Stop();}
std::size_t SuiteBridgeControlPlane::operationIndex(Operation o){auto v=static_cast<std::uint16_t>(o);return v>=1&&v<=4?static_cast<std::size_t>(v-1):0;}
bool SuiteBridgeControlPlane::Start(Handler h,Logger l){
 if(running_.load()||!h||!safePath(socketPath_))return false;handler_=std::move(h);logger_=std::move(l);stopRequested_.store(false);
 listenFd_=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,0);if(listenFd_<0)return false;
 sockaddr_un a{};a.sun_family=AF_UNIX;std::copy(socketPath_.begin(),socketPath_.end(),a.sun_path);a.sun_path[socketPath_.size()]='\0';unlink(socketPath_.c_str());
 if(bind(listenFd_,reinterpret_cast<const sockaddr*>(&a),sizeof(a))!=0||chmod(socketPath_.c_str(),0660)!=0||listen(listenFd_,32)!=0){close(listenFd_);listenFd_=-1;unlink(socketPath_.c_str());return false;}
 running_.store(true);workerThread_=std::thread(&SuiteBridgeControlPlane::workerLoop,this);acceptThread_=std::thread(&SuiteBridgeControlPlane::acceptLoop,this);return true;
}
void SuiteBridgeControlPlane::Stop(){if(!running_.exchange(false))return;stopRequested_.store(true);if(listenFd_>=0){shutdown(listenFd_,SHUT_RDWR);close(listenFd_);listenFd_=-1;}queueChanged_.notify_all();if(acceptThread_.joinable())acceptThread_.join();closeQueued(Result::ProviderUnavailable,"shutdown");queueChanged_.notify_all();if(workerThread_.joinable())workerThread_.join();unlink(socketPath_.c_str());}
bool SuiteBridgeControlPlane::Running()const{return running_.load();}
SuiteBridgeControlPlane::Metrics SuiteBridgeControlPlane::SnapshotMetrics()const{Metrics m;for(std::size_t i=0;i<4;++i){m.admittedByOperation[i]=admittedByOperation_[i].load();m.executedByOperation[i]=executedByOperation_[i].load();}m.rejected=rejected_.load();m.overloaded=overloaded_.load();m.deadlineExpiredBeforeExecution=deadlineExpiredBeforeExecution_.load();m.protocolMismatch=protocolMismatch_.load();m.peerRejected=peerRejected_.load();m.queueHighWaterMark=queueHighWaterMark_.load();m.queueWaitNanoseconds=queueWaitNanoseconds_.load();m.executionNanoseconds=executionNanoseconds_.load();{std::lock_guard<std::mutex>g(queueMutex_);m.queueDepth=queue_.size();}m.running=running_.load();return m;}
bool SuiteBridgeControlPlane::validatePeer(int fd){
#ifdef SO_PEERCRED
 ucred c{};socklen_t n=sizeof(c);if(getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&c,&n)!=0)return false;return c.uid==geteuid()||c.uid==0;
#else
 (void)fd;return false;
#endif
}
bool SuiteBridgeControlPlane::receiveRequest(int fd,Request&r,std::string&why){pollfd p{};p.fd=fd;p.events=POLLIN;int ready;do{ready=poll(&p,1,100);}while(ready<0&&errno==EINTR);if(ready<=0||!(p.revents&POLLIN)){why="request_read_timeout";return false;}std::vector<std::uint8_t>b(MaximumRequestFrameBytes+1);ssize_t n=recv(fd,b.data(),b.size(),MSG_TRUNC);if(n<=0||static_cast<std::size_t>(n)>MaximumRequestFrameBytes){why="request_frame_size";return false;}b.resize(static_cast<std::size_t>(n));return decodeRequest(b,r,why);}
void SuiteBridgeControlPlane::sendResult(int fd,const Request&q,Result result,int code,const std::string&payload,const char*why){Response r;r.result=result;r.requestId=q.requestId==0?1:q.requestId;r.replyCode=code;r.payload=payload.size()<=MaximumResponsePayloadBytes?payload:"response_payload_too_large";std::vector<std::uint8_t>b;if(encodeResponse(r,b)){pollfd p{};p.fd=fd;p.events=POLLOUT;int ready;do{ready=poll(&p,1,100);}while(ready<0&&errno==EINTR);if(ready>0&&(p.revents&POLLOUT))(void)send(fd,b.data(),b.size(),MSG_NOSIGNAL);}log(q.operation,q.requestId,why);}
void SuiteBridgeControlPlane::log(Operation o,std::uint64_t id,const char*why)const{if(!logger_)return;std::ostringstream s;s<<"suitebridge: control-plane operation="<<operationName(o)<<" request-id="<<id<<" reason="<<(why?why:"unknown");logger_(s.str());}
void SuiteBridgeControlPlane::acceptLoop(){while(!stopRequested_.load()){pollfd p{};p.fd=listenFd_;p.events=POLLIN;int ready;do{ready=poll(&p,1,100);}while(ready<0&&errno==EINTR);if(ready<=0||!(p.revents&POLLIN))continue;int client=accept(listenFd_,nullptr,nullptr);if(client<0)continue;if(!configure(client)){close(client);continue;}Request q;if(!validatePeer(client)){++peerRejected_;++rejected_;q.requestId=1;sendResult(client,q,Result::PeerRejected,0,{},"peer_rejected");close(client);continue;}std::string why;if(!receiveRequest(client,q,why)){++protocolMismatch_;++rejected_;if(q.requestId==0)q.requestId=1;sendResult(client,q,Result::ProtocolMismatch,0,{},"protocol_mismatch");close(client);continue;}auto now=monotonicNowNanoseconds();if(now==0||now>=q.deadlineNanoseconds){++deadlineExpiredBeforeExecution_;++rejected_;sendResult(client,q,Result::DeadlineExpired,0,{},"deadline_expired_at_admission");close(client);continue;}bool admitted=false;std::size_t depth=0;{std::lock_guard<std::mutex>g(queueMutex_);if(!stopRequested_.load()&&queue_.size()<queueCapacity_){queue_.push_back(Pending{client,q,now});admitted=true;depth=queue_.size();}}if(!admitted){++overloaded_;++rejected_;sendResult(client,q,Result::Overloaded,0,{},"queue_full");close(client);continue;}++admittedByOperation_[operationIndex(q.operation)];auto high=queueHighWaterMark_.load();while(depth>high&&!queueHighWaterMark_.compare_exchange_weak(high,depth)){}queueChanged_.notify_one();}}
void SuiteBridgeControlPlane::workerLoop(){for(;;){Pending p;{std::unique_lock<std::mutex>l(queueMutex_);queueChanged_.wait(l,[this]{return stopRequested_.load()||!queue_.empty();});if(queue_.empty()){if(stopRequested_.load())break;continue;}p=std::move(queue_.front());queue_.pop_front();}auto before=monotonicNowNanoseconds();if(before==0||before>=p.request.deadlineNanoseconds){++deadlineExpiredBeforeExecution_;++rejected_;sendResult(p.fd,p.request,Result::DeadlineExpired,0,{},"deadline_expired_before_execution");close(p.fd);continue;}queueWaitNanoseconds_.fetch_add(before-p.admittedAtNanoseconds);auto start=monotonicNowNanoseconds();auto r=handler_(p.request.operation,p.request.payload);auto end=monotonicNowNanoseconds();if(end>=start)executionNanoseconds_.fetch_add(end-start);++executedByOperation_[operationIndex(p.request.operation)];if(!r.handled){++rejected_;sendResult(p.fd,p.request,Result::ProviderUnavailable,r.replyCode,r.payload,"provider_unavailable");}else{auto result=r.replyCode>=200&&r.replyCode<300?Result::Success:Result::NativeRejected;sendResult(p.fd,p.request,result,r.replyCode,r.payload,result==Result::Success?"success":"native_rejected");}close(p.fd);}}
void SuiteBridgeControlPlane::closeQueued(Result result,const char*why){std::deque<Pending>q;{std::lock_guard<std::mutex>g(queueMutex_);q.swap(queue_);}for(auto&e:q){++rejected_;sendResult(e.fd,e.request,result,0,{},why);close(e.fd);}}
